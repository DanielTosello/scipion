/***************************************************************************
 *
 * Authors:    Joaquin Oton                (joton@cnb.csic.es)
 *             J.M.de la Rosa Trevin       (jmdelarosa@cnb.csic.es)
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
 * MERCHANTABILITY or FITNESS FO A PARTICULAR PURPOSE.  See the
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

#include <data/xmipp_image.h>
#include <data/metadata.h>
#include <data/xmipp_program.h>

typedef enum { HEADER_PRINT, HEADER_EXTRACT, HEADER_ASSIGN, HEADER_RESET } HeaderOperation;

class ProgHeader: public XmippMetadataProgram
{
protected:
    HeaderOperation operation;
    bool round_shifts;
    MDRow row;

    void defineParams()
    {
        produces_an_output = true;
        XmippMetadataProgram::defineParams();
        addUsageLine("Operate with image files headers. By default in Xmipp, geometrical transformations");
        addUsageLine("comming in images files headers are ignored. Instead this information is read from");
        addUsageLine("the images metadata if exist. With this program geometrical transformations can be");
        addUsageLine("extracted to a metadata or assigned to header, also allows print or reset image file headers.");
        addParamsLine("[   --print <decompose=0>]    : Print the geometrical transformations in image file headers.");
        addParamsLine("                              : if input is stack and decompose=1 print header of each individual image.");
        addParamsLine("       alias -p;");
        addParamsLine("or --extract     : The output is a selfile with geometrical transformations read from image file headers.");
        addParamsLine("       alias -e;");
        addParamsLine("      requires -o;");
        addParamsLine("or --assign     : Write the geometrical transformations from selfile to the image file headers.");
        addParamsLine("       alias -a;");
        addParamsLine("or --reset      : Reset the geometrical transformations in image file headers.");
        addParamsLine("       alias -r;");
        addParamsLine("   [--round_shifts]    :Round shifts to integers");
        addExampleLine("Print the header of the images in metadata: ", false);
        addExampleLine("xmipp_header -i images.sel");
        addExampleLine("Extract geometrical transformations from image file headers: ", false);
        addExampleLine("xmipp_header -i smallStack.stk --extract -o header.doc");
        addExampleLine("Assign the geometrical transformations from the metadata to header: ", false);
        addExampleLine("xmipp_header -i header.doc --assign");
        addSeeAlsoLine("transform_geometry");
        addKeywords("header, geometric, transformation, print");
    }

    void readParams()
    {
        if (checkParam("--extract"))
            operation = HEADER_EXTRACT;
        else if (checkParam("--assign"))
            operation = HEADER_ASSIGN;
        else if (checkParam("--reset"))
            operation = HEADER_RESET;
        else
        {
            operation = HEADER_PRINT;
            allow_time_bar = false;
            decompose_stacks = getIntParam("--print") == 1;
        }
        XmippMetadataProgram::readParams();
        round_shifts = checkParam("--round_shifts");
        if (operation != HEADER_EXTRACT && checkParam("-o"))
          REPORT_ERROR(ERR_PARAM_INCORRECT, "Argument -o is not valid for this operation");
    }

    void show()
    {
        String msg;
        switch (operation)
        {
        case HEADER_PRINT:
            msg = "Printing headers...";
            break;
        case HEADER_EXTRACT:
            msg = "Extracting image(s) geometrical transformations from header to metadata...";
            break;
        case HEADER_ASSIGN:
            msg = "Assigning image(s) geometrical transformations from metadata to header...";
            break;
        case HEADER_RESET:
            msg = "Reseting geometrical transformations from headers...";
            break;
        }
        std::cout << msg << std::endl << "Input: " << fn_in << std::endl;

        if (checkParam("-o"))
            std::cout << "Output: " << fn_out << std::endl;

    }

    void roundShifts()
    {
        double aux = 0.;
        if (row.getValue(MDL_SHIFTX, aux))
        {
            aux = (double)ROUND(aux);
            row.setValue(MDL_SHIFTX, aux);
        }
        if (row.getValue(MDL_SHIFTY, aux))
        {
            aux = (double)ROUND(aux);
            row.setValue(MDL_SHIFTY, aux);
        }
    }

    void processImage(const FileName &fnImg, const FileName &fnImgOut, size_t objId)
    {

        ImageGeneric img;

        switch (operation)
        {
        case HEADER_PRINT:
            img.read(fnImg, _HEADER_ALL);
            img.print();
            break;
        case HEADER_EXTRACT:
            img.read(fnImg, _HEADER_ALL);
            row = img.getGeometry();
            if (round_shifts)
                roundShifts();
            mdIn.setRow(row, objId);
            break;
        case HEADER_ASSIGN:
            mdIn.getRow(row, objId);
            if (round_shifts)
                roundShifts();
            img.readApplyGeo(fnImg, row, HEADER);
            img.setDataMode(_HEADER_ALL);
            img.write(fnImg, ALL_IMAGES, fnImg.isInStack(), WRITE_REPLACE);
            break;
        case HEADER_RESET:
            img.read(fnImg, _HEADER_ALL);
            img.initGeometry();
            img.write(fnImg, ALL_IMAGES, fnImg.isInStack(), WRITE_REPLACE);
            break;
        }
    }

    void finishProcessing()
    {
        if (operation ==HEADER_EXTRACT)
        {
            mdOut = mdIn;
            single_image = false;
        }
        XmippMetadataProgram::finishProcessing();
    }
}
;// end of class ProgHeader

RUN_XMIPP_PROGRAM(ProgHeader);


