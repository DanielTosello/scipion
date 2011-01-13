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

#include <mpi.h>
#include <cstring>
#include <cstdlib>
#include <data/funcs.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <data/args.h>
#include <data/image.h>
#include <data/metadata.h>
#include <reconstruction/angular_class_average.h>

#define TAG_WORKFORWORKER   0
#define TAG_STOP   1
#define TAG_WAIT   2
#define TAG_FREEWORKER   3
#define TAG_WORKFROMWORKER   4

double  * output_values;
int       output_values_size;

class ProgMPIAngularClassAverage: public ProgAngularClassAverage
{
public:
    //int rank, size, num_img_tot;

    /** Number of Procesors **/
    int nProcs;

    /** Dvide the job in this number block with this number of images */
    //int mpi_job_size;

    /** computing node number. Master=0 */
    int rank;

    /** status after am MPI call */
    MPI_Status status;

    /*  constructor ------------------------------------------------------- */
    ProgMPIAngularClassAverage()
    {
        //parent class constructor will be called by deault without parameters
        MPI_Comm_size(MPI_COMM_WORLD, &(nProcs));
        MPI_Comm_rank(MPI_COMM_WORLD, &(rank));
        if (nProcs < 2)
            error_exit("This program cannot be executed in a single working node");
        //Blocks until all process have reached this routine.
        //very likelly this is useless
        MPI_Barrier(MPI_COMM_WORLD);
    }

    /* Read parameters --------------------------------------------------------- */
    void readParams()
    {
        ProgAngularClassAverage::readParams();

    }

    /* Usage ------------------------------------------------------------------- */
    void defineParams()
    {
        ProgAngularClassAverage::defineParams();
    }


    /* Pre Run --------------------------------------------------------------------- */
    void preRun()
    {
        produceSideInfo();
        //        MPI_Bcast(&max_number_of_images_in_around_a_sampling_point,
        //                  1, MPI_INT, 0, MPI_COMM_WORLD);
        int reserve = 0;
        if (nr_iter > 0)
            reserve = DF.size();
        output_values_size=AVG_OUPUT_SIZE*reserve+5;
        output_values = (double *) malloc(output_values_size*sizeof(double));

        // Only for do_add: append input docfile to add_to docfile
        if (rank == 0 && do_add)
        {
            FileName fn_tmp=fn_out+".doc";
            if (exists(fn_tmp))
            {
                MetaData DFaux = DF;
                // Don't do any fancy merging or sorting because those functions are really slow...
                DFaux.merge(fn_tmp);
                DFaux.write(fn_tmp);
            }
            else
                DF.write(fn_tmp);
        }
    }

    /* Run --------------------------------------------------------------------- */
    void run()
    {
        double myw[4];
        int number_of_references_image=1;
        if (rank == 0)
        {
            FileName fn_tmp;
            int nr_ref = DFlib.size();
            int c = XMIPP_MAX(1, nr_ref / 80);
            init_progress_bar(nr_ref);

            int stopTagsSent =0;
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
                // worker is free
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
                    double w, w1, w2;
                    int myref_number;
                    // Output classes sel and doc files
                    myref_number = ROUND(output_values[0]);
                    w  = output_values[1];
                    w1 = output_values[2];
                    w2 = output_values[3];
                    addClassAverage(myref_number,w,w1,w2);
#ifdef DEBUG

                    std::cerr << "Mr2.5 received work from worker " <<  status.MPI_SOURCE << std::endl;
                    std::cerr << " w w1 w2 myref_number"
                    << w  << " "
                    << w1 << " "
                    << w2 << " "
                    << myref_number <<  std::endl;
#endif

                    if (nr_iter > 0 )
                    {
                        int nr_images = ROUND(output_values[4] / AVG_OUPUT_SIZE);
                        for (int i = 0; i < nr_images; i++)
                        {
                            int this_image = ROUND(output_values[i*AVG_OUPUT_SIZE+5]);
                            if (!(this_image < 0))
                            {
                                //FIXME: The next line has no sense since the MDL_IMAGE is string
                                // and 'this_image' is of type int...
                                REPORT_ERROR(ERR_MD_BADTYPE, "The next line has no sense since the MDL_IMAGE is string \
                                             and 'this_image' is of type int...");
                                DF.gotoFirstObject(MDValueEQ(MDL_IMAGE,this_image));
                                DF.setValue(MDL_ANGLEROT,output_values[i*AVG_OUPUT_SIZE+6]);
                                DF.setValue(MDL_ANGLETILT,output_values[i*AVG_OUPUT_SIZE+7]);
                                DF.setValue(MDL_ANGLEPSI,output_values[i*AVG_OUPUT_SIZE+8]);
                                DF.setValue(MDL_SHIFTX,output_values[i*AVG_OUPUT_SIZE+9]);
                                DF.setValue(MDL_SHIFTY,output_values[i*AVG_OUPUT_SIZE+10]);
                                DF.setValue(MDL_REF,output_values[i*AVG_OUPUT_SIZE+11]);
                                DF.setValue(MDL_FLIP,output_values[i*AVG_OUPUT_SIZE+12]);
                                DF.setValue(MDL_SCALE,output_values[i*AVG_OUPUT_SIZE+13]);
                                DF.setValue(MDL_MAXCC,output_values[i*AVG_OUPUT_SIZE+14]);
                            }
                        }
                    }

                }//TAG_WORKFROMWORKER
                // worker is free
                if (status.MPI_TAG == TAG_FREEWORKER)
                {
                    MPI_Recv(0, 0, MPI_INT, MPI_ANY_SOURCE, TAG_FREEWORKER,
                             MPI_COMM_WORLD, &status);
#ifdef DEBUG

                    std::cerr << "Mr3 received TAG_FREEWORKER from worker " <<  status.MPI_SOURCE
                    << std::endl;
#endif

                    if(number_of_references_image>nr_ref)
                    {
                        MPI_Send(0, 0, MPI_INT, status.MPI_SOURCE, TAG_STOP, MPI_COMM_WORLD);
                        stopTagsSent++;
#ifdef DEBUG

                        std::cerr << "Ms4 sent stop tag to worker " <<  status.MPI_SOURCE << std::endl
                        << std::endl;
#endif

                        break;
                    }
                    else
                    {
                        //DFlib.adjust_to_data_line();
                        //dataforworker=DFlib(ABS(col_ref) - 1);
                        MPI_Send(&number_of_references_image,
                                 1,
                                 MPI_INT,
                                 status.MPI_SOURCE,
                                 TAG_WORKFORWORKER,
                                 MPI_COMM_WORLD);
                        number_of_references_image++;//////////////////////
                        if (number_of_references_image % c == 0)
                            progress_bar(number_of_references_image);
                        //prm.DFlib.next();
                    }
#ifdef DEBUG
                    std::cerr << "Ms5 sent TAG_WORKFORWORKER for " <<  status.MPI_SOURCE << std::endl
                    << std::endl;
#endif

                }//TAG_FREEWORKER
            }//while
            progress_bar(nr_ref);

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
                    double w, w1, w2;
                    int myref_number;
                    // Output classes sel and doc files
                    myref_number = ROUND(output_values[0]);
                    w  = output_values[1];
                    w1 = output_values[2];
                    w2 = output_values[3];
                    addClassAverage(myref_number,w,w1,w2);
#ifdef DEBUG

