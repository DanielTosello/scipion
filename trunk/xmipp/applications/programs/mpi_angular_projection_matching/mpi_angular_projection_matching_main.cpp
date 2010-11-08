/***************************************************************************
 *
 * Authors:     Roberto Marabini (roberto@cnb.csic.es)
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 *
 *  All comments concerning this program package may be sent to the
 *  e-mail address 'xmipp@cnb.csic.es'
 ***************************************************************************/

//#include "mpi_run.h"

#include <mpi.h>
#include <cstring>
#include <cstdlib>
#include <data/funcs.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <data/args.h>
#include <reconstruction/angular_projection_matching.h>
#include <data/sampling.h>
#include <data/symmetries.h>

#define TAG_WORKFORWORKER   0
#define TAG_STOP   1
#define TAG_WAIT   2
#define TAG_FREEWORKER   3
#define TAG_WORKFROMWORKER   4

int     * input_images;
int       input_images_size;
double  * output_values;
int       output_values_size;
//consider
//class Prog_mpi_projection_matching_prm:public Prog_projection_matching_prm
//to access parent variables
class ProgMpiAngularProjectionMatching: public ProgAngularProjectionMatching
{
public:
    //int rank, size, num_img_tot;

    /** Number of Procesors **/
    int nProcs;

    /** Dvide the job in this number block with this number of images */
    int mpi_job_size;

    /** classify the experimental data making voronoi regions
        with an hexagonal grid mapped onto a sphere surface */
    double chunk_angular_distance;

    /** computing node number. Master=0 */
    int rank;

    /** status after am MPI call */
    MPI_Status status;

    /** total number of images */
    int num_img_tot;

    /** symmetry file */
    FileName        fn_sym;

    /** sampling object */
    Sampling chunk_mysampling;

    /** Symmetry. One of the 17 possible symmetries in
        single particle electron microscopy.
         */
    int symmetry;

    /** For infinite groups symmetry order*/
    int sym_order;

    /*  constructor ------------------------------------------------------- */
    ProgMpiAngularProjectionMatching()
    {
        //parent class constructor will be called by deault without parameters
        MPI_Comm_size(MPI_COMM_WORLD, &(nProcs));
        MPI_Comm_rank(MPI_COMM_WORLD, &(rank));
        if (nProcs < 2)
            error_exit("This program cannot be executed in a single working node");
        //Blocks until all process have reached this routine.
        //very likelly this is
        MPI_Barrier(MPI_COMM_WORLD);
    }


    /* Read parameters --------------------------------------------------------- */
    void readParams()
    {
        ProgAngularProjectionMatching::readParams();
        mpi_job_size=getIntParam("--mpi_job_size");
        chunk_angular_distance = getDoubleParam("--chunk_angular_distance");
        fn_sym = getParam("--sym");
    }

    /* Usage ------------------------------------------------------------------- */
    void defineParams()
    {
        ProgAngularProjectionMatching::defineParams();
        addParamsLine("  [--mpi_job_size <size=10>]   : Number of images sent to a cpu in a single job ");
        addParamsLine("                                : 10 may be a good value");
        addParamsLine("                                : if  -1 the computer will fill the value for you");
        addParamsLine("  [--chunk_angular_distance <dist=-1>]  : sample the projection sphere with this ");
        addParamsLine("                                 :using the voronoi regions");
        addParamsLine("  [--sym <cn=\"c1\">]            : One of the 17 possible symmetries in");
        addParamsLine("                                 :single particle electronmicroscopy");
        addParamsLine("                                 :i.e.  ci, cs, cn, cnv, cnh, sn, dn, dnv,");
        addParamsLine("                                 :dnh, t, td, th, o, oh, i1 (default MDB), i2, i3, i4, ih");
        addParamsLine("                                 :i1h (default MDB), i2h, i3h, i4h");
        addParamsLine("                                : where n may change from 1 to 99");
    }


