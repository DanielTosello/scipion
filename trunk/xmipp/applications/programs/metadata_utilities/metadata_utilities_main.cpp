/***************************************************************************
 * Authors:     Roberto Marabini roberto@cnb.csic.es
 *              J.M. de la Rosa  jmdelarosa@cnb.csic.es
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your param) any later version.
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

#include <data/argsparser.h>
#include <data/program.h>
#include <string.h>
#include <data/metadata.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>

class ProgMetadataUtilities: public XmippProgram
{
private:
    WriteModeMetaData mode;
    FileName fn_in, fn_out, fn_md2;
    MetaData mdIn, md2;
    MDLabel label;
    std::vector<MDLabel> labels;
    String operation;
    bool doWrite;

protected:
    void defineParams()
    {
        addUsageLine("Perform several operation over the metadata files. If the -o option is not used");
        addUsageLine("the original metadata will be modified after applying some operations.");
        addUsageLine("Also you can use the --print option just to print out the result metadata to screen.");
        addUsageLine("The combination of -i and -o without other operations can serve to extract data blocks");
        addUsageLine("inside a medata and write to an independent one.");
        addSeeAlsoLine("metadata_import");

        addParamsLine(" -i <metadata>         : Input metadata file");
        addParamsLine("   [-o  <metadata>]    : Output metadata file, if not provided result will overwrite input file");

        addParamsLine("  [--set <set_operation> <md2_file> <label=image>]   : Set operations");
        addParamsLine("         where <set_operation>");
        addParamsLine("   union               : Union with metadata md2");
        addParamsLine("   intersection        : Intersection with metadata md2");
        addParamsLine("   subtraction         : Subtraction with metadata md2");
        addParamsLine("   join                : Inner join with md2 using label l1");
        addParamsLine("   natural_join        : Natural  join with md2 using all common labels");
        addParamsLine("   merge               : Merge columns with md2, label is ignored");
        addParamsLine("                       : Both metadatas should have same size, and elements should be in same order,");
        addParamsLine("                       : if not, you should use 'join' instead, but this constrain having a common label");
        addParamsLine("           alias -s;                                             ");

        addParamsLine("or --operate <operation>     : Operations on the metadata structure");
        addParamsLine("         where <operation>");
        addParamsLine("   sort <label=image>       : Sort metadata using a label as identifier");
        addParamsLine("                            : for sorting according to a component of a vector label");
        addParamsLine("                            : use label:col, e.g., NMADisplacements:0");
        addParamsLine("                            : The first column is column number 0");
        addParamsLine("   randomize                            : Randomize elements of metadata");
        addParamsLine("    add_column <labels>                 : Add some columns(label list) to metadata");
        addParamsLine("    drop_column <labels>                : Drop some columns(label list) from metadata");
        addParamsLine("    modify_values <expression>          : Use an SQLite expression to modify the metadata");
        addParamsLine("                                        : This option requires knowledge of basic SQL syntax(more specific SQLite");
        addParamsLine("           alias -e;                                             ");

        addParamsLine("or  --file <file_operation>     : File operations");
        addParamsLine("         where <file_operation>");
        addParamsLine("   copy <directory> <label=image>  : Copy files in metadata md1 to directory path (file names at label column)");
        addParamsLine("   move <directory>  <label=image> : Move files in metadata md1 to directory path (file names at label column)");
        addParamsLine("   delete  <label=image>      : Delete files in metadata md1 (file names at label column)");
        addParamsLine("   convert2db                 : Convert metadata to sqlite database");
        addParamsLine("   convert2xml                : Convert metadata to xml file");
        addParamsLine("   import_txt <labels>        : Import a text file specifying its columns");
        addParamsLine("           alias -f;                                             ");

        addParamsLine("or --query <query_operation>   : Query operations");
        addParamsLine("         where <query_operation>");
        addParamsLine("   select <expression>        : Create new metadata with those entries that satisfy the expression");
        addParamsLine("   count  <label>             : for each value of a given label create new metadata with the number of times the value appears");
        addParamsLine("   sum  <label1> <label2>   : group metadata by label1 and add quantities in label2");
        addParamsLine("   size                       : print Metadata size");
        addParamsLine("           alias -q;                                             ");

        addParamsLine("or --fill <labels> <fill_mode>                  : Fill a column values(should be of same type)");
        addParamsLine("   where <fill_mode>");
        addParamsLine("     constant  <value>                        : Fill with a constant value");
        addParamsLine("     lineal  <init_value> <step>              : Fill with a lineal serie starting at init_value with an step");
        addParamsLine("     rand_uniform  <a=0.> <b=1.>              : Follow a uniform distribution between a and b");
        addParamsLine("     rand_gaussian <mean=0.> <stddev=1.>      : Follow a gaussian distribution with mean and stddev");
        addParamsLine("     rand_student  <mean=0.> <stddev=1.> <df=3.> : Follow a student distribution with mean, stddev and df degrees of freedom.");
        addParamsLine("     expand                                   : Treat the column as the filename of an row metadata and expand values");
        addParamsLine("           alias -l;                                             ");

        addParamsLine("  [--print ]                    : Just print medata to stdout, or if -o is specified written to disk.");
        addParamsLine("                                : this option is useful for extrating data blocks inside a metadata.");
        addParamsLine("           alias -p;                                             ");

        addParamsLine(" [--mode+ <mode=overwrite>]   : Metadata writing mode.");
        addParamsLine("    where <mode>");
        addParamsLine("     overwrite   : Replace the content of the file with the Metadata");
        addParamsLine("     append      : Write the Metadata as a new block, removing the old one");

        addExampleLine(" Concatenate two metadatas. If label is not provided, by default is 'image'", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --set union mD2.doc  -o out.doc");
        addExampleLine(" Intersect two metadatas using label 'order_'", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --set intersection mD2.doc order_ -o out.doc");
        addExampleLine(" Combine columns from two metadatas. Be sure of both have same number of rows and also", false);
        addExampleLine(" there aren't common columns, in that case second metadata columns will be used", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --set merge mD2.doc -o out.doc");
        addExampleLine(" Sort the elements in metadata (using default label 'image').", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc -s sort -o out.doc");
        addExampleLine(" Add columns 'shiftX' and 'shiftY' to metadata.", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --operate add_column \"shiftX shiftY\" -o out.doc");
        addExampleLine(" You can also add columns and 'filling' its values with different options", false);
        addExampleLine("By example, to add the column 'shiftX' with uniform random value between 0 and 10", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --fill shiftX rand_uniform 0 10 -o out.doc");
        addExampleLine("Or for initialize metadata columns 'shiftX' and 'shiftY' with a constant value of 5", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc -l \"shiftX shiftY\" constant 5 -o out.doc");
        addExampleLine("If you have columns that represent the filename of a metadata with other data (ex CTFParams)", false);
        addExampleLine("you cant 'expand' the column with the values in that metadta", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --fill CTFParams expand -o outExpanded.doc");
        addExampleLine("For check all options availables for 'filling' mode, use: ", false);
        addExampleLine ("   xmipp_metadata_utilities --help fill");
        addExampleLine(" Dump metadata content to Sqlite3 database. (use xmipp_sqlite3 to visualize results)", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --operate convert2db -o mD1.db; xmipp_sqlite3 out.db");
        addExampleLine(" Dump metadata content to xml file.", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --operate convert2xml -o mD1.xml");
        addExampleLine(" Copy files in metadata to a location. The metadata will be also copied to new location", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --file copy /home/pepe/newLocation");
        addExampleLine(" Delete files in metadata.", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --file delete");
        addExampleLine(" Select elements in metadata that satisfy a given constrain.", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --query select \"angleRot > 10 AND anglePsi < 0.5\" -o out.doc");
        addExampleLine(" You can also modify your data using SQLite syntax expression", false);
        addExampleLine("  xmipp_metadata_utilities  -i a.doc --operate modify_values \"angleRot=(angleRot*3.1416/180.)\" -o b.doc");
        addExampleLine("  xmipp_metadata_utilities  -i a.doc --operate modify_values \"image=replace(image, 'xmp','spi')\" -o b.doc");
        addExampleLine(" Count number of images per CTF", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc -q count CTFModel -o out.doc");
        addExampleLine(" images asigned a ctfgroup", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc -q sum defocusGroup count -o out.doc");
        addExampleLine(" Print the metadata Size", false);
        addExampleLine ("   xmipp_metadata_utilities -i mD1.doc --query size");

    }

    void readParams()
    {
        fn_in = getParam("-i");
        if (!checkParam("--file") || getParam("--file") != "import_txt")
          mdIn.read(fn_in);
        doWrite = true;
        fn_out = checkParam("-o") ? getParam("-o") : fn_in;
    }

    void doSet()
    {
        operation = getParam("--set", 0);
        md2.read(getParam("--set", 1));
        MDLabel label = MDL::str2Label(getParam("--set", 2));

        if (operation == "union")
            mdIn.unionDistinct(md2, label);
        else if (operation == "intersection")
            mdIn.intersection(md2, label);
        else if (operation == "subtraction")
            mdIn.subtraction(md2, label);
        else if (operation == "join")
        {
            MetaData md;
            md.join(mdIn, md2, label);
            mdIn = md;
        }
        else if (operation == "natural_join")
        {
            MetaData md;
            md.join(mdIn, md2, label,NATURAL);
            mdIn = md;
        }
        else if (operation == "merge")
            mdIn.merge(md2);
    }//end of function doSet

    void doOperate()
    {
        operation = getParam("--operate", 0);

        if (operation == "add_column" || operation == "drop_column")
        {
            MDL::str2LabelVector(getParam("--operate", 1), labels);
            for (int i = 0; i < labels.size(); ++i)
            {
                if (operation == "add_column")
                    mdIn.addLabel(labels[i]);
                else if (operation == "drop_column")
                    mdIn.removeLabel(labels[i]);
            }
        }
        else if (operation == "modify_values")// modify_values
            mdIn.operate(getParam("--operate", 1));
        else
        {
            MetaData md(mdIn);
            if (operation == "sort")
                mdIn.sort(md, getParam("--operate", 1));
            else if (operation == "randomize")
                mdIn.randomize(md);
        }
    }//end of function doOperate

    void doFill()
    {

        MDL::str2LabelVector(getParam("--fill", 0), labels);

        if (labels.empty())
            REPORT_ERROR(ERR_PARAM_INCORRECT, "You should provide at least one label to fill out");

        operation = getParam("--fill", 1);
        MDValueGenerator * generator;

        // Select wich generator to use
        if (operation == "constant")
            generator = new MDConstGenerator(getParam("--fill", 2));
        else if (operation.find("rand_") == 0)
        {
            double op1 = getDoubleParam("--fill", 2);
            double op2 = getDoubleParam("--fill", 3);
            double op3 = 0.;
            String type = findAndReplace(operation, "rand_", "");
            if (type == "student")
                op3 = getDoubleParam("--fill", 4);
            generator = new MDRandGenerator(op1, op2, type, op3);
        }
        else if (operation == "expand")
            generator = new MDExpandGenerator();
        else if (operation == "lineal")
            generator = new MDLinealGenerator(getDoubleParam("--fill", 2), getDoubleParam("--fill", 3));

        //Fill columns
        for (int i = 0; i < labels.size(); ++i)
        {
            generator->label = labels[i];
            generator->fill(mdIn);
        }

        delete generator;
    }//end of function doFill

    void doQuery()
    {
        operation = getParam("--query", 0);
        String expression;

        if (operation == "count")//note second label is a dummy parameter
        {
            label = MDL::str2Label(getParam("--query", 1));
            md2 = mdIn;
            mdIn.aggregate(md2, AGGR_COUNT,label,label,MDL_COUNT);
        }
        else if (operation == "sum")
        {
            label = MDL::str2Label(getParam("--query", 1));
            MDLabel label2;
            label2 = MDL::str2Label(getParam("--query", 2));
            md2 = mdIn;
            mdIn.aggregate(md2, AGGR_SUM,label,label2,MDL_SUM);
        }
        else if (operation == "select")
        {
            expression = getParam("--query", 1);
            md2 = mdIn;
            mdIn.importObjects(md2, MDExpression(expression));
        }
        else if (operation == "size")
        {
            doWrite = false;
            std::cout << fn_in + " size is: " << mdIn.size() << std::endl;
        }
    }//end of function doQuery

    void doFile()
    {
        operation = getParam("--file", 0);

        if (operation == "convert2db")
        {
            doWrite = false;
            MDSql::dumpToFile(fn_out);
        }
        else if (operation == "convert2xml")
        {
            doWrite = false;
            mdIn.convertXML(fn_out);
        }
        else if (operation == "import_txt")
        {
          mdIn.readPlain(fn_in, getParam("--file", 1));
        }
        else
        {
            bool doDelete;
            FileName path, inFnImg, outFnImg;

            if (!(doDelete = operation == "delete"))//copy or move
            {
                path = getParam("--file", 1);
                if (!exists(path))
                    if (mkpath(path, 0755) != 0)
                        REPORT_ERROR(ERR_IO_NOPERM, (String)"Cannot create directory "+ path);
            }
            label = MDL::str2Label(getParam("--file", doDelete ? 1 : 2));
            doWrite = !doDelete;

            FOR_ALL_OBJECTS_IN_METADATA(mdIn)
            {

                mdIn.getValue(label, inFnImg, __iter.objId);

                if (doDelete)
                    remove(inFnImg.c_str());
                else
                {
                    outFnImg = inFnImg.removeDirectories();
                    mdIn.setValue(label, outFnImg, __iter.objId);
                    outFnImg = path + "/" + outFnImg;

                    if (operation == "copy")
                        inFnImg.copyFile(outFnImg);
                    else if (operation == "move")
                        rename(inFnImg.c_str(), outFnImg.c_str());
                }
            }
            fn_out = path + "/" + fn_out.removeDirectories();
        }
    }

public:
    void run()
    {
        if (checkParam("--set"))
            doSet();
        else if (checkParam("--operate"))
            doOperate();
        else if (checkParam("--file"))
            doFile();
        else if (checkParam("--query"))
            doQuery();
        else if (checkParam("--fill"))
            doFill();

        if (checkParam("--print"))
            mdIn.write(std::cout);
        else if (doWrite)
            mdIn.write(fn_out);
    }

}
;//end of class ProgMetaDataUtilities

int main(int argc, char **argv)
{
    ProgMetadataUtilities program;
    program.read(argc, argv);
    return program.tryRun();
}
