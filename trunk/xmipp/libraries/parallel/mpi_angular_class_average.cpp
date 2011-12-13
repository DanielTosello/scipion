/***************************************************************************
 * Authors:     AUTHOR_NAME (aerey@cnb.csic.es)
 *
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

#include "mpi_angular_class_average.h"

MpiProgAngularClassAverage::MpiProgAngularClassAverage(int argc, char **argv)
{}

void MpiProgAngularClassAverage::read(int argc, char** argv)
{
    node = new MpiNode(argc, argv);
    // Master should read first
    if (node->isMaster())
        XmippProgram::read(argc, argv);
    node->barrierWait();
    if (!node->isMaster())
    {
        verbose = 0;//disable verbose for slaves
        XmippProgram::read(argc, argv);
    }
}


// Read arguments ==========================================================
void MpiProgAngularClassAverage::readParams()
{
    // Read command line
    DFlib.read(getParam("--lib"));
    fn_out = getParam("-o");
    col_select = getParam("--select");

    do_limit0 = checkParam("--limit0");
    if (do_limit0)
        limit0 = getDoubleParam("--limit0");
    do_limitF = checkParam("--limitF");
    if (do_limitF)
        limitF = getDoubleParam("--limitF");

    inFile = getParam("-i");

    if (checkParam("--limitRclass"))
    {
        limitRclass = getDoubleParam("--limitRclass")/100.;
        if (limitRclass < -1. || limitRclass > 1.)
            REPORT_ERROR(ERR_VALUE_INCORRECT,
                         "limitRclass should be a percentage: provide values between -100 and 100.");
        if (limitRclass > 0.)
            do_limitR0class = true;
        else if (limitRclass < 0.)
        {
            limitRclass *= -1.;
            do_limitRFclass = true;
        }
    }

    if (checkParam("--limitRper"))
    {
        limitRper = getDoubleParam("--limitRper");
        if (limitRper < -100. || limitRper > 100.)
            REPORT_ERROR(ERR_VALUE_INCORRECT,
                         "limitRper should be a percentage: provide values between -100 and 100.");
        if (limitRper > 0.)
            do_limitR0per = true;
        else if (limitRper < 0.)
        {
            limitRper *= -1.;
            do_limitRFper = true;
        }
    }

    if ((do_limitR0per && (do_limitR0class ||  do_limitRFclass)) ||
        (do_limitR0per && (do_limit0 || do_limitF )) ||
        ((do_limitR0class ||  do_limitRFclass) && (do_limit0 || do_limitF )))
        REPORT_ERROR(ERR_VALUE_INCORRECT, "You can not use different kind of limits at the same time.");

    // Perform splitting of the data?
    do_split = checkParam("--split");

    // Perform Wiener filtering of average?
    fn_wien = getParam("--wien");
    pad = XMIPP_MAX(1.,getDoubleParam("--pad"));

    // Internal re-alignment of the class averages
    Ri = getIntParam("--Ri");
    Ro = getIntParam("--Ro");
    nr_iter = getIntParam("--iter");
    max_shift = getDoubleParam("--max_shift");
    max_shift_change = getDoubleParam("--max_shift_change");
    max_psi_change = getDoubleParam("--max_psi_change");
    do_mirrors = true;

    do_save_images_assigned_to_classes = checkParam("--save_images_assigned_to_classes");
    mpi_job_size = getIntParam("--mpi_job_size");
}

// Define parameters ==========================================================
void MpiProgAngularClassAverage::defineParams()
{
    addUsageLine(
        "Make class average images and corresponding selfiles from angular_projection_matching docfiles.");

    addSeeAlsoLine("angular_project_library, angular_projection_matching");

    addParamsLine(
        "    -i <doc_file>          : Docfile with assigned angles for all experimental particles");
    addParamsLine(
        "    --lib <doc_file>       : Docfile with angles used to generate the projection matching library");
    addParamsLine(
        "    -o <root_name>         : Output rootname for class averages and selfiles");
    addParamsLine(
        "   [--split ]              : Also output averages of random halves of the data");
    addParamsLine(
        "   [--wien <img=\"\"> ]    : Apply this Wiener filter to the averages");
    addParamsLine(
        "   [--pad <factor=1.> ]    : Padding factor for Wiener correction");
    addParamsLine(
        "   [--save_images_assigned_to_classes]    : Save images assigned te each class in output metadatas");
    addParamsLine("alias --siatc;");
    addParamsLine(
        "==+ IMAGE SELECTION BASED ON INPUT DOCFILE (select one between: limit 0, F and R ==");
    addParamsLine(
        "   [--select <col=\"maxCC\">]     : Column to use for image selection (limit0, limitF or limitR)");
    addParamsLine("   [--limit0 <l0>]         : Discard images below <l0>");
    addParamsLine("   [--limitF <lF>]         : Discard images above <lF>");
    addParamsLine(
        "   [--limitRclass <lRc>]         : if (lRc>0 && lRc< 100): discard lowest  <lRc> % in each class");
    addParamsLine(
        "                           : if (lRc<0 && lR>-100): discard highest <lRc> % in each class");
    addParamsLine(
        "   [--limitRper <lRp>]         : if (lRp>0 && lRp< 100): discard lowest  <lRa> %");
    addParamsLine(
        "                           : if (lRp<0 && lRp>-100): discard highest <lRa> %");

    addParamsLine("==+ REALIGNMENT OF CLASSES ==");
    addParamsLine(
        "   [--iter <nr_iter=0>]      : Number of iterations for re-alignment");
    addParamsLine(
        "   [--Ri <ri=1>]             : Inner radius to limit rotational search");
    addParamsLine(
        "   [--Ro <r0=-1>]            : Outer radius to limit rotational search");
    addParamsLine("                           : ro = -1 -> dim/2-1");
    addParamsLine(
        "   [--max_shift <ms=999.>]        : Maximum shift (larger shifts will be set to 0)");
    addParamsLine(
        "   [--max_shift_change <msc=999.>] : Discard images that change shift more in the last iteration ");
    addParamsLine(
        "   [--max_psi_change <mps=360.>]   : Discard images that change psi more in the last iteration ");

    addParamsLine("  [--mpi_job_size <size=10>]   : Number of images sent to a cpu in a single job ");
    addParamsLine("                                : 10 may be a good value");

    addExampleLine(
        "Sample at default values and calculating output averages of random halves of the data",
        false);
    addExampleLine(
        "xmipp_angular_class_average -i proj_match.doc --lib ref_angles.doc -o out_dir --split");

    addKeywords("class average images");
}


/* Run --------------------------------------------------------------------- */
void MpiProgAngularClassAverage::run()
{
    mpi_preprocess();

    int lockIndex;
    size_t order_index;
    int ref3d_index;
    double weight,weights1,weights2;

    double lockWeightIndexes[lockWeightIndexesSize];

    double * jobListRows = new double[ArraySize * mpi_job_size + 1];

    if (node->rank == 0)
    {
        //for (int iCounter = 0; iCounter < nJobs; )//increase counter after I am free
        int jobId = 0, size;
        int finishedNodes = 1;
        bool whileLoop = true;

        size_t id, order, count;
        int ctfGroup, ref3d, ref2d;
        id = mdJobList.firstObject();
        MDIterator __iterJobs(mdJobList);
        while (whileLoop)
        {
            //wait until a worker is available
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            switch (status.MPI_TAG)
            {
            case TAG_I_AM_FREE:

                size = 0;
                for (int i=0;i<mpi_job_size && jobId < nJobs;i++,size++,jobId++)
                {
                    //Some test values for defocus, 3D reference and projection direction
                    mdJobList.getValue(MDL_REF3D, ref3d,  __iterJobs.objId);
                    jobListRows[index_3DRef + ArraySize * i + 1] = (double) ref3d;
                    mdJobList.getValue(MDL_DEFGROUP, ctfGroup, __iterJobs.objId);
                    jobListRows[index_DefGroup + ArraySize * i + 1] = (double) ctfGroup;
                    mdJobList.getValue(MDL_ORDER, order,  __iterJobs.objId);
                    jobListRows[index_Order + ArraySize * i + 1] = (double) order;
                    mdJobList.getValue(MDL_COUNT, count,  __iterJobs.objId);
                    jobListRows[index_Count + ArraySize * i + 1] = (double) count;
                    mdJobList.getValue(MDL_REF, ref2d,  __iterJobs.objId);
                    jobListRows[index_2DRef + ArraySize * i + 1] = (double) ref2d;
                    jobListRows[index_jobId + ArraySize * i + 1] = (double) __iterJobs.objId;
                    mdJobList.getValue(MDL_ANGLEROT, jobListRows[index_Rot + ArraySize * i + 1],  __iterJobs.objId);
                    mdJobList.getValue(MDL_ANGLETILT, jobListRows[index_Tilt + ArraySize * i + 1],  __iterJobs.objId);
                    __iterJobs.moveNext();
                }
                jobListRows[0]=size;

                //read worker call just to remove it from the queue
                MPI_Recv(0, 0, MPI_INT, MPI_ANY_SOURCE, TAG_I_AM_FREE, MPI_COMM_WORLD,
                         &status);
                if(size > 0)
                {
                    //send work, first int defocus, second 3D reference, 3rd projection
                    // direction and job number
#define DEBUG_MPI
#ifdef DEBUG_MPI
                    usleep(10000);
                    std::cerr << "Sending job to worker " << status.MPI_SOURCE <<std::endl;
#endif

                    MPI_Send(jobListRows, ArraySize * mpi_job_size + 1, MPI_DOUBLE, status.MPI_SOURCE,
                             TAG_WORK, MPI_COMM_WORLD);
                    //__iterJobs.moveNext();
                }
                else
                {
                    MPI_Send(0, 0, MPI_INT, status.MPI_SOURCE, TAG_STOP, MPI_COMM_WORLD);
                    finishedNodes ++;
                    if (finishedNodes >= node->size)
                        whileLoop=false;
                }
                break;
            case TAG_MAY_I_WRITE:
                //where do you want to write?
                MPI_Recv(&lockIndex, 1, MPI_INT, MPI_ANY_SOURCE, TAG_MAY_I_WRITE, MPI_COMM_WORLD, &status);

                mdJobList.getValue(MDL_ORDER, order_index, lockIndex);
                mdJobList.getValue(MDL_REF3D, ref3d_index, lockIndex);

#define DEBUG_MPI
#ifdef DEBUG_MPI

                std::cerr << "Blocking. lockIndex: " << lockIndex << " | status.MPI_SOURCE: " << status.MPI_SOURCE
                << " | lockArray[" << order_index<< "," <<ref3d_index<< "]: " << dAij(lockArray,order_index,ref3d_index) << std::endl;
#endif

                if (dAij(lockArray,order_index,ref3d_index))
                {//Locked
                    MPI_Send(0, 0, MPI_INT, status.MPI_SOURCE,
                             TAG_DO_NOT_DARE_TO_WRITE, MPI_COMM_WORLD);
                }
                else
                {//Unlocked

                    dAij(lockArray,order_index,ref3d_index)=true;
                    // Send the old weight
                    lockWeightIndexes[index_lockIndex] = lockIndex;
                    lockWeightIndexes[index_weight] = dAij(weightArray,order_index,ref3d_index);
                    lockWeightIndexes[index_weights1] = dAij(weightArrays1,order_index,ref3d_index);
                    lockWeightIndexes[index_weights2] = dAij(weightArrays2,order_index,ref3d_index);
                    lockWeightIndexes[index_ref3d] = ref3d_index;
                    MPI_Send(lockWeightIndexes, lockWeightIndexesSize, MPI_DOUBLE, status.MPI_SOURCE,
                             TAG_YES_YOU_MAY_WRITE, MPI_COMM_WORLD);
                }
                break;
            case TAG_I_FINISH_WRITTING:
                //release which lock?
                MPI_Recv(lockWeightIndexes, lockWeightIndexesSize, MPI_DOUBLE, MPI_ANY_SOURCE, TAG_I_FINISH_WRITTING,
                         MPI_COMM_WORLD, &status);

                lockIndex = lockWeightIndexes[index_lockIndex];
#define DEBUG_MPI
#ifdef DEBUG_MPI

                std::cerr << "Unblocking. lockIndex: " << lockIndex << " | status.MPI_SOURCE: " << status.MPI_SOURCE << std::endl;
#endif

                mdJobList.getValue(MDL_ORDER, order_index, lockIndex);
                mdJobList.getValue(MDL_REF3D, ref3d_index, lockIndex);
                dAij(lockArray,order_index,ref3d_index)=false;
                dAij(weightArray,order_index,ref3d_index) += lockWeightIndexes[index_weight];
                dAij(weightArrays1,order_index,ref3d_index) += lockWeightIndexes[index_weights1];
                dAij(weightArrays2,order_index,ref3d_index) += lockWeightIndexes[index_weights2];
                break;
            default:
                std::cerr << "WRONG TAG RECEIVED" << std::endl;
                break;
            }
        }
        std::cerr << "out while" <<std::endl;
    }
    else
    {
        bool whileLoop = true;
        while (whileLoop)
        {
#define DEBUG_MPI
#ifdef DEBUG_MPI
            std::cerr << "[" << node->rank << "] Asking for a job " <<std::endl;
#endif
            //I am free
            MPI_Send(0, 0, MPI_INT, 0, TAG_I_AM_FREE, MPI_COMM_WORLD);
            //wait for message
            MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            switch (status.MPI_TAG)
            {
            case TAG_STOP://I am free
                MPI_Recv(0, 0, MPI_INT, 0, TAG_STOP,
                         MPI_COMM_WORLD, &status);
                whileLoop=false;
                break;
            case TAG_WORK://work to do
                MPI_Recv(jobListRows, ArraySize * mpi_job_size + 1, MPI_DOUBLE, 0, TAG_WORK,
                         MPI_COMM_WORLD, &status);
                mpi_process_loop(jobListRows);
                break;
            default:
                break;
            }
        }
    }

    if (node->rank == 0)
        mpi_postprocess();

    MPI_Finalize();
}


