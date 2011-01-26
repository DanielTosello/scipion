/***************************************************************************
 *
 * Authors:     Javier Angel Velazquez Muriel (javi@cnb.csic.es)
 *              Carlos Oscar Sanchez Sorzano
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

#include "ctf_estimate_from_micrograph.h"

#include <data/args.h>
#include <data/micrograph.h>
#include <data/metadata.h>
#include <data/image.h>
#include <data/fft.h>
#include <data/basic_pca.h>

/* Read parameters ========================================================= */
void Prog_assign_CTF_prm::read(const FileName &fn_prm, bool do_not_read_files)
{
    // Read parameters for adjust CTF from input file
    adjust_CTF_prm.read(fn_prm);

    // Read specific parameters for this program from input file
    FILE *fh_param;
    if ((fh_param = fopen(fn_prm.c_str(), "r")) == NULL)
        REPORT_ERROR(ERR_IO_NOTOPEN, (std::string)"assign_CTF: There is a problem "
                     "opening the file " + fn_prm);

    reversed          = checkParameter(fh_param, "reverse endian");
    N_horizontal      = textToInteger(getParameter(fh_param, "N_horizontal", 0));
    N_vertical        = textToInteger(getParameter(fh_param, "N_vertical", 0, "-1"));
    if (N_vertical == -1)
        N_vertical = N_horizontal;

    compute_at_particle  = checkParameter(fh_param, "compute_at_particle");
    micrograph_averaging = checkParameter(fh_param, "micrograph_averaging");
    piece_averaging      = checkParameter(fh_param, "piece_averaging");
    Nside_piece          = textToInteger(getParameter(fh_param, "Nside_piece", 0, "5"));
    if (checkParameter(fh_param, "periodogram"))
        PSD_mode = Periodogram;
    else
        PSD_mode = ARMA;
    dont_adjust_CTF      = checkParameter(fh_param, "dont_adjust_CTF");
    bootstrapN           = textToInteger(getParameter(fh_param, "bootstrapN", 0, "-1"));

    if (!do_not_read_files)
    {
        image_fn          = getParameter(fh_param, "image", 0, "");
        selfile_mode = image_fn == "";
        if (selfile_mode)
            micrograph_averaging = true;
        if (selfile_mode)
            selfile_fn = getParameter(fh_param, "selfile", 0);
        else
            selfile_fn = getParameter(fh_param, "selfile", 0, "");
        picked_fn         = getParameter(fh_param, "picked", 0, "");
        FileName fn_root  = image_fn.withoutExtension();
        if (PSD_mode == ARMA)
            PSDfn_root = fn_root + "_ARMA";
        else
            PSDfn_root = fn_root + "_Periodogram";
    }
    fclose(fh_param);

    // Read ARMA parameters from input file
    if (PSD_mode == ARMA)
        ARMA_prm.read(fn_prm);
}

/* Write parameters ========================================================= */
void Prog_assign_CTF_prm::write(const FileName &fn_prm,
                                const std::string &directory)
{
    std::ofstream fh_param;
    fh_param.open(fn_prm.c_str(), std::ios::out);
    if (!fh_param)
        REPORT_ERROR(ERR_IO_NOTOPEN, (std::string)"assign_CTF: There is a problem "
                     "opening the file " + fn_prm + " for write");
    fh_param << "# Assign CTF parameters\n";
    std::string aux;
    bool remove_directories = directory != "";
    if (!remove_directories)
        aux = image_fn;
    else
        aux = directory + "/" + image_fn.removeDirectories();
    if (!selfile_mode)
        fh_param << "image="                << aux                  << std::endl;
    fh_param << "N_horizontal="         << N_horizontal         << std::endl
    << "N_vertical="           << N_vertical           << std::endl;
    if (!remove_directories)
        aux = selfile_fn;
    else
        aux = directory + "/" + selfile_fn.removeDirectories();
    fh_param << "selfile="              << aux                  << std::endl;
    if (!remove_directories)
        aux = picked_fn;
    else
        aux = directory + "/" + picked_fn.removeDirectories();
    fh_param << "picked="               << aux                  << std::endl;
    if (compute_at_particle)
        fh_param << "compute_at_particle=yes\n";
    if (micrograph_averaging)
        fh_param << "micrograph_averaging=yes\n";
    if (piece_averaging)
    {
        fh_param << "piece_averaging=yes\n"
        << "Nside_piece=" << Nside_piece << std::endl;
    }
    if (PSD_mode == Periodogram)
        fh_param << "Periodogram=yes\n";
    if (dont_adjust_CTF)
        fh_param << "dont_adjust_CTF=yes\n";
    if (bootstrapN>-1)
        fh_param << "bootstrapN=" << bootstrapN << std::endl;

    fh_param << std::endl;
    fh_param.close();

    adjust_CTF_prm.write(fn_prm, false);
    ARMA_prm.write(fn_prm, false);
}

