/***************************************************************************
 *
 * Authors:    Carlos Oscar            coss@cnb.csic.es (2011)
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
#include <math.h>
#include "micrograph_automatic_picking2.h"
#include <data/filters.h>
#include <data/rotational_spectrum.h>
#include <reconstruction/denoise.h>
#include <data/xmipp_fft.h>
#include <data/xmipp_filename.h>
#include <algorithm>
#include <classification/uniform.h>

AutoParticlePicking2::AutoParticlePicking2(const FileName &fn, Micrograph *_m,
        int size, int filterNum, int pcaNum, int corrNum, int procprec)
{
    __m=_m;
    fn_micrograph=fn;
    microImage.read(fn_micrograph);
    double t=std::max(0.25,50.0/size);
    scaleRate=std::min(1.0,t);
    particle_radius=(size*scaleRate)*0.5;
    particle_size=particle_radius * 2;
    NRsteps=particle_size/2-3;
    filter_num=filterNum;
    corr_num=corrNum;
    proc_prec=procprec;
    NPCA=pcaNum;
    NRPCA=20;
    num_correlation=filter_num+((filter_num-corr_num)*corr_num);
    classifier.setParameters(8.0, 0.125);
    classifier2.setParameters(1.0, 0.25);//(2.0, 0.5);
}

//Generate filter bank from the micrograph image
void filterBankGenerator(MultidimArray<double> &inputMicrograph,
                         const FileName &fnFilterBankStack,
                         int filter_num)
{
    fnFilterBankStack.deleteFile();
    Image<double> Iaux;
    Iaux()=inputMicrograph;
    FourierFilter filter;
    filter.raised_w=0.02;
    filter.FilterShape=RAISED_COSINE;
    filter.FilterBand=BANDPASS;
    MultidimArray<std::complex<double> > micrographFourier;
    FourierTransformer transformer;
    transformer.FourierTransform(Iaux(),micrographFourier,true);
    for (int i=0;i<filter_num;i++)
    {
        filter.w1=0.025*i;
        filter.w2=(filter.w1)+0.025;
        transformer.setFourier(micrographFourier);
        filter.applyMaskFourierSpace(inputMicrograph,transformer.fFourier);
        transformer.inverseFourierTransform();
        Iaux.write(fnFilterBankStack,i+1,true,WRITE_APPEND);
    }
}

/*
 *This method do the correlation between two polar images
 *n1 and n2 in mIPolar (Stack) and put it at the nF place
 *of the mIpolarCorr (Stack)
 */
void correlationBetweenPolarChannels(int n1,int n2,int nF,
                                     MultidimArray<double> &mIpolar,
                                     MultidimArray<double> &mIpolarCorr,
                                     CorrelationAux &aux)
{
    MultidimArray<double> imgPolar1, imgPolar2, imgPolarCorr, corr2D;
    imgPolar1.aliasImageInStack(mIpolar, n1);
    imgPolar2.aliasImageInStack(mIpolar, n2);
    imgPolarCorr.aliasImageInStack(mIpolarCorr,nF);
    // The correlation between images n1 and n2 and put in nF
    correlation_matrix(imgPolar1,imgPolar2,corr2D,aux);
    imgPolarCorr=corr2D;
}

//To calculate the euclidean distance between to points
double euclidean_distance(const Particle2 &p1, const Particle2 &p2)
{
    double dx=(p1.x-p2.x);
    double dy= (p1.y-p2.y);
    return sqrt(dx*dx+dy*dy);
}

bool AutoParticlePicking2::checkDist(Particle2 &p)
{
    int num_part=__m->ParticleNo();
    int dist=0,min;
    Particle2 posSample;

    posSample.x=(__m->coord(0).X)*scaleRate;
    posSample.y=(__m->coord(0).Y)*scaleRate;
    min=euclidean_distance(p,posSample);
    for (int i=1;i<num_part;i++)
    {
        posSample.x=(__m->coord(i).X)*scaleRate;
        posSample.y=(__m->coord(i).Y)*scaleRate;
        dist= euclidean_distance(p,posSample);
        if (dist<min)
            min=dist;
    }

    if (min>(0.25*particle_radius))
        return true;
    else
        return false;
}

/*
 * This method check if a point is local maximum according to
 * the eight neighbors.
 */
bool isLocalMaxima(MultidimArray<double> &inputArray, int x, int y)
{
    if (DIRECT_A2D_ELEM(inputArray,y-1,x-1)<DIRECT_A2D_ELEM(inputArray,y,x) &&
        DIRECT_A2D_ELEM(inputArray,y-1,x)<DIRECT_A2D_ELEM(inputArray,y,x) &&
        DIRECT_A2D_ELEM(inputArray,y-1,x+1)<DIRECT_A2D_ELEM(inputArray,y,x) &&
        DIRECT_A2D_ELEM(inputArray,y,x-1)<DIRECT_A2D_ELEM(inputArray,y,x) &&
        DIRECT_A2D_ELEM(inputArray,y,x+1)<DIRECT_A2D_ELEM(inputArray,y,x) &&
        DIRECT_A2D_ELEM(inputArray,y+1,x-1)<DIRECT_A2D_ELEM(inputArray,y,x) &&
        DIRECT_A2D_ELEM(inputArray,y+1,x)<DIRECT_A2D_ELEM(inputArray,y,x) &&
        DIRECT_A2D_ELEM(inputArray,y+1,x+1)<DIRECT_A2D_ELEM(inputArray,y,x))
        return true;
    else
        return false;
}

void AutoParticlePicking2::polarCorrelation(MultidimArray<double> &Ipolar,
        MultidimArray<double> &IpolarCorr)
{
    int nF = NSIZE(Ipolar);
    CorrelationAux aux;

    for (int n=0; n<nF;++n)
        correlationBetweenPolarChannels(n,n,n,Ipolar,IpolarCorr,aux);
    for (int i=0; i<(filter_num-corr_num);i++)
        for (int j=1;j<=corr_num;j++)
            correlationBetweenPolarChannels(i,i+j,nF++,Ipolar,IpolarCorr,aux);
}

AutoParticlePicking2::~AutoParticlePicking2()
{}