void MpiProgAngularClassAverage::mpi_process_loop(double * Def_3Dref_2Dref_JobNo)
{
    int size = ROUND(Def_3Dref_2Dref_JobNo[0]);

    for(int i=0;i<size;i++)
    {
        mpi_process(Def_3Dref_2Dref_JobNo+i*ArraySize+1);
    }

}

void MpiProgAngularClassAverage::mpi_process(double * Def_3Dref_2Dref_JobNo)
{
#define DEBUG
#ifdef DEBUG
    std::cerr<<"["<<node->rank<<"]"
    << " 3DRef:    "  << ROUND(Def_3Dref_2Dref_JobNo[index_3DRef])
    << " DefGroup: "  << ROUND(Def_3Dref_2Dref_JobNo[index_DefGroup])
    << " 2DRef:    "  << ROUND(Def_3Dref_2Dref_JobNo[index_2DRef])
    << " Order:    "  << ROUND(Def_3Dref_2Dref_JobNo[index_Order])
    << " Count:    "  << ROUND(Def_3Dref_2Dref_JobNo[index_Count])
    << " jobId:    "  << ROUND(Def_3Dref_2Dref_JobNo[index_jobId])
    << " rot:      "  << Def_3Dref_2Dref_JobNo[index_Rot]
    << " tilt:     "  << Def_3Dref_2Dref_JobNo[index_Tilt]
    << " Sat node: "  << node->rank
    << std::endl;
#endif
#undef DEBUG

    Image<double> img, avg, avg1, avg2;
    FileName fn_img, fn_tmp;
    MetaData SFclass, SFclass1, SFclass2;
    MetaData SFclassDiscarded;
    double rot, tilt, psi, xshift, yshift, val, w, w1, w2, my_limitR, scale;
    bool mirror;
    int ref_number, this_image, ref3d, defGroup;
    static int defGroup_last = 0;
    int isplit, lockIndex;
    MetaData _DF;
    size_t id;
    size_t order_number;

    w = 0.;
    w1 = 0.;
    w2 = 0.;
    this_image = 0;

    order_number = ROUND(Def_3Dref_2Dref_JobNo[index_Order]);
    ref_number   = ROUND(Def_3Dref_2Dref_JobNo[index_2DRef]);
    defGroup     = ROUND(Def_3Dref_2Dref_JobNo[index_DefGroup]);
    ref3d        = ROUND(Def_3Dref_2Dref_JobNo[index_3DRef]);
    lockIndex    = ROUND(Def_3Dref_2Dref_JobNo[index_jobId]);

    if (fn_wien != "" && defGroup_last != defGroup)
    {
        // Read wiener filter
        FileName fn_wfilter;
        Image<double> auxImg;

        fn_wfilter.compose(defGroup,fn_wien);
        auxImg.read(fn_wfilter);
        Mwien = auxImg();
        defGroup_last = defGroup;
    }

    MDValueEQ eq1(MDL_REF3D, ref3d);
    MDValueEQ eq2(MDL_DEFGROUP, defGroup);
    MDValueEQ eq3(MDL_ORDER, order_number);
    MDMultiQuery multi;

    multi.addAndQuery(eq1);
    multi.addAndQuery(eq2);
    multi.addAndQuery(eq3);

    _DF.importObjects(DF, multi);

    if (_DF.size() == 0)
        REPORT_ERROR(ERR_DEBUG_IMPOSIBLE,
                     "Program should never execute this line, something went wrong");

    Matrix2D<double> A(3, 3);
    std::vector<int> exp_number, exp_split;
    std::vector<Image<double> > exp_imgs;

    Iempty.setEulerAngles(Def_3Dref_2Dref_JobNo[index_Rot], Def_3Dref_2Dref_JobNo[index_Tilt], 0.);
    Iempty.setShifts(0., 0.);
    Iempty.setFlip(0.);
    avg = Iempty;
    avg1 = Iempty;
    avg2 = Iempty;

    // Loop over all images in the input docfile
    FOR_ALL_OBJECTS_IN_METADATA(_DF)
    {
        _DF.getValue(MDL_IMAGE, fn_img, __iter.objId);
        this_image++;

        std::cerr<<"["<<node->rank<<"]: "<< "mpi_process_03_b1. fn_img: "<< fn_img << std::endl;

        _DF.getValue(MDL_ANGLEPSI, psi, __iter.objId);
        _DF.getValue(MDL_SHIFTX, xshift, __iter.objId);
        _DF.getValue(MDL_SHIFTY, yshift, __iter.objId);
        if (do_mirrors)
            _DF.getValue(MDL_FLIP, mirror, __iter.objId);
        _DF.getValue(MDL_SCALE, scale, __iter.objId);

        img.read(fn_img);
        img().setXmippOrigin();
        img.setEulerAngles(0., 0., psi);
        img.setShifts(xshift, yshift);

        if (do_mirrors)
            img.setFlip(mirror);
        img.setScale(scale);

        if (do_split)
            isplit = ROUND(rnd_unif());
        else
            isplit = 0;
        // For re-alignment of class: store all images in memory
        if (nr_iter > 0)
        {
            exp_imgs.push_back(img);
            exp_number.push_back(this_image);
            exp_split.push_back(isplit);
        }

        // Apply in-plane transformation
        img.getTransformationMatrix(A);
        if (!A.isIdentity())
            selfApplyGeometry(BSPLINE3, img(), A, IS_INV, DONT_WRAP);

        // Add to average
        if (isplit == 0)
        {
            avg1() += img();
            w1 += 1.;
            id = SFclass1.addObject();
            SFclass1.setValue(MDL_IMAGE, fn_img, id);
            SFclass1.setValue(MDL_ANGLEROT, Def_3Dref_2Dref_JobNo[index_Rot], id);
            SFclass1.setValue(MDL_ANGLETILT, Def_3Dref_2Dref_JobNo[index_Tilt], id);
            SFclass1.setValue(MDL_REF, ref_number, id);
            SFclass1.setValue(MDL_REF3D, ref3d, id);
            SFclass1.setValue(MDL_DEFGROUP, defGroup, id);
            SFclass1.setValue(MDL_ORDER, order_number, id);
        }
        else
        {
            avg2() += img();
            w2 += 1.;
            id = SFclass2.addObject();
            SFclass2.setValue(MDL_IMAGE, fn_img, id);
            SFclass2.setValue(MDL_ANGLEROT, Def_3Dref_2Dref_JobNo[index_Rot], id);
            SFclass2.setValue(MDL_ANGLETILT, Def_3Dref_2Dref_JobNo[index_Tilt], id);
            SFclass2.setValue(MDL_REF, ref_number, id);
            SFclass2.setValue(MDL_REF3D, ref3d, id);
            SFclass2.setValue(MDL_DEFGROUP, defGroup, id);
            SFclass2.setValue(MDL_ORDER, order_number, id);
        }

    }

    // Re-alignment of the class
//    if (nr_iter > 0)
//    {
//        SFclass = SFclass1;
//        SFclass.unionAll(SFclass2);
//        avg() = avg1() + avg2();
//        w = w1 + w2;
//        avg.setWeight(w);
//
//        // Reserve memory for output from class realignment
//        int reserve = DF.size();
//        std::cerr << "reserve: " <<reserve<<std::endl;
//        double my_output[AVG_OUPUT_SIZE * reserve + 1];
//
//        reAlignClass(avg1, avg2, SFclass1, SFclass2, exp_imgs, exp_split,
//                     exp_number, order_number, my_output);
//        w1 = avg1.weight();
//        w2 = avg2.weight();
//    }

    // Apply Wiener filters
    if (fn_wien != "")
    {
        if (w1 > 0)
            applyWienerFilter(avg1());
        if (w2 > 0)
            applyWienerFilter(avg2());
    }

    // Output total and split averages and selfiles to disc
    SFclass = SFclass1;
    SFclass.unionAll(SFclass2);

    avg() = avg1() + avg2();
    w = w1 + w2;
    avg.setWeight(w);
    avg1.setWeight(w1);
    avg2.setWeight(w2);

    mpi_writeController(order_number, avg, avg1, avg2, SFclass, SFclass1, SFclass2,
                        SFclassDiscarded, w1, w2, lockIndex);

}



