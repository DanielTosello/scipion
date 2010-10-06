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

#include "convert_vol2pseudo.h"
#include "fourier_filter.h"
#include <algorithm>
#include <stdio.h>

/* Pseudo atoms ------------------------------------------------------------ */
PseudoAtom::PseudoAtom()
{
    location.initZeros(3);
    intensity=0;
}

bool operator <(const PseudoAtom &a, const PseudoAtom &b)
{
    return a.intensity<b.intensity;
}

std::ostream& operator << (std::ostream &o, const PseudoAtom &a)
{
    o << a.location.transpose() << " " << a.intensity;
    return o;
}

/* I/O --------------------------------------------------------------------- */
void ProgConvertVol2Pseudo::read(int argc, char **argv)
{
    fnVol = getParameter(argc,argv,"-i");
    fnOut = getParameter(argc,argv,"-o","");
    useMask = checkParameter(argc, argv, "-mask");
    if (useMask) mask_prm.read(argc, argv);
    sigma = textToFloat(getParameter(argc,argv,"-sigma","1.5"));
    targetError = textToFloat(getParameter(argc,argv,"-targetError","0.02"));
    stop = textToFloat(getParameter(argc,argv,"-stop","0.001"));
    initialSeeds = textToInteger(getParameter(argc,argv,"-initialSeeds","300"));
    growSeeds = textToFloat(getParameter(argc,argv,"-growSeeds","30"));
    allowMovement = !checkParameter(argc,argv,"-dontAllowMovement");
    allowIntensity = !checkParameter(argc,argv,"-dontAllowIntensity");
    intensityFraction = textToFloat(getParameter(argc,argv,"-intensityFraction","0.01"));
    intensityColumn = getParameter(argc,argv,"-intensityColumn","occupancy");
    Nclosest = textToInteger(getParameter(argc,argv,"-Nclosest","3"));
    minDistance = textToFloat(getParameter(argc,argv,"-minDistance","0.001"));
    penalty = textToFloat(getParameter(argc,argv,"-penalty","10"));
    numThreads = textToInteger(getParameter(argc,argv,"-thr","1"));
    sampling = textToFloat(getParameter(argc,argv,"-sampling_rate","1"));
    dontScale = checkParameter(argc,argv,"-dontScale");
}

void ProgConvertVol2Pseudo::show() const
{
    std::cout << "Input volume:   " << fnVol             << std::endl
              << "Output volume:  " << fnOut             << std::endl
              << "Sigma:          " << sigma             << std::endl
              << "Initial seeds:  " << initialSeeds      << std::endl
              << "Grow seeds:     " << growSeeds         << std::endl
              << "Target error:   " << targetError       << std::endl
              << "Stop:           " << stop              << std::endl
              << "AllowMovement:  " << allowMovement     << std::endl
              << "AllowIntensity: " << allowIntensity    << std::endl
              << "Intensity Frac: " << intensityFraction << std::endl
              << "Intensity Col:  " << intensityColumn   << std::endl
              << "Nclosest:       " << Nclosest          << std::endl
              << "Min. Distance:  " << minDistance       << std::endl
              << "Penalty:        " << penalty           << std::endl
              << "Threads:        " << numThreads        << std::endl
              << "Sampling Rate:  " << sampling          << std::endl
              << "Don't scale:    " << dontScale         << std::endl
    ;    
    if (useMask) mask_prm.show();
    else std::cout << "No mask\n";
}

