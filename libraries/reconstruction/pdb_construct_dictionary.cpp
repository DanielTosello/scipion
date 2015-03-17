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

#include "pdb_construct_dictionary.h"
#include <data/xmipp_image_extension.h>

/** Explore FLANN http://www.cs.ubc.ca/research/flann/, as a way to speed-up calculations */

void ProgConstructPDBDictionary::defineParams()
{
    // Usage
    addUsageLine("This program takes a set of PDB files at low and high resolution and constructs a dictionary for them.");

    // Parameters
    addParamsLine(" --low <metadata>      : Metadata with the low resolution volumes");
    addParamsLine(" --high <metadata>     : Metadata with the high resolution volumes");
    addParamsLine(" [--oroot <root=dictionary>] : Rootname for the output files");
    addParamsLine(" [--patchSize <n=5>] : Patch size for the dictionary");
    addParamsLine(" [--stdThreshold <s=0.25>] : Threshold on the standard deviation to include a patch");
    addParamsLine(" [--angleThreshold <s=5>] : Threshold in degrees on the angle to include a patch");
}

void ProgConstructPDBDictionary::readParams()
{
	fnLow=getParam("--low");
	fnHigh=getParam("--high");
	fnRoot=getParam("--oroot");
	patchSize=getIntParam("--patchSize");
	stdThreshold=getDoubleParam("--stdThreshold");
	angleThreshold=cos(DEG2RAD(getDoubleParam("--angleThreshold")));
}

/* Show -------------------------------------------------------------------- */
void ProgConstructPDBDictionary::show()
{
    if (verbose)
		std::cout
		<< "Input low volumes:   " << fnLow     << std::endl
		<< "Input high volumes:  " << fnHigh    << std::endl
		<< "Output rootname:     " << fnRoot    << std::endl
		<< "Patch size:          " << patchSize << std::endl
		;
}

//#define DEBUG
void ProgConstructPDBDictionary::run()
{
    show();
    if (fileExists(fnRoot+"_low.mrcs"))
    	loadDictionaries();

    MetaData mdlow, mdhigh;
    mdlow.read(fnLow);
    mdhigh.read(fnHigh);

    FileName fnVol;
    Image<double> Vlow, Vhigh;
#ifdef DEBUG
    Image<double>Vmask;
#endif
    MultidimArray<double> patchLow, patchHigh;
    int patchSize_2=patchSize/2;

    patchLow.resize(patchSize_2,patchSize_2,patchSize_2);
    patchLow.setXmippOrigin();

    FOR_ALL_OBJECTS_IN_METADATA2(mdlow,mdhigh)
    {
    	// Read the low and high resolution volumes
    	mdlow.getValue(MDL_IMAGE,fnVol,__iter.objId);
    	Vlow.read(fnVol);
    	std::cout << "Processing " << fnVol << " and ";
    	mdhigh.getValue(MDL_IMAGE,fnVol,__iter2.objId);
    	std::cout << fnVol << std::endl;
    	Vhigh.read(fnVol);

    	// Go through the volumes and decide whether to move into the dictionary
    	const MultidimArray<double> &mVlow=Vlow();
    	const MultidimArray<double> &mVhigh=Vhigh();

    	double minLow, maxLow, meanLow, stdLow=0;
    	double minHigh, maxHigh, meanHigh, stdHigh=0;
    	mVlow.computeStats(meanLow,stdLow,minLow,maxLow);
    	mVhigh.computeStats(meanHigh,stdHigh,minHigh,maxHigh);
#ifdef DEBUG
    	Vmask().initZeros(Vhigh());
#endif

    	size_t Npatches=0, NcandidatePatches=0, NsuccessfulPatches=0;
    	init_progress_bar(ZSIZE(mVlow));
        for (int k=patchSize_2; k<(int)ZSIZE(mVlow)-patchSize_2; ++k)
        {
            for (int i=patchSize_2; i<(int)ZSIZE(mVlow)-patchSize_2; ++i)
                for (int j=patchSize_2; j<(int)ZSIZE(mVlow)-patchSize_2; ++j)
                {
                	 ++Npatches;
                     mVlow.window(patchLow,k,i,j,k+patchSize-1,i+patchSize-1,j+patchSize-1);
                     mVhigh.window(patchHigh,k,i,j,k+patchSize-1,i+patchSize-1,j+patchSize-1);

                 	double minPatchLow, maxPatchLow, meanPatchLow, stdPatchLow=0;
                 	double minPatchHigh, maxPatchHigh, meanPatchHigh, stdPatchHigh=0;
                 	patchLow.computeStats(meanPatchLow,stdPatchLow,minPatchLow,maxPatchLow);
                 	patchHigh.computeStats(meanPatchHigh,stdPatchHigh,minPatchHigh,maxPatchHigh);

                 	if (stdPatchLow > stdThreshold*stdLow && stdPatchHigh > stdThreshold*stdHigh)
                 	{
                 		++NcandidatePatches;

                 		// Candidate patch
                 		double inormPatchLow=1.0/sqrt(patchLow.sum2());
                 		patchLow*=inormPatchLow;
                 		if (notInDictionary(patchLow))
                 		{
                 			++NsuccessfulPatches;
                 			dictionaryLow.push_back(patchLow);
                     		patchHigh*=inormPatchLow;
                 			dictionaryHigh.push_back(patchHigh);
                 		}

#ifdef DEBUG
                 		Vmask(k,i,j)=1;
#endif
                 	}
                }
            progress_bar(k);
        }
        progress_bar(ZSIZE(mVlow));
        std::cout << "Candidate patches =" << NcandidatePatches << "(" << ((double)NcandidatePatches)/Npatches*100 << "%)"
        		  << "  successful=" << NsuccessfulPatches << "(" << ((double)NsuccessfulPatches)/NcandidatePatches*100 << "%)" << std::endl;
        saveDictionaries();
#ifdef DEBUG
        Vmask.write("PPPmask.vol");
		std::cout << "Press any key\n";
		char c; std::cin >> c;
#endif
    }
}
#undef DEBUG

