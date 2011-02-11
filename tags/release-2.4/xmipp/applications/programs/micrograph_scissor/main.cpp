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

#include <data/micrograph.h>
#include <data/args.h>

void Usage();

int main(int argc, char **argv)
{
    FileName fn_orig, fn_micrograph, fn_pos, fn_root, fn_transform;
    FileName fn_tilted, fn_root_tilted;
    int      Ydim, Xdim;
    int      startN;
    bool     reverse_endian;
    bool     compute_transmitance = false;
    bool     compute_inverse = false;
    double   alpha, down_transform;
    bool     pair_mode;
    Matrix2D<double> Mtransform(3,3);
    try
    {
        fn_micrograph = getParameter(argc, argv, "-i");
        pair_mode = checkParameter(argc, argv, "-tilted");
        fn_root       = getParameter(argc, argv, "-root");
        Xdim          = textToInteger(getParameter(argc, argv, "-Xdim"));
        if (checkParameter(argc, argv, "-Ydim"))
            Ydim       = textToInteger(getParameter(argc, argv, "-Ydim"));
        else Ydim = Xdim;
        startN        = textToInteger(getParameter(argc, argv, "-start", "1"));
        reverse_endian = checkParameter(argc, argv, "-reverse_endian");
        compute_inverse = checkParameter(argc, argv, "-invert");
        compute_transmitance = checkParameter(argc, argv, "-log");
        if (!pair_mode)
        {
            fn_pos        = getParameter(argc, argv, "-pos");
            fn_orig       = getParameter(argc, argv, "-orig", "");
            alpha         = textToFloat(getParameter(argc, argv, "-alpha", "0"));
        }
        else
        {
            fn_root_tilted = getParameter(argc, argv, "-root_tilted");
            fn_tilted     = getParameter(argc, argv, "-tilted");
        }
        fn_transform = getParameter(argc, argv, "-transform","");
	down_transform = textToFloat(getParameter(argc, argv, "-down_transform","1"));
    }
    catch (Xmipp_error XE)
    {
        std::cout << XE;
        Usage();
        exit(1);
    }
    try
    {
        if (!pair_mode)
        {
            Micrograph m;
            m.open_micrograph(fn_micrograph, reverse_endian);
            m.set_window_size(Xdim, Ydim);
            m.read_coordinates(0, fn_pos);
	    if (fn_transform!="")
	    {
		if (down_transform != 1.)
		    m.scale_coordinates(1./down_transform);
		std::ifstream fh_transform;
		fh_transform.open(fn_transform.c_str());
		if (!fh_transform)
		    REPORT_ERROR(1, (std::string)"Scissor: Cannot open file" + fn_transform);
		fh_transform >> Mtransform;
		fh_transform.close();
		m.transform_coordinates(Mtransform);
		if (down_transform != 1.)
		    m.scale_coordinates(down_transform);
	    }
            m.add_label("");
            m.set_transmitance_flag(compute_transmitance);
            m.set_inverse_flag(compute_inverse);
            m.produce_all_images(0, -1, fn_root, startN, fn_orig, alpha);
        }
        else
        {
            // Read angles
            FileName fn_ang = fn_micrograph.without_extension();
            fn_ang = fn_ang.add_extension("ang");
            std::ifstream fh_ang;
            fh_ang.open(fn_ang.c_str());
            if (!fh_ang)
                REPORT_ERROR(1, (std::string)"Scissor: Cannot open file" + fn_ang);
            std::string aux;
            getline(fh_ang, aux);
            double alpha_u, alpha_t, tilt_angle;
            fh_ang >> alpha_u >> alpha_t >> tilt_angle;
            fh_ang.close();

            // Generate the images for the untilted image
            Micrograph m;
            m.open_micrograph(fn_micrograph, reverse_endian);
            m.set_window_size(Xdim, Ydim);
            m.read_coordinates(0, fn_micrograph.add_extension("Common.pos"));
            m.add_label("");
            m.set_transmitance_flag(compute_transmitance);
            m.set_inverse_flag(compute_inverse);
            m.produce_all_images(0, -1, fn_root, startN, "", alpha_u);
            m.close_micrograph();

            // Generate the images for the tilted image
            Micrograph mt;
            mt.open_micrograph(fn_tilted, reverse_endian);
            mt.set_window_size(Xdim, Ydim);
            mt.read_coordinates(0, fn_tilted.add_extension("Common.pos"));
            mt.add_label("");
            mt.set_transmitance_flag(compute_transmitance);
            mt.set_inverse_flag(compute_inverse);
            mt.produce_all_images(0, -1, fn_root_tilted, startN, "", 0., tilt_angle, alpha_t);
            mt.close_micrograph();
        }
    }
    catch (Xmipp_error XE)
    {
        std::cout << XE;
    }
}