void MpiProgAngularClassAverage::mpi_write(
    size_t dirno,
    int ref3dIndex,
    Image<double> avg,
    Image<double> avg1,
    Image<double> avg2,
    MetaData SFclass,
    MetaData SFclass1,
    MetaData SFclass2,
    MetaData SFclassDiscarded,
    double w1,
    double w2,
    double old_w,
    double old_w1,
    double old_w2)
{
    FileName fileNameXmd, fileNameStk;

    formatStringFast(fileNameStk, "%s_refGroup%06lu.stk", fn_out.c_str(), ref3dIndex);
    mpi_writeFile(avg, dirno, fileNameStk, old_w);
    std::cerr<<"["<<node->rank<<"]: "<< "mpi_write_02"<<std::endl;

    if (do_split)
    {
        if (w1 > 0)
        {
            formatStringFast(fileNameStk, "%s_refGroup%06lu.stk", fn_out1.c_str(), ref3dIndex);
            mpi_writeFile(avg1, dirno, fileNameStk, old_w1);
        }
        if (w2 > 0)
        {
            formatStringFast(fileNameStk, "%s_refGroup%06lu.stk",
                             fn_out2.c_str(), ref3dIndex);
            mpi_writeFile(avg2, dirno, fileNameStk, old_w2);
        }
    }
}


