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

#include <data/docfile.h>
#include <data/args.h>

void Usage();

int main(int argc, char *argv[])
{
    FileName fn_DF, fn_SF, fn_SF_out, fn_tst;
    int      col;
    double   limit0, limitF;
    bool     en_limit0, en_limitF, do_clean;

    // Get input parameters .................................................
    try
    {
        fn_DF     = getParameter(argc, argv, "-doc");
        fn_SF     = getParameter(argc, argv, "-sel", "");
        fn_SF_out = getParameter(argc, argv, "-o", "");
        col       = textToInteger(getParameter(argc, argv, "-col", "1"));
        col--;
        en_limit0 = checkParameter(argc, argv, "-limit0");
        if (en_limit0)
            limit0 = textToFloat(getParameter(argc, argv, "-limit0"));
        en_limitF = checkParameter(argc, argv, "-limitF");
        if (en_limitF)
            limitF = textToFloat(getParameter(argc, argv, "-limitF"));
        do_clean = checkParameter(argc, argv, "-clean");

    }
    catch (XmippError XE)
    {
        std::cout << XE;
        Usage();
        exit(0);
    }

    // Process ..............................................................
    try
    {
        DocFile DF;
        DF.read(fn_DF);
        DF.go_beginning();
        SelFile SF;
        if (DF.get_current_line().Is_comment()) fn_tst = (DF.get_current_line()).get_text();
        if (strstr(fn_tst.c_str(), "Headerinfo") == NULL)
        {
            // Non-NewXmipp type document file
            std::cerr << "Docfile is of non-NewXmipp type. " << std::endl;
            if (fn_SF == "")
                REPORT_ERROR(ERR_ARG_MISSING, "Select images: Please provide the corresponding selfile as well.");
            SF.read(fn_SF);
        }
        else DF.get_selfile(SF);

        // Actually select images
        if (SF.ImgNo(SelLine::ACTIVE) + SF.ImgNo(SelLine::DISCARDED) !=
            DF.dataLineNo())
            REPORT_ERROR(ERR_MD_OBJECTNUMBER, "Select images: SelFile and DocFile do not have the "
                         "same number of lines");
        if (col >= DF.FirstLine_colNumber())
            REPORT_ERROR(ERR_ARG_INCORRECT, "Select images: Column not valid for this DocFile");
        select_images(DF, SF, col, en_limit0, limit0, en_limitF, limitF);

        // Cleaning
        if (do_clean) SF.clean();

        // Write output
        if (fn_SF_out == "") fn_SF_out = fn_DF.without_extension().add_extension("sel");
        SF.write(fn_SF_out);

    }
    catch (XmippError XE)
    {
        std::cout << XE;
    }
}

/* Usage ------------------------------------------------------------------- */
void Usage()
{
    std::cerr << "Usage: select_images\n"
    << "   -doc <docfile>        : Input document file\n"
    << "  [-sel <selfile>]       : Corresponding selfile (only for non-NewXmipp docfiles)\n"
    << "  [-o  <docroot.sel>]    : Output selfile. By default, the docfile root + .sel\n"
    << "  [-col <col=1>]         : Column of the docfile (first column is 1) \n"
    << "  [-limit0 <limit0>]     : Values below this are discarded\n"
    << "  [-limitF <limitF>]     : Values above this are discarded\n"
    << "  [-clean]               : Remove discarded images from the selfile \n"
    ;
}