void ProgConvertVol2Pseudo::usage() const
{
    std::cout << "Approximation algorithm:\n"
              << "   -i <volume>                     : Input volume\n"
              << "  [-o <rootname>]                  : Output rootname\n"
              << "  [-sigma <s=1.5>]                 : Sigma of gaussians (in Angstroms)\n"
              << "                                     It should be comparable to the sampling rate\n"
              << "  [-initialSeeds <N=300>]          : Initial number of gaussians\n"
              << "  [-growSeeds <%=30>]              : Percentage of growth\n"
              << "  [-stop <p=0.001>]                : Stop criterion (0<p<1) for inner iterations\n"
              << "  [-targetError <e=0.02>]          : Finish when the average representation\n"
              << "                                     error is below this threshold\n"
              << "  [-dontAllowMovement]             : Don't allow Gaussians to move\n"
              << "  [-dontAllowIntensity]            : Don't allow Gaussians to change intensity\n"
              << "  [-intensityFraction <f=0.01>]    : In case of all Gaussian with the same intensity\n"
              << "                                     this parameter determines the fraction of intensity\n"
              << "                                     held by each pseudoatom\n"
              << "  [-intensityColumn <s=occupancy>] : Where to write the intensity in the PDB file\n"
              << "                                     Valid values: occupancy, Bfactor\n"
              << "  [-Nclosest <N=3>]                : N closest atoms, it is used only for the\n"
              << "                                     distance histogram\n"
              << "  [-minDistance <d=0.001>]         : Minimum distance between two atoms\n"
              << "                                     Set it to -1 to disable\n"
              << "  [-penalty <p=10>]                : Penalty for overshooting\n"
              << "  [-sampling_rate <Ts=1>]          : Sampling rate Angstroms/pixel\n"
              << "  [-dontScale]                     : Don't scale atom weights in the PDB\n"
              << "  [-thr <n=1>]                     : Number of threads\n"
    ;
    mask_prm.usage();
}

void ProgConvertVol2Pseudo::produceSideInfo()
{
    sigma/=sampling;

    if (intensityColumn!="occupancy" && intensityColumn!="Bfactor")
        REPORT_ERROR(ERR_VALUE_INCORRECT,(std::string)"Unknown column: "+intensityColumn);

    Vin.read(fnVol);
    Vin().setXmippOrigin();
    
    if (fnOut=="")
        fnOut=fnVol.withoutExtension();
    
    Vcurrent().initZeros(Vin());
    mask_prm.generate_mask(Vin());
    
    sigma3=3*sigma;
    gaussianTable.resize(CEIL(sigma3*sqrt(3.0)*1000));
    FOR_ALL_ELEMENTS_IN_ARRAY1D(gaussianTable)
        gaussianTable(i)=gaussian1D(i/1000.0,sigma);
    
    energyOriginal=0;
    double N=0;
    double minval=1e38, maxval=-1e38;
    const MultidimArray<int> &iMask3D=mask_prm.get_binary_mask();
    FOR_ALL_ELEMENTS_IN_ARRAY3D(Vin())
    {
        if (useMask && iMask3D(k,i,j)==0) continue;
        double v=Vin(k,i,j);
        energyOriginal+=v*v;
        minval=XMIPP_MIN(minval,v);
        maxval=XMIPP_MAX(maxval,v);
        N++;
    }
    energyOriginal/=N;
    
    Histogram1D hist;
    if (useMask)
        compute_hist_within_binary_mask(iMask3D, Vin(), hist,
            minval, maxval, 200);
    else
        compute_hist(Vin(), hist, minval, maxval, 200);
    percentil1=hist.percentil(1);
    if (percentil1<=0)
        percentil1=maxval/500;
    range=hist.percentil(99)-percentil1;
    smallAtom=range*intensityFraction;
    
    // Create threads
    barrier_init(&barrier,numThreads+1);
    threadIds=(pthread_t *)malloc(numThreads*sizeof(pthread_t));
    threadArgs=(Prog_Convert_Vol2Pseudo_ThreadParams *)
        malloc(numThreads*sizeof(Prog_Convert_Vol2Pseudo_ThreadParams));
    for (int i=0; i<numThreads; i++)
    {
        threadArgs[i].myThreadID=i;
        threadArgs[i].parent=this;
        pthread_create( (threadIds+i), NULL, optimizeCurrentAtomsThread,
            (void *) (threadArgs+i));
    }
}