/* Compute PSD by piece averaging ========================================== */
//#define DEBUG
void Prog_assign_CTF_prm::PSD_piece_by_averaging(MultidimArray<double> &piece,
        MultidimArray<double> &psd)
{
    int small_Ydim = 2 * YSIZE(piece) / Nside_piece;
    int small_Xdim = 2 * XSIZE(piece) / Nside_piece;
    MultidimArray<double> small_piece(small_Ydim, small_Xdim);

    int Xstep = (XSIZE(piece) - small_Xdim) / (Nside_piece - 1);
    int Ystep = (YSIZE(piece) - small_Ydim) / (Nside_piece - 1);
    psd.initZeros(small_piece);
#ifdef DEBUG

    Image<double> save;
    save()=piece;
    save.write("PPPpiece.xmp");
#endif

    for (int ii = 0; ii < Nside_piece; ii++)
        for (int jj = 0; jj < Nside_piece; jj++)
        {
            // Take the corresponding small piece from the piece
            int i0 = ii * Xstep;
            int j0 = jj * Ystep;

            int i, j, ib, jb;
            for (i = 0, ib = i0; i < small_Ydim; i++, ib++)
                for (j = 0, jb = j0; j < small_Xdim; j++, jb++)
                    DIRECT_A2D_ELEM(small_piece, i, j) =
                        DIRECT_A2D_ELEM(piece, ib, jb);

#ifdef DEBUG

            save()=small_piece;
            save.write("PPPsmall_piece.xmp");
#endif

            // Compute the PSD of the small piece
            MultidimArray<double> small_psd;
            small_psd.initZeros(small_piece);
            if (PSD_mode == ARMA)
            {
                // Compute the ARMA model
                MultidimArray<double> ARParameters, MAParameters;
                double dSigma = CausalARMA(small_piece, ARMA_prm.N_AR, ARMA_prm.M_AR,
                                           ARMA_prm.N_MA, ARMA_prm.M_MA, ARParameters, MAParameters);
                ARMAFilter(small_piece, small_psd, ARParameters, MAParameters, dSigma);
            }
            else
            {
                // Compute the periodogram
                MultidimArray< std::complex<double> > Periodogram;
                FourierTransform(small_piece, Periodogram);
                FFT_magnitude(Periodogram, small_psd);
                small_psd *= small_psd;
                small_psd *= small_Ydim * small_Xdim;
            }

#ifdef DEBUG
            save()=small_psd;
            save.write("PPPsmall_psd.xmp");
#endif

            // Add to the average
            psd += small_psd;
        }

    // Compute the average of all the small pieces and enlarge
    psd /= (Nside_piece * Nside_piece);

#ifdef DEBUG

    save()=psd;
    save.write("PPPpsd1.xmp");
#endif


    CenterFFT(psd, true);
    selfScaleToSize(BSPLINE3, psd, YSIZE(piece), XSIZE(piece));
    CenterFFT(psd, false);
    psd.threshold("below", 0, 0);

#ifdef DEBUG

    save()=psd;
    save.write("PPPpsd2.xmp");
    std::cout << "Press any key\n";
    char c;
    std::cin >> c;
#endif
}
#undef DEBUG

