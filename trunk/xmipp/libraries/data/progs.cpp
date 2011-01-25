/***************************************************************************
 *
 * Authors:     Carlos Oscar S. Sorzano (coss@cnb.csic.es)
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
#include "progs.h"
#include "metadata.h"
#include "metadata_extension.h"
#include "args.h"

/* Common functions -------------------------------------------------------- */
void Prog_parameters::read(int argc, char **argv)
{
    fn_in  = getParameter(argc, argv, "-i");
    fn_out = getParameter(argc, argv, "-o", "");
    oext   = getParameter(argc, argv, "-oext", "");
    oroot  = getParameter(argc, argv, "-oroot", "");
    quiet  = checkParameter(argc, argv, "-quiet");
    // For each_image_produces_an_output there exists no possibility to apply_geo
    // This because it would require a back-transformation, which deteriorates the images
    if (!each_image_produces_an_output)
    {
        apply_geo = !checkParameter(argc, argv, "-dont_apply_geo");
    }
}

void Prog_parameters::show()
{
    if (quiet)
        return;
    std::cout << "Input File: " << fn_in << std::endl;
    /////////////////FIXME!!
    //if (apply_geo && !Is_VolumeXmipp(fn_in))
    if (apply_geo)
        std::cout << "Applying transformation stored in header of 2D-image" << std::endl;
    if (each_image_produces_an_output)
    {
        if (fn_out != "")
            std::cout << "Output File: " << fn_out << std::endl;
        if (oext != "")
            std::cout << "Output Extension: " << oext << std::endl;
        if (oroot != "")
            std::cout << "Output Root: " << oroot << std::endl;
    }
}

void Prog_parameters::usage()
{
    std::cerr << "   -i <input file>          : either an image/volume or a metadata file\n";
    if (each_image_produces_an_output)
    {
        std::cerr << "  [-o <output file>]        : if wanted in case of a single image\n"
        << "  [-oext <extension>]       : if wanted in case of a metadata file\n"
        << "  [-oroot <root>]           : if wanted in case of a metadata file\n"
        << "  [-quiet]                  : do not show anything on screen\n";
    }
    else
    {
        std::cerr  << "  [-dont_apply_geo]         : for 2D-images: do not apply transformation stored in the header\n";
    }
}

void Prog_parameters::get_input_size(int &Zdim, int &Ydim, int &Xdim)
{
    Image<double> I;
    int dum;
    unsigned long dum2;
    if (fn_in.isMetaData())
    {
        MetaData SF;
        SF.read(fn_in);
        ImgSize(SF, Xdim, Ydim, dum, dum2);
        Zdim = 1;
    }
    else
    {
        I.read2(fn_in);
        I.getDimensions(Xdim, Ydim, Zdim, dum2);
    }
}

int Prog_parameters::get_images_to_process()
{
    if (fn_in.isMetaData())
    {
        MetaData SF;
        SF.read(fn_in);
        SF.removeObjects(MDValueEQ(MDL_ENABLED, -1));
        return SF.size();
    }
    else
        return 1;
}