//#define DEBUG
void ProgConvertVol2Pseudo::placeSeeds(int Nseeds)
{
    // Convolve the difference with the Gaussian to know
    // where it would be better to put a Gaussian
    ProgFourierFilter Filter;
    Filter.FilterBand=LOWPASS;
    Filter.FilterShape=REALGAUSSIAN;
    Filter.w1=sigma;
    Filter.generateMask(Vin());
    Filter.do_generate_3dmask=false;
 
    MultidimArray<double> Vdiff=Vin();
    Vdiff-=Vcurrent();
    Filter.applyMaskSpace(Vdiff);
    
    // Place all seeds
    int rmax=3*sigma;
    const MultidimArray<int> &iMask3D=mask_prm.get_binary_mask();
    for (int n=0; n<Nseeds; n++)
    {
        // Look for the maximum error
        bool first=true;
        int kmax, imax, jmax;
        double maxVal;
        FOR_ALL_ELEMENTS_IN_ARRAY3D(Vdiff)
        {
            if (useMask && iMask3D(k,i,j)==0) continue;
            if (first || Vdiff(k,i,j)>maxVal)
            {
                kmax=k;
                imax=i;
                jmax=j;
                maxVal=Vdiff(k,i,j);
                first=false;
            }
        }
        
        // Keep this as an atom
        PseudoAtom a;
        a.location(0)=kmax;
        a.location(1)=imax;
        a.location(2)=jmax;
        if (allowIntensity) a.intensity=maxVal;
        else 
        {
            if (maxVal<smallAtom) break;
            a.intensity=smallAtom;
        }
        atoms.push_back(a);
        
        // Remove this density from the difference
        drawGaussian(kmax,imax,jmax,Vdiff,-a.intensity);
        
        #ifdef DEBUG
            std::cout << "New atom: " << a << std::endl;
            VolumeXmipp save;
            save()=Vdiff; save.write("PPPDiff.vol");
            std::cout << "Press any key\n";
            char c; std::cin >> c;
        #endif
    }
} 
#undef DEBUG

/* Remove seeds ------------------------------------------------------------ */
void ProgConvertVol2Pseudo::removeSeeds(int Nseeds)
{
    int fromNegative=ROUND(Nseeds*0.5);
    int fromSmall=Nseeds-fromNegative;

    if (allowIntensity)
    {
        // Remove too small atoms
        std::sort(atoms.begin(),atoms.end());
        atoms.erase(atoms.begin(),atoms.begin()+fromSmall);
    }
    else
    {
        fromNegative=Nseeds;
        fromSmall=0;
    }        
    
    // Remove atoms from regions in which the error is too negative
    MultidimArray<double> Vdiff=Vin();
    Vdiff-=Vcurrent();
    int alreadyRemoved=0;
    double vmin=Vdiff.computeMin();
    if (vmin<0)
    {
        for (double v=vmin+vmin/20; v<0; v-=vmin/20)
        {
            int oldListSize;
            do {
                oldListSize=atoms.size();

                // Search for a point within a negative region
                bool found=false;
                int kneg, ineg, jneg;
                const MultidimArray<int> &iMask3D=mask_prm.get_binary_mask();
                for (int k=STARTINGZ(Vdiff); k<=FINISHINGZ(Vdiff) && !found; k++)
                    for (int i=STARTINGY(Vdiff); i<=FINISHINGY(Vdiff) && !found; i++)
                        for (int j=STARTINGX(Vdiff); j<=FINISHINGX(Vdiff) && !found; j++)
                        {
                            if (useMask && iMask3D(k,i,j)==0) continue;
                            if (Vdiff(k,i,j)<v)
                            {
                                kneg=k;
                                ineg=i;
                                jneg=j;
                                Vdiff(k,i,j)=0;
                                found=true;
                            }
                        }

                // If found such a point, search for a nearby atom
                if (found)
                {
                    // Search for atom
                    int nmax=atoms.size();
                    for (int n=0; n<nmax; n++)
                    {
                        double r=
                            (kneg-atoms[n].location(0))*(kneg-atoms[n].location(0))+
                            (ineg-atoms[n].location(1))*(ineg-atoms[n].location(1))+
                            (jneg-atoms[n].location(2))*(jneg-atoms[n].location(2));                        r=sqrt(r);
                        if (r<sigma3)
                        {
                            drawGaussian(atoms[n].location(0),
                                atoms[n].location(1),
                                atoms[n].location(2),
                                Vdiff,
                                atoms[n].intensity);
                            atoms.erase(atoms.begin()+n);
                            alreadyRemoved++;
                            break;
                        }
                    }
                }
            } while (oldListSize>atoms.size() && alreadyRemoved<fromNegative);
            if (alreadyRemoved==fromNegative) break;
        }
    }
    
    removeTooCloseSeeds();
}