void MpiProgAngularClassAverage::mpi_writeController(
    size_t dirno,
    Image<double> avg,
    Image<double> avg1,
    Image<double> avg2,
    MetaData SFclass,
    MetaData SFclass1,
    MetaData SFclass2,
    MetaData SFclassDiscarded,
    double w1,
    double w2,
    int lockIndex)
{
    double weight, weights1, weights2;
    double weight_old, weights1_old, weights2_old;
    double lockWeightIndexes[lockWeightIndexesSize];
    int ref3dIndex;

    //    std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeController_00"<<std::endl;
    bool whileLoop = true;
    while (whileLoop)
    {
#ifdef DEBUG_MPI
        std::cerr << "[" << node->rank << "] May I write. lockIndex: " << lockIndex <<std::endl;
#endif

        MPI_Send(&lockIndex, 1, MPI_INT, 0, TAG_MAY_I_WRITE, MPI_COMM_WORLD);
        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch (status.MPI_TAG)
        {
        case TAG_DO_NOT_DARE_TO_WRITE://I am free
            //            std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeController_01"<<std::endl;
            MPI_Recv(0, 0, MPI_INT, 0, MPI_ANY_TAG,
                     MPI_COMM_WORLD, &status);
            std::cerr << "Sleeping 0.1 seg at " << node->rank << std::endl;
            usleep(100000);//microsecond
            break;
        case TAG_YES_YOU_MAY_WRITE://I am free
            //            std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeController_02"<<std::endl;

            MPI_Recv(lockWeightIndexes, lockWeightIndexesSize, MPI_DOUBLE, 0, MPI_ANY_TAG,
                     MPI_COMM_WORLD, &status);

            lockIndex = ROUND(lockWeightIndexes[index_lockIndex]);
            //            std::cerr << "lockIndex4: " << lockIndex << std::endl;
#ifdef DEBUG_MPI

            std::cerr << "[" << node->rank << "] TAG_YES_YOU_MAY_WRITE.lockIndex: " << lockIndex <<std::endl;
#endif

            weight_old = lockWeightIndexes[index_weight];
            weights1_old = lockWeightIndexes[index_weights1];
            weights2_old = lockWeightIndexes[index_weights2];
            ref3dIndex = ROUND(lockWeightIndexes[index_ref3d]);
            //            std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeController_03"<<std::endl;
            mpi_write(dirno, ref3dIndex, avg, avg1, avg2, SFclass, SFclass1, SFclass2,
                      SFclassDiscarded, w1, w2, weight_old, weights1_old, weights2_old);
            //            std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeController_04"<<std::endl;
            whileLoop=false;
            break;
        default:
            std::cerr << "process WRONG TAG RECEIVED at " << node->rank << std::endl;
            break;
        }
    }
#ifdef DEBUG_MPI
    std::cerr << "[" << node->rank << "] Finish writer " <<std::endl;
#endif

    lockWeightIndexes[index_lockIndex] = lockIndex;
    lockWeightIndexes[index_weight]= w1 + w2;
    lockWeightIndexes[index_weights1] = w1;
    lockWeightIndexes[index_weights2] = w2;
    lockWeightIndexes[index_ref3d] = ref3dIndex;

    MPI_Send(lockWeightIndexes, lockWeightIndexesSize, MPI_DOUBLE, 0, TAG_I_FINISH_WRITTING, MPI_COMM_WORLD);

}


void MpiProgAngularClassAverage::mpi_writeFile(
    Image<double> avg,
    size_t dirno,
    FileName fileNameStk,
    double w_old)
{
    FileName fn_tmp;
    double w = avg.weight();
    Image<double> old;

    //    std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeFile_01"<<std::endl;

    if (w > 0.)
    {
        //        std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeFile_02"<<std::endl;

        if (w != 1.)
            avg() /= w;
        //        std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeFile_03"<<std::endl;

        //How are we going to handle weights, are they in the header, I do not think so....
        //in spider should be OK in general...!!
        //A more independent approach would be nice
        if (fileNameStk.exists())
        {
            //            std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeFile_04"<<std::endl;

            fn_tmp.compose(dirno, fileNameStk);
            old.read(fn_tmp);
            //            std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeFile_05"<<std::endl;

            //w_old = old.weight();
            FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(old())
            {
                dAij(old(),i,j) = (w_old * dAij(old(),i,j) + w
                                   * dAij(avg(),i,j)) / (w_old + w);
            }
            //            std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeFile_06"<<std::endl;

            old.setWeight(w_old + w);
            old.write(fileNameStk, dirno, true, WRITE_REPLACE);
        }
        else
        {
            //            std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeFile_07"<<std::endl;

            avg.write(fileNameStk, dirno, true, WRITE_REPLACE);
        }
    }
    //    std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeFile_08"<<std::endl;

    //    // Write class selfile to disc (even if its empty)
    //    if (write_selfile)
    //    {
    //        std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeFile_09"<<std::endl;
    //
    //        MetaData auxMd;
    //        String
    //        comment =
    //            (String) "This file contains one block per each pair (Projection direction, "
    //            + "Reference volume), groupClassXXXXXX refers to the projection direction, "
    //            + "refGroupXXXXXX refers to the reference volume. Additionally the blocks named "
    //            + "refGroupXXXXXX contains the images needed to reconstruct the reference XXXXXX.";
    //        SF.setComment(comment);
    //        //        }
    //        std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeFile_10"<<std::endl;
    //
    //        SF.write(fileNameXmd, MD_APPEND);
    //        std::cerr<<"["<<node->rank<<"]: "<< "mpi_writeFile_11"<<std::endl;
    //
    //    }

}


