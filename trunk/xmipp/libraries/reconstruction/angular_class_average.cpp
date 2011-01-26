/***************************************************************************
 *
 * Authors:    Sjors Scheres            scheres@cnb.csic.es (2008)
 *
 * Unidad de Bioinformatica del Centro Nacional de Biotecnologia , CSIC
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

#include "angular_class_average.h"

// Read arguments ==========================================================
void ProgAngularClassAverage::readParams()
{
    // Read command line
    DF.read(getParam("-i"));
    DFlib.read(getParam("--lib"));
    if (checkParam("--add_to"))
    {
        do_add = true;
        fn_out = getParam("--add_to");
    }
    else
    {
        do_add = false;
        fn_out = getParam("-o");
    }
    col_select = getParam("--select");
    if (checkParam("--limitR"))
    {
        limitR = getDoubleParam("--limitR");
        if (limitR < -100. || limitR > 100.)
            REPORT_ERROR(ERR_VALUE_INCORRECT,
                         "limitR should be a percentage: provide values between -100 and 100.");
        if (limitR > 0.)
            do_limitR0 = true;
        else if (limitR < 0.)
        {
            limitR *= -1.;
            do_limitRF = true;
        }
    }
    do_limit0=checkParam("--limit0");
    if (do_limit0)
    {
        limit0 = getDoubleParam("--limit0");
    }
    do_limitF=checkParam("--limitF");
    if (do_limitF)
    {
        limitF = getDoubleParam("--limitF");
    }

    // Perform splitting of the data?
    do_split = checkParameter(argc, argv, "--split");

    // Perform Wiener filtering of average?
    fn_wien = getParameter(argc, argv, "--wien","");
    pad = XMIPP_MAX(1.,getDoubleParam("--pad"));

    // Skip writing selfiles?
    dont_write_selfiles = checkParam("--dont_write_selfiles");

    // Internal re-alignment of the class averages
    Ri      = getIntParam("--Ri");
    Ro      = getIntParam("--Ro");
    nr_iter = getIntParam("--iter");
    max_shift        = getDoubleParam("--max_shift");
    max_shift_change = getDoubleParam("--max_shift_change");
    max_psi_change   = getDoubleParam("--max_psi_change");
}


// Define parameters ==========================================================
void ProgAngularClassAverage::defineParams()
{
    addUsageLine("Make class average images and corresponding selfiles from angular_projection_matching docfiles.");
    addUsageLine("Example of use: Sample at default values and calculating output averages of random halves of the data");
    addUsageLine("   xmipp_angular_class_average  -i proj_match.doc --lib ref_angles.doc -o out_dir --split");

    addParamsLine("    -i <doc_file>         	: Docfile with assigned angles for all experimental particles");
    addParamsLine("   --lib <doc_file>       	: Docfile with angles used to generate the projection matching library");
    addParamsLine("    -o <root_name>        	: Output rootname for class averages and selfiles");
    addParamsLine("or --add_to <root_name>   	: Add output to existing files");
    addParamsLine("   [--split ]             	: Also output averages of random halves of the data");
    addParamsLine("   [--wien <img=\"\"> ]   	: Apply this Wiener filter to the averages");
    addParamsLine("   [--pad <factor=1.> ]    	: Padding factor for Wiener correction");
    addParamsLine("   [--dont_write_selfiles]   : Do not write class selfiles to disc");

    addUsageLine("   IMAGE SELECTION BASED ON INPUT DOCFILE ");
    addParamsLine("   [--select <col=\"\">]    	: Column to use for image selection (limit0, limitF or limitR)");
    addParamsLine("   [--limit0 <l0>]       	: Discard images below <l0>");
    addParamsLine("   [--limitF <lF>]       	: Discard images above <lF>");
    addParamsLine("   [--limitR <lR>]      		: if (lR>0 && lR< 100): discard lowest  <lR> % in each class");
    addParamsLine("                       		: if (lR<0 && lR>-100): discard highest <lR> % in each class");

    addUsageLine("    REALIGNMENT OF CLASSES ");
    addParamsLine("   [--iter <nr_iter=0>]      : Number of iterations for re-alignment");
    addParamsLine("   [--Ri <ri=1>]             : Inner radius to limit rotational search");
    addParamsLine("   [--Ro <r0=-1>]            : Outer radius to limit rotational search");
    addParamsLine("                          	: ro = -1 -> dim/2-1");
    addParamsLine("   [--max_shift <ms=999.>]        : Maximum shift (larger shifts will be set to 0)");
    addParamsLine("   [--max_shift_change <msc=999.>] : Discard images that change shift more in the last iteration ");
    addParamsLine("   [--max_psi_change <mps=360.>]   : Discard images that change psi more in the last iteration ");

    addKeywords("class average images");
}

// Run ====================================================================
void ProgAngularClassAverage::run()
{
    int              i, nmax, nr_ref, nr_images, reserve;
    double           ref_number, rot, tilt, psi, xshift, yshift, mirror;
    double           w, w1, w2;
    Matrix1D<double> dataline(8);

    produceSideInfo();

    // Only for do_add: append input docfile to add_to docfile
    if (do_add)
    {
        FileName fn_tmp=fn_out+".doc";
        if (exists(fn_tmp))
        {
            MetaData DFaux = DF;
            // Don't do any fancy merging or sorting because those functions are really slow...
            DFaux.merge(fn_tmp);
            DFaux.write(fn_tmp);
        }
        else
        {
            DF.write(fn_tmp);
        }
    }

    // Making class averages

    // Reserve memory for output from class realignment
    if (nr_iter > 0)
        reserve = DF.size();
    else
        reserve = 0;
    double output_values[AVG_OUPUT_SIZE*reserve+1];

    nr_ref = DFlib.size();
    init_progress_bar(nr_ref);

    // Loop over all classes

    for (int dirno = 1; dirno <= nr_ref; dirno++)
    {
        // Do the actual work
        processOneClass(dirno, output_values);

        // Output classes sel and doc files
        w = output_values[1];
        w1 = output_values[2];
        w2 = output_values[3];
        addClassAverage(dirno,w,w1,w2);

        // Fill new docfile (with params after realignment)
        size_t id;
        if (nr_iter > 0)
        {
            nr_images = ROUND(output_values[4] / AVG_OUPUT_SIZE);
            for (int i = 0; i < nr_images; i++)
            {
                int this_image = ROUND(output_values[i*AVG_OUPUT_SIZE+5]);
                if (!(this_image < 0))
                {
                    //FIXME: The next line has no sense since the MDL_IMAGE is string
                    // and 'this_image' is of type int...
                    REPORT_ERROR(ERR_UNCLASSIFIED,
                                 "The next line has no sense since the MDL_IMAGE is string \
                                 and 'this_image' is of type int...");
                    id = DF.firstObject(MDValueEQ(MDL_IMAGE,this_image));

                    DF.setValue(MDL_ANGLEROT,output_values[i*AVG_OUPUT_SIZE+6],id);
                    DF.setValue(MDL_ANGLETILT,output_values[i*AVG_OUPUT_SIZE+7],id);
                    DF.setValue(MDL_ANGLEPSI,output_values[i*AVG_OUPUT_SIZE+8],id);
                    DF.setValue(MDL_SHIFTX,output_values[i*AVG_OUPUT_SIZE+9],id);
                    DF.setValue(MDL_SHIFTY,output_values[i*AVG_OUPUT_SIZE+10],id);
                    DF.setValue(MDL_REF,output_values[i*AVG_OUPUT_SIZE+11],id);
                    DF.setValue(MDL_FLIP,output_values[i*AVG_OUPUT_SIZE+12],id);
                    DF.setValue(MDL_MAXCC,output_values[i*AVG_OUPUT_SIZE+13],id);
                }
            }
        }

        progress_bar(dirno);

    }
    progress_bar(nr_ref);

    // Write selfiles and docfiles with all class averages
    finalWriteToDisc();

}


// Side info stuff ===================================================================
void ProgAngularClassAverage::produceSideInfo()
{
    FileName fn_img;

    // Set up output rootnames
    if (do_split)
    {
        fn_out1 = fn_out+"_split_1";
        fn_out2 = fn_out+"_split_2";
    }

    // get column numbers from NewXmipp-type docfile header
    bool auxflip;
    size_t id = DF.firstObject();
    //FIXME: CHECK THIS????
//    if (DF.getValue(MDL_FLIP, auxflip, id))
//        do_mirrors = false;
//    else
//        do_mirrors = true;
    do_mirrors = DF.getValue(MDL_FLIP, auxflip, id);

    // Read empty image with the correct dimensions
    DF.getValue(MDL_IMAGE,fn_img, id);
    Iempty.read(fn_img);
    Iempty().setXmippOrigin();
    Iempty().initZeros();
    dim = XSIZE(Iempty());

    // Read Wiener filter image
    if (fn_wien!="")
    {
        // Get padding dimensions
        paddim = ROUND(pad * dim);
        Image<double> It;
        It.read(fn_wien);
        Mwien=It();
        if (XSIZE(Mwien) != paddim)
        {
            std::cerr<<"image size= "<<dim<<" padding factor= "<<pad<<" padded image size= "<<paddim<<" Wiener filter size= "<<XSIZE(Mwien)<<std::endl;
            REPORT_ERROR(ERR_VALUE_INCORRECT,
                         "Incompatible padding factor for this Wiener filter");
        }
    }
    // Set ring defaults
    if (Ri<1)
        Ri=1;
    if (Ro<0)
        Ro=(dim/2)-1;

    // Set limitR
    if (do_limitR0 || do_limitRF)
    {
        std::vector<double> vals;
        MDLabel codifyLabel=MDL::str2Label(col_select);
        FOR_ALL_OBJECTS_IN_METADATA(DF)
        {
            double auxval;
            DF.getValue(codifyLabel,auxval,__iter.objId);
            vals.push_back(auxval);
        }
        int nn = vals.size();
        std::sort(vals.begin(), vals.end());
        if (do_limitR0)
        {
            double val = vals[ROUND((limitR/100.) * vals.size())];
            if (do_limit0)
                limit0 = XMIPP_MAX(limit0, val);
            else
            {
                limit0 = val;
                do_limit0 = true;
            }
        }
        else if (do_limitRF)
        {
            double val = vals[ROUND(((100. - limitR)/100.) * vals.size())];
            if (do_limitF)
                limitF = XMIPP_MIN(limitF, val);
            else
            {
                limitF = val;
                do_limitF = true;
            }
        }
    }

    // Randomization
    if (do_split)
        randomize_random_generator();

    // Set up FFTW transformers
    MultidimArray<double> Maux;
    Polar<double> P;
    Polar<std::complex<double> > fP;

    produceSplineCoefficients(BSPLINE3,Maux,Iempty());
    P.getPolarFromCartesianBSpline(Maux,Ri,Ro);
    P.calculateFftwPlans(global_plans);
    fourierTransformRings(P,fP,global_plans,false);
    corr.resize(P.getSampleNoOuterRing());
    global_transformer.setReal(corr);
    global_transformer.FourierTransform();
}

void ProgAngularClassAverage::getPolar(MultidimArray<double> &img, Polar<std::complex <double> > &fP,
                                       bool conjugated, float xoff, float yoff)
{
    MultidimArray<double> Maux;
    Polar<double> P;

    // Calculate FTs of polar rings and its stddev
    produceSplineCoefficients(BSPLINE3,Maux,img);
    P.getPolarFromCartesianBSpline(Maux,Ri,Ro,3,xoff,yoff);
    fourierTransformRings(P,fP,global_plans,conjugated);
}

void ProgAngularClassAverage::reAlignClass(Image<double> &avg1,
        Image<double> &avg2,
        MetaData   &SFclass1,
        MetaData   &SFclass2,
        std::vector< Image<double> > imgs,
        std::vector<int> splits,
        std::vector<int> numbers,
        int dirno,
        double * my_output)
{
    Polar<std::complex <double> >   fPref, fPrefm, fPimg;
    std::vector<double>             ccfs(splits.size());
    MultidimArray<double>           ang;
    MultidimArray<double>           Mimg, Mref, Maux;
    double                          maxcorr, diff_psi, diff_shift, new_xoff, new_yoff;
    double                          w1, w2, opt_flip=0., opt_psi=0., opt_xoff=0., opt_yoff=0.;
    bool                            do_discard;

    SFclass1.clear();
    SFclass2.clear();
    Mref = avg1() + avg2();
    //#define DEBUG
#ifdef DEBUG

    Image<double> It;
    It() = Mref;
    It.write("ref.xmp");
#endif


    for (int iter = 0; iter < nr_iter; iter++)
    {
        // Initialize iteration
        getPolar(Mref,fPref,true);
        getPolar(Mref,fPrefm,false);
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
            getPolar(imgs[imgno](),fPimg,false,(float)-imgs[imgno].Xoff(),(float)-imgs[imgno].Yoff());
            // A. Check straight image
            rotationalCorrelation(fPimg,fPref,ang,global_transformer);
            for (int k = 0; k < XSIZE(corr); k++)
            {
                if (corr(k)> maxcorr)
                {
                    maxcorr = corr(k);
                    opt_psi = ang(k);
                    opt_flip = 0.;
                }
            }

            // B. Check mirrored image
            rotationalCorrelation(fPimg,fPrefm,ang,global_transformer);
            for (int k = 0; k < XSIZE(corr); k++)
            {
                if (corr(k)> maxcorr)
                {
                    maxcorr = corr(k);
                    opt_psi = realWRAP(360. - ang(k), -180., 180.);
                    opt_flip = 1.;
                }
            }

            // Check max_psi_change in last iteration
            if (iter == nr_iter - 1)
            {
                diff_psi = ABS(realWRAP(imgs[imgno].psi() - opt_psi, -180., 180.));
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
                    Matrix2D<double> A(3,3);
                    A.initIdentity();
                    A(0, 0) *= -1.;
                    A(0, 1) *= -1.;
                    applyGeometry(LINEAR, Mimg, imgs[imgno](), A, IS_INV, DONT_WRAP);
                    selfRotate(BSPLINE3,Mimg,opt_psi,DONT_WRAP);
                }
                else
                    rotate(BSPLINE3,Mimg,imgs[imgno](),opt_psi,DONT_WRAP);

                if (max_shift > 0)
                {
                    best_shift(Mref,Mimg,opt_xoff,opt_yoff);
                    if (opt_xoff * opt_xoff + opt_yoff * opt_yoff > max_shift * max_shift)
                    {
                        new_xoff = new_yoff = 0.;
                    }
                    else
                    {
                        selfTranslate(BSPLINE3,Mimg,vectorR2(opt_xoff,opt_yoff),true);
                        new_xoff =  opt_xoff*COSD(opt_psi) + opt_yoff*SIND(opt_psi);
                        new_yoff = -opt_xoff*SIND(opt_psi) + opt_yoff*COSD(opt_psi);
                    }

                    // Check max_shift_change in last iteration
                    if (iter == nr_iter - 1)
                    {
                        opt_yoff = imgs[imgno].Yoff();
                        opt_xoff = imgs[imgno].Xoff();
                        if (imgs[imgno].flip() == 1.)
                            opt_xoff *= -1.;
                        diff_shift = (new_xoff - opt_xoff) * (new_xoff - opt_xoff) +
                                     (new_yoff - opt_yoff) * (new_yoff - opt_yoff);
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
                ccfs[imgno] = correlation_index(Mref,Mimg);
                imgs[imgno].setPsi(opt_psi);
                imgs[imgno].setFlip(opt_flip);
                imgs[imgno].setShifts(new_xoff,new_yoff);
                if (opt_flip==1.)
                    imgs[imgno].setShifts(-new_xoff,new_yoff);

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
#ifdef DEBUG
        //It() = Mref;
        //It.weight()=w1+w2;
        //It.write("ref.xmp");
#endif

    }

    avg1.setWeight(w1);
    avg2.setWeight(w2);

    // Report the new angles, offsets and selfiles
    my_output[4] = imgs.size() * AVG_OUPUT_SIZE;
    for (int imgno = 0; imgno < imgs.size(); imgno++)
    {
        if (splits[imgno] < 0)
            my_output[imgno * AVG_OUPUT_SIZE + 5]  = -numbers[imgno];
        else
            my_output[imgno * AVG_OUPUT_SIZE + 5]  = numbers[imgno];
        my_output[imgno * AVG_OUPUT_SIZE + 6]  = avg1.rot();
        my_output[imgno * AVG_OUPUT_SIZE + 7]  = avg1.tilt();
        my_output[imgno * AVG_OUPUT_SIZE + 8]  = imgs[imgno].psi();
        my_output[imgno * AVG_OUPUT_SIZE + 9]  = imgs[imgno].Xoff();
        my_output[imgno * AVG_OUPUT_SIZE + 10] = imgs[imgno].Yoff();
        my_output[imgno * AVG_OUPUT_SIZE + 11] = dirno;
        my_output[imgno * AVG_OUPUT_SIZE + 12] = imgs[imgno].flip();
        my_output[imgno * AVG_OUPUT_SIZE + 13] = ccfs[imgno];

        if (splits[imgno] == 0)
        {
            SFclass1.setValue(MDL_IMAGE,imgs[imgno].name(), SFclass1.addObject());
        }
        else if (splits[imgno] == 1)
        {
            SFclass2.setValue(MDL_IMAGE,imgs[imgno].name(), SFclass2.addObject());
        }
    }
}

void ProgAngularClassAverage::applyWienerFilter(MultidimArray<double> &img)
{
    MultidimArray<std::complex<double> > Faux;
    if (paddim > dim)
    {
        // pad real-space image
        int x0 = FIRST_XMIPP_INDEX(paddim);
        int xF = LAST_XMIPP_INDEX(paddim);
        img.window(x0, x0, xF,xF, 0.);
    }
    FourierTransform(img,Faux);
    FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(Mwien)
    {
        dAij(Faux,i,j) *= dAij(Mwien,i,j);
    }
    InverseFourierTransform(Faux,img);
    if (paddim > dim)
    {
        // de-pad real-space image
        int x0 = FIRST_XMIPP_INDEX(dim);
        int xF = LAST_XMIPP_INDEX(dim);
        img.window(x0, x0, xF,xF, 0.);
    }
}

void ProgAngularClassAverage::processOneClass(int &dirno,
        double * my_output)
{
    Image<double> img, avg, avg1, avg2;
    FileName   fn_img, fn_tmp;
    MetaData   SFclass, SFclass1, SFclass2;
    double     rot, tilt, psi, xshift, yshift, mirror, val, w, w1, w2, my_limitR;
    int        ref_number, this_image;
    int        isplit;
    Matrix2D<double> A(3,3);
    std::vector<int> exp_number, exp_split;
    std::vector< Image<double> > exp_imgs;

    // Get reference angles and preset to averages
    DFlib.getValue(MDL_ANGLEROT,rot,dirno);
    DFlib.getValue(MDL_ANGLETILT,tilt,dirno);
    Iempty.setEulerAngles(rot, tilt, 0.);
    Iempty.setShifts(0., 0.);
    Iempty.setFlip(0.);
    avg=Iempty;
    avg1=Iempty;
    avg2=Iempty;

    // Loop over all images in the input docfile
    w = 0.;
    w1 = 0.;
    w2 = 0.;
    this_image=0;
    MetaData _DF;
    _DF.importObjects(DF,MDValueEQ(MDL_REF,dirno));
    if (_DF.size()==0)
    {//no images asigned to this class
        my_output[0] = (double)dirno;
        my_output[1] = w;
        my_output[2] = w1;
        my_output[3] = w2;
        return;
    }
    FOR_ALL_OBJECTS_IN_METADATA(_DF)
    {
        _DF.getValue(MDL_IMAGE,fn_img, __iter.objId);
        this_image++;
        //_DF.getValue(MDL_REF,ref_number);
        //if (ref_number == dirno)
        {
            bool is_selected = true;
            double auxval;
            _DF.getValue(MDL::str2Label(col_select),auxval, __iter.objId);
            if (do_limit0)
            {
                if (auxval < limit0)
                    is_selected = false;
            }
            if (do_limitF)
            {
                if (auxval > limitF)
                    is_selected = false;
            }
            if (is_selected)
            {
                _DF.getValue(MDL_ANGLEPSI, psi, __iter.objId);
                _DF.getValue(MDL_SHIFTX, xshift, __iter.objId);
                _DF.getValue(MDL_SHIFTY, yshift, __iter.objId);
                if (do_mirrors)
                    _DF.getValue(MDL_FLIP,mirror, __iter.objId);
                //TODO: Check this????
                img.read(fn_img);
                img().setXmippOrigin();
                img.setEulerAngles(0., 0., psi);
                img.setShifts(xshift, yshift);
                if (do_mirrors)
                    img.setFlip(mirror);

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
                    selfApplyGeometry(BSPLINE3, img(), A, IS_INV,WRAP);

                // Add to average
                if (isplit==0)
                {
                    avg1() += img();
                    w1 += 1.;
                    SFclass1.setValue(MDL_IMAGE,fn_img, SFclass1.addObject());
                }
                else
                {
                    avg2() += img();
                    w2 += 1.;
                    SFclass2.setValue(MDL_IMAGE,fn_img, SFclass2.addObject());
                }
            }
        }
    }
    if (SFclass1.size()==0 && SFclass2.size()==0)
    {//no images asigned to this class
        my_output[0] = (double)dirno;
        my_output[1] = w;
        my_output[2] = w1;
        my_output[3] = w2;
        return;
    }

    // Re-alignment of the class
    if (nr_iter > 0)
    {
        SFclass=SFclass1;
        SFclass.unionAll(SFclass2);
        avg() = avg1() + avg2();
        w = w1 + w2;
        avg.setWeight(w);
        writeToDisc(avg,dirno,SFclass,fn_out,false,"ref.xmp");
        reAlignClass(avg1, avg2, SFclass1, SFclass2,
                     exp_imgs, exp_split, exp_number,
                     dirno, my_output);
        w1 = avg1.weight();
        w2 = avg2.weight();
    }

    // Apply Wiener filters
    if (fn_wien != "")
    {
        applyWienerFilter(avg1());
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
    writeToDisc(avg,dirno,SFclass,fn_out+"_class",!dont_write_selfiles);
    if (do_split)
    {
        writeToDisc(avg1,dirno,SFclass1,fn_out1+"_class",!dont_write_selfiles);
        writeToDisc(avg2,dirno,SFclass2,fn_out2+"_class",!dont_write_selfiles);
    }

    my_output[0] = (double)dirno;
    my_output[1] = w;
    my_output[2] = w1;
    my_output[3] = w2;

}

void ProgAngularClassAverage::writeToDisc(Image<double> avg,
        int        dirno,
        MetaData    SF,
        FileName   fn,
        bool       write_selfile,
        FileName   oext)
{
    FileName   fn_tmp;
    double     w = avg.weight(), w_old;
    Image<double> old;
    MetaData    SFold;

    if (w > 0.)
    {
        avg()/=w;
        // Write class average to disc
        fn_tmp.compose(fn,dirno,oext);
        if (do_add && exists(fn_tmp) )
        {
            old.read(fn_tmp);
            w_old = old.weight();
            FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(old())
            {
                dAij(old(),i,j) = ( w_old * dAij(old(),i,j) + w * dAij(avg(),i,j) ) / (w_old + w);
            }
            old.setWeight(w_old + w);
            old.write(fn_tmp);
        }
        else
        {
            avg.write(fn_tmp);
        }
        // Write class selfile to disc
        if (write_selfile)
        {
            fn_tmp.compose(fn,dirno,"sel");
            if (do_add && exists(fn_tmp) )
            {
                SF.merge(fn_tmp);
                MetaData SFaux;
                SFaux.sort(SF,MDL_IMAGE);
                SF=SFaux;
            }
            SF.write(fn_tmp);
        }

        if (ROUND(w) != SF.size())
        {
            std::cerr<<" w = "<< w <<" SF.size()= "<< SF.size() <<" dirno = "<<dirno<<std::endl;
            REPORT_ERROR(ERR_MD_OBJECTNUMBER,"Selfile and average weight do not correspond!");
        }
    }
}


void ProgAngularClassAverage::addClassAverage(int dirno,
        double w,
        double w1,
        double w2)
{
    double rot, tilt;
    FileName fn_tmp;

    DFlib.getValue(MDL_ANGLEROT,rot,dirno);
    DFlib.getValue(MDL_ANGLETILT,tilt,dirno);
    double d=0.;
    bool f=false;
    size_t id;
    if (w > 0.)
    {
        fn_tmp.compose(fn_out+"_class",dirno,"xmp");
        id = SFclasses.addObject();
        SFclasses.setValue(MDL_IMAGE,fn_tmp,id);
        SFclasses.setValue(MDL_ANGLEROT,rot,id);
        SFclasses.setValue(MDL_ANGLETILT,tilt,id);
        //this may help to keep compatibility with the next program
        SFclasses.setValue(MDL_ANGLEPSI,d,id);
        SFclasses.setValue(MDL_SHIFTX,d,id);
        SFclasses.setValue(MDL_SHIFTY,d,id);

        SFclasses.setValue(MDL_WEIGHT,w,id);
        //this may help to keep compatibility with the next program
        SFclasses.setValue(MDL_FLIP,f,id);

    }
    if (do_split)
    {
        if (w1 > 0.)
        {
            fn_tmp.compose(fn_out1+"_class",dirno,"xmp");
            id = SFclasses1.addObject();
            SFclasses1.setValue(MDL_IMAGE,fn_tmp,id);
            SFclasses1.setValue(MDL_ANGLEROT,rot,id);
            SFclasses1.setValue(MDL_ANGLETILT,tilt,id);
            //this may help to keep compatibility with the next program
            SFclasses1.setValue(MDL_ANGLEPSI,d,id);
            SFclasses1.setValue(MDL_SHIFTX,d,id);
            SFclasses1.setValue(MDL_SHIFTY,d,id);

            SFclasses1.setValue(MDL_WEIGHT,w1,id);

            //this may help to keep compatibility with the next program
            SFclasses1.setValue(MDL_FLIP,f,id);
        }
        if (w2 > 0.)
        {
            fn_tmp.compose(fn_out2+"_class",dirno,"xmp");
            id = SFclasses2.addObject();
            SFclasses2.setValue(MDL_IMAGE,fn_tmp,id);
            SFclasses2.setValue(MDL_ANGLEROT,rot,id);
            SFclasses2.setValue(MDL_ANGLETILT,tilt,id);
            //this may help to keep compatibility with the next program
            SFclasses2.setValue(MDL_ANGLEPSI,d,id);
            SFclasses2.setValue(MDL_SHIFTX,d,id);
            SFclasses2.setValue(MDL_SHIFTY,d,id);

            SFclasses2.setValue(MDL_WEIGHT,w2,id);

            //this may help to keep compatibility with the next program
            SFclasses2.setValue(MDL_FLIP,f,id);
        }
    }
}

void ProgAngularClassAverage::finalWriteToDisc()
{
    FileName fn_tmp;
    MetaData  auxSF, auxDF;

    // Write selfiles containing all classes
    fn_tmp=fn_out+"_classes.sel";
    if (do_add && exists(fn_tmp))
        SFclasses.merge(fn_tmp);
    auxSF.sort(SFclasses,MDL_IMAGE);
    auxSF.write(fn_tmp);
    if (do_split)
    {
        fn_tmp=fn_out1+"_classes.sel";
        if (do_add && exists(fn_tmp))
            SFclasses1.merge(fn_tmp);
        SFclasses1.write(fn_tmp);
        fn_tmp=fn_out2+"_classes.sel";
        if (do_add && exists(fn_tmp))
            SFclasses2.merge(fn_tmp);
        SFclasses2.write(fn_tmp);
    }

    // Write docfiles with angles and weights of all classes
    fn_tmp=fn_out+"_classes.doc";
    if (do_add && exists(fn_tmp))
    {
        MetaData MDaux;
        MDaux.read(fn_tmp);
        MDaux.unionAll(SFclasses);
        SFclasses.aggregate(MDaux, AGGR_SUM, MDL_IMAGE, MDL_WEIGHT, MDL_SUM);
    }
    SFclasses.write(fn_tmp);
    if (do_split)
    {
        fn_tmp=fn_out1+"_classes.doc";
        if (do_add && exists(fn_tmp))
        {
            MetaData MDaux;
            MDaux.read(fn_tmp);
            MDaux.unionAll(SFclasses1);
            SFclasses1.aggregate(MDaux, AGGR_SUM,MDL_IMAGE,MDL_WEIGHT,MDL_SUM);
        }
        SFclasses1.write(fn_tmp);
        fn_tmp=fn_out2+"_classes.doc";
        if (do_add && exists(fn_tmp))
        {
            MetaData MDaux;
            MDaux.read(fn_tmp);
            MDaux.unionAll(SFclasses2);
            SFclasses2.aggregate(MDaux, AGGR_SUM,MDL_IMAGE,MDL_WEIGHT,MDL_SUM);
        }
        SFclasses2.write(fn_tmp);
    }

    // Write docfile with data for all realigned individual images
    if (nr_iter > 0)
    {
        fn_tmp=fn_out+"_realigned.doc";
        if (do_add && exists(fn_tmp))
            DF.merge(fn_tmp);
        DF.write(fn_tmp);
    }
}