void ProgConvertVol2Pseudo::removeTooCloseSeeds()
{
    // Remove atoms that are too close to each other
    if (minDistance>0 && allowIntensity)
    {
        std::vector<int> toRemove;
        int nmax=atoms.size();
        double minDistance2=minDistance*minDistance;
        for (int n1=0; n1<nmax; n1++)
        {
            bool found=false;
            int nn=0, nnmax=toRemove.size();
            while (nn<nnmax)
            {
                if (toRemove[nn]==n1) {found=true; break;}
                else if (toRemove[nn]>n1) break;
                nn++;
            }
            if (found) continue;
            for (int n2=n1+1; n2<nmax; n2++)
            {
                nn=0;
                found=false;
                while (nn<nnmax)
                {
                    if (toRemove[nn]==n2) {found=true; break;}
                    else if (toRemove[nn]>n2) break;
                    nn++;
                }
                if (found) continue;
                double diffZ=atoms[n1].location(0)-atoms[n2].location(0);
                double diffY=atoms[n1].location(1)-atoms[n2].location(1);
                double diffX=atoms[n1].location(2)-atoms[n2].location(2);
                double d2=diffZ*diffZ+diffY*diffY+diffX*diffX;
                if (d2<minDistance2)
                {
                    if (atoms[n1].intensity<atoms[n2].intensity)
                    {
                        toRemove.push_back(n1);
                        break;
                    }
                    else
                        toRemove.push_back(n2);
                    std::sort(toRemove.begin(),toRemove.end());
                }
            }
        }
        for (int n=toRemove.size()-1; n>=0; n--)
            atoms.erase(atoms.begin()+toRemove[n]);
    }
}

/* Draw approximation ------------------------------------------------------ */
void ProgConvertVol2Pseudo::drawApproximation()
{
    Vcurrent().initZeros(Vin());
    int nmax=atoms.size();
    for (int n=0; n<nmax; n++)
        drawGaussian(atoms[n].location(0),atoms[n].location(1),
            atoms[n].location(2),Vcurrent(),atoms[n].intensity);

    energyDiff=0;
    double N=0;
    percentageDiff=0;
    const MultidimArray<int> &iMask3D=mask_prm.get_binary_mask();
    FOR_ALL_ELEMENTS_IN_ARRAY3D(Vcurrent())
    {
        if (useMask && iMask3D(k,i,j)==0) continue;
        double Vinv=Vin(k,i,j);
        if (Vinv<=0) continue;
        double vdiff=Vinv-Vcurrent(k,i,j);
        double vperc=ABS(vdiff);
        energyDiff+=vdiff*vdiff;
        percentageDiff+=vperc;
        N++;
    }
    energyDiff/=N;
    percentageDiff/=(N*range);
}

/* Gaussian operations ----------------------------------------------------- */
double ProgConvertVol2Pseudo::computeAverage(int k, int i, int j,
		MultidimArray<double> &V)
{
    int k0=XMIPP_MAX(STARTINGZ(V),k-sigma3);
    int i0=XMIPP_MAX(STARTINGY(V),i-sigma3);
    int j0=XMIPP_MAX(STARTINGX(V),j-sigma3);
    int kF=XMIPP_MIN(FINISHINGZ(V),k+sigma3);
    int iF=XMIPP_MIN(FINISHINGY(V),i+sigma3);
    int jF=XMIPP_MIN(FINISHINGX(V),j+sigma3);
    double sum=0;
    for (int kk=k0; kk<=kF; kk++)
        for (int ii=i0; ii<=iF; ii++)
            for (int jj=j0; jj<=jF; jj++)
                sum+=V(kk,ii,jj);
    return sum/((kF-k0+1)*(iF-i0+1)*(jF-j0+1));
}