void MpiProgAngularClassAverage::mpi_produceSideInfo()
{

    //init with 0 by default through memset
    Iempty().resizeNoCopy(Xdim, Ydim);
    Iempty().setXmippOrigin();

    // Randomization
    if (do_split)
        randomize_random_generator();

    if (Ri<1)
        Ri=1;
    if (Ro<0)
        Ro=(Xdim/2)-1;
    // Set up FFTW transformers
    //Is this needed if  no alignment is required?
    MultidimArray<double> Maux;
    Polar<double> P;
    Polar<std::complex<double> > fP;

    produceSplineCoefficients(BSPLINE3, Maux, Iempty());
    P.getPolarFromCartesianBSpline(Maux, Ri, Ro);
    P.calculateFftwPlans(global_plans);
    fourierTransformRings(P, fP, global_plans, false);
    corr.resize(P.getSampleNoOuterRing());
    global_transformer.setReal(corr);
    global_transformer.FourierTransform();

    // Set ring defaults
    if (Ri < 1)
        Ri = 1;
    if (Ro < 0)
        Ro = (Xdim / 2) - 1;

    //    // Set limitR
    //    if (do_limitR0abs || do_limitRFabs)
    //    {
    //        MetaData tmpMT;
    //        MDLabel codifyLabel = MDL::str2Label(col_select);
    //        tmpMT.sort(DF,codifyLabel);
    //        int size = tmpMT.size();
    //        ROUND((limitRabs/100.) * size);

    //
    //        FOR_ALL_OBJECTS_IN_METADATA(tmpMT)
    //        {
    //            double auxval;
    //            DF.getValue(codifyLabel, auxval, __iter.objId);
    //            vals.push_back(auxval);
    //        }
    //        int nn = vals.size();
    //        std::sort(vals.begin(), vals.end());
    //        if (do_limitR0)
    //        {
    //            double val = vals[ROUND((limitR/100.) * vals.size())];
    //            if (do_limit0)
    //                limit0 = XMIPP_MAX(limit0, val);
    //            else
    //            {
    //                limit0 = val;
    //                do_limit0 = true;
    //            }
    //        }
    //        else if (do_limitRF)
    //        {
    //            double val = vals[ROUND(((100. - limitR)/100.) * vals.size())];
    //            if (do_limitF)
    //                limitF = XMIPP_MIN(limitF, val);
    //            else
    //            {
    //                limitF = val;
    //                do_limitF = true;
    //            }
    //        }
    //    }
    //}
}
void MpiProgAngularClassAverage::mpi_preprocess()
{
    initFileNames();

    if (node->rank==0)
    {
        master_seed = randomize_random_generator();
    }
    MPI_Bcast(&master_seed,1,MPI_UNSIGNED ,0,MPI_COMM_WORLD);
    init_random_generator(master_seed);

    filterInputMetadata();

    if (node->rank==0)
    {
        saveDiscardedImages();
        createJobList();
        initDimentions();
        initWeights();
        initOutputFiles();
    }

    MPI_Bcast(&Xdim,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&Ydim,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&Zdim,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&Ndim,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&nJobs,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&ref3dNum,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&ctfNum,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&paddim,1,MPI_INT,0,MPI_COMM_WORLD);

    mpi_produceSideInfo();
    node->barrierWait();
}


void MpiProgAngularClassAverage::initFileNames()
{

    // Set up output rootnames
    if (do_split)
    {
        fn_out1 = fn_out + "_split_1";
        fn_out2 = fn_out + "_split_2";
    }

}

void MpiProgAngularClassAverage::filterInputMetadata()
{
    MetaData auxDF,auxF1;

    auxDF.read((String)"ctfGroup[0-9][0-9][0-9][0-9][0-9][0-9]$@" + inFile);

    MDMultiQuery multi;
    MDValueGE eq1(MDL::str2Label(col_select), limit0);
    MDValueLE eq2(MDL::str2Label(col_select), limitF);

    // remove percent of images
    if (do_limitR0per || do_limitRFper)
    {
        bool asc;
        MDLabel codifyLabel = MDL::str2Label(col_select);
        int size = auxDF.size();
        int limit = size - ROUND((limitRper/100.) * size);

        if (do_limitR0per)
            asc=false;
        else
            asc=true;

        std::cerr << "[filterInputMetadata] size: " << size << " | limit: " << limit << std::endl;
        auxF1.sort(auxDF,codifyLabel,asc,limit,0);
    }
    //remove inages bellow (above) these limits
    else if(do_limit0 || do_limitF)
    {
        if (do_limit0)
            multi.addAndQuery(eq1);
        if (do_limitF)
            multi.addAndQuery(eq2);
        auxF1.importObjects(auxDF, multi);
    }
    // remove percentage of images from each class
    else if (do_limitR0class || do_limitRFclass)
    {
        //Delete a percentage of images in each class
        MetaData auxMdJobList;

        //!a take 1: using a copy of mdJobList
        const MDLabel myGroupByLabels[] =
            {
                MDL_REF3D, MDL_DEFGROUP, MDL_ORDER, MDL_REF, MDL_ANGLEROT, MDL_ANGLETILT
            };
        std::vector<MDLabel> groupbyLabels(myGroupByLabels,myGroupByLabels+6);
        auxMdJobList.aggregateGroupBy(auxDF, AGGR_COUNT, groupbyLabels, MDL_ORDER, MDL_COUNT);

        // Output stack size (number of valid projection directions)
        MDObject mdValueOut1(MDL_ORDER);
        auxMdJobList.aggregateSingleSizeT(mdValueOut1, AGGR_MAX ,MDL_ORDER);
        mdValueOut1.getValue(Ndim);

        MDObject mdValueOut2(MDL_DEFGROUP);
        auxMdJobList.aggregateSingleInt(mdValueOut2, AGGR_MAX ,MDL_DEFGROUP);
        mdValueOut2.getValue(ctfNum);

        MDObject mdValueOut3(MDL_REF3D);
        auxMdJobList.aggregateSingleInt(mdValueOut3, AGGR_MAX ,MDL_REF3D);
        mdValueOut3.getValue(ref3dNum);
        MultidimArray <int> multiCounter(Ndim+1, ctfNum+1, ref3dNum+1);
        multiCounter.initZeros();

        int ref3d, defgroup;
        size_t order, jobCount, jobCount2;

        FOR_ALL_OBJECTS_IN_METADATA(auxMdJobList)
        {
            auxMdJobList.getValue(MDL_REF3D, ref3d, __iter.objId);
            auxMdJobList.getValue(MDL_DEFGROUP, defgroup, __iter.objId);
            auxMdJobList.getValue(MDL_ORDER, order, __iter.objId);

            auxMdJobList.getValue(MDL_COUNT,jobCount, __iter.objId);

            jobCount2 = ROUND(limitRclass * jobCount);

            if (jobCount2 == 0)
                if(rnd_unif(0,1)<limitRclass)
                    jobCount2 = 1;

            dAkij(multiCounter, order, defgroup, ref3d) = jobCount2;
        }

        // sort
        if(do_limitR0class)
            auxF1.sort(auxDF, MDL::str2Label(col_select), true);
        else
            auxF1.sort(auxDF, MDL::str2Label(col_select), false);

        auxF1.removeDisabled();
        auxF1.addLabel(MDL_ENABLED);
        auxF1.setValueCol(MDL_ENABLED, 1);

        FOR_ALL_OBJECTS_IN_METADATA(auxF1)
        {
            auxF1.getValue(MDL_REF3D, ref3d, __iter.objId);
            auxF1.getValue(MDL_DEFGROUP, defgroup, __iter.objId);
            auxF1.getValue(MDL_ORDER, order, __iter.objId);

            if (dAkij(multiCounter, order, defgroup, ref3d) > 0)
            {
                auxF1.setValue(MDL_ENABLED,-1,__iter.objId);
                dAkij(multiCounter, order, defgroup, ref3d)--;
            }
        }
        auxF1.removeDisabled();
    }
    DF.sort(auxF1,MDL_IMAGE);

}