    /* Pre Run --------------------------------------------------------------------- */
    void preRun()
    {
        produceSideInfo();
        int max_number_of_images_in_around_a_sampling_point=0;
        if (rank == 0)
        {
            //read experimental doc file
            DFexp.read(fn_exp);
            //process the symmetry file
            if (!chunk_mysampling.SL.isSymmetryGroup(fn_sym, symmetry, sym_order))
                REPORT_ERROR(ERR_NUMERICAL, (std::string)"mpi_angular_proj_match::prerun Invalid symmetry: " +  fn_sym);
            chunk_mysampling.SL.read_sym_file(fn_sym);
            // find a value for chunk_angular_distance if != -1
            if( chunk_angular_distance == -1)
                compute_chunk_angular_distance(symmetry, sym_order);
            //store symmetry matrices, this is faster than computing them
            //each time we need them
            chunk_mysampling.fill_L_R_repository();
            int remaining_points=0;
            while(remaining_points==0)
            {
                //first set sampling rate
                chunk_mysampling.SetSampling(chunk_angular_distance);
                //create sampling points in the whole sphere
                chunk_mysampling.Compute_sampling_points(false,91.,-91.);
                //precompute product between symmetry matrices
                //and experimental data
                chunk_mysampling.fill_exp_data_projection_direction_by_L_R(fn_exp);
                //remove redundant sampling points: symmetry
                chunk_mysampling.remove_redundant_points(symmetry, sym_order);
                remaining_points=chunk_mysampling.no_redundant_sampling_points_angles.size();
                if (chunk_angular_distance>2)
                    chunk_angular_distance -= 1;
                else
                    chunk_angular_distance /=2;
                //if(remaining_points ==0)
                std::cerr << "New chunk_angular_distance "
                << chunk_angular_distance << std::endl;
                if(chunk_angular_distance < 0)
                {
                    std::cerr << " Can compute chunk_angular_distance\n";
                    exit(1);
                }
            }
            //remove sampling points too far away from experimental data
            chunk_mysampling.remove_points_far_away_from_experimental_data();
            //for each sampling point find the experimental images
            //closer to that point than to any other
            chunk_mysampling.find_closest_experimental_point();
            //print number of points per node
            //#define DEBUG
#ifdef DEBUG

            std::cerr << "voronoi region, number of elements" << std::endl;
            for (int j = 0;
                 j < chunk_mysampling.my_exp_img_per_sampling_point.size();
                 j++)
                std::cerr << j
                << " "
                << chunk_mysampling.my_exp_img_per_sampling_point[j].size()
                << std::endl;
#endif
            #undef DEBUG

            for (int j = 0;
                 j < chunk_mysampling.my_exp_img_per_sampling_point.size();
                 j++)
            {
                if (max_number_of_images_in_around_a_sampling_point
                    < chunk_mysampling.my_exp_img_per_sampling_point[j].size())
                    max_number_of_images_in_around_a_sampling_point
                    = chunk_mysampling.my_exp_img_per_sampling_point[j].size();
            }
            std::cerr << "number of subsets "
            << chunk_mysampling.my_exp_img_per_sampling_point.size()
            << std::endl;
            std::cerr << "biggest subset (EXPERIMENTAL images per chunk)"
            << max_number_of_images_in_around_a_sampling_point
            << std::endl;
            std::cerr << "maximun number of references in memory "
            <<  max_nr_refs_in_memory
            << std::endl;
            //alloc memory for buffer
            if (mpi_job_size == -1)
            {
                int numberOfJobs=nProcs-1;//one node is the master
                mpi_job_size=ceil((double)DFexp.size()/numberOfJobs);
            }
        }
        MPI_Bcast(&max_number_of_images_in_around_a_sampling_point,
                  1, MPI_INT, 0, MPI_COMM_WORLD);
        input_images_size = max_number_of_images_in_around_a_sampling_point+1 +1;
        input_images  = (int *)    malloc(input_images_size*sizeof(int));
        output_values_size=MY_OUPUT_SIZE*max_number_of_images_in_around_a_sampling_point+1;
        output_values = (double *) malloc(output_values_size*sizeof(double));

        //only one node will write in the console
        if (rank != 1)
            verbose = 0;
        else
            verbose = 1;
        //initialize each node, this shoud be out of run
        //because is made once per node but not one per packet

        //many sequential programs free object alloc in side_info
        //becareful with that
    }