void ProgConvertVol2Pseudo::drawGaussian(double k, double i, double j,
		MultidimArray<double> &V, double intensity)
{
    int k0=CEIL(XMIPP_MAX(STARTINGZ(V),k-sigma3));
    int i0=CEIL(XMIPP_MAX(STARTINGY(V),i-sigma3));
    int j0=CEIL(XMIPP_MAX(STARTINGX(V),j-sigma3));
    int kF=FLOOR(XMIPP_MIN(FINISHINGZ(V),k+sigma3));
    int iF=FLOOR(XMIPP_MIN(FINISHINGY(V),i+sigma3));
    int jF=FLOOR(XMIPP_MIN(FINISHINGX(V),j+sigma3));
    for (int kk=k0; kk<=kF; kk++)
    {
        double diffkk2=(kk-k)*(kk-k);
        for (int ii=i0; ii<=iF; ii++)
        {
            double diffiikk2=(ii-i)*(ii-i)+diffkk2;
            for (int jj=j0; jj<=jF; jj++)
            {
                double r=sqrt(diffiikk2+(jj-j)*(jj-j));
                V(kk,ii,jj)+=intensity*
                    DIRECT_A1D_ELEM(gaussianTable,ROUND(r*1000));
            }
        }
    }
}

void ProgConvertVol2Pseudo::extractRegion(int idxGaussian,
		MultidimArray<double> &region, bool extended) const
{
    double k=atoms[idxGaussian].location(0);
    double i=atoms[idxGaussian].location(1);
    double j=atoms[idxGaussian].location(2);

    double sigma3ToUse=sigma3;
    if (extended)
        sigma3ToUse+=1.5;

    int k0=CEIL(XMIPP_MAX(STARTINGZ(Vcurrent()),k-sigma3ToUse));
    int i0=CEIL(XMIPP_MAX(STARTINGY(Vcurrent()),i-sigma3ToUse));
    int j0=CEIL(XMIPP_MAX(STARTINGX(Vcurrent()),j-sigma3ToUse));
    int kF=FLOOR(XMIPP_MIN(FINISHINGZ(Vcurrent()),k+sigma3ToUse));
    int iF=FLOOR(XMIPP_MIN(FINISHINGY(Vcurrent()),i+sigma3ToUse));
    int jF=FLOOR(XMIPP_MIN(FINISHINGX(Vcurrent()),j+sigma3ToUse));
    
    region.resize(kF-k0+1,iF-i0+1,jF-j0+1);
    STARTINGZ(region)=k0;
    STARTINGY(region)=i0;
    STARTINGX(region)=j0;
    for (int k=k0; k<=kF; k++)
        for (int i=i0; i<=iF; i++)
            for (int j=j0; j<=jF; j++)
                region(k,i,j)=Vcurrent(k,i,j);
}

double ProgConvertVol2Pseudo::evaluateRegion(const MultidimArray<double> &region)
    const 
{
    double avgDiff=0;
    double N=0;
    const MultidimArray<int> &iMask3D=mask_prm.get_binary_mask();
    FOR_ALL_ELEMENTS_IN_ARRAY3D(region)
    {
        double Vinv=Vin(k,i,j);
        if (Vinv<=0) continue;
        if (useMask && iMask3D(k,i,j)==0) continue;
        double vdiff=region(k,i,j)-Vinv;
        double vperc=(vdiff<0)?-vdiff:penalty*vdiff;
        avgDiff+=vperc;
        N++;
    }
    return avgDiff/(N*range);
}

void ProgConvertVol2Pseudo::insertRegion(const MultidimArray<double> &region)
{
    FOR_ALL_ELEMENTS_IN_ARRAY3D(region)
        Vcurrent(k,i,j)=A3D_ELEM(region,k,i,j);
}