void Usage()
{
    std::cerr << "Purpose: Cut the images marked with xmipp_mark\n"
    << "Usage: scissor [options]\n"
    << "For single images -------------------------\n"
    << "   -i <input micrograph>      : From which the images will be cutted\n"
    << "  [-orig <original micrograph>: unless this parameter is specified\n"
    << "   -root <root name>          : for the cutted images\n"
    << "   -pos <position file>       : order X,Y\n"
    << "                                from transmitance\n"
    << "  [-transform <.mat-file>]    : transform all coordinates according to this 3x3 matrix\n"
    << "  [-down_transform <int=1>]   : the transformation matrix was determined with this downsampling rate\n"
    << "  [-alpha <ang>]              : Angle from Y axis to tilt axis\n"
    << "                                as it comes out from xmipp_mark\n"
    << "For image pairs ---------------------------\n"
    << "   -i <untilted micrograph>   : From which the images will be cutted\n"
    << "   -tilted <tilted micrograph>: From which the images will be cutted\n"
    << "   -root <root name>          : for the cutted images\n"
    << "   -root_tilted <root name>   : for the tilted cutted images\n"
    << "For both ----------------------------------\n"
    << "   -Xdim <window X dim>       : in pixels\n"
    << "  [-Ydim <window Y dim>]      : if not given Ydim=Xdim\n"
    << "  [-start <N=1>]              : Number of the first image\n"
    << "  [-reverse_endian]           : of the input micrographs\n"
    << "  [-invert]                   : Invert contrast\n"
    << "  [-log]                      : Compute optical density\n"
    ;
}

/* Colimate menu =========================================================== */
/*Colimate:
   PROGRAM Scissor {
      url="http://www.cnb.uam.es/~bioinfo/NewXmipp/Applications/Src/Scissor/Help/scissor.html";
      help="Cut particles from raw micrographs";
      OPEN MENU Scissor;
      COMMAND LINES {
         + usual: xmipp_scissor -i $FILEIN [-orig $FILEORIG]
                   -root $ROOT -pos $POSFILE
                   -Xdim $XDIM [-Ydim $YDIM]
                  [-start $START] [-reverse_endian]
    [-alpha $ALPHA]
      }
      PARAMETER DEFINITIONS {
         $FILEIN {
            label="Input micrograph";
            help="Raw file";
            type=file existing;
         }
         $FILEORIG {
            label="Original micrograph";
            help="If supplied, particles will be cut from this file
                  although the positions are referrend to the Input
                  micrograph";
            type=file existing;
         }
         $ROOT {
            label="Output file root name";
            help="Cut images will start with this name";
            type=text;
         }
         $POSFILE {
            label="File with positions";
            help="Positions referred to the Input micrograph";
            type=file existing;
         }
         $XDIM {
            label="Output X dim for particles";
            type=natural;
         }
         $YDIM {
            label="Output Y dim for particles";
            type=natural;
            by default=$XDIM;
         }
         $START {
            label="Start numbering images at";
            type=natural;
            by default=1;
         }
         OPT(-reverse_endian) {label="Reverse endian";}
         $ALPHA {
            label="Angle from Y axis to tilt axis";
            type=float;
     help="As it comes out from xmipp_mark";
         }
      }
   }
   MENU Scissor {
      "I/O parameters"
      $FILEIN
      OPT($FILEORIG)
      $ROOT
      $POSFILE
      "Cut images definition"
      $XDIM
      OPT($YDIM)
      OPT($START)
      OPT(-reverse_endian)
      OPT($ALPHA)
   }
*/