    /* Run --------------------------------------------------------------------- */
    void run()
    {
        if (rank == 0)
        {
            int N = chunk_mysampling.my_exp_img_per_sampling_point.size();
            int killed_jobs=0;
            int index=0;
            int index_counter=0;
            int tip=-1;
            int number_of_processed_images=0;
            int stopTagsSent =0;
            int total_number_of_images=DFexp.size();
            Matrix1D<double>                 dataline(8);
            //DocFile DFo;
            FileName                         fn_tmp;

            // Initialize progress bar
            int c = XMIPP_MAX(1, total_number_of_images / 80);
            init_progress_bar(total_number_of_images);

            while(1)
            {
                //Wait until any message arrives
                //be aware that mpi_Probe will block the program untill a message is received
                //#define DEBUG
#ifdef DEBUG
                std::cerr << "Mp1 waiting for any  message " << std::endl;
#endif

                MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
#ifdef DEBUG

                std::cerr << "Mp2 received tag from worker " <<  status.MPI_SOURCE << std::endl;
#endif
                // worker sends work
                if (status.MPI_TAG == TAG_WORKFROMWORKER)
                {
                    MPI_Recv(output_values,
                             output_values_size,
                             MPI_DOUBLE,
                             MPI_ANY_SOURCE,
                             TAG_WORKFROMWORKER,
                             MPI_COMM_WORLD,
                             &status);
                    int number= ROUND(output_values[0]/MY_OUPUT_SIZE);
                    //create doc file
                    for (int i = 0; i < number; i++)
                    {
                        int lineNumber=ROUND(output_values[i*MY_OUPUT_SIZE+1]);
                        DFexp.goToObject(lineNumber);
                        DFexp.setValue(MDL_ANGLEROT, output_values[i*MY_OUPUT_SIZE+2]);
                        DFexp.setValue(MDL_ANGLETILT,output_values[i*MY_OUPUT_SIZE+3]);
                        DFexp.setValue(MDL_ANGLEPSI, output_values[i*MY_OUPUT_SIZE+4]);
                        DFexp.setValue(MDL_SHIFTX,   output_values[i*MY_OUPUT_SIZE+5]);
                        DFexp.setValue(MDL_SHIFTY,   output_values[i*MY_OUPUT_SIZE+6]);
                        DFexp.setValue(MDL_REF,(int)(output_values[i*MY_OUPUT_SIZE+7]));
                        DFexp.setValue(MDL_FLIP,    (output_values[i*MY_OUPUT_SIZE+8]>0));
                        DFexp.setValue(MDL_MAXCC,    output_values[i*MY_OUPUT_SIZE+9]);
                    }
                }
                // worker is free
                else if (status.MPI_TAG == TAG_FREEWORKER)
                {
                    MPI_Recv(&tip, 1, MPI_INT, MPI_ANY_SOURCE, TAG_FREEWORKER,
                             MPI_COMM_WORLD, &status);
                    //#define DEBUG
#ifdef DEBUG

                    std::cerr << "Mr3 received TAG_FREEWORKER from worker " <<  status.MPI_SOURCE
                    << "with tip= " << tip
                    << "\nnumber_of_processed_images "
                    << number_of_processed_images
                    << "\ntotal_number_of_images "
                    << total_number_of_images
                    << std::endl;
#endif
                    #undef DEBUG

                    if(number_of_processed_images>=total_number_of_images)
                    {
                        MPI_Send(0, 0, MPI_INT, status.MPI_SOURCE, TAG_STOP, MPI_COMM_WORLD);
                        stopTagsSent++;
#ifdef DEBUG

                        std::cerr << "Ms4 sent stop tag to worker " <<  status.MPI_SOURCE << std::endl
                        << std::endl;
#endif

                        break;
                    }
                    if(tip==-1)
                    {
                        index=index_counter;
                    }
                    else
                        index=tip;
                    while( chunk_mysampling.my_exp_img_per_sampling_point[index].size()<=0)
                    {
                        index_counter++;
                        index_counter = index_counter%N;
                        index = index_counter;
                    }
                    int number_of_images_to_transfer=XMIPP_MIN(
                                                         chunk_mysampling.my_exp_img_per_sampling_point[index].size(),
                                                         mpi_job_size);

                    input_images[0]=index;
                    input_images[1]=number_of_images_to_transfer;


                    for(int k=2;k<number_of_images_to_transfer+2;k++)
                    {
                        number_of_processed_images++;
                        input_images[k]=chunk_mysampling.my_exp_img_per_sampling_point[index].back();
                        chunk_mysampling.my_exp_img_per_sampling_point[index].pop_back();
                    }

                    MPI_Send(input_images,
                             number_of_images_to_transfer+2,
                             MPI_INT,
                             status.MPI_SOURCE,
                             TAG_WORKFORWORKER,
                             MPI_COMM_WORLD);
#ifdef DEBUG

                    std::cerr << "Ms_s send work for worker "
                    <<  status.MPI_SOURCE
                    << "with index " << index%N
                    << std::endl;
#endif
                    // Update progress bar
                    if (number_of_processed_images % c == 0)
                        progress_bar(number_of_processed_images);

                }//TAG_FREEWORKER
            }//while
            progress_bar(total_number_of_images);

            while (stopTagsSent < (nProcs-1))
            {
                MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                if (status.MPI_TAG == TAG_WORKFROMWORKER)
                {
                    MPI_Recv(output_values,
                             output_values_size,
                             MPI_DOUBLE,
                             MPI_ANY_SOURCE,
                             TAG_WORKFROMWORKER,
                             MPI_COMM_WORLD,
                             &status);
                    int number= ROUND(output_values[0]/MY_OUPUT_SIZE);
                    //create doc file
                    for (int i = 0; i < number; i++)
                    {
                        int lineNumber=ROUND(output_values[i*MY_OUPUT_SIZE+1]+1);
                        DFexp.goToObject(lineNumber);
                        DFexp.setValue(MDL_ANGLEROT, output_values[i*MY_OUPUT_SIZE+2]);
                        DFexp.setValue(MDL_ANGLETILT,output_values[i*MY_OUPUT_SIZE+3]);
                        DFexp.setValue(MDL_ANGLEPSI, output_values[i*MY_OUPUT_SIZE+4]);
                        DFexp.setValue(MDL_SHIFTX,   output_values[i*MY_OUPUT_SIZE+5]);
                        DFexp.setValue(MDL_SHIFTY,   output_values[i*MY_OUPUT_SIZE+6]);
                        DFexp.setValue(MDL_REF,(int)(output_values[i*MY_OUPUT_SIZE+7]));
                        DFexp.setValue(MDL_FLIP,    (output_values[i*MY_OUPUT_SIZE+8]>0));
                        DFexp.setValue(MDL_MAXCC,    output_values[i*MY_OUPUT_SIZE+9]);
                    }
                }
                else if (status.MPI_TAG == TAG_FREEWORKER)
                {
                    MPI_Recv(&tip, 1, MPI_INT, MPI_ANY_SOURCE, TAG_FREEWORKER,
                             MPI_COMM_WORLD, &status);
#ifdef DEBUG

                    std::cerr << "Mr received TAG_FREEWORKER from worker " <<  status.MPI_SOURCE << std::endl;
                    std::cerr << "Ms sent TAG_STOP to worker" << status.MPI_SOURCE << std::endl;
#endif

                    MPI_Send(0, 0, MPI_INT, status.MPI_SOURCE, TAG_STOP, MPI_COMM_WORLD);
                    stopTagsSent++;
                }
                else
                {
                    error_exit("Received unknown TAG I quit (master)");
                }

            }
            //close temperoal file with results
            DFexp.write(fn_root + ".doc");


        }
        else //rank !=0
        {
            // Select only relevant part of selfile for this rank
            // job number
            // job size
            // aux variable
            int worker_tip=-1;
            ;
            while (1)
            {
                int jobNumber=0;
                MPI_Send(&worker_tip, 1, MPI_INT, 0, TAG_FREEWORKER, MPI_COMM_WORLD);
                //#define DEBUG
#ifdef DEBUG

                std::cerr << "W" << rank << " " << "sent TAG_FREEWORKER to master " << std::endl;
#endif
                //get your next task
                MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
#ifdef DEBUG

                std::cerr << "W" << rank << " " << "probe MPI_ANY_TAG " << std::endl;
#endif

                if (status.MPI_TAG == TAG_STOP)//no more jobs exit
                {
                    //If I  do not read this tag
                    //master will no further process
                    //a posibility is a non-blocking send
                    MPI_Recv(0, 0, MPI_INT, 0, TAG_STOP,
                             MPI_COMM_WORLD, &status);
#ifdef DEBUG

                    std::cerr << "Wr" << rank
                    << " " << "TAG_STOP" << std::endl;
#endif

                    break;
                }
                if (status.MPI_TAG == TAG_WORKFORWORKER)
                    //there is still some work to be done
                {
                    //get the jobs number
                    MPI_Recv(input_images,
                             input_images_size,
                             MPI_INT,
                             0,
                             TAG_WORKFORWORKER,
                             MPI_COMM_WORLD,
                             &status);
                    worker_tip =  input_images[0];
                    int number_of_transfered_images =  input_images[1];
                    //#define DEBUG
#ifdef DEBUG

                    std::cerr << "Wr " << rank << " " << "TAG_WORKFORWORKER" << std::endl;
                    std::cerr << "\n rank, tip, input_images_size "
                    << rank
                    << " "
                    << worker_tip
                    << " "
                    << input_images[1]
                    << std::endl;
                    for (int i=2; i < number_of_transfered_images+2 ; i++)
                        std::cerr << input_images[i] << " " ;
                    std::cerr << std::endl;
#endif
      #undef DEBUG
                    /////////////
                    processSomeImages(&(input_images[1]),output_values);
#ifdef DEBUG

                    std::cerr << "Ws " << rank << " " << "TAG_WORKFROMWORKER" << std::endl;
                    std::cerr << "\n rank, size "
                    << rank
                    << " "
                    << output_values[0]
                    << std::endl;
                    std::cerr << std::endl;
                    for (int i=0; i < output_values[0]; i++)
                        std::cerr << output_values[i] << " " ;
                    std::cerr << std::endl;
#endif

                    MPI_Send(output_values,
                             output_values_size,
                             MPI_DOUBLE,
                             0,
                             TAG_WORKFROMWORKER,
                             MPI_COMM_WORLD);
                    ///////////////
                }
                else
                {
                    error_exit("Received unknown TAG I quit (worker)");
                }
            }//while(1)
        }//worker
    }