bool ProgConstructPDBDictionary::notInDictionary(const MultidimArray<double> &candidatePatch) const
{
	size_t imax=dictionaryLow.size();

	for (size_t i=0; i<imax; ++i)
	{
		const MultidimArray<double> &dictionaryPatch=dictionaryLow[i];
		double dotProduct=0;
		FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(dictionaryPatch)
			dotProduct+=DIRECT_MULTIDIM_ELEM(dictionaryPatch,n)*DIRECT_MULTIDIM_ELEM(candidatePatch,n);
		if (dotProduct>angleThreshold)
			return false;
	}
	return true;
}

void ProgConstructPDBDictionary::loadDictionaries()
{
	std::cout << "Loading dictionaries .." << std::endl;
	size_t Xdim, Ydim, Zdim, Ndim;
	FileName fnLow=fnRoot+"_low.mrcs";
	FileName fnHigh=fnRoot+"_high.mrcs";
	getImageSize(fnLow, Xdim, Ydim, Zdim, Ndim);
	dictionaryLow.reserve(Ndim);
	dictionaryHigh.reserve(Ndim);
	Image<double> aux;
	for (size_t n=0; n<Ndim; ++n)
	{
		aux.read(fnLow,DATA,n+1);
		dictionaryLow.push_back(aux());
		aux.read(fnHigh,DATA,n+1);
		dictionaryHigh.push_back(aux());
	}
}

void ProgConstructPDBDictionary::saveDictionaries() const
{
	std::cout << "Saving dictionaries ..." << std::endl;
	size_t imax=dictionaryLow.size();
	FileName fnLow=fnRoot+"_low.mrcs";
	FileName fnHigh=fnRoot+"_high.mrcs";
	createEmptyFile(fnLow,patchSize,patchSize,patchSize,imax,true);
	createEmptyFile(fnHigh,patchSize,patchSize,patchSize,imax,true);
	Image<double> aux;
	for (size_t i=0; i<imax; ++i)
	{
		aux()=dictionaryLow[i];
		aux.write(fnLow,i+1,true,WRITE_REPLACE);
		aux()=dictionaryHigh[i];
		aux.write(fnHigh,i+1,true,WRITE_REPLACE);
	}
}