                    std::cerr << "Mr2.5 received work from worker " <<  status.MPI_SOURCE << std::endl;
                    std::cerr << " w w1 w2 myref_number"
                    << w  << " "
                    << w1 << " "
                    << w2 << " "
                    << myref_number <<  std::endl;
#endif

                    if (nr_iter > 0 )
                    {
                        int nr_images = ROUND(output_values[4] / AVG_OUPUT_SIZE);
                        for (int i = 0; i < nr_images; i++)
                        {
                            int this_image = ROUND(output_values[i*AVG_OUPUT_SIZE+5]);
                            if (!(this_image < 0))
                            {
                                //FIXME: The next line has no sense since the MDL_IMAGE is string
                                // and 'this_image' is of type int...
                                REPORT_ERROR(ERR_MD_BADTYPE, "The next line has no sense since the MDL_IMAGE is string \
                                             and 'this_image' is of type int...");
                                DF.gotoFirstObject(MDValueEQ(MDL_IMAGE,this_image));

                                DF.setValue(MDL_ANGLEROT,output_values[i*AVG_OUPUT_SIZE+6]);
                                DF.setValue(MDL_ANGLETILT,output_values[i*AVG_OUPUT_SIZE+7]);
                                DF.setValue(MDL_ANGLEPSI,output_values[i*AVG_OUPUT_SIZE+8]);
                                DF.setValue(MDL_SHIFTX,output_values[i*AVG_OUPUT_SIZE+9]);
                                DF.setValue(MDL_SHIFTY,output_values[i*AVG_OUPUT_SIZE+10]);
                                DF.setValue(MDL_REF,output_values[i*AVG_OUPUT_SIZE+11]);
                                DF.setValue(MDL_FLIP,output_values[i*AVG_OUPUT_SIZE+12]);
                                DF.setValue(MDL_SCALE,output_values[i*AVG_OUPUT_SIZE+13]);
                                DF.setValue(MDL_MAXCC,output_values[i*AVG_OUPUT_SIZE+14]);
                            }
                        }
                    }
                }
                else if (status.MPI_TAG == TAG_FREEWORKER)
                {
                    MPI_Recv(0, 0, MPI_INT, MPI_ANY_SOURCE, TAG_FREEWORKER,
                             MPI_COMM_WORLD, &status);
#ifdef DEBUG

                    std::cerr << "Mr received TAG_FREEWORKER from worker " <<  status.MPI_SOURCE << std::endl;
#endif

                    MPI_Send(0, 0, MPI_INT, status.MPI_SOURCE, TAG_STOP, MPI_COMM_WORLD);
                    stopTagsSent++;
                }
            }

            // Write selfiles and docfiles with all class averages
            finalWriteToDisc();

        }
        else //rank !=0
        {
            // Select only relevant part of selfile for this rank
            // job number
            // job size
            // aux variable
            while (1)
            {
                MPI_Send(0, 0, MPI_INT, 0, TAG_FREEWORKER, MPI_COMM_WORLD);
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
                    break;
                }
                if (status.MPI_TAG == TAG_WORKFORWORKER)
                    //there is still some work to be done
                {
                    //get the jobs number
                    MPI_Recv(&number_of_references_image,
                             1,
                             MPI_INT,
                             0,
                             TAG_WORKFORWORKER,
                             MPI_COMM_WORLD,
                             &status);
#ifdef DEBUG

                    std::cerr << "Wr" << rank << " " << "TAG_WORKFORWORKER" << std::endl;
#endif

                    processOneClass(number_of_references_image, output_values);
                    MPI_Send(output_values,
                             output_values_size,
                             MPI_DOUBLE,
                             0,
                             TAG_WORKFROMWORKER,
                             MPI_COMM_WORLD);
                }
            }
        }
    }

    /* a short function to print a message and exit */
    void error_exit(char * msg)
    {
        fprintf(stderr, "%s", msg);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

};

int main(int argc, char *argv[])
{
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
    {
        fprintf(stderr, "MPI initialization error\n");
        exit(EXIT_FAILURE);
    }

    ProgMPIAngularClassAverage prm;
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
