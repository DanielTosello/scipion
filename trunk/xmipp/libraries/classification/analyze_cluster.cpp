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

#include "analyze_cluster.h"
#include <data/args.h>
#include <data/filters.h>
#include <data/mask.h>

// Read arguments ==========================================================
void ProgAnalyzeCluster::readParams()
{
    fnSel = getParam("-i");
    fnOut = getParam("-o");
    if (checkParam("--basis"))
        fnOutBasis = getParam("--basis");
    NPCA  = getIntParam("--NPCA");
    Niter  = getIntParam("--iter");
    distThreshold = getDoubleParam("--maxDist");
    dontMask = checkParam("--dontMask");
}

// Show ====================================================================
void ProgAnalyzeCluster::show()
{
    if (verbose>0)
        std::cerr
        << "Input metadata file:    " << fnSel         << std::endl
        << "Output metadata:        " << fnOut         << std::endl
        << "Output basis stack:     " << fnOutBasis    << std::endl
        << "PCA dimension:          " << NPCA          << std::endl
        << "Iterations:             " << Niter         << std::endl
        << "Maximum distance:       " << distThreshold << std::endl
        << "Don't mask:             " << dontMask      << std::endl
        ;
}

// usage ===================================================================
void ProgAnalyzeCluster::defineParams()
{
    addUsageLine("Score the images in a cluster according to their PCA projection");
    addUsageLine("It is assumed that the cluster is aligned as is the case of the output of CL2D or ML2D");
    addParamsLine("   -i <metadatafile>             : metadata file  with images assigned to the cluster");
    addParamsLine("   -o <metadatafile>             : output metadata");
    addParamsLine("  [--basis <stackName>]          : write the average (image 1), standard deviation (image 2)");
    addParamsLine("                                 : and basis of the PCA in a stack");
    addParamsLine("  [--NPCA <dim=2>]               : PCA dimension");
    addParamsLine("  [--iter <N=10>]                : Number of iterations");
    addParamsLine("  [--maxDist <d=3>]              : Maximum distance");
    addParamsLine("                                 : Set to -1 if you don't want to filter images");
    addParamsLine("  [--dontMask]                   : Don't use a circular mask");
    addExampleLine("xmipp_classify_analyze_cluster -i images.sel --ref referenceImage.xmp -o sortedImages.xmd --basis basis.stk");
}

// Produce side info  ======================================================
//#define DEBUG
void ProgAnalyzeCluster::produceSideInfo()
{
    basis = fnOutBasis!="";

    // Read input selfile and reference
    SFin.read(fnSel);
    if (SFin.size()==0)
        return;
    if (SFin.containsLabel(MDL_ENABLED))
        SFin.removeObjects(MDValueEQ(MDL_ENABLED, -1));

    // Prepare mask
    int Xdim,Ydim,Zdim;
    size_t Ndim;
    ImgSize(SFin,Xdim,Ydim,Zdim,Ndim);
    mask.resize(Ydim,Xdim);
    mask.setXmippOrigin();
    if (dontMask)
        mask.initConstant(1);
    else
        BinaryCircularMask(mask,Xdim/2, INNER_MASK);
    int Npixels=(int)mask.sum();

    // Read all images in the class and subtract the mean
    // once aligned
    Image<double> I;
    pcaAnalyzer.clear();
    pcaAnalyzer.reserve(SFin.size());
    if (verbose>0)
    {
        std::cerr << "Processing cluster ...\n";
        init_progress_bar(SFin.size());
    }
    int n=0;
    MultidimArray<float> v(Npixels);
    FOR_ALL_OBJECTS_IN_METADATA(SFin)
    {
        I.readApplyGeo( SFin, __iter.objId );
        I().setXmippOrigin();
        int idx=0;
        FOR_ALL_ELEMENTS_IN_ARRAY2D(mask)
        if (A2D_ELEM(mask,i,j))
            A1D_ELEM(v,idx++)=IMGPIXEL(I,i,j);
        pcaAnalyzer.addVector(v);
        if ((++n)%10==0 && verbose>0)
            progress_bar(n);
    }
    if (verbose>0)
        progress_bar(SFin.size());
}
#undef DEBUG

// Run  ====================================================================
void ProgAnalyzeCluster::run()
{
    show();
    produceSideInfo();

    pcaAnalyzer.evaluateZScore(NPCA, Niter);

    // Output
    size_t N=SFin.size();
    double Ngood=0;
    SFout=SFin;
    for (size_t n=0; n<N; n++)
    {
        int trueIdx=pcaAnalyzer.getSorted(n);
        double zscore=pcaAnalyzer.getSortedZscore(n);
        SFout.setValue(MDL_ZSCORE, zscore, trueIdx+1);
        if (zscore<distThreshold || distThreshold<0)
            SFout.setValue(MDL_ENABLED,1, trueIdx+1);
        else
            SFout.setValue(MDL_ENABLED,-1, trueIdx+1);
    }
    SFout.write(fnOut,MD_APPEND);
    if (basis)
    {
        fnOutBasis.deleteFile();
        Image<double> save;
        for (int ii=0; ii<pcaAnalyzer.PCAbasis.size(); ii++)
        {
            save().initZeros(mask);
            int idx=0;
            FOR_ALL_ELEMENTS_IN_ARRAY2D(mask)
            if (A2D_ELEM(mask,i,j))
                IMGPIXEL(save,i,j)=A1D_ELEM(pcaAnalyzer.PCAbasis[ii],idx++);
            save.write(fnOutBasis,ii+2,true,WRITE_APPEND);
        }
    }
}