void AutoParticlePicking2::extractStatics(MultidimArray<double> &inputVec,
        MultidimArray<double> &features)
{
    MultidimArray<double> sortedVec;
    features.resize(1,1,1,12);
    DIRECT_A1D_ELEM(features,0)=inputVec.computeAvg();
    DIRECT_A1D_ELEM(features,1)=inputVec.computeStddev();
    normalize_OldXmipp(inputVec);
    // Sorting the image in order to find the quantiles
    inputVec.sort(sortedVec);
    int step=floor(XSIZE(sortedVec)*0.1);
    for (int i=2;i<12;i++)
        DIRECT_A1D_ELEM(features,i)=DIRECT_A1D_ELEM(sortedVec,(i-1)*step);
}

void AutoParticlePicking2::buildVector(MultidimArray<double> &inputVec,
                                       MultidimArray<double> &staticVec,
                                       MultidimArray<double> &featureVec,
                                       MultidimArray<double> &pieceImage)
{
    MultidimArray<double> avg;
    MultidimArray<double> pcaBase;
    MultidimArray<double> pcaRBase;
    MultidimArray<double> vec;

    featureVec.resize(1,1,1,num_correlation*NPCA+NRPCA+12);
    // Read the polar correlation from the stack and project on
    // PCA basis and put the value as the feature.
    for (int i=0;i<num_correlation;i++)
    {
        avg.aliasImageInStack(pcaModel,i*(NPCA + 1));
        avg.resize(1,1,1,XSIZE(avg)*YSIZE(avg));
        vec.aliasImageInStack(inputVec, i);
        vec.resize(1,1,1,XSIZE(vec)*YSIZE(vec));
        vec-=avg;
        for (int j=0;j<NPCA;j++)
        {
            pcaBase.aliasImageInStack(pcaModel,(i*(NPCA+1)+1)+j);
            pcaBase.resize(1,1,1,XSIZE(pcaBase)*YSIZE(pcaBase));
            DIRECT_A1D_ELEM(featureVec,j+(i*NPCA))=PCAProject(pcaBase,vec);
        }
    }
    // Extract the statics from the image
    for (int j=0;j<12;j++)
        DIRECT_A1D_ELEM(featureVec,j+num_correlation*NPCA)=DIRECT_A1D_ELEM(staticVec,j);

    // Projecting the image on rotational PCA basis
    for (int j=0;j<NRPCA;j++)
    {
        pcaRBase.aliasImageInStack(pcaRotModel,j);
        pcaRBase.resize(1,1,1,XSIZE(pcaRBase)*YSIZE(pcaRBase));
        DIRECT_A1D_ELEM(featureVec,num_correlation*NPCA+12+j)=PCAProject(pcaRBase,pieceImage);
    }
}

void AutoParticlePicking2::buildInvariant(MultidimArray<double> &invariantChannel,int x,int y)
{
    MultidimArray<double> pieceImage;
    MultidimArray<double> Ipolar;
    MultidimArray<double> mIpolar,filter;
    Ipolar.initZeros(filter_num,1,NangSteps,NRsteps);
    // First put the polar channels in a stack
    for (int j=0;j<filter_num;++j)
    {
        filter.aliasImageInStack(micrographStack(),j);
        extractParticle(x,y,filter,pieceImage,true);
        mIpolar.aliasImageInStack(Ipolar,j);
        convert2Polar(pieceImage,mIpolar);
    }
    // Obtain the correlation between different channels
    polarCorrelation(Ipolar,invariantChannel);
}

double AutoParticlePicking2::PCAProject(MultidimArray<double> &pcaBasis,
                                        MultidimArray<double> &vec)
{
    double dotProduct=0;
    FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY1D(pcaBasis)
    dotProduct+=DIRECT_A1D_ELEM(pcaBasis,i)*DIRECT_A1D_ELEM(vec,i);
    return dotProduct;
}

void AutoParticlePicking2::trainPCA(const FileName &fnPositiveFeat)
{
    ImageGeneric positiveInvariant;
    MultidimArray<float> pcaVec;
    ArrayDim aDim;

    FileName fnPositiveInvariatn=fnPositiveFeat+"_invariant_Positive.stk";
    positiveInvariant.read(fnPositiveInvariatn,HEADER);
    positiveInvariant.getDimensions(aDim);
    int steps=aDim.ndim/num_correlation;
    // Read the channel correlation one by one and obtain the basis for them.
    for (int i=1;i<=num_correlation;i++)
    {
        for (int j=0;j<steps;j++)
        {
            positiveInvariant.readMapped(fnPositiveInvariatn,j*num_correlation+i);
            positiveInvariant().getImage(pcaVec);
            pcaVec.resize(1,1,1,XSIZE(pcaVec)*YSIZE(pcaVec));
            pcaAnalyzer.addVector(pcaVec);
        }
        pcaAnalyzer.subtractAvg();
        pcaAnalyzer.learnPCABasis(NPCA,10);
        savePCAModel(fnPositiveFeat);
        pcaAnalyzer.clear();
    }
}

void AutoParticlePicking2::trainRotPCA(const FileName &fnAvgModel,const FileName &fnPCARotModel)
{
    // just set the parameters and call the run method of the object
    // in order to obtain the rotational PCA basis.
    rotPcaAnalyzer.fnIn=fnAvgModel;
    rotPcaAnalyzer.fnRoot=fnPCARotModel;
    rotPcaAnalyzer.psi_step=2;
    rotPcaAnalyzer.Nthreads=4;
    rotPcaAnalyzer.Neigen=20;
    rotPcaAnalyzer.shift_step=1;
    rotPcaAnalyzer.Nits=2;
    rotPcaAnalyzer.maxNimgs=-1;
    rotPcaAnalyzer.max_shift_change=0;
    rotPcaAnalyzer.run();
}

void AutoParticlePicking2::trainSVM(const FileName &fnModel,
                                    int numClassifier)
{

    if (numClassifier==1)
    {
        classifier.SVMTrain(dataSet,classLabel);
        classifier.SaveModel(fnModel);
    }
    else
    {
        classifier2.SVMTrain(dataSet1,classLabel1);
        classifier2.SaveModel(fnModel);
    }
}

