/***************************************************************************
 *
 * Authors:     Jose Roman Bilbao Castro (jrbcast@cnb.csic.es)
 *              Carlos Oscar Sanchez Sorzano (coss.eps@ceu.es)
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 * Lab. de Bioingenieria, Univ. San Pablo CEU
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
#include <reconstruction/angular_discrete_assign.h>

int main(int argc, char **argv)
{
    // Initialize MPI
    int rank, NProcessors;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &NProcessors);

    Prog_angular_predict_prm prm;
    try
    {
        // Read input parameters
	prm.MPIversion=true;
        prm.read(argc, argv);
    }
    catch (Xmipp_error XE)
    {
        if (rank == 0)
        {
            std::cout << XE;
            prm.usage();
        }
        MPI_Finalize();
        exit(1);
    }

    try
    {
        // Everyone produces its own side info
        prm.produce_side_info(rank);

        // Divide the selfile in chunks
        int imgNbr = prm.DFexp.size();

        // Make the alignment, rank=0 receives all the assignments
        // The rest of the ranks compute the angular parameters for their
        // assigned images
        double v[7];
        if (rank == 0)
        {
            int toGo = imgNbr;
            std::cerr << "Assigning angles ...\n";
            init_progress_bar(imgNbr);
            MPI_Status status;
            while (toGo > 0)
            {
                MPI_Recv(v, 7, MPI_DOUBLE, MPI_ANY_SOURCE,
                         MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                int i = (int)v[0];
                prm.predicted_rot[i] = v[1];
                prm.predicted_tilt[i] = v[2];
                prm.predicted_psi[i] = v[3];
                prm.predicted_shiftX[i] = v[4];
                prm.predicted_shiftY[i] = v[5];
                prm.predicted_corr[i] = v[6];
                toGo--;
                if (toGo % (imgNbr / 60) == 0)
                {
                    progress_bar(imgNbr - toGo);
                    std::cerr.flush();
                }
            }
            progress_bar(imgNbr);
        }
        else
        {
            int i=0;
            FOR_ALL_OBJECTS_IN_METADATA(prm.DFexp)
            {
                if ((i+1)%(NProcessors-1)+1==rank)
                {
                    std::string fnImg;
                    prm.DFexp.getValue(MDL_IMAGE,fnImg);
                    Image<double> img;
                    img.read(fnImg);
                    double shiftX, shiftY, psi, rot, tilt;
                    double corr = prm.predict_angles(img, shiftX, shiftY, rot, tilt, psi);

                    // Send the alignment parameters to the master
                    v[0] = i;
                    v[1] = rot;
                    v[2] = tilt;
                    v[3] = psi;
                    v[4] = shiftX;
                    v[5] = shiftY;
                    v[6] = corr;
                    MPI_Send(v, 7, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
                }
                i++;
            }
        }

        if (rank == 0) prm.finish_processing();
    }
    catch (Xmipp_error XE)
    {
        std::cout << XE << std::endl;
        MPI_Finalize();
	return 1 ;
    }
    MPI_Finalize();
    return 0 ;
}