void MpiProgAngularClassAverage::saveDiscardedImages()
{

    MetaData auxDF, auxDFsort;
    FileName fileNameXmd;
    std::stringstream comment;

    auxDF.read((String)"ctfGroup[0-9][0-9][0-9][0-9][0-9][0-9]$@" + inFile);

    comment << "Discarded images";
    if (do_limit0)
        comment << ". Min value = " << limit0;
    if (do_limitF)
        comment << ". Max value = " << limitF;
    if (do_limitR0per)
        comment << ". Drop " << limitRper*100 << "% of images with lower " << col_select;
    if (do_limitRFper)
        comment << ". Drop " << limitRper*100 << "% of images with higher " << col_select;
    if (do_limitR0class)
        comment << ". Drop " << limitRclass*100 << "% of images (per class) with lower " << col_select
        << ". If the ROUND(num_images_per_class * limitRclass / 100) == 0 then images are randomly dropped"
        << " so the percentage is satisfied";
    if (do_limitRFclass)
        comment << ". Drop " << limitRclass*100 << "% of images (per class) with higher " << col_select
        << ". If the ROUND(num_images_per_class * limitRclass / 100) == 0 then images are randomly dropped"
        << " so the percentage is satisfied";

    auxDF.subtraction(DF,MDL_IMAGE);
    auxDF.setComment(comment.str());
    formatStringFast(fileNameXmd, "discarded@%s_discarded.xmd", fn_out.c_str());

    auxDFsort.sort(auxDF,MDL::str2Label(col_select));
    auxDFsort.write(fileNameXmd);

}

void MpiProgAngularClassAverage::initDimentions()
{
    getImageSize(DF, Xdim, Ydim, Zdim, Ndim);

    // Output stack size (number of valid projection directions)
    MDObject mdValueOut1(MDL_ORDER);
    mdJobList.aggregateSingleSizeT(mdValueOut1, AGGR_MAX ,MDL_ORDER);
    mdValueOut1.getValue(Ndim);

    MDObject mdValueOut2(MDL_DEFGROUP);
    mdJobList.aggregateSingleInt(mdValueOut2, AGGR_MAX ,MDL_DEFGROUP);
    mdValueOut2.getValue(ctfNum);

    MDObject mdValueOut3(MDL_REF3D);
    mdJobList.aggregateSingleInt(mdValueOut3, AGGR_MAX ,MDL_REF3D);
    mdValueOut3.getValue(ref3dNum);

    //Check Wiener filter image has correct size
    if (fn_wien != "")
    {
        int x,y,z;
        size_t n;
        getImageSize(fn_wien,x,y,z,n);

        // Get and check padding dimensions
        paddim = ROUND(pad * Xdim);
        if (x != paddim)
            REPORT_ERROR(ERR_VALUE_INCORRECT,
                         "Incompatible padding factor for this Wiener filter");
    }

}

void MpiProgAngularClassAverage::initWeights()
{
    lockArray.initZeros(Ndim+1,ref3dNum+1);
    weightArray.initZeros(Ndim+1,ref3dNum+1);
    weightArrays1.initZeros(Ndim+1,ref3dNum+1);
    weightArrays2.initZeros(Ndim+1,ref3dNum+1);
}

void MpiProgAngularClassAverage::initOutputFiles()
{
    //alloc space for output files
    FileName fn_tmp;

    for (int i = 1; i <= ref3dNum; i++)
    {
        formatStringFast(fn_tmp, "_refGroup%06lu", i);

        unlink((fn_out + fn_tmp + ".xmd").c_str());
        unlink((fn_out + fn_tmp + ".stk").c_str());
        createEmptyFile(fn_out + fn_tmp + ".stk", Xdim, Ydim, Zdim, Ndim, true,
                        WRITE_OVERWRITE);
        if (do_split)
        {
            unlink((fn_out1 + fn_tmp + ".xmd").c_str());
            unlink((fn_out1 + fn_tmp + ".stk").c_str());
            createEmptyFile(fn_out1 + fn_tmp + ".stk", Xdim, Ydim, Zdim, Ndim,
                            true, WRITE_OVERWRITE);
            unlink((fn_out2 + fn_tmp + ".xmd").c_str());
            unlink((fn_out2 + fn_tmp + ".stk").c_str());
            createEmptyFile(fn_out2 + fn_tmp + ".stk", Xdim, Ydim, Zdim, Ndim,
                            true, WRITE_OVERWRITE);
        }
        unlink((fn_out + fn_tmp + "_discarded.xmd").c_str());
    }

}