int AutoParticlePicking2::automaticallySelectParticles(bool use2Classifier,bool fast)
{

    double label, score;
    Particle2 p;
    MultidimArray<double> IpolarCorr;
    MultidimArray<double> featVec;
    MultidimArray<double> pieceImage;
    MultidimArray<double> staticVec, dilatedVec;
    std::vector<Particle2> positionArray;
    IpolarCorr.initZeros(num_correlation,1,NangSteps,NRsteps);
    // Obtain the positions in micrograph where there maybe particles
    buildSearchSpace(positionArray,fast);
#ifdef DEBUG_AUTO

    std::ofstream fh_training;
    fh_training.open("particles_cord1.txt");
#endif

    int num=positionArray.size()*(proc_prec/100.0);
    for (int k=0;k<num;k++)
    {
        int j=positionArray[k].x;
        int i=positionArray[k].y;
#ifdef DEBUG_AUTO

        fh_training << j * (1.0 / scaleRate) << " " << i * (1.0 / scaleRate)
        << " " << positionArray[k].cost << std::endl;
#endif

        buildInvariant(IpolarCorr,j,i);
        extractParticle(j,i,microImage(),pieceImage,false);
        pieceImage.resize(1,1,1,XSIZE(pieceImage)*YSIZE(pieceImage));
        extractStatics(pieceImage,staticVec);
        buildVector(IpolarCorr,staticVec,featVec,pieceImage);
        // Normalizing the feature vector according to the max and mean of the vector
        double max=featVec.computeMax();
        double min=featVec.computeMin();
        FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY1D(featVec)
        {
            DIRECT_A1D_ELEM(featVec,i)=0+((1)*((DIRECT_A1D_ELEM(featVec,i)-min)/(max-min)));
        }
        label=classifier.predict(featVec, score);
        if (label==1)
        {
            // If it is a particle check if it is not a false positive
            if (use2Classifier==true)
            {
                label=classifier2.predict(featVec,score);
                if (label==1)
                {
                    p.x=j;
                    p.y=i;
                    p.status=1;
                    p.cost=score;
                    p.vec=featVec;
                    auto_candidates.push_back(p);
                }
            }
            else
            {
                p.x=j;
                p.y=i;
                p.status=1;
                p.cost=score;
                p.vec=featVec;
                auto_candidates.push_back(p);
            }
        }
    }
    // Remove the occluded particles
    for (int i=0;i<auto_candidates.size();++i)
        for (int j=0;j<auto_candidates.size()-i-1;j++)
            if (auto_candidates[j].cost<auto_candidates[j+1].cost)
            {
                p=auto_candidates[j+1];
                auto_candidates[j+1]=auto_candidates[j];
                auto_candidates[j]=p;
            }
    for (int i=0;i<auto_candidates.size()-1;++i)
    {
        if (auto_candidates[i].status==-1)
            continue;
        p=auto_candidates[i];
        for (int j=i+1;j<auto_candidates.size();j++)
        {
            if (auto_candidates[j].x>p.x-particle_radius
                && auto_candidates[j].x<p.x+particle_radius
                && auto_candidates[j].y>p.y-particle_radius
                && auto_candidates[j].y<p.y+particle_radius)
            {
                if (p.cost<auto_candidates[j].cost)
                {
                    auto_candidates[i].status=-1;
                    p=auto_candidates[j];
                }
                else
                    auto_candidates[j].status=-1;
            }
        }
    }
    return auto_candidates.size();
}

void AutoParticlePicking2::add2Dataset(const FileName &fn_Invariant,
                                       const FileName &fnParticles,
                                       int label)
{
    ImageGeneric positiveInvariant;
    MultidimArray<double> vec;
    MultidimArray<double> staticVec;
    MultidimArray<double> avg;
    MultidimArray<double> pcaBase;
    MultidimArray<double> pcaRBase;
    MultidimArray<double> binaryVec;
    MultidimArray<double> dilatedVec;
    FourierFilter filter;
    ArrayDim aDim;
    int yDataSet=YSIZE(dataSet);

    positiveInvariant.read(fn_Invariant,HEADER);
    positiveInvariant.getDimensions(aDim);
    int steps=aDim.ndim/num_correlation;
    // Resize the dataset for the new data
    dataSet.resize(1,1,yDataSet+steps,num_correlation*NPCA+NRPCA+12);
    classLabel.resize(1,1,1,YSIZE(dataSet));
    for (int n=yDataSet;n<XSIZE(classLabel);n++)
        classLabel(n)=label;
    // Here we take each channel of the particle and try to project it
    // on related PCA basis. So we first do it for first channel and obtain
    // all the features for all particles and then move to the next channel.
    for (int i=0;i<num_correlation;i++)
    {
        avg.aliasImageInStack(pcaModel,i*(NPCA+1));
        avg.resize(1,1,1,XSIZE(avg)*YSIZE(avg));
        for (int j=0;j<NPCA;j++)
        {
            pcaBase.aliasImageInStack(pcaModel,i*(NPCA+1)+1+j);
            pcaBase.resize(1,1,1,XSIZE(pcaBase)*YSIZE(pcaBase));
            for (int k=0;k<steps;k++)
            {
                positiveInvariant.readMapped(fn_Invariant,k*num_correlation+i+1);
                positiveInvariant().getImage(vec);
                vec.resize(1,1,1,XSIZE(vec)*YSIZE(vec));
                vec-=avg;
                DIRECT_A2D_ELEM(dataSet,k+yDataSet,j+(i*NPCA))=PCAProject(pcaBase,vec);
            }
        }
    }
    // Obtain the statics for each particle
    for (int i=0;i<steps;i++)
    {
        positiveInvariant.readMapped(fnParticles,i+1);
        positiveInvariant().getImage(vec);
        vec.resize(1,1,1,XSIZE(vec)*YSIZE(vec));
        extractStatics(vec,staticVec);
        for (int j=0;j<12;j++)
            DIRECT_A2D_ELEM(dataSet,i+yDataSet,j+num_correlation*NPCA)=DIRECT_A1D_ELEM(staticVec,j);
        // Project each particles on rotational PCA basis.
        for (int j=0;j<NRPCA;j++)
        {
            pcaRBase.aliasImageInStack(pcaRotModel,j);
            pcaRBase.resize(1,1,1,XSIZE(pcaRBase)*YSIZE(pcaRBase));
            DIRECT_A2D_ELEM(dataSet,i+yDataSet,num_correlation*NPCA+12+j)=PCAProject(pcaRBase,vec);
        }
    }
    fn_Invariant.deleteFile();
    fnParticles.deleteFile();
}

