
/***************************************************************************
 *
 * Authors:     Sjors H.W. Scheres (scheres@cnb.csic.es)
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

#include "ctf_group.h"
#include <data/fft.h>
#include <data/metadata_extension.h>

/* Read parameters from command line. -------------------------------------- */
void ProgCtfGroup::readParams()
{
    fn_sel        = getParam("-i");
    fn_ctfdat     = getParam("--ctfdat");
    fn_root       = getParam("-o");
    phase_flipped = checkParam("--phase_flipped");
    do_discard_anisotropy = checkParam("--discard_anisotropy");
    do_auto       = !checkParam("--split");
    pad           = XMIPP_MAX(1., getDoubleParam("--pad"));
    if (do_auto)
    {
        max_error     = getDoubleParam("--error");
        resol_error   = getDoubleParam("--resol");
    }
    else
    {
        fn_split = getParam("--split");
    }
    do_wiener = checkParam("--wiener");
    wiener_constant = getDoubleParam("--wc");
}

/* Show -------------------------------------------------------------------- */
void ProgCtfGroup::show()
{
    std::cerr << "  Input sel file          : "<< fn_sel << std::endl;
    std::cerr << "  Input ctfdat file       : "<< fn_ctfdat << std::endl;
    std::cerr << "  Output rootname         : "<< fn_root << std::endl;
    if (pad > 1.)
    {
        std::cerr << "  Padding factor          : "<< pad << std::endl;
    }
    if (do_discard_anisotropy)
    {
        std::cerr << " -> Exclude anisotropic CTFs from the groups"<<std::endl;
    }
    if (do_auto)
    {
        std::cerr << " -> Using automated mode for making groups"<<std::endl;
        std::cerr << " -> With a maximum allowed error of "<<max_error
        <<" at "<<resol_error<<" dig. freq."<<std::endl;
    }
    else
    {
        std::cerr << " -> Group based on defocus values in "<<fn_split<<std::endl;
    }
    if (phase_flipped)
    {
        std::cerr << " -> Assume that data are PHASE FLIPPED"<<std::endl;
    }
    else
    {
        std::cerr << " -> Assume that data are NOT PHASE FLIPPED"<<std::endl;
    }
    if (do_wiener)
    {
        std::cerr << " -> Also calculate Wiener filters, with constant= "<<wiener_constant<<std::endl;
    }
    std::cerr << "----------------------------------------------------------"<<std::endl;
}

/* Usage ------------------------------------------------------------------- */
void ProgCtfGroup::defineParams()
{
    addUsageLine("Generate CTF (or defocus) groups from a single CTFdat file");
    addUsageLine("Example of use: Sample using automated mode (resolution = 15 Ang.)");
    addUsageLine("   xmipp_ctf_group -i input.sel --ctfdat input.ctfdat --pad 1.5 --wiener --phase_flipped --resol 15");
    addUsageLine("Example of use: Sample using manual mode (after manual editing of ctf_group_split.doc)");
    addUsageLine("   xmipp_ctf_group -i input.sel --ctfdat input.ctfdat --pad 1.5 --wiener --phase_flipped --split ctf_group_split.doc");

    addParamsLine("   -i <sel_file>              : Input selfile");
    addParamsLine("   --ctfdat <ctfdat_file>     : Input CTFdat file for all data");
    addParamsLine("   [-o <oext=\"ctf\">]        : Output root name");
    addParamsLine("   [--pad <float=1>]          : Padding factor ");
    addParamsLine("   [--phase_flipped]          : Output filters for phase-flipped data");
    addParamsLine("   [--discard_anisotropy]     : Exclude anisotropic CTFs from groups");
    addParamsLine("   [--wiener]                 : Also calculate Wiener filters");
    addParamsLine("   [--wc <float=-1>]          : Wiener-filter constant (if < 0: use FREALIGN default)");
    addParamsLine(" == MODE 1: AUTOMATED: == ");
    addParamsLine("   [--error <float=0.5> ]     : Maximum allowed error");
    addParamsLine("   [--resol <float=-1> ]      : Resol. (in Ang) for error calculation. Default (-1) = Nyquist");
    addParamsLine(" == MODE 2: MANUAL: == ");
    addParamsLine("   [--split <docfile> ]       : 1-column docfile with defocus values where to split the data ");

}