    /* a short function to print a message and exit */
    void error_exit(char * msg)
    {
        fprintf(stderr, "%s", msg);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    /** guess a good value for chunk_angular_distance.
        this value is use to divide the work between
        the different working nodes */
    void compute_chunk_angular_distance(int symmetry, int sym_order)
    {
        double non_reduntant_area_of_ewald_sphere =
            chunk_mysampling.SL.non_redundant_evald_sphere(symmetry,sym_order);
        double number_cpus  = (double) nProcs-1;
        //NEXT ONE IS SAMPLING NOT ANOTHERSAMPLING
        double neighborhood_radius= ABS(acos(mysampling.cos_neighborhood_radius));
        //NEXT ONE IS SAMPLING NOT ANOTHERSAMPLING
        if (mysampling.cos_neighborhood_radius<-1.001)
            neighborhood_radius=0;
        int counter=0;
        while(1)
        {
            if(counter++ > 1000)
            {
                chunk_angular_distance= 0.001;
                std::cerr << "****************************************************" << std::endl;
                std::cerr << "* WARNING: The neighbourhood does not fit in memory " << std::endl;
                std::cerr << "****************************************************" << std::endl;
                break;
            }
            double area_chunk=non_reduntant_area_of_ewald_sphere/number_cpus;
            //area chunk is area of spheric casket=2 PI h
            chunk_angular_distance=acos(1-area_chunk/(2*PI));
            double area_chunck_neigh= 2 * PI *( 1 - cos(chunk_angular_distance+neighborhood_radius));
            //double area_chunck= 2 * PI *( 1 - cos(chunk_angular_distance));
            //let us see how many references from the reference library fit
            //in area_chunk, that is divide area_chunk between the voronoi
            //region of the sampling points of the reference library
            double areaVoronoiRegionReferenceLibrary = 2 *( 3 *(  acos(
                        //NEXT ONE IS SAMPLING NOT ANOTHERSAMPLING
                        cos(mysampling.sampling_rate_rad)/(1+cos(mysampling.sampling_rate_rad)) )  ) - PI);
            int number_of_images_that_fit_in_a_chunck_neigh =
                ceil(area_chunck_neigh / areaVoronoiRegionReferenceLibrary);
            //#define DEBUG
#ifdef DEBUG

            std::cerr << "\n\ncounter " << counter << std::endl;
            std::cerr << "area_chunk " << area_chunk << std::endl;
            std::cerr << "2*chunk_angular_distance " << 2*chunk_angular_distance << std::endl;
            //NEXT ONE IS SAMPLING NOT ANOTHERSAMPLING
            std::cerr << "sampling_rate_rad " << mysampling.sampling_rate_rad
            << " " << mysampling.sampling_rate_rad*180/PI
            <<  std::endl;
            std::cerr << "neighborhood_radius " << neighborhood_radius
            <<  std::endl;
            std::cerr << "areaVoronoiRegionReferenceLibrary " << areaVoronoiRegionReferenceLibrary << std::endl;
            std::cerr << "number_of_images_that_fit_in_a_chunck_neigh " << number_of_images_that_fit_in_a_chunck_neigh << std::endl;
            std::cerr << "number_cpus " << number_cpus << std::endl;
            std::cerr << "max_nr_imgs_in_memory " << max_nr_imgs_in_memory << std::endl;
#endif
            #undef DEBUG

            if(number_of_images_that_fit_in_a_chunck_neigh>max_nr_imgs_in_memory)
                number_cpus  = 1.2*number_cpus;
            else
                break;
        }
        //chunk_angular_distance -= neighborhood_radius;
        chunk_angular_distance *= 2.0;

        //#define DEBUG
#ifdef DEBUG

        std::cerr << "chunk_angular_distance "  << chunk_angular_distance
        <<  std::endl
        << "neighborhood_radius "     << neighborhood_radius
        <<  std::endl;
#endif
            #undef DEBUG
        //chuck should not be bigger than a triangle in the icosahedra
        if(chunk_angular_distance >= 0.5 * cte_w)
            chunk_angular_distance  = 0.5 * cte_w;
        chunk_angular_distance *= (180./PI);
        //#define DEBUG
#ifdef DEBUG

        std::cerr << "chunk_angular_distance_degrees "  << chunk_angular_distance
        <<  std::endl;
#endif
            #undef DEBUG

    }

};

int main(int argc, char *argv[])
{
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
    {
        fprintf(stderr, "MPI initialization error\n");
        exit(EXIT_FAILURE);
    }

    ProgMpiAngularProjectionMatching prm;
    bool finalize_worker=false;

    if (prm.rank == 0)
    {
        try
        {
            prm.read(argc, argv);
        }
        catch (XmippError XE)
        {
            std::cerr << XE;
            MPI_Finalize();
            exit(1);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (prm.rank != 0)
    {
        try
        {
            prm.read(argc, argv);
        }
        catch (XmippError XE)
        {
            std::cerr << XE;
            MPI_Finalize();
            exit(1);
        }
    }

    try
    {
        prm.preRun();
        prm.run();
        MPI_Finalize();
    }
    catch (XmippError XE)
    {
        std::cerr << XE;
        exit(1);
    }
    exit(0);
}