/* Optimize ---------------------------------------------------------------- */
static pthread_mutex_t mutexUpdateVolume=PTHREAD_MUTEX_INITIALIZER;

void* ProgConvertVol2Pseudo::optimizeCurrentAtomsThread(
    void * threadArgs)
{
    Prog_Convert_Vol2Pseudo_ThreadParams *myArgs=
        (Prog_Convert_Vol2Pseudo_ThreadParams *) threadArgs;
    ProgConvertVol2Pseudo *parent=myArgs->parent;
    std::vector< PseudoAtom > &atoms=parent->atoms;
    bool allowIntensity=parent->allowIntensity;
    bool allowMovement=parent->allowMovement;
    MultidimArray<double> region, regionBackup;
    
    barrier_t *barrier=&(parent->barrier);
    do
    {
        barrier_wait( barrier );
        if (parent->threadOpCode==KILLTHREAD)
            return NULL;

        myArgs->Nintensity=0;
        myArgs->Nmovement=0;
        int nmax=atoms.size();
        for (int n=0; n<nmax; n++)
        {
            if ((n+1)%parent->numThreads!=myArgs->myThreadID)
                continue;
        
            parent->extractRegion(n,region,true);
            double currentRegionEval=parent->evaluateRegion(region);
            parent->drawGaussian(atoms[n].location(0), atoms[n].location(1),
                atoms[n].location(2),region,-atoms[n].intensity);
            regionBackup=region;

            // Change intensity
            if (allowIntensity)
            {
                // Try with a Gaussian that is of different intensity
                double tryCoeffs[5]={0, 0.9, 0.99, 1.01, 1.1};
                double bestRed=0;
                int bestT=-1;
                for (int t=0; t<5; t++)
                {
                    region=regionBackup;
                    parent->drawGaussian(atoms[n].location(0),
                        atoms[n].location(1), atoms[n].location(2),region,
                        tryCoeffs[t]*atoms[n].intensity);
                    double trialRegionEval=parent->evaluateRegion(region);
                    double reduction=trialRegionEval-currentRegionEval;
                    if (reduction<bestRed)
                    {
                        bestRed=reduction;
                        bestT=t;
                    }
                }
                if (bestT!=-1)
                {
                    atoms[n].intensity*=tryCoeffs[bestT];
                    region=regionBackup;
                    parent->drawGaussian(atoms[n].location(0), atoms[n].location(1),
                        atoms[n].location(2),region,atoms[n].intensity);
                    pthread_mutex_lock(&mutexUpdateVolume);
                        parent->insertRegion(region);
                    pthread_mutex_unlock(&mutexUpdateVolume);
                    currentRegionEval=parent->evaluateRegion(region);
                    parent->drawGaussian(atoms[n].location(0),
                        atoms[n].location(1), atoms[n].location(2),region,
                        -atoms[n].intensity);
                    regionBackup=region;
                    myArgs->Nintensity++;
                }
            }
            
            // Change location
            if (allowMovement && atoms[n].intensity>0)
            {
                double tryX[6]={-0.45,0.5, 0.0 ,0.0, 0.0 ,0.0};
                double tryY[6]={ 0.0 ,0.0,-0.45,0.5, 0.0 ,0.0};
                double tryZ[6]={ 0.0 ,0.0, 0.0 ,0.0,-0.45,0.5};
                double bestRed=0;
                int bestT=-1;
                for (int t=0; t<6; t++)
                {
                    region=regionBackup;
                    parent->drawGaussian(atoms[n].location(0)+tryZ[t],
                        atoms[n].location(1)+tryY[t],
                        atoms[n].location(2)+tryX[t],
                        region,atoms[n].intensity);
                    double trialRegionEval=parent->evaluateRegion(region);
                    double reduction=trialRegionEval-currentRegionEval;
                    if (reduction<bestRed)
                    {
                        bestRed=reduction;
                        bestT=t;
                    }
                }
                if (bestT!=-1)
                {
                    atoms[n].location(0)+=tryZ[bestT];
                    atoms[n].location(1)+=tryY[bestT];
                    atoms[n].location(2)+=tryX[bestT];
                    region=regionBackup;
                    parent->drawGaussian(atoms[n].location(0),
                        atoms[n].location(1), atoms[n].location(2),region,
                        atoms[n].intensity);
                    pthread_mutex_lock(&mutexUpdateVolume);
                        parent->insertRegion(region);
                    pthread_mutex_unlock(&mutexUpdateVolume);
                    myArgs->Nmovement++;
                }
            }
        }

        barrier_wait( barrier );
    } while (true);
}