/* Produce Side information ------------------------------------------------ */
void ProgCtfGroup::produceSideInfo()
{
    FileName fnt_img, fnt_ctf, fnt;
    Image<double> img;
    CTFDescription ctf;
    MetaData ctfdat, SF;
    MultidimArray<double> Mctf;
    MultidimArray<std::complex<double> >  ctfmask;
    std::vector<FileName> dum;
    int ydim, imgno;
    bool is_unique, found, is_first;
    double avgdef;
    std::vector<FileName> mics_fnctf;

    std::cerr<<" Reading all data ..."<<std::endl;
    SF.read(fn_sel);
    ImgSize(SF,dim,ydim);
    if ( dim != ydim )
        REPORT_ERROR(ERR_MULTIDIM_SIZE,"Only squared images are allowed!");

    paddim = ROUND(pad*dim);
    Mctf.resize(paddim,paddim);
    ctfdat.read(fn_ctfdat);

    if (do_wiener)
    {
        Mwien.resize(paddim,paddim);
        Mwien.initZeros();
    }

    mics_fnctf.clear();
    is_first=true;
    imgno = 0;
    int n= SF.size();
    int c = XMIPP_MAX(1, n / 60);
    init_progress_bar(n);
    FOR_ALL_OBJECTS_IN_METADATA(SF)
    {
        SF.getValue(MDL_IMAGE,fnt,__iter.objId);

        // Find which CTF group it belongs to
        found = false;
        FOR_ALL_OBJECTS_IN_METADATA(ctfdat)
        {
            ctfdat.getValue(MDL_IMAGE,fnt_img,__iter.objId);
            ctfdat.getValue(MDL_CTFMODEL,fnt_ctf,__iter.objId);

            if (fnt_img == fnt)
            {
                found = true;
                is_unique=true;
                for (int i = 0; i< mics_fnctf.size(); i++)
                {
                    if (fnt_ctf == mics_fnctf[i])
                    {
                        is_unique=false;
                        mics_fnimgs[i].push_back(fnt_img);
                        mics_count[i]++;
                        //std::cerr<<" add "<<fnt_img<<" to mic "<<i<<"count= "<<mics_count[i]<<std::endl;
                        break;
                    }
                }
                if (is_unique)
                {
                    // Read CTF in memory
                    ctf.read(fnt_ctf);
                    ctf.enable_CTF = true;
                    ctf.enable_CTFnoise = false;
                    ctf.Produce_Side_Info();
                    if (is_first)
                    {
                        pixel_size = ctf.Tm;
                        if (do_auto)
                        {
                            if (resol_error < 0)
                            {
                                // Set to Nyquist
                                resol_error = 2. * pixel_size;
                            }
                            // Set resolution limits in dig freq:
                            resol_error = pixel_size / resol_error;
                            resol_error = XMIPP_MIN(0.5, resol_error);
                            // and in pixels:

                            iresol_error = ROUND(resol_error * paddim);
                            //std::cerr<<" Resolution for error limit = "<<resol_error<< " (dig. freq.) = "<<iresol_error<<" (pixels)"<<std::endl;
                        }
                        is_first=false;
                    }
                    else if (pixel_size != ctf.Tm)
                        REPORT_ERROR(ERR_VALUE_INCORRECT,
                                     "Cannot mix CTFs with different sampling rates!");
                    if (!do_discard_anisotropy || isIsotropic(ctf))
                    {
                        avgdef = (ctf.DeltafU + ctf.DeltafV)/2.;
                        ctf.DeltafU = avgdef;
                        ctf.DeltafV = avgdef;
                        ctf.Generate_CTF(paddim, paddim, ctfmask);
                        FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(ctfmask)
                        {
                            if (phase_flipped)
                                dAij(Mctf, i, j) = fabs(dAij(ctfmask, i, j).real());
                            else
                                dAij(Mctf, i, j) = dAij(ctfmask, i, j).real();
                        }
                        // Fill vectors
                        mics_fnctf.push_back(fnt_ctf);
                        mics_count.push_back(1);
                        mics_fnimgs.push_back(dum);
                        mics_fnimgs[mics_fnimgs.size()-1].push_back(fnt_img);
                        mics_ctf2d.push_back(Mctf);
                        mics_defocus.push_back(avgdef);
                        //std::cerr<<" uniq "<<fnt_img<<" in mic "<<mics_fnimgs.size()-1<<"def= "<<avgdef<<std::endl;
                    }
                    else
                    {
                        std::cerr<<" Discard CTF "<<fnt_ctf<<" because of too large anisotropy"<<std::endl;
                    }
                }
                break;
            }
        }
        if (!found)
            REPORT_ERROR(ERR_MD_NOOBJ, "Did not find image "+fnt+" in the CTFdat file");
        imgno++;
        if (imgno % c == 0)
            progress_bar(imgno);
    }
    progress_bar(n);

    // Precalculate denominator term of the Wiener filter
    if (do_wiener)
    {
        double sumimg = 0.;
        for (int imic=0; imic < mics_count.size(); imic++)
        {
            sumimg += (double)mics_count[imic];
            FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(Mwien)
            {
                dAij(Mwien,i,j) += mics_count[imic] * dAij(mics_ctf2d[imic],i,j) *
                                   dAij(mics_ctf2d[imic],i,j);
            }
        }
        // Divide by sumimg (Wiener filter is for summing images, not averaging!)
        Mwien /= sumimg;
        // Add Wiener constant
        if (wiener_constant < 0.)
        {
            // Use Grigorieff's default for Wiener filter constant: 10% of average over all Mwien terms
            // Grigorieff JSB 157(1) (2006), pp 117-125
            wiener_constant = 0.1 * Mwien.computeAvg();
        }
        Mwien += wiener_constant;
    }

    // Now that we have all information, sort micrographs on defocus value
    // And update all corresponding pointers and data vectors.
    std::vector<MultidimArray<double> > newmics_ctf2d=mics_ctf2d;
    std::vector<int> newmics_count=mics_count;
    std::vector< std::vector <FileName> > newmics_fnimgs=mics_fnimgs;
    std::vector<double> sorted_defocus=mics_defocus;

    std::sort(sorted_defocus.begin(), sorted_defocus.end());
    std::reverse(sorted_defocus.begin(), sorted_defocus.end());
    for (int isort = 0; isort < sorted_defocus.size(); isort++)
        for (int imic=0; imic < mics_defocus.size(); imic++)
            if (ABS(sorted_defocus[isort] -  mics_defocus[imic]) < XMIPP_EQUAL_ACCURACY)
            {
                newmics_ctf2d[isort]  = mics_ctf2d[imic];
                newmics_count[isort]  = mics_count[imic];
                newmics_fnimgs[isort] = mics_fnimgs[imic];
                // Make sure this micrograph never gets counted again
                // (This could happen if two mics have exactly the same defocus value!)
                mics_defocus[imic] = sorted_defocus[0] + 1.;
                break;
            }
    mics_ctf2d   = newmics_ctf2d;
    mics_count   = newmics_count;
    mics_fnimgs  = newmics_fnimgs;
    mics_defocus = sorted_defocus;

    /*
        for (int isort = 0; isort < sorted_defocus.size(); isort++)
            std::cerr<<"sorted mic "<<isort<<" has "<<mics_count[isort]<<" images, and defocus "<<mics_defocus[isort] <<std::endl;
    */
}

