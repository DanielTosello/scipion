/***************************************************************************
 *
 * Authors:     Alberto Pascual Montano (pascual@cnb.csic.es)
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

#include <data/image.h>
#include <data/args.h>
#include <data/metadata.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>

int main(int argc, char **argv)
{
    FILE *fp;
    float tmpR;
    char *fname, *iname, *bmname;
    std::string selname;
    Image<double> mask;
    std::vector < std::vector <float> > dataPoints;
    std::vector < std::string > labels;
    bool nomask = false;
    bool verb = false;


    // Read arguments

    try
    {
        selname = getParameter(argc, argv, "-sel");
        fname = getParameter(argc, argv, "-fname", "out.dat");
        bmname = getParameter(argc, argv, "-mname", "mask.spi");
        if (checkParameter(argc, argv, "-nomask"))
            nomask = true;
        if (checkParameter(argc, argv, "-verb"))
            verb = true;
    }
    catch (XmippError)
    {
        std::cout << "img2data: Convert a set of volumes into a set of data vectors" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "-sel           : Sel file name" << std::endl;
        std::cout << "-mname         : Input Mask file name (default: mask.spi)" << std::endl;
        std::cout << "[-nomask]      : set if the mask is not going to be used" << std::endl;
        std::cout << "[-fname]       : Output file name (default: out.dat)" << std::endl;
        std::cout << "[-verb]        : Verbosity (default: false)" << std::endl;
        exit(1);
    }


    std::cout << "Given parameters are: " << std::endl;
    std::cout << "sel = " << selname << std::endl;
    if (!nomask)
    {
        std::cout << "mname = " << bmname << std::endl;
    }
    else
        std::cout << "No mask is going to be used" << std::endl;
    std::cout << "fname = " << fname << std::endl;


    // Read spider mask

    if (!nomask)
    {
        std::cout << std::endl << "reading mask " << bmname << "......" << std::endl << std::endl;
        mask.read(bmname);        // Reads the mask
        //Adjust the range to 0-1
        mask().rangeAdjust(0, 1); // just in case
        std::cout << mask;    // Output Volumen Information
    }

    std::cout << "generating data......" << std::endl;

    MetaData SF((FileName) selname);
    // Read Sel file
    FOR_ALL_OBJECTS_IN_METADATA(SF)
    {
        std::string image_name;
        SF.getValue(MDL_IMAGE,image_name,__iter.objId);
        if (verb)
            std::cout << "generating points for image " << image_name << "......" << std::endl;
        Image<double> image;
        image.readApplyGeo(image_name,SF,__iter.objId); // reads image

        // Extract the data
        image().setXmippOrigin();  // sets origin at the center of the image.
        mask().setXmippOrigin();   // sets origin at the center of the mask.

        // Generates coordinates (data points)
        std::vector <float> imagePoints;
        for (int z = STARTINGZ(image()); z <= FINISHINGZ(image()); z++)
            for (int y = STARTINGY(image()); y <= FINISHINGY(image()); y++)
                for (int x = STARTINGX(image()); x <= FINISHINGX(image()); x++)
                {
                    // Checks if voxel is different from zero (it's inside the binary mask)
                    bool cond;
                    if (!nomask)
                        cond = mask(z, y, x) != 0;
                    else
                        cond = true;
                    if (cond)
                    {
                        imagePoints.push_back(image(z, y, x));
                    }
                } // for x
        labels.push_back(image_name);
        dataPoints.push_back(imagePoints);
    } // while

    std::cout << std::endl << "Saving points......" << std::endl;
    fp = fopen(fname, "w");
    fprintf(fp, "%d %d\n", dataPoints[0].size(), dataPoints.size()); // Save dimension
    for (unsigned i = 0; i < dataPoints.size(); i++)
    {
        for (unsigned j = 0; j < dataPoints[0].size(); j++)
            fprintf(fp, "%3.3f ", dataPoints[i][j]);
        fprintf(fp, "%s \n", labels[i].c_str());
    }
    fclose(fp);    // close file
    exit(0);
}