/* Main ==================================================================== */
//#define DEBUG
void Prog_assign_CTF_prm::process()
{
    // Open input files -----------------------------------------------------
    // Open coordinates
    std::ifstream PosFile; // File with picked coordinates
    if (picked_fn != "")
        PosFile.open(picked_fn.c_str());

    // Open selfile with images
    MetaData SF; // Selfile
    if (selfile_fn != "")
    {
        SF.read(selfile_fn);
        SF.removeObjects(MDValueEQ(MDL_ENABLED,-1));
    }

    // Open the selfile for the CTFs, if there is a selfile of particles
    FileName fn_root;
    if (!selfile_mode)
        fn_root = image_fn.withoutExtension();
    else
        fn_root = selfile_fn.withoutExtension();
    std::ofstream OutputFile_ctf;
    if (selfile_fn != "")
        OutputFile_ctf.open(
            (selfile_fn.withoutExtension() + ".ctfdat").c_str());

    // Open the micrograph
    Micrograph M_in;
    //bits is never used
    //**int bits; // Micrograph depth
    int Ydim, Xdim; // Micrograph dimensions
    if (!selfile_mode)
    {
        M_in.open_micrograph(image_fn/*, reversed*/);
        //**bits = M_in.depth();
        M_in.size(Xdim, Ydim);
    }

    // Compute the number of divisions --------------------------------------
    int div_Number = 0;
    int div_NumberX, div_NumberY;
    if (compute_at_particle)
    {
        // Check if sel file is empty
        if (SF.size() == 0)
        {
            REPORT_ERROR(ERR_MD_OBJECTNUMBER, (std::string)"Prog_assign_CTF_prm: sel file " + SF.getFilename() +
                         "is empty ");
        }

        // Count the number of lines in the Position file
        std::string line;
        PosFile.clear();
        PosFile.seekg(0, std::ios::beg);
        while (getline(PosFile, line))
            if (line[0] != '#')
                div_Number++;

        // check that the number of entries in the pos file is the right one
        if (SF.size() != div_Number)
        {
            std::cerr << "Prog_assign_CTF_prm: number of entries in "
            << "pos file: " << picked_fn.c_str()
            << "(" << div_Number << ") "
            << " and sel file "
            << SF.getFilename() << "(" << SF.size() << ") "
            << "is different.\n"
            << "I cannot go any further sorry\n";
            exit(1);
        }
    }
    else if (micrograph_averaging)
    {
        if (!selfile_mode)
        {
            // If averaging, allow overlap among pieces
            div_NumberX = CEIL((double)Xdim / (N_horizontal / 2)) - 1;
            div_NumberY = CEIL((double)Ydim / (N_vertical  / 2)) - 1;
            div_Number = div_NumberX * div_NumberY;
        }
        else
            div_Number = SF.size();
    }
    else
    {
        // If not averaging, do not allow overlap
        div_NumberX = CEIL((double)Xdim / N_horizontal);
        div_NumberY = CEIL((double)Ydim / N_vertical);
        div_Number = div_NumberX * div_NumberY;
    }

    // Process each piece ---------------------------------------------------
    PosFile.clear();
    PosFile.seekg(0, std::ios::beg); // Start of file
    MDIterator iter(SF);
    Image<double> psd_avg, psd_std;
    std::cerr << "Computing models of each piece ...\n";
    init_progress_bar(div_Number);

    // Prepare these filenames in case they are needed
    FileName fn_avg, fn_avg_model;
    if (micrograph_averaging && PSD_mode == ARMA)
        fn_avg = fn_root + "_ARMAavg.psd";
    else if (micrograph_averaging && PSD_mode == Periodogram)
        fn_avg = fn_root + "_Periodogramavg.psd";
    int N = 1;    // Index of current piece
    int i = 0, j = 0; // top-left corner of the current piece
    MultidimArray< std::complex<double> > Periodogram;
    MultidimArray<double> piece(N_vertical, N_horizontal);
    PCAMahalanobisAnalyzer pcaAnalyzer;
    MultidimArray<int> PCAmask;
    MultidimArray<float> PCAv;
    Image<double> psd, psd2;
    while (N <= div_Number)
    {
        // Compute the top-left corner of the piece ..........................
        if (compute_at_particle)
        {
            // Read position of the particle
            std::string line;
            getline(PosFile, line);
            while (line[0] == '#')
                getline(PosFile, line);
            float fi, fj;
            sscanf(line.c_str(), "%f %f", &fj, &fi);
            i = (int) fi;
            j = (int) fj;
#ifdef DEBUG

            std::cout << "line" << line << std::endl;
            std::cout << "read from file (j,i)= (" << j << "," << i << ")" << std::endl;
            FileName fnt;
            SF.getValue(MDL_IMAGE,fnt, iter.objId);
            std::cout << "Particle file name: " <<  fnt << std::endl;
#endif

            // j,i are the window center, we need the top-left corner
            j -= (int)(N_horizontal / 2);
            i -= (int)(N_vertical / 2);
            if (i < 0)
                i = 0;
            if (j < 0)
                j = 0;
            if (i > Ydim - N_horizontal)
                i = Ydim - N_horizontal - 1;
            if (j > Xdim - N_vertical)
                j = Xdim - N_vertical - 1;
        }
        else
        {
            if (!selfile_mode)
            {
                int Xstep = N_horizontal, Ystep = N_vertical;
                if (micrograph_averaging)
                {
                    Xstep /= 2;
                    Ystep /= 2;
                }
                i = ((N - 1) / div_NumberX) * Ystep;
                j = ((N - 1) % div_NumberX) * Xstep;
            }
        }

        // test if the full piece is inside the micrograph
        if (!selfile_mode)
        {
            if (i + N_vertical > Ydim)
                i = Ydim - N_vertical;
            if (j + N_horizontal > Xdim)
                j = Xdim - N_horizontal;
        }

        // Extract micrograph piece ..........................................
        if (!selfile_mode)
        {
            for (int k = 0; k < YSIZE(piece); k++)
                for (int l = 0; l < XSIZE(piece); l++)
                    DIRECT_A2D_ELEM(piece, k, l) = M_in(i+k, j+l);
        }
        else
        {
            Image<double> I;
            FileName fni;
            SF.getValue(MDL_IMAGE, fni, iter.objId);
            I.read(fni,true,-1,true);
            piece = I();
        }
        piece.statisticsAdjust(0, 1);

        // Estimate the power spectrum .......................................
        psd().resize(piece);
        if (!piece_averaging)
            if (PSD_mode == ARMA)
            {
                // Compute the ARMA model
                MultidimArray<double> ARParameters, MAParameters;
                double dSigma = CausalARMA(piece, ARMA_prm.N_AR, ARMA_prm.M_AR,
                                           ARMA_prm.N_MA, ARMA_prm.M_MA, ARParameters, MAParameters);
                ARMAFilter(piece, psd(), ARParameters, MAParameters, dSigma);
            }
            else
            {
                // Compute the periodogram
                FourierTransform(piece, Periodogram);
                FFT_magnitude(Periodogram, psd());
                psd() *= psd();
                psd() *= N_vertical * N_horizontal;
            }
        else
            PSD_piece_by_averaging(piece, psd());
        psd2()=psd();
        psd2()*=psd();

        // Perform averaging if applicable ...................................
        if (micrograph_averaging)
        {
            // Compute average and standard deviation
            if (N == 1)
            {
                psd_avg() = psd();
                psd_std() = psd2();
            }
            else
            {
                psd_avg() += psd();
                psd_std() += psd2();
            }

            // Keep psd for the PCA
            if (XSIZE(PCAmask)==0)
            {
                PCAmask.initZeros(psd());
                Matrix1D<int>    idx(2);  // Indexes for Fourier plane
                Matrix1D<double> freq(2); // Frequencies for Fourier plane
                int PCAdim=0;
                FOR_ALL_ELEMENTS_IN_ARRAY2D(PCAmask)
                {
                    VECTOR_R2(idx, j, i);
                    FFT_idx2digfreq(psd(), idx, freq);
                    double w = freq.module();
                    if (w>0.05 && w<0.4)
                    {
                        PCAmask(i,j)=1;
                        ++PCAdim;
                    }
                }
                PCAv.initZeros(PCAdim);
            }

            int ii=-1;
            FOR_ALL_ELEMENTS_IN_ARRAY2D(PCAmask)
            if (A2D_ELEM(PCAmask,i,j))
                A1D_ELEM(PCAv,++ii)=(float)psd(i,j);
            pcaAnalyzer.addVector(PCAv);

            // Write the PSD if ARMA
            if (micrograph_averaging && PSD_mode == ARMA)
            {
                psd_avg.write(fn_avg);
                if (N == 1)
                    system(((std::string)"xmipp_show -psd " +
                            fn_avg + " -poll &").c_str());
            }
        }

        // Compute the theoretical model if not averaging ....................
        if (!micrograph_averaging)
        {
            if (bootstrapN!=-1)
                REPORT_ERROR(ERR_VALUE_INCORRECT,
                             "Bootstrapping is only available for micrograph averages");

            FileName piece_fn, piece_fn_root;
            if (compute_at_particle)
            {
                SF.getValue(MDL_IMAGE, piece_fn, iter.objId);
                piece_fn_root = piece_fn.getBaseName();
                iter.moveNext();
            }
            else
                piece_fn_root = PSDfn_root + integerToString(N, 5);

            psd.write(piece_fn_root + ".psd");
            std::cerr << psd.name() << "\t top-left corner located at (X,Y)=("
            << j << "," << i << ")\n";

            if (!dont_adjust_CTF)
            {
                // Estimate the CTF parameters of this piece
                adjust_CTF_prm.fn_psd = piece_fn_root + ".psd";
                if (!dont_adjust_CTF)
                {
                    CTFDescription ctfmodel;
                    double fitting_error = ROUT_Adjust_CTF(adjust_CTF_prm,
                                                           ctfmodel, false);
                    if (compute_at_particle)
                        OutputFile_ctf << piece_fn << " "
                        << piece_fn_root+".psd\n";
                }
            }
        }

        // Increment the division counter
        progress_bar(++N);
        if (selfile_mode)
            iter.moveNext();
    }
    M_in.close_micrograph();
    progress_bar(div_Number);

    // If averaging, compute the CTF model ----------------------------------
    if (micrograph_averaging)
    {
        // Compute the avg and stddev of the local PSDs
        psd_avg() /= div_Number;
        psd_avg.write(fn_avg);

        psd_std() /= div_Number;
        psd_std() -= psd_avg()*psd_avg();
        FOR_ALL_ELEMENTS_IN_ARRAY2D(psd_std())
        if (psd_std(i,j)<0)
            psd_std(i,j)=0;
        else
            psd_std(i,j)=sqrt(psd_std(i,j));

        if (!dont_adjust_CTF)
        {
            // Estimate the CTF parameters
            std::cerr << "Adjusting CTF model to the PSD ...\n";
            adjust_CTF_prm.fn_psd = fn_avg;
            CTFDescription ctfmodel;
            if (bootstrapN==-1)
            {
                // Compute the PCA of the local PSDs
                pcaAnalyzer.standardarizeVariables();
                // pcaAnalyzer.subtractAvg();
                pcaAnalyzer.learnPCABasis(1,10);

#ifdef DEBUG

                Image<double> save;
                save().initZeros(psd());
                int ii=-1;
                FOR_ALL_ELEMENTS_IN_ARRAY2D(PCAmask)
                if (PCAmask(i,j))
                    save(i,j)=pcaAnalyzer.PCAbasis[0](++ii);
                save.write("PPPbasis.xmp");
#endif

                Matrix2D<double> CtY;
                pcaAnalyzer.projectOnPCABasis(CtY);
                Matrix1D<double> p;
                CtY.toVector(p);
                double pavg=p.sum(true);
                double pstd=p.sum2()/VEC_XSIZE(p)-pavg*pavg;
                pstd=(pstd<0)?0:sqrt(pstd);

                std::string psign;
                FOR_ALL_ELEMENTS_IN_MATRIX1D(p)
                if (p(i)<0)
                    psign+="-";
                else
                    psign+="+";
                double zrandomness=checkRandomness(psign);

                double fitting_error = ROUT_Adjust_CTF(adjust_CTF_prm,
                                                       ctfmodel, false);

                // Evaluate PSD variance and write into the CTF
                double stdQ=0;
                FOR_ALL_ELEMENTS_IN_ARRAY2D(psd_std())
                stdQ+=psd_std(i,j)/psd_avg(i,j);
                stdQ/=MULTIDIM_SIZE(psd_std());

                MetaData MD;
                MD.read(fn_avg.withoutExtension() + ".ctfparam");
                MD.setValue(MDL_CTF_CRITERION_PSDVARIANCE,stdQ);
                MD.setValue(MDL_CTF_CRITERION_PSDPCA1VARIANCE,pstd);
                MD.setValue(MDL_CTF_CRITERION_PSDPCARUNSTEST,zrandomness);
                MD.write(fn_avg.withoutExtension() + ".ctfparam");
            }
            else
            {
                MultidimArray<double> CTFs(bootstrapN,32);
                adjust_CTF_prm.bootstrap=true;
                adjust_CTF_prm.show_optimization=true;
                FileName fnBase=fn_avg.withoutExtension();
                std::cerr << "Computing bootstrap ...\n";
                init_progress_bar(bootstrapN);
                for (int n=0; n<bootstrapN; n++)
                {
                    CTFs(n,31) = ROUT_Adjust_CTF(adjust_CTF_prm,
                                                 ctfmodel, false);
                    CTFs(n, 0)=ctfmodel.Tm;
                    CTFs(n, 1)=ctfmodel.kV;
                    CTFs(n, 2)=ctfmodel.DeltafU;
                    CTFs(n, 3)=ctfmodel.DeltafV;
                    CTFs(n, 4)=ctfmodel.azimuthal_angle;
                    CTFs(n, 5)=ctfmodel.Cs;
                    CTFs(n, 6)=ctfmodel.Ca;
                    CTFs(n, 7)=ctfmodel.espr;
                    CTFs(n, 8)=ctfmodel.ispr;
                    CTFs(n, 9)=ctfmodel.alpha;
                    CTFs(n,10)=ctfmodel.DeltaF;
                    CTFs(n,11)=ctfmodel.DeltaR;
                    CTFs(n,12)=ctfmodel.Q0;
                    CTFs(n,13)=ctfmodel.K;
                    CTFs(n,14)=ctfmodel.gaussian_K;
                    CTFs(n,15)=ctfmodel.sigmaU;
                    CTFs(n,16)=ctfmodel.sigmaV;
                    CTFs(n,17)=ctfmodel.cU;
                    CTFs(n,18)=ctfmodel.cV;
                    CTFs(n,19)=ctfmodel.gaussian_angle;
                    CTFs(n,20)=ctfmodel.sqrt_K;
                    CTFs(n,21)=ctfmodel.sqU;
                    CTFs(n,22)=ctfmodel.sqV;
                    CTFs(n,23)=ctfmodel.sqrt_angle;
                    CTFs(n,24)=ctfmodel.base_line;
                    CTFs(n,25)=ctfmodel.gaussian_K2;
                    CTFs(n,26)=ctfmodel.sigmaU2;
                    CTFs(n,27)=ctfmodel.sigmaV2;
                    CTFs(n,28)=ctfmodel.cU2;
                    CTFs(n,29)=ctfmodel.cV2;
                    CTFs(n,30)=ctfmodel.gaussian_angle2;

                    std::string command=(std::string)"mv -i "+fnBase+
                                        ".ctfparam "+fnBase+"_bootstrap_"+
                                        integerToString(n,4)+".ctfparam";
                    system(command.c_str());
                    command=(std::string)"mv -i "+fnBase+
                            ".ctfmodel_quadrant "+fnBase+"_bootstrap_"+
                            integerToString(n,4)+".ctfmodel_quadrant";
                    system(command.c_str());
                    command=(std::string)"mv -i "+fnBase+
                            ".ctfmodel_halfplane "+fnBase+"_bootstrap_"+
                            integerToString(n,4)+".ctfmodel_halfplane";
                    system(command.c_str());

                    progress_bar(n);
                }
                progress_bar(bootstrapN);
                //CTFs.write("bootstrap.txt");
            }
        }
    }

    // Assign a CTF to each particle ----------------------------------------
    if (!compute_at_particle && selfile_fn != "" && !dont_adjust_CTF)
    {
        // Process the Selfile
        if (!selfile_mode)
        {
            PosFile.close();
            PosFile.open(picked_fn.c_str());
            if (!PosFile)
                REPORT_ERROR(ERR_IO_NOTOPEN, (std::string)"Prog_assign_CTF_prm::process: Could not open " +
                             picked_fn + " for reading");
        }
        FOR_ALL_OBJECTS_IN_METADATA(SF)
        {
            FileName fn_img;
            SF.getValue(MDL_IMAGE, fn_img, __iter.objId);
            if (!selfile_mode)
            {
                std::string line;
                getline(PosFile, line);
                while (line[0] == '#')
                    getline(PosFile, line);
                if (!micrograph_averaging)
                {
                    // Read coordinates of the particle
                    float fX, fY;
                    sscanf(line.c_str(), "%f %f", &fX, &fY);
                    int Y = (int) fY;
                    int X = (int) fX;

                    // Decide which is its piece
                    int idx_X = FLOOR((double)X / N_horizontal);
                    int idx_Y = FLOOR((double)Y / N_vertical);
                    int idx_piece = idx_Y * div_NumberX + idx_X + 1;
                    OutputFile_ctf << fn_img << " "
                    << PSDfn_root + integerToString(idx_piece, 5) + ".psd\n";
                }
                else
                    OutputFile_ctf << fn_img << " "
                    << fn_avg.withoutExtension() + ".psd\n";

            }
            else
            {
                OutputFile_ctf << fn_img << " "
                << fn_avg.withoutExtension() + ".psd\n";
            }
        }
    }
    if (selfile_fn != "")
        OutputFile_ctf.close();
    PosFile.close();
}