void MpiProgAngularClassAverage::mpi_postprocess()
{
    // Write class selfile to disc (even if its empty)
    std::cerr<< "mpi_postprocess01"<<std::endl;

    FileName imageName, fileNameXmd, blockNameXmd;
    FileName imageNames1, fileNameXmds1, imageNames2, fileNameXmds2;
    MetaData auxMd,auxMd2,auxMd3;
    size_t order_number;
    int ref3d;
    double weights1, weights2;

    MDValueEQ eq1(MDL_REF3D, 0), eq2(MDL_ORDER, (size_t) 0);
    MDMultiQuery multi;

    String
    comment =
        (String) "This file contains a list of class averages with direction projections and weights.";
    auxMd2.setComment(comment);

    for (int i=1; i<=ref3dNum; i++)
    {
        auxMd.clear();
        auxMd2.clear();

        formatStringFast(fileNameXmd,
                         "refGroup%06lu@%s_RefGroup%06lu.xmd", i, fn_out.c_str(), i);



        auxMd.importObjects(mdJobList, MDValueEQ(MDL_REF3D, i));

        const MDLabel myGroupByLabels[] =
            {
                MDL_REF3D, MDL_ORDER, MDL_REF, MDL_ANGLEROT, MDL_ANGLETILT
            };
        std::vector<MDLabel> groupbyLabels(myGroupByLabels,myGroupByLabels+5);
        auxMd2.aggregateGroupBy(auxMd, AGGR_SUM, groupbyLabels, MDL_COUNT, MDL_WEIGHT);

        auxMd2.addLabel(MDL_IMAGE, 1);
        auxMd2.addLabel(MDL_ANGLEPSI);
        auxMd2.setValueCol(MDL_ANGLEPSI,0.);

        FOR_ALL_OBJECTS_IN_METADATA(auxMd2)
        {
            auxMd2.getValue(MDL_ORDER, order_number,__iter.objId);
            formatStringFast(imageName, "%06lu@%s_refGroup%06lu.stk", order_number,  fn_out.c_str(), i);
            auxMd2.setValue(MDL_IMAGE, imageName,__iter.objId);
        }
        auxMd2.write(fileNameXmd);

        if (do_save_images_assigned_to_classes)
        {
            for (size_t j= 1; j <= Ndim; j++)
            {
                eq1.setValue(i);
                eq2.setValue(j);
                multi.clear();

                multi.addAndQuery(eq1);
                multi.addAndQuery(eq2);

                auxMd3.importObjects(DF, multi);

                if(auxMd3.size() != 0)
                {
                    formatStringFast(blockNameXmd, "orderGroup%06lu_%s", j, fileNameXmd.c_str());
                    auxMd3.write(blockNameXmd, MD_APPEND);
                }
            }
        }
        auxMd2.addLabel(MDL_ENABLED);
        auxMd2.setValueCol(MDL_ENABLED, 1);

        if(do_split)
        {
            MetaData auxMds1(auxMd2);
            MetaData auxMds2(auxMd2);

            formatStringFast(fileNameXmds1,
                             "refGroup%06lu@%s_RefGroup%06lu.xmd", i, fn_out1.c_str(), i);

            formatStringFast(fileNameXmds2,
                             "refGroup%06lu@%s_RefGroup%06lu.xmd", i, fn_out2.c_str(), i);

            FOR_ALL_OBJECTS_IN_METADATA2(auxMds1, auxMds2)
            {
                auxMds1.getValue(MDL_ORDER, order_number,__iter.objId);
                auxMds1.getValue(MDL_REF3D, ref3d,__iter.objId);

                weights1 = dAij(weightArrays1,order_number, ref3d);
                if(weights1 == 0)
                    auxMds1.setValue(MDL_ENABLED,-1,__iter.objId);
                else
                {
                    auxMds1.setValue(MDL_WEIGHT, weights1,__iter.objId);
                    formatStringFast(imageName, "%06lu@%s_refGroup%06lu.stk", order_number,  fn_out1.c_str(), i);
                    auxMds1.setValue(MDL_IMAGE, imageName,__iter.objId);
                }

                weights2 = dAij(weightArrays2,order_number, ref3d);
                if(weights2 == 0)
                    auxMds2.setValue(MDL_ENABLED,-1,__iter2.objId);
                else
                {
                    auxMds2.setValue(MDL_WEIGHT, weights2,__iter2.objId);
                    formatStringFast(imageName, "%06lu@%s_refGroup%06lu.stk", order_number,  fn_out2.c_str(), i);
                    auxMds2.setValue(MDL_IMAGE, imageName,__iter2.objId);
                }

            }
            auxMds1.removeDisabled();
            auxMds1.write(fileNameXmds1);
            auxMds2.removeDisabled();
            auxMds2.write(fileNameXmds2);
        }
    }
    std::cerr<< "mpi_writeFile_11"<<std::endl;


    std::cerr << "lockArray:"  <<std::endl;
    std::cerr << lockArray <<std::endl;
    std::cerr << "weightArray:" <<std::endl;
    std::cerr << weightArray <<std::endl;
    std::cerr << "weightArrays1:" <<std::endl;
    std::cerr << weightArrays1 <<std::endl;
    std::cerr << "weightArrays2:" <<std::endl;
    std::cerr << weightArrays2 <<std::endl;

}

void MpiProgAngularClassAverage::createJobList()
{
    const MDLabel myGroupByLabels[] =
        {
            MDL_REF3D, MDL_DEFGROUP, MDL_ORDER, MDL_REF, MDL_ANGLEROT, MDL_ANGLETILT
        };
    std::vector<MDLabel> groupbyLabels(myGroupByLabels,myGroupByLabels+6);
    mdJobList.aggregateGroupBy(DF, AGGR_COUNT, groupbyLabels, MDL_ORDER, MDL_COUNT);
    nJobs = mdJobList.size();
}


void MpiProgAngularClassAverage::getPolar(MultidimArray<double> &img,
        Polar<std::complex<double> > &fP, bool conjugated, float xoff,
        float yoff)
{
    MultidimArray<double> Maux;
    Polar<double> P;

    // Calculate FTs of polar rings and its stddev
    produceSplineCoefficients(BSPLINE3, Maux, img);
    P.getPolarFromCartesianBSpline(Maux, Ri, Ro, 3, xoff, yoff);
    fourierTransformRings(P, fP, global_plans, conjugated);
}