void ProgConvertVol2Pseudo::optimizeCurrentAtoms()
{
    if (!allowIntensity && !allowMovement) return;
    bool finished=false;
    int iter=0;
    do
    {
        double oldError=percentageDiff;
        
        threadOpCode=WORKTHREAD;
        // Launch workers
        barrier_wait(&barrier);
        // Wait for workers to finish
        barrier_wait(&barrier);
        
        // Retrieve results
        int Nintensity=0;
        int Nmovement=0;
        for (int i=0; i<numThreads; i++)
        {
            Nintensity+=threadArgs[i].Nintensity;
            Nmovement+=threadArgs[i].Nmovement;
        }
        
        // Remove all the removed atoms
        int nmax=atoms.size();
        for (int n=nmax-1; n>=0; n--)
            if (atoms[n].intensity==0)
                atoms.erase(atoms.begin()+n);

        drawApproximation();
        std::cout << "Iteration " << iter << " error= " << percentageDiff
                  << " Natoms= " << atoms.size()
                  << " Intensity= " << Nintensity
                  << " Location= " << Nmovement
                  << std::endl;
        
        if (iter>0)
            if ((oldError-percentageDiff)/oldError<stop) finished=true;
        iter++;
    } while (!finished);
}

/* Write ------------------------------------------------------------------- */
void ProgConvertVol2Pseudo::writeResults()
{
    Vcurrent.write(fnOut+"_approximation.vol");

    // Compute the histogram of intensities
    MultidimArray<double> intensities;
    intensities.initZeros(atoms.size());
    FOR_ALL_ELEMENTS_IN_ARRAY1D(intensities)
        intensities(i)=atoms[i].intensity;
    Histogram1D hist;
    compute_hist(intensities, hist, 0, intensities.computeMax(), 100);
    hist.write(fnOut+"_approximation.hist");

    // Compute the histogram of distances
    int Natoms=atoms.size();
    MultidimArray<double> NclosestDistances;
    NclosestDistances.resize((Natoms-1)*Nclosest);
    for (int i=0; i<Natoms; i++)
    {
        std::vector<double> NclosestToThisAtom;
        for (int j=i+1; j<Natoms; j++)
        {
            double dist=(atoms[i].location-atoms[j].location).module();
            int closestSoFar=NclosestToThisAtom.size();
            if (closestSoFar==0)
                NclosestToThisAtom.push_back(dist);
            else
            {
                int idx=0;
                while (idx<closestSoFar && NclosestToThisAtom[idx]<dist)
                    idx++;
                if (idx<closestSoFar)
                {
                    NclosestToThisAtom.insert(
                        NclosestToThisAtom.begin()+idx,1,dist);
                    if (NclosestToThisAtom.size()>Nclosest)
                        NclosestToThisAtom.erase(NclosestToThisAtom.begin()+
                            Nclosest);
                }
                if (idx==closestSoFar && closestSoFar<Nclosest)
                    NclosestToThisAtom.push_back(dist);
            }
        }
        if (i<Natoms-1)
            for (int k=0; k<Nclosest; k++)
                NclosestDistances(i*Nclosest+k)=sampling*NclosestToThisAtom[k];
    }
    compute_hist(NclosestDistances, hist, 0, NclosestDistances.computeMax(),
        100);
    hist.write(fnOut+"_distance.hist");

    // Save the difference
    Image<double> Vdiff;
    Vdiff()=Vin()-Vcurrent();
    const MultidimArray<int> &iMask3D=mask_prm.get_binary_mask();
    if (useMask && XSIZE(iMask3D)!=0)
        FOR_ALL_ELEMENTS_IN_ARRAY3D(Vdiff())
            if (!iMask3D(k,i,j))
                Vdiff(k,i,j)=0;
    Vdiff.write(fnOut+"_rawDiff.vol");
    
    Vdiff()/=range;
    Vdiff.write(fnOut+"_relativeDiff.vol");
    
    // Write the PDB
    double minIntensity=intensities.computeMin();
    double maxIntensity=intensities.computeMax();
    double a=0.99/(maxIntensity-minIntensity);
    if (dontScale) a=1;
    
    FILE *fhOut=NULL;
    fhOut=fopen((fnOut+".pdb").c_str(),"w");
    if (!fhOut)
        REPORT_ERROR(ERR_IO_NOWRITE,fnOut+".pdb");
    int nmax=atoms.size();
    int col=1;
    if (intensityColumn=="Bfactor") col=2;
    fprintf(fhOut,"REMARK xmipp_convert_vol2pseudo\n");
    fprintf(fhOut,"REMARK fixedGaussian %f\n",sigma*sampling);
    fprintf(fhOut,"REMARK intensityColumn %s\n",intensityColumn.c_str());
    for (int n=0; n<nmax; n++)
    {
        double intensity=1.0;
        if (allowIntensity)
            intensity=0.01+ROUND(100*a*(atoms[n].intensity-minIntensity))/100.0;
        if (col==1)
            fprintf(fhOut,
                "ATOM  %5d DENS DENS%5d    %8.3f%8.3f%8.3f%6.2f     1      DENS\n",
                n+1,n+1,
                (float)(atoms[n].location(2)*sampling),
                (float)(atoms[n].location(1)*sampling),
                (float)(atoms[n].location(0)*sampling),
                (float)intensity);
        else
            fprintf(fhOut,
                "ATOM  %5d DENS DENS%5d    %8.3f%8.3f%8.3f     1%6.2f      DENS\n",
                n+1,n+1,
                (float)(atoms[n].location(2)*sampling),
                (float)(atoms[n].location(1)*sampling),
                (float)(atoms[n].location(0)*sampling),
                (float)intensity);
    }
    fclose(fhOut);
}