void AutoParticlePicking2::add2Dataset()
{
    // Here we have the features and we just want to put them
    // in the dataset.
    int yDataSet=YSIZE(dataSet);
    int newSize=rejected_particles.size()+accepted_particles.size();
    dataSet.resize(1,1,yDataSet+newSize,num_correlation*NPCA+NRPCA+12);
    classLabel.resize(1,1,1,YSIZE(dataSet));
    if (rejected_particles.size() > 0)
    {
        int limit = rejected_particles.size() + yDataSet;
        for (int n=yDataSet;n<limit;n++)
            classLabel(n)=3;
        for (int i=0;i<rejected_particles.size();i++)
            for (int j=0;j<XSIZE(dataSet);j++)
                DIRECT_A2D_ELEM(dataSet,i+yDataSet,j)=DIRECT_A1D_ELEM(rejected_particles[i].vec,j);
    }

    if (accepted_particles.size() > 0)
    {
        int limit = yDataSet+newSize;
        int size = yDataSet + rejected_particles.size();
        for (int n=size;n<limit;n++)
            classLabel(n)=1;
        for (int i=0;i<accepted_particles.size();i++)
            for (int j=0;j<XSIZE(dataSet);j++)
                DIRECT_A2D_ELEM(dataSet,i+size,j)=DIRECT_A1D_ELEM(accepted_particles[i].vec,j);
    }
}

void AutoParticlePicking2::extractPositiveInvariant(const FileName &fnInvariantFeat,
        const FileName &fnParticles,
        bool avgFlag)

{

    MultidimArray<double> IpolarCorr;
    MultidimArray<double> pieceImage;
    AlignmentAux aux;
    CorrelationAux aux2;
    RotationalCorrelationAux aux3;
    Matrix2D<double> M;
    int num_part =__m->ParticleNo();
    Image<double> II;
    FileName fnPositiveInvariatn=fnInvariantFeat+"_Positive.stk";
    FileName fnPositiveParticles=fnParticles+"_Positive.stk";
    IpolarCorr.initZeros(num_correlation,1,NangSteps,NRsteps);

    for (int i=0;i<num_part;i++)
    {
        double cost = __m->coord(i).cost;
        if (cost == 0)
            continue;
        int x=(__m->coord(i).X)*scaleRate;
        int y=(__m->coord(i).Y)*scaleRate;

        buildInvariant(IpolarCorr,x,y);
        extractParticle(x,y,microImage(),pieceImage,false);
        II()=pieceImage;
        II.write(fnPositiveParticles,ALL_IMAGES,true,WRITE_APPEND);
        // Compute the average of the manually picked particles after doing aligning
        // We just do it on manually picked particles and the flag show that if we
        // are in this step.
        if (avgFlag==false)
        {
            pieceImage.setXmippOrigin();
            particleAvg.setXmippOrigin();
            if (particleAvg.computeMax()==0)
                particleAvg=particleAvg+pieceImage;
            else
            {
                alignImages(particleAvg,pieceImage,M,true,aux,aux2,aux3);
                particleAvg=particleAvg+pieceImage;
            }
        }
        II()=IpolarCorr;
        II.write(fnPositiveInvariatn,ALL_IMAGES,true,WRITE_APPEND);
    }
    if (avgFlag==false)
        particleAvg/=num_part;
}

void AutoParticlePicking2::extractNegativeInvariant(const FileName &fnInvariantFeat,
        const FileName &fnParticles)
{
    MultidimArray<double> IpolarCorr;
    MultidimArray<double> randomValues;
    MultidimArray<double> pieceImage;
    MultidimArray<int> randomIndexes;
    std::vector<Particle2> negativeSamples;
    int num_part=__m->ParticleNo();
    Image<double> II;
    FileName fnNegativeInvariatn=fnInvariantFeat+"_Negative.stk";
    FileName fnNegativeParticles=fnParticles+"_Negative.stk";
    IpolarCorr.initZeros(num_correlation,1,NangSteps,NRsteps);
    // first we obtain all the places in which there could be
    // a negative particles.
    extractNonParticle(negativeSamples);
    // Choose some random positions from the previous step.
    RandomUniformGenerator<double> randNum(0, 1);
    randomValues.resize(1,1,1,negativeSamples.size());
    FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY1D(randomValues)
    DIRECT_A1D_ELEM(randomValues,i)=randNum();
    randomValues.indexSort(randomIndexes);
    int numNegatives;
    // If the number of particles is lower than 15 then the
    // number of the negatives is equal to 15 else it is equal
    // to the number of particles times 2.
    if (num_part<15)
        numNegatives=15;
    else
        numNegatives=num_part*2;
    for (int i=0;i<numNegatives;i++)
    {
        int x=negativeSamples[DIRECT_A1D_ELEM(randomIndexes,i)-1].x;
        int y=negativeSamples[DIRECT_A1D_ELEM(randomIndexes,i)-1].y;
        extractParticle(x,y,microImage(),pieceImage,false);
        II()=pieceImage;
        II.write(fnNegativeParticles,ALL_IMAGES,true,WRITE_APPEND);
        buildInvariant(IpolarCorr,x,y);
        II()=IpolarCorr;
        II.write(fnNegativeInvariatn,ALL_IMAGES,true,WRITE_APPEND);
    }
}

void AutoParticlePicking2::extractInvariant(const FileName &fnInvariantFeat,
        const FileName &fnParticles,
        bool avgFlag)
{
    extractPositiveInvariant(fnInvariantFeat,fnParticles,avgFlag);
    extractNegativeInvariant(fnInvariantFeat,fnParticles);
}

void AutoParticlePicking2::extractParticle(const int x, const int y,
        MultidimArray<double> &filter,
        MultidimArray<double> &particleImage,
        bool normal)
{
    int startX, startY, endX, endY;
    startX = x - particle_radius;
    startY = y - particle_radius;
    endX = x + particle_radius;
    endY = y + particle_radius;

    filter.window(particleImage, startY, startX, endY, endX);
    if (normal)
        normalize_OldXmipp(particleImage);
}

void AutoParticlePicking2::extractNonParticle(std::vector<Particle2> &negativePosition)
{
    int endX,endY;
    int gridStep=particle_radius/2;
    Particle2 negSample;
    endX = XSIZE(microImage())-particle_radius*2;
    endY = YSIZE(microImage())-particle_radius*2;
    MultidimArray<double> pieceImage;
    Image<double> II;

    for (int i=particle_radius*2;i<endY; i=i+gridStep)
        for (int j= particle_radius*2;j<endX;j=j+gridStep)
        {
            negSample.y=i;
            negSample.x=j;
            if (checkDist(negSample))
            {
                extractParticle(j,i,microImage(),pieceImage,false);
                II()=pieceImage;
                negativePosition.push_back(negSample);
            }
        }
}
void AutoParticlePicking2::convert2Polar(MultidimArray<double> &particleImage,
        MultidimArray<double> &polar)
{
    Matrix1D<double> R;
    particleImage.setXmippOrigin();
    image_convertCartesianToPolar_ZoomAtCenter(particleImage,polar,R,1,3,
            XSIZE(particleImage)/2,NRsteps,0,2*PI,NangSteps);
}