// Check whether a CTF is anisotropic
bool ProgCtfGroup::isIsotropic(CTFDescription &ctf)
{
    double xp, yp, xpp, ypp;
    double cosp, sinp, ctfp, diff;
    Matrix1D<double> freq(2);

    cosp = COSD(ctf.azimuthal_angle);
    sinp = SIND(ctf.azimuthal_angle);

    for (double digres = 0; digres < resol_error; digres+= 0.001)
    {
        XX(freq) = cosp * digres;
        YY(freq) = sinp * digres;
        digfreq2contfreq(freq, freq, pixel_size);
        ctf.precomputeValues(XX(freq), YY(freq));
        ctfp = ctf.CTF_at();
        ctf.precomputeValues(YY(freq), XX(freq));
        diff = ABS(ctfp - ctf.CTF_at());
        if (diff > max_error)
        {
            std::cerr<<" Anisotropy!"<<digres<<" "<<max_error<<" "<<diff<<" "<<ctfp
            <<" "<<ctf.CTF_at()<<std::endl;
            return false;
        }
    }
    return true;
}

// Do the actual work
void ProgCtfGroup::autoRun()
{
    double diff, mindiff;
    int iopt_group, nr_groups;
    std::vector<int> dum;

    // Make the actual groups
    nr_groups = 0;
    init_progress_bar(mics_ctf2d.size());
    for (int imic=0; imic < mics_ctf2d.size(); imic++)
    {
        mindiff = 99999.;
        iopt_group = -1;
        for (int igroup=0; igroup < nr_groups; igroup++)
        {
            // loop over all mics in this group
            for (int igmic=0; igmic < pointer_group2mic[igroup].size(); igmic++)
            {

                for (int iresol=0; iresol<=iresol_error; iresol++)
                {
                    diff = ABS( dAij(mics_ctf2d[imic],iresol,0) -
                                dAij(mics_ctf2d[pointer_group2mic[igroup][igmic]],iresol,0) );
                    if (diff > max_error)
                    {
                        break;
                    }
                }
                if (diff <= max_error && diff < mindiff)
                {
                    mindiff = diff;
                    iopt_group = igroup;
                }
            }
        }
        if (iopt_group < 0)
        {
            // add new group
            pointer_group2mic.push_back(dum);
            pointer_group2mic[nr_groups].push_back(imic);
            nr_groups++;
        }
        else
        {
            //add to existing group
            pointer_group2mic[iopt_group].push_back(imic);
        }
        progress_bar(imic);
    }
    progress_bar(mics_ctf2d.size());
    std::cerr<<" Number of CTF groups= "<<nr_groups<<std::endl;
}