/* With arguments ---------------------------------------------------------- */
void SF_main(int argc, char **argv,
             Prog_parameters *prm,
             void *process_img, int operation_mode)
{
    // Some variables .......................................................
    Image<double> img, orig_img;
    Image<std::complex<double> > IMG;
    MetaData SF_in, SF_out;

    bool(*fi2i)(Image<double> &, const Prog_parameters *) = NULL;
    bool(*fi2I)(Image<double> &, Image<std::complex<double> > &, const Prog_parameters *) = NULL;
    bool(*fI2i)(Image<std::complex<double> > &, Image<double> &, const Prog_parameters *) = NULL;
    bool(*fI2I)(Image<std::complex<double> > &, const Prog_parameters *) = NULL;
    bool(*fi2F)(Image<double> &, const FileName &, const Prog_parameters *) = NULL;
    bool(*fF2F)(const FileName &, const FileName &, const Prog_parameters *) = NULL;

    // Read command line ....................................................
    try
    {
        (*(Prog_parameters *)prm).read(argc, argv);
    }
    catch (XmippError XE)
    {
        std::cerr << XE;
        std::cerr << "Usage: \n"
        << "   " << argv[0] << " [options]\n";
        prm->usage();
        exit(1);
    }
    prm->show();

    try
    {
        if (!exists(prm->fn_in))
            REPORT_ERROR(ERR_IO_NOTEXIST,
                         (std::string)argv[0] + ": " + prm->fn_in + " doesn't exist");
        bool success;

        FileName fn_out;
        if (prm->fn_out == "")
            fn_out = prm->fn_in;
        else
            fn_out = prm->fn_out;

        if (prm->fn_in.isMetaData())
            // For a selection file .................................................
        {
            SF_in.read(prm->fn_in);

            // Initialise progress bar
            time_config();
            int i = 0;
            if (prm->allow_time_bar && !prm->quiet)
                init_progress_bar(SF_in.size());
            int istep = CEIL((double)SF_in.size() / 60.0);

            // Process all metadatafile
            FOR_ALL_OBJECTS_IN_METADATA(SF_in)
            {
                FileName fn_read;
                SF_in.getValue( MDL_IMAGE, fn_read);
                if (prm->each_image_produces_an_output)
                    if (prm->oext == "" && prm->oroot == "")
                        prm->fn_out = fn_read;
                    else if (prm->oroot != "")
                    {
                        prm->fn_out = prm->oroot + fn_read.withoutRoot();
                        if (operation_mode == IMAGE2FOURIER)
                            if (fn_read.getExtension() == "xmp")
                                prm->fn_out = prm->fn_out.withoutExtension() + ".fft";
                            else
                                prm->fn_out = prm->fn_out.withoutExtension() + ".fft3";
                        if (operation_mode == FOURIER2IMAGE)
                            if (fn_read.getExtension() == "fft")
                                prm->fn_out = prm->fn_out.withoutExtension() + ".xmp";
                            else
                                prm->fn_out = prm->fn_out.withoutExtension() + ".vol";
                    }
                    else
                    {
                        prm->fn_out = fn_read.withoutExtension() + "." + prm->oext;
                    }

                if (img.isRealImage(fn_read))
                {
                  MDRow row;
                  SF_in.getRow(row);
                    img.read(fn_read, true, -1, prm->apply_geo, false, &row);
                    img().setXmippOrigin();
                    switch (operation_mode)
                    {
                    case IMAGE2IMAGE:
                        fi2i = (bool(*)(Image<double> &, const Prog_parameters *)) process_img;
                        success = fi2i(img, prm);
                        if (prm->each_image_produces_an_output)
                        {
                            /*
                            if (prm->apply_geo) {
                              std::cerr << "BUG: apply_geo and each_image_produces_an_output should not co-exist"
                             << " the only exception is the apply_geo program";
                        }
                            */
                            if (prm->fn_out.isInStack())
                            {
                                int no;
                                std::string filename;
                                prm->fn_out.decompose(no,filename);
                                img.write(filename,no,true,WRITE_REPLACE);
                            }
                            else
                                img.write(prm->fn_out);
                        }
                        break;
                    case IMAGE2FOURIER:
                        fi2I = (bool(*)(Image<double> &, Image<std::complex<double> > &, const Prog_parameters *)) process_img;
                        success = fi2I(img, IMG, prm);
                        if (prm->each_image_produces_an_output)
                            IMG.write(prm->fn_out);
                        break;
                    case IMAGE2FILE:
                        fi2F = (bool(*)(Image<double> &, const FileName &, const Prog_parameters *)) process_img;
                        success = fi2F(img, prm->fn_out, prm);
                        break;
                    }
                }
                else if (IMG.isComplexImage(fn_read))
                {
                    IMG.read2(fn_read);
                    IMG().setXmippOrigin();
                    switch (operation_mode)
                    {
                    case FOURIER2FOURIER:
                        fI2I = (bool(*)(Image<std::complex<double> > &, const Prog_parameters *)) process_img;
                        success = fI2I(IMG, prm);
                        if (prm->each_image_produces_an_output)
                            IMG.write(prm->fn_out);
                        break;
                    case FOURIER2IMAGE:
                        fI2i = (bool(*)(Image<std::complex<double> > &, Image<double> &, const Prog_parameters *)) process_img;
                        success = fI2i(IMG, img, prm);
                        if (prm->each_image_produces_an_output)
                            img.write(prm->fn_out);
                        break;
                    }
                }
                else if (operation_mode == FILE2FILE)
                {
                    fF2F = (bool(*)(const FileName &, const FileName &, const Prog_parameters *)) process_img;
                    success = fF2F(fn_read, prm->fn_out, prm);
                }

                if (prm->each_image_produces_an_output)
                {
                    SF_out.addObject();
                    SF_out.setValue( MDL_IMAGE, prm->fn_out);
                    if (success)
                        SF_out.setValue( MDL_ENABLED, 1);
                    else
                        SF_out.setValue( MDL_ENABLED,-1);
                }

                if (i++ % istep == 0 && prm->allow_time_bar && !prm->quiet)
                    progress_bar(i);
            }

            if (prm->allow_time_bar && !prm->quiet)
                progress_bar(SF_in.size());
            if (prm->each_image_produces_an_output)
                if (prm->oext != "")
                {
                    prm->fn_out = prm->fn_in.insertBeforeExtension(prm->oext);
                    SF_out.write(prm->fn_out);
                }
        }
        else
        {
            if (img.isRealImage(prm->fn_in))
            {
                // For single image .....................................................
                img.read(prm->fn_in, true, -1, prm->apply_geo);
                img().setXmippOrigin();
                switch (operation_mode)
                {
                case IMAGE2IMAGE:
                    fi2i = (bool(*)(Image<double> &, const Prog_parameters *)) process_img;
                    success = fi2i(img, prm);
                    if (prm->each_image_produces_an_output)
                    {
                        /*       if (prm->apply_geo) {
                          std::cerr << "BUG: apply_geo and each_image_produces_an_output should not co-exist"
                            << " the only exception is the apply_geo program";
                            }
                        */
                        img.write(fn_out);
                    }
                    break;
                case IMAGE2FOURIER:
                    fi2I = (bool(*)(Image<double> &, Image<std::complex<double> > &, const Prog_parameters *)) process_img;
                    success = fi2I(img, IMG, prm);
                    if (prm->each_image_produces_an_output)
                        IMG.write(fn_out);
                    break;
                case IMAGE2FILE:
                    fi2F = (bool(*)(Image<double> &, const FileName &, const Prog_parameters *)) process_img;
                    success = fi2F(img, fn_out, prm);
                    break;
                }
            }
            else if (IMG.isComplexImage(prm->fn_in))
            {
                IMG.read2(prm->fn_in);
                IMG().setXmippOrigin();
                switch (operation_mode)
                {
                case FOURIER2FOURIER:
                    fI2I = (bool(*)(Image<std::complex<double> > &, const Prog_parameters *)) process_img;
                    success = fI2I(IMG, prm);
                    if (prm->each_image_produces_an_output)
                        IMG.write(fn_out);
                    break;
                case FOURIER2IMAGE:
                    fI2i = (bool(*)(Image<std::complex<double> > &, Image<double> &, const Prog_parameters *)) process_img;
                    success = fI2i(IMG, img, prm);
                    if (prm->each_image_produces_an_output)
                        img.write(fn_out);
                    break;
                }
            }
        }
    }
    catch (XmippError XE)
    {
        std::cout << XE;
        exit(1);
    }
}