void AutoParticlePicking2::loadTrainingSet(const FileName &fn)
{
    int x,y;
    std::ifstream fhTrain;
    fhTrain.open(fn.c_str());
    fhTrain >>y>>x;
    dataSet.resize(1,1,y,x);
    classLabel.resize(1,1,1,y);
    // Load the train set and put the values in training array
    for (int i=0;i<y;i++)
    {
        fhTrain >>classLabel(i);
        for (int j= 0;j<x;j++)
            fhTrain>>DIRECT_A2D_ELEM(dataSet,i,j);
    }
    fhTrain.close();
}

void AutoParticlePicking2::saveTrainingSet(const FileName &fn)
{
    std::ofstream fhTrain;
#ifdef DEBUG_SAVETRAINSET

    std::ofstream fhtest;
    fhtest.open("WNDataset.txt");
#endif

    fhTrain.open(fn.c_str());
    fhTrain<<YSIZE(dataSet)<< " "<< XSIZE(dataSet)<< std::endl;
    for (int i=0;i<YSIZE(dataSet);i++)
    {
        fhTrain<<classLabel(i)<< std::endl;
#ifdef DEBUG_SAVETRAINSET

        fhtest<<classLabel(i)<<" ";
#endif

        for (int j=0;j<XSIZE(dataSet);j++)
            fhTrain<<DIRECT_A2D_ELEM(dataSet,i,j)<<" ";
#ifdef DEBUG_SAVETRAINSET

        fhtest<<j+1<<":"<<DIRECT_A2D_ELEM(dataSet,i,j)<<" ";
#endif

        fhTrain<<std::endl;
#ifdef DEBUG_SAVETRAINSET

        fhtest<<std::endl;
#endif

    }
    fhTrain.close();
#ifdef DEBUG_SAVETRAINSET

    fhtest.close();
#endif

}

void AutoParticlePicking2::savePCAModel(const FileName &fn_root)
{
    FileName fnPcaModel=fn_root+"_pca_model.stk";
    Image<double> II;
    MultidimArray<double> avg;

    avg=pcaAnalyzer.avg;
    avg.resize(1,1,NangSteps,NRsteps);
    II()=avg;
    II.write(fnPcaModel,ALL_IMAGES,true, WRITE_APPEND);
    for (int i=0; i<NPCA;i++)
    {
        MultidimArray<double> pcaBasis;
        pcaBasis=pcaAnalyzer.PCAbasis[i];
        pcaBasis.resize(1,1,NangSteps,NRsteps);
        II()=pcaBasis;
        II.write(fnPcaModel,ALL_IMAGES,true,WRITE_APPEND);
    }
}

/* Save particles ---------------------------------------------------------- */
int AutoParticlePicking2::saveAutoParticles(const FileName &fn) const
{
    MetaData MD;
    size_t nmax=auto_candidates.size();
    for (size_t n=0;n<nmax;++n)
    {
        const Particle2 &p=auto_candidates[n];
        if (p.cost>0 && p.status==1)
        {
            size_t id=MD.addObject();
            MD.setValue(MDL_XCOOR,int(p.x*(1.0/scaleRate)),id);
            MD.setValue(MDL_YCOOR,int(p.y*(1.0/scaleRate)),id);
            MD.setValue(MDL_COST, p.cost,id);
            MD.setValue(MDL_ENABLED,1,id);
        }
    }
    MD.write(fn,MD_OVERWRITE);
    return MD.size();
}

/* Save features of Autoparticles ---------------------------------------------------------- */
void AutoParticlePicking2::saveAutoVectors(const FileName &fn)
{
    std::ofstream fhVectors;
    fhVectors.open(fn.c_str());
    int X=XSIZE(auto_candidates[0].vec);
    fhVectors<<auto_candidates.size()<<" " <<X<<std::endl;
    size_t nmax = auto_candidates.size();
    for (size_t n=0; n<nmax;++n)
    {
        const Particle2 &p=auto_candidates[n];
        if (p.cost>0 && p.status==1)
        {
            for (int j=0;j<X;++j)
                fhVectors<<DIRECT_A1D_ELEM(p.vec,j)<<" ";
            fhVectors<<std::endl;
        }
    }
    fhVectors.close();
}

/* Load features of Autoparticles ---------------------------------------------------------- */
void AutoParticlePicking2::loadAutoVectors(const FileName &fn)
{
    int numVector;
    int numFeature;
    std::ifstream fhVectors;
    fhVectors.open(fn.c_str());
    fhVectors>>numVector>>numFeature;
    for (int n=0; n<numVector;++n)
    {
        Particle2 p;
        p.vec.resize(1,1,1,numFeature);
        for (int j=0; j<numFeature;++j)
            fhVectors >> DIRECT_A1D_ELEM(p.vec,j);
        auto_candidates.push_back(p);
    }
    fhVectors.close();
}

/* Save features of rejected and kept particles ---------------------------------------------------------- */
void AutoParticlePicking2::saveVectors(const FileName &fn)
{
    if (rejected_particles.size()>0)
    {
        std::ofstream fhRejected;
        FileName fnRejectedVectors=fn+"_rejected_vector.txt";
        fhRejected.open(fnRejectedVectors.c_str());
        int rejSize=XSIZE(rejected_particles[0].vec);
        fhRejected<<rejected_particles.size()<<" "<<rejSize<< std::endl;
        size_t nmax=rejected_particles.size();
        for (size_t n=0;n<nmax;++n)
        {
            const Particle2 &p=rejected_particles[n];
            for (int j=0;j<rejSize;++j)
                fhRejected<<DIRECT_A1D_ELEM(p.vec,j)<<" ";
            fhRejected <<std::endl;
        }
        fhRejected.close();
    }
    if (accepted_particles.size()>0)
    {
        std::ofstream fhAccepted;
        FileName fnAcceptedVectors=fn+"_accepted_vector.txt";
        fhAccepted.open(fnAcceptedVectors.c_str());
        int accSize=XSIZE(accepted_particles[0].vec);
        fhAccepted<<accepted_particles.size()<<" "<<accSize<< std::endl;
        size_t nmax=accepted_particles.size();
        for (size_t n=0;n<nmax;++n)
        {
            const Particle2 &p=accepted_particles[n];
            for (int j=0;j<accSize;++j)
                fhAccepted<<DIRECT_A1D_ELEM(p.vec,j)<<" ";
            fhAccepted <<std::endl;
        }
        fhAccepted.close();
    }
}