void ProgCtfGroup::manualRun()
{
    MetaData DF;
    double defocus, split, oldsplit=99.e99;
    int count, nr_groups = 0;
    Matrix1D<double> dataline(1);
    std::vector<int> dum;

    pointer_group2mic.clear();
    DF.read(fn_split);
    // Append -99.e-99 to the end of the docfile
    DF.addObject();
    DF.setValue(MDL_CTF_DEFOCUSU,-99.e99);

    // Loop over all splits
    int n=DF.size();
    int in = 0;
    init_progress_bar(n);
    FOR_ALL_OBJECTS_IN_METADATA(DF)
    {
        DF.getValue(MDL_CTF_DEFOCUSU,split,__iter.objId);
        count = 0;
        for (int imic=0; imic < mics_ctf2d.size(); imic++)
        {
            defocus = mics_defocus[imic];
            if (defocus < oldsplit && defocus >= split)
            {
                if (count == 0)
                {
                    // add new group
                    pointer_group2mic.push_back(dum);
                }
                pointer_group2mic[nr_groups].push_back(imic);
                count++;
            }
        }
        if (count == 0)
        {
            std::cerr<<" Warning: group with defocus values between "<<oldsplit<<" and "<<split<<" is empty!"<<std::endl;
        }
        else
        {
            nr_groups++;
        }
        oldsplit = split;
        in++;
        progress_bar(in);
    }
    progress_bar(n);
}