void MpiProgAngularClassAverage::reAlignClass(Image<double> &avg1,
        Image<double> &avg2, MetaData &SFclass1, MetaData &SFclass2,
        std::vector<Image<double> > imgs, std::vector<int> splits,
        std::vector<int> numbers, size_t dirno, double * my_output)
{
    Polar<std::complex<double> > fPref, fPrefm, fPimg;
    std::vector<double> ccfs(splits.size());
    MultidimArray<double> ang;
    MultidimArray<double> Mimg, Mref, Maux;
    double maxcorr, diff_psi, diff_shift, new_xoff, new_yoff;
    double w1, w2, opt_flip = 0., opt_psi = 0., opt_xoff = 0., opt_yoff = 0.;
    bool do_discard;

    SFclass1.clear();
    SFclass2.clear();
    Mref = avg1() + avg2();
    //#define DEBUG
#ifdef DEBUG

    Image<double> auxImg;
    auxImg() = Mref;
    auxImg.write("ref.xmp");
#endif

    for (int iter = 0; iter < nr_iter; iter++)
    {
        // Initialize iteration
        getPolar(Mref, fPref, true);
        getPolar(Mref, fPrefm, false);
        avg1().initZeros();
        avg2().initZeros();
        w1 = w2 = 0.;

#ifdef DEBUG

        std::cerr<<" entering iter "<<iter<<std::endl;
#endif

        for (int imgno = 0; imgno < imgs.size(); imgno++)
        {
            do_discard = false;
            maxcorr = -99.e99;
            // Rotationally align
            getPolar(imgs[imgno](), fPimg, false, (float) -imgs[imgno].Xoff(),
                     (float) -imgs[imgno].Yoff());
            // A. Check straight image
            rotationalCorrelation(fPimg, fPref, ang, global_transformer);
            for (int k = 0; k < XSIZE(corr); k++)
            {
                if (corr(k) > maxcorr)
                {
                    maxcorr = corr(k);
                    opt_psi = ang(k);
                    opt_flip = 0.;
                }
            }

            // B. Check mirrored image
            rotationalCorrelation(fPimg, fPrefm, ang, global_transformer);
            for (int k = 0; k < XSIZE(corr); k++)
            {
                if (corr(k) > maxcorr)
                {
                    maxcorr = corr(k);
                    opt_psi = realWRAP(360. - ang(k), -180., 180.);
                    opt_flip = 1.;
                }
            }

            // Check max_psi_change in last iteration
            if (iter == nr_iter - 1)
            {
                diff_psi
                = ABS(realWRAP(imgs[imgno].psi() - opt_psi, -180., 180.));
                if (diff_psi > max_psi_change)
                {
                    do_discard = true;
#ifdef DEBUG
                    //std::cerr<<"discard psi: "<<diff_psi<<opt_psi<<" "<<opt_flip<<" "<<imgs[imgno].psi()<<std::endl;
#endif

                }
            }

            // Translationally align
            if (!do_discard)
            {
                if (opt_flip == 1.)
                {
                    // Flip experimental image
                    Matrix2D<double> A(3, 3);
                    A.initIdentity();
                    A(0, 0) *= -1.;
                    A(0, 1) *= -1.;
                    applyGeometry(LINEAR, Mimg, imgs[imgno](), A, IS_INV,
                                  DONT_WRAP);
                    selfRotate(BSPLINE3, Mimg, opt_psi, DONT_WRAP);
                }
                else
                    rotate(BSPLINE3, Mimg, imgs[imgno](), opt_psi, DONT_WRAP);

                if (max_shift > 0)
                {
                    bestShift(Mref, Mimg, opt_xoff, opt_yoff);
                    if (opt_xoff * opt_xoff + opt_yoff * opt_yoff > max_shift
                        * max_shift)
                    {
                        new_xoff = new_yoff = 0.;
                    }
                    else
                    {
                        selfTranslate(BSPLINE3, Mimg,
                                      vectorR2(opt_xoff, opt_yoff), true);
                        new_xoff = opt_xoff * COSD(opt_psi) + opt_yoff
                                   *SIND(opt_psi);
                        new_yoff = -opt_xoff * SIND(opt_psi) + opt_yoff
                                   *COSD(opt_psi);
                    }

                    // Check max_shift_change in last iteration
                    if (iter == nr_iter - 1)
                    {
                        opt_yoff = imgs[imgno].Yoff();
                        opt_xoff = imgs[imgno].Xoff();
                        if (imgs[imgno].flip() == 1.)
                            opt_xoff *= -1.;
                        diff_shift = (new_xoff - opt_xoff) * (new_xoff
                                                              - opt_xoff) + (new_yoff - opt_yoff) * (new_yoff
                                                                                                     - opt_yoff);
                        if (diff_shift > max_shift_change * max_shift_change)
                        {
                            do_discard = true;
#ifdef DEBUG

                            std::cerr <<"discard shift: "<<diff_shift<<" "<<new_xoff<<" "<<opt_xoff<<" "<<imgs[imgno].Xoff()<<" "<<new_yoff<<" "<<opt_yoff<<" "<<imgs[imgno].Yoff()<<std::endl;
#endif

                        }
                    }
                }
            }

            if (!do_discard)
            {
                ccfs[imgno] = correlationIndex(Mref, Mimg);
                imgs[imgno].setPsi(opt_psi);
                imgs[imgno].setFlip(opt_flip);
                imgs[imgno].setShifts(new_xoff, new_yoff);
                if (opt_flip == 1.)
                    imgs[imgno].setShifts(-new_xoff, new_yoff);

                // Check max_shift_change in last iteration
                // Add to averages
                if (splits[imgno] == 0)
                {
                    w1 += 1.;
                    avg1() += Mimg;
                }
                else if (splits[imgno] == 1)
                {
                    w2 += 1.;
                    avg2() += Mimg;
                }
            }
            else
            {
                splits[imgno] = -1;
                ccfs[imgno] = 0.;
            }
        }
        Mref = avg1() + avg2();

    }

    avg1.setWeight(w1);
    avg2.setWeight(w2);

    // Report the new angles, offsets and selfiles
    my_output[4] = imgs.size() * AVG_OUPUT_SIZE;
    for (int imgno = 0; imgno < imgs.size(); imgno++)
    {
        if (splits[imgno] < 0)
            my_output[imgno * AVG_OUPUT_SIZE + 5] = -numbers[imgno];
        else
            my_output[imgno * AVG_OUPUT_SIZE + 5] = numbers[imgno];
        my_output[imgno * AVG_OUPUT_SIZE + 6] = avg1.rot();
        my_output[imgno * AVG_OUPUT_SIZE + 7] = avg1.tilt();
        my_output[imgno * AVG_OUPUT_SIZE + 8] = imgs[imgno].psi();
        my_output[imgno * AVG_OUPUT_SIZE + 9] = imgs[imgno].Xoff();
        my_output[imgno * AVG_OUPUT_SIZE + 10] = imgs[imgno].Yoff();
        my_output[imgno * AVG_OUPUT_SIZE + 11] = (double) dirno;
        my_output[imgno * AVG_OUPUT_SIZE + 12] = imgs[imgno].flip();
        my_output[imgno * AVG_OUPUT_SIZE + 13] = ccfs[imgno];

        if (splits[imgno] == 0)
        {
            SFclass1.setValue(MDL_IMAGE, imgs[imgno].name(),
                              SFclass1.addObject());
        }
        else if (splits[imgno] == 1)
        {
            SFclass2.setValue(MDL_IMAGE, imgs[imgno].name(),
                              SFclass2.addObject());
        }
    }
}

void MpiProgAngularClassAverage::applyWienerFilter(MultidimArray<double> &img)
{
    MultidimArray<std::complex<double> > Faux;

    if (paddim > Xdim)
    {
        // pad real-space image
        int x0 = FIRST_XMIPP_INDEX(paddim);
        int xF = LAST_XMIPP_INDEX(paddim);
        img.selfWindow(x0, x0, xF, xF);
    }
    FourierTransform(img, Faux);
    FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(Mwien)
    {
        dAij(Faux,i,j) *= dAij(Mwien,i,j);
    }
    InverseFourierTransform(Faux, img);
    if (paddim > Xdim)
    {
        // de-pad real-space image
        int x0 = FIRST_XMIPP_INDEX(Xdim);
        int xF = LAST_XMIPP_INDEX(Xdim);
        img.selfWindow(x0, x0, xF, xF);
    }
}