/* Load features of rejected and kept particles ---------------------------------------------------------- */
void AutoParticlePicking2::loadVectors(const FileName &fn)
{
    int numVector;
    int numFeature;
    FileName fnRejectedVectors=fn+"_rejected_vector.txt";
    FileName fnAcceptedVectors=fn+"_accepted_vector.txt";

    if (fnRejectedVectors.exists())
    {
        std::ifstream fhRejected;
        fhRejected.open(fnRejectedVectors.c_str());
        fhRejected>>numVector>>numFeature;
        for (int n=0; n<numVector;++n)
        {
            Particle2 p;
            p.vec.resize(1,1,1,numFeature);
            for (int j=0; j<numFeature;++j)
                fhRejected >> DIRECT_A1D_ELEM(p.vec,j);
            rejected_particles.push_back(p);
        }
        fhRejected.close();
        fnRejectedVectors.deleteFile();
    }
    if (fnAcceptedVectors.exists())
    {
        std::ifstream fhAccepted;
        fhAccepted.open(fnAcceptedVectors.c_str());
        fhAccepted>>numVector>>numFeature;
        for (int n=0; n<numVector;++n)
        {
            Particle2 p;
            p.vec.resize(1,1,1,numFeature);
            for (int j=0; j<numFeature;++j)
                fhAccepted >> DIRECT_A1D_ELEM(p.vec,j);
            accepted_particles.push_back(p);
        }
        fhAccepted.close();
        fnAcceptedVectors.deleteFile();
    }
}

void AutoParticlePicking2::generateTrainSet()
{
    int cnt=0;
    FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY1D(classLabel)
    if (DIRECT_A1D_ELEM(classLabel,i)==1 || DIRECT_A1D_ELEM(classLabel,i)==3)
        cnt++;
    dataSet1.resize(1,1,cnt,XSIZE(dataSet));
    classLabel1.resize(1,1,1,cnt);
    cnt=0;
    FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY1D(classLabel)
    {
        if (DIRECT_A1D_ELEM(classLabel,i)==1 || DIRECT_A1D_ELEM(classLabel,i)==3)
        {
            if (DIRECT_A1D_ELEM(classLabel,i)==3)
            {
                DIRECT_A1D_ELEM(classLabel1,cnt)=2;
                DIRECT_A1D_ELEM(classLabel,i)=2;
            }
            else
                DIRECT_A1D_ELEM(classLabel1,cnt)=1;
            for (int j=0;j<XSIZE(dataSet);j++)
                DIRECT_A2D_ELEM(dataSet1,cnt,j)=DIRECT_A2D_ELEM(dataSet,i,j);
            cnt++;
        }
    }
}

void AutoParticlePicking2::normalizeDataset(int a,int b,const FileName &fn)
{

    double max,min;
    MultidimArray<double> maxA;
    MultidimArray<double> minA;

    maxA.resize(1,1,1,YSIZE(dataSet));
    minA.resize(1,1,1,YSIZE(dataSet));

    // Computing the maximum and minimum of each row
    for (int i=0;i<YSIZE(dataSet);i++)
    {
        max=min=DIRECT_A2D_ELEM(dataSet,i,0);
        for (int j=1;j<XSIZE(dataSet);j++)
        {
            if (max<DIRECT_A2D_ELEM(dataSet,i,j))
                max=DIRECT_A2D_ELEM(dataSet,i,j);
            if (min>DIRECT_A2D_ELEM(dataSet,i,j))
                min=DIRECT_A2D_ELEM(dataSet,i,j);
        }
        DIRECT_A1D_ELEM(maxA,i)=max;
        DIRECT_A1D_ELEM(minA,i)=min;
    }
    // Normalizing the dataset according to the max and mean
    for (int i=0;i<YSIZE(dataSet);i++)
    {
        max=DIRECT_A1D_ELEM(maxA,i);
        min=DIRECT_A1D_ELEM(minA,i);
        for (int j=0;j<XSIZE(dataSet);j++)
            DIRECT_A2D_ELEM(dataSet,i,j)=a+((b-a)*((DIRECT_A2D_ELEM(dataSet,i,j)-min)/(max-min)));
    }
}

void AutoParticlePicking2::buildSearchSpace(std::vector<Particle2> &positionArray,bool fast)
{
    int endX,endY;
    Particle2 p;

    endX=XSIZE(microImage())-particle_radius;
    endY=YSIZE(microImage())-particle_radius;
    applyConvolution(fast);

    for (int i=particle_radius;i<endY;i++)
        for (int j=particle_radius;j<endX;j++)
            if (isLocalMaxima(convolveRes,j,i))
            {
                p.y=i;
                p.x=j;
                p.cost=DIRECT_A2D_ELEM(convolveRes,i,j);
                p.status=0;
                positionArray.push_back(p);
            }
    for (int i=0;i<positionArray.size();++i)
        for (int j=0;j<positionArray.size()-i-1;j++)
            if (positionArray[j].cost<positionArray[j + 1].cost)
            {
                p=positionArray[j+1];
                positionArray[j+1]=positionArray[j];
                positionArray[j]=p;
            }
}