void ProgCtfGroup::writeOutputToDisc()
{
    FileName fnt;
    MetaData SFo, DFo;
    Image<double> img;
    MultidimArray<double> Mavg;
    double sumw, avgdef, mindef, maxdef, split, oldmin=99.e99;
    int imic;
    std::ofstream fh, fh2, fh3, fh4;

    fh.open((fn_root+ "_groups.imgno").c_str(), std::ios::out);
    fh  << "# Number of images in each group \n";
    fh3.open((fn_root+ "_groups.defocus").c_str(), std::ios::out);
    fh3 << "# Defocus values for each group (avg, max & min) \n";

    for (int igroup=0; igroup < pointer_group2mic.size(); igroup++)
    {
        SFo.clear();
        SFo.setComment("Defocus values to split into "+
                       integerToString(pointer_group2mic.size())+" ctf groups");
        Mavg.initZeros(paddim,paddim);
        sumw = 0.;
        avgdef = 0.;
        mindef = 99.e99;
        maxdef = -99.e99;
        for (int igmic=0; igmic < pointer_group2mic[igroup].size(); igmic++)
        {
            imic = pointer_group2mic[igroup][igmic];
            sumw += (double) mics_count[imic];
            // calculate (weighted) average Mctf
            Mavg += mics_count[imic] * mics_ctf2d[imic];
            // Calculate avg, min and max defocus values in this group
            avgdef += mics_count[imic] * mics_defocus[imic];
            mindef = XMIPP_MIN(mics_defocus[imic],mindef);
            maxdef = XMIPP_MAX(mics_defocus[imic],maxdef);
            // Fill SelFile
            size_t id;
            for (int iimg=0; iimg < mics_fnimgs[imic].size(); iimg++)
            {
                id = SFo.addObject();
                SFo.setValue(MDL_IMAGE,mics_fnimgs[imic][iimg], id);
            }
        }
        Mavg /= sumw;
        avgdef /= sumw;
        fnt.compose(fn_root+"_group",igroup+1,"");
        std::cerr<<" Group "<<fnt <<" contains "<< pointer_group2mic[igroup].size()<<" ctfs and "<<sumw<<" images and has average defocus "<<avgdef<<std::endl;

        // 1. write selfile
        SFo.write(fnt+".sel");
        // 2. write average Mctf
        img() = Mavg;
        img.setWeight(sumw);
        img.write(fnt+".ctf");
        // 3. Output to file with numinsertber of images per group
        fh << integerToString(igroup+1);
        fh.width(10);
        fh << floatToString(sumw)<<std::endl;
        // 4. Output to file with avgdef, mindef and maxdef per group
        fh3 << integerToString(igroup+1);
        fh3.width(10);
        fh3 << floatToString(avgdef);
        fh3.width(10);
        fh3 << floatToString(maxdef);
        fh3.width(10);
        fh3 << floatToString(mindef)<<std::endl;
        // 5. Output to docfile for manual grouping
        if (oldmin < 9.e99)
        {
            split = (oldmin + maxdef) / 2.;
            DFo.addObject();
            DFo.setValue(MDL_CTF_DEFOCUSU,split);
        }
        oldmin = mindef;
        // 6. Write file with 1D profiles
        fh2.open((fnt+".ctf_profiles").c_str(), std::ios::out);
        for (int i=0; i < paddim/2; i++)
        {
            fh2 << floatToString((double)i / (pixel_size*paddim));
            fh2.width(10);
            fh2 << floatToString(dAij(Mavg,i,0));
            fh2.width(10);
            for (int igmic=0; igmic < pointer_group2mic[igroup].size(); igmic++)
            {
                imic = pointer_group2mic[igroup][igmic];
                fh2 << floatToString(dAij(mics_ctf2d[imic],i,0));
                fh2.width(10);
            }
            fh2 << std::endl;
        }
        fh2.close();
        // 7. Write Wiener filter
        if (do_wiener)
        {
            FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(Mavg)
            {
                dAij(Mavg,i,j) /= dAij(Mwien,i,j);
            }
            img() = Mavg;
            img.setWeight(sumw);
            img.write(fnt+".wien");
            fh4.open((fnt+".wien_profile").c_str(), std::ios::out);
            for (int i=0; i < paddim/2; i++)
            {
                fh4 << floatToString((double)i / (pixel_size*paddim));
                fh4.width(10);
                fh4 << floatToString(dAij(Mavg,i,0));
                fh4.width(10);
                fh4 << std::endl;
            }
            fh4.close();
        }
    }

    // 3. Write file with number of images per group
    fh.close();
    // 4. Write file with avgdef, mindef and maxdef per group
    fh3.close();
    // 5. Write docfile with defocus values to split manually
    DFo.write(fn_root+"_groups_split.doc");
}

void ProgCtfGroup::run()
{
    produceSideInfo();

    if (do_auto)
    {
        autoRun();
    }
    else
    {
        manualRun();
    }
    writeOutputToDisc();

    std::cerr << " Done!" <<std::endl;
}
