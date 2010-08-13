/***************************************************************************
 *
 * Authors:    Carlos Oscar           coss@cnb.csic.es (2010)
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
#ifndef _PROG_SORT_IMAGES
#define _PROG_SORT_IMAGES

#include <data/funcs.h>
#include <data/metadata.h>
#include <data/image.h>

/**@defgroup SortImagesProgram sort images
   @ingroup ReconsLibrary */
//@{
/** Sort images parameters. */
class Prog_sort_images_prm
{
public:
    /** Filename selection file containing the images */
    FileName fnSel;

    /**  Filename output rootname */
    FileName fnRoot;

    /** Also process the corresponding selfiles */
    bool processSelfiles;
public:
    // Output selfile
    MetaData SFout;

    // Output selfile
    MetaData SFoutOriginal;

    // SelFile images
    std::vector< FileName > toClassify;

    // Image holding current reference
    Image<double> lastImage;

    // Mask of the background
    MultidimArray<int> mask;
public:
    /// Read argument from command line
    void read(int argc, char **argv);

    /// Show
    void show();

    /// Usage
    void usage();

    /// Produce side info
    void produceSideInfo();

    /// Choose next image
    void chooseNextImage();

    /// Main routine
    void run();
};
//@}
#endif