void AutoParticlePicking2::applyConvolution(bool fast)
{

    MultidimArray<double> tempConvolve;
    MultidimArray<double> avgRotated, avgRotatedLarge;
    MultidimArray<int> mask;
    CorrelationAux aux;
    FourierFilter filter;
    int sizeX = XSIZE(microImage());
    int sizeY = YSIZE(microImage());

    //Generating Mask
    mask.resize(particleAvg);
    mask.setXmippOrigin();
    BinaryCircularMask(mask, XSIZE(particleAvg)/2);
    normalize_NewXmipp(particleAvg,mask);
    particleAvg.setXmippOrigin();

    filter.raised_w=0.02;
    filter.FilterShape=RAISED_COSINE;
    filter.FilterBand=BANDPASS;
    filter.w1=1.0/double(particle_size);
    filter.w2=1.0/(double(particle_size)/3);
    if (fast)
    {
        // In fast mode we just do the convolution with the average of the
        // rotated templates
        avgRotatedLarge=particleAvg;
        for (int deg=3;deg<360;deg+=3)
        {
            rotate(LINEAR,avgRotated,particleAvg,double(deg));
            avgRotated.setXmippOrigin();
            avgRotatedLarge.setXmippOrigin();
            avgRotatedLarge+=avgRotated;
        }
        avgRotatedLarge/=120;
        avgRotatedLarge.selfWindow(FIRST_XMIPP_INDEX(sizeY),FIRST_XMIPP_INDEX(sizeX),
                                   LAST_XMIPP_INDEX(sizeY),LAST_XMIPP_INDEX(sizeX));
        correlation_matrix(microImage(),avgRotatedLarge,convolveRes,aux,false);
        filter.do_generate_3dmask=true;
        filter.generateMask(convolveRes);
        filter.applyMaskSpace(convolveRes);
    }
    else
    {
        avgRotatedLarge=particleAvg;
        avgRotatedLarge.selfWindow(FIRST_XMIPP_INDEX(sizeY),FIRST_XMIPP_INDEX(sizeX),
                                   LAST_XMIPP_INDEX(sizeY),LAST_XMIPP_INDEX(sizeX));
        correlation_matrix(microImage(),avgRotatedLarge,convolveRes,aux,false);
        filter.do_generate_3dmask=true;
        filter.generateMask(convolveRes);
        filter.applyMaskSpace(convolveRes);

        int cnt=1;
        for (int deg=3;deg<360;deg+=3)
        {
            // We first rotate the template and then put it in the big image in order to
            // the convolution
            rotate(LINEAR,avgRotated,particleAvg,double(deg));
            avgRotatedLarge=avgRotated;
            avgRotatedLarge.setXmippOrigin();
            avgRotatedLarge.selfWindow(FIRST_XMIPP_INDEX(sizeY),FIRST_XMIPP_INDEX(sizeX),
                                       LAST_XMIPP_INDEX(sizeY),LAST_XMIPP_INDEX(sizeX));
            correlation_matrix(aux.FFT1,avgRotatedLarge,tempConvolve,aux,false);
            filter.applyMaskSpace(tempConvolve);
            FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(convolveRes)
            if (DIRECT_A2D_ELEM(tempConvolve,i,j)>DIRECT_A2D_ELEM(convolveRes,i,j))
                DIRECT_A2D_ELEM(convolveRes,i,j)=DIRECT_A2D_ELEM(tempConvolve,i,j);
        }
    }
    CenterFFT(convolveRes,true);
}

// ==========================================================================
// Section: Program interface ===============================================
// ==========================================================================
void ProgMicrographAutomaticPicking2::readParams()
{
    fn_micrograph = getParam("-i");
    fn_model = getParam("--model");
    size = getIntParam("--particleSize");
    mode = getParam("--mode");
    if (mode == "buildinv")
    {
        fn_train = getParam("--mode", 1);
    }
    NPCA = getIntParam("--NPCA");
    filter_num = getIntParam("--filter_num");
    corr_num = getIntParam("--NCORR");
    Nthreads = getIntParam("--thr");
    fn_root = getParam("--outputRoot");
    fast = checkParam("--fast");
    incore = checkParam("--in_core");
    procprec = getIntParam("--autoPercent");
}

void ProgMicrographAutomaticPicking2::defineParams()
{
    addUsageLine("Automatic particle picking for micrographs");
    addUsageLine("+The algorithm is designed to learn the particles from the user, as well as from its own errors.");
    addUsageLine("+The algorithm is fully described in [[http://www.ncbi.nlm.nih.gov/pubmed/19555764][this paper]].");
    addParamsLine("  -i <micrograph>               : Micrograph image");
    addParamsLine("  --outputRoot <rootname>       : Output rootname");
    addParamsLine("  --mode <mode>                 : Operation mode");
    addParamsLine("         where <mode>");
    addParamsLine("                    try              : Try to autoselect within the training phase.");
    addParamsLine("                    train            : Train the classifier using the invariants features.");
    addParamsLine("                                     : <rootname>_auto_feature_vectors.txt contains the particle structure created by this program when used in automatic selection mode");
    addParamsLine("                                     : <rootname>_false_positives.xmd contains the list of false positives among the automatically picked particles");
    addParamsLine("                    autoselect  : Autoselect");
    addParamsLine("                    buildinv <posfile=\"\"> : posfile contains the coordinates of manually picked particles");
    addParamsLine("  --model <model_rootname>      : Bayesian model of the particles to pick");
    addParamsLine("  --particleSize <size>         : Particle size in pixels");
    addParamsLine("  [--thr <p=1>]                 : Number of threads for automatic picking");
    addParamsLine("  [--fast]                      : Perform a fast preprocessing of the micrograph (Fourier filter instead of Wavelet filter)");
    addParamsLine("  [--in_core]                   : Read the micrograph in memory");
    addParamsLine("  [--filter_num <n=6>]          : The number of filters in filter bank");
    addParamsLine("  [--NPCA <n=4>]               : The number of PCA components");
    addParamsLine("  [--NCORR <n=2>]               : The number of PCA components");
    addParamsLine("  [--autoPercent <n=90>]               : The number of PCA components");
    addExampleLine("Automatically select particles during training:", false);
    addExampleLine("xmipp_micrograph_automatic_picking -i micrograph.tif --particleSize 100 --model model --thr 4 --outputRoot micrograph --mode try ");
    addExampleLine("Training:", false);
    addExampleLine("xmipp_micrograph_automatic_picking -i micrograph.tif --particleSize 100 --model model --thr 4 --outputRoot micrograph --mode train manual.pos");
    addExampleLine("Automatically select particles after training:", false);
    addExampleLine("xmipp_micrograph_automatic_picking -i micrograph.tif --particleSize 100 --model model --thr 4 --outputRoot micrograph --mode autoselect");
}