/* Run --------------------------------------------------------------------- */
void ProgConvertVol2Pseudo::run()
{
    int iter=0;
    double previousNAtoms=0;
    do
    {
        // Place seeds
        if (iter==0) placeSeeds(initialSeeds);
        else
        {
            double Natoms=atoms.size();
            std::cout << "Removing seeds" << std::endl;
            removeSeeds(FLOOR(Natoms*(growSeeds/2)/100));
            std::cout << "Placing seeds" << std::endl;
            placeSeeds(FLOOR(Natoms*growSeeds/100));
        }
        std::cout << "Drawing approximation" << std::endl;
        drawApproximation();
        if (iter==0)
            std::cout << "Initial error with " << atoms.size()
                      << " pseudo-atoms " << percentageDiff << std::endl;
        
        // Optimize seeds until convergence
        optimizeCurrentAtoms();
        std::cout << "Error with " << atoms.size() << " pseudo-atoms "
                  << percentageDiff << std::endl;
        writeResults();
        iter++;
        
        if (ABS(previousNAtoms-atoms.size())/atoms.size()<0.01)
        {
            std::cout << "The required precision cannot be attained\n"
                      << "Suggestion: Reduce sigma and/or minDistance\n"
                      << "Writing best approximation with current parameters\n";
            
            break;
        }
        previousNAtoms=atoms.size();
    } while (percentageDiff>targetError);
    removeTooCloseSeeds();
    writeResults();
    
    // Kill threads
    threadOpCode=KILLTHREAD;
    barrier_wait(&barrier);
    free(threadIds);
    free(threadArgs);
}