void ProgMicrographAutomaticPicking2::run()
{
    Micrograph m;
    m.open_micrograph(fn_micrograph);

    FileName fnFilterBank=fn_root+"_filterbank.stk";
    FileName familyName=fn_model.removeDirectories();
    FileName fnAutoParticles=familyName+"@"+fn_root+"_auto.pos";
    FileName fnInvariant=fn_model+"_invariant";
    FileName fnParticles=fn_model+"_particle";
    FileName fnPCAModel=fn_model+"_pca_model.stk";
    FileName fnPCARotModel=fn_model+"_rotpca_model.stk";
    FileName fnSVMModel=fn_model+"_svm.txt";
    FileName fnSVMModel2=fn_model+"_svm2.txt";
    FileName fnVector=fn_model+"_training.txt";
    FileName fnAutoVectors=fn_model+"_auto_vector.txt";
    FileName fnRejectedVectors=fn_model+"_rejected_vector.txt";
    FileName fnAvgModel=fn_model+"_particle_avg.xmp";

    AutoParticlePicking2 *autoPicking=new AutoParticlePicking2(fn_micrograph,&m,size,filter_num,
                                      NPCA,corr_num,procprec);
    if (mode!="train")
    {
        // Resize the Micrograph
        selfScaleToSizeFourier((m.Ydim)*autoPicking->scaleRate,(m.Xdim)*autoPicking->scaleRate,
                               autoPicking->microImage(),2);
        // Generating the filter bank
        filterBankGenerator(autoPicking->microImage(),fnFilterBank,filter_num);
        autoPicking->micrographStack.read(fnFilterBank,DATA);
    }

    if (mode=="buildinv")
    {
        MetaData MD;
        // Insert all true positives
        if (fn_train!="")
        {
            MD.read(fn_train);
            int x, y;
            FOR_ALL_OBJECTS_IN_METADATA(MD)
            {
                MD.getValue(MDL_XCOOR,x, __iter.objId);
                MD.getValue(MDL_YCOOR,y, __iter.objId);
                m.add_coord(x,y,0,1);
            }
            // Insert the false positives
            if (fnAutoParticles.existsTrim())
            {

                int idx=0;
                MD.read(fnAutoParticles);
                if (MD.size()>0)
                {
                    autoPicking->loadAutoVectors(fnAutoVectors);
                    FOR_ALL_OBJECTS_IN_METADATA(MD)
                    {
                        int enabled;
                        MD.getValue(MDL_ENABLED,enabled,__iter.objId);
                        if (enabled==-1)
                        {
                            autoPicking->rejected_particles.push_back(
                                autoPicking->auto_candidates[idx]);
                        }
                        else
                        {
                            autoPicking->accepted_particles.push_back(
                                autoPicking->auto_candidates[idx]);
                            MD.getValue(MDL_XCOOR, x, __iter.objId);
                            MD.getValue(MDL_YCOOR, y, __iter.objId);
                            m.add_coord(x, y, 0, 0);
                        }
                        ++idx;
                    }
                }
                autoPicking->saveVectors(fn_model);
            }
        }
        if (fnAvgModel.exists())
        {
            Image<double> II;
            II.read(fnAvgModel);
            autoPicking->particleAvg=II();
        }
        else
        {
            autoPicking->particleAvg.initZeros(autoPicking->particle_size+1,autoPicking->particle_size+1);
            autoPicking->particleAvg.setXmippOrigin();
        }
        // If the PCA model exist then we have obtained the templatea and then we do not want
        // to continue it anymore
        if (fnPCAModel.exists())
            autoPicking->extractInvariant(fnInvariant,fnParticles,true);
        else
        {
            autoPicking->extractInvariant(fnInvariant,fnParticles,false);
            Image<double> II;
            II()=autoPicking->particleAvg;
            II.write(fnAvgModel);
        }
    }

    if (mode=="try" || mode=="autoselect")
    {
        if (mode=="autoselect")
        {
            MetaData MD;
            MD.read(fn_model.beforeLastOf("/")+"/config.xmd");
            MD.getValue( MDL_PICKING_AUTOPICKPERCENT,autoPicking->proc_prec,MD.firstObject());
        }
        autoPicking->micrographStack.read(fnFilterBank, DATA);
        // Read the PCA Model
        Image<double> II;
        II.read(fnPCAModel);
        autoPicking->pcaModel=II();
        // Read rotational PCA model
        II.read(fnPCARotModel);
        autoPicking->pcaRotModel=II();
        // Read the average of the particles for convolution
        II.read(fnAvgModel);
        autoPicking->particleAvg=II();
        // Read the SVM model
        autoPicking->classifier.LoadModel(fnSVMModel);
        // If we have generated the second SVM model then we use
        // two classifiers.
        if (fnSVMModel2.exists())
        {
            autoPicking->classifier2.LoadModel(fnSVMModel2);
            int num=autoPicking->automaticallySelectParticles(true,fast);
        }
        else
            int num=autoPicking->automaticallySelectParticles(false,fast);
        autoPicking->saveAutoParticles(fnAutoParticles);
        if (mode=="try")
            autoPicking->saveAutoVectors(fnAutoVectors);
    }
    if (mode=="train")
    {
        // If PCA does not exist obtain the PCA basis and save them
        if (!fnPCAModel.exists())
            autoPicking->trainPCA(fn_model);
        if (!fnPCARotModel.exists())
            autoPicking->trainRotPCA(fnAvgModel,fnPCARotModel.removeAllExtensions());

        // If we have the models then we just load it
        Image<double> II;
        II.read(fnPCAModel);
        autoPicking->pcaModel=II();
        II.read(fnPCARotModel);
        autoPicking->pcaRotModel=II();
        if (fnVector.exists())
            autoPicking->loadTrainingSet(fnVector);
        autoPicking->add2Dataset(fnInvariant+"_Positive.stk",fnParticles+"_Positive.stk",1);
        autoPicking->add2Dataset(fnInvariant+"_Negative.stk",fnParticles+"_Negative.stk",2);
        // If we have some false positives also add it
        // Load the rejected vectors features as false positives
        autoPicking->loadVectors(fn_model);
        if (autoPicking->rejected_particles.size()!= 0 || autoPicking->accepted_particles.size()!= 0)
            autoPicking->add2Dataset();
        // We just save the un normalized dataset
        autoPicking->saveTrainingSet(fnVector);
        autoPicking->normalizeDataset(0,1,fn_model);
        // Generate two different dataset
        autoPicking->generateTrainSet();

        autoPicking->trainSVM(fnSVMModel,1);
        autoPicking->trainSVM(fnSVMModel2,2);
    }
    if (mode!="train")
        fnFilterBank.deleteFile();
    delete autoPicking;
}
