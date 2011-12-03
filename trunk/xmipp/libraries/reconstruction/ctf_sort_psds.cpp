/***************************************************************************
 *
 * Authors:     Carlos Oscar S. Sorzano (coss@cnb.csic.es)
 *              Slavica Jonic (slavica.jonic@impmc.jussieu.fr)
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

#include <vector>
#include "ctf_sort_psds.h"
#include "ctf_enhance_psd.h"
#include "ctf_estimate_from_micrograph.h"
#include <data/args.h>
#include <data/filters.h>
#include <data/transformations.h>
#include <data/histogram.h>

/* Read parameters --------------------------------------------------------- */
void ProgPSDSort::readParams()
{
    fnSel = getParam("-i");
    filter_w1 = getDoubleParam("-f1");
    filter_w2 = getDoubleParam("-f2");
    decay_width = getDoubleParam("-decay");
    mask_w1 = getDoubleParam("-m1");
    mask_w2 = getDoubleParam("-m2");
}

/* Usage ------------------------------------------------------------------- */
void ProgPSDSort::defineParams()
{
    addUsageLine("Evaluate the CTFs and PSDs of a set of micrographs.");
    addUsageLine("This process is strongly coupled to the output produced by the preprocessing micrographs step of the Xmipp protocols. ");
    addUsageLine("For each input PSD, the program writes its enhanced version since it is used in the computation of some of the criteria.");
    addUsageLine("+The different criteria for evaluating the PSDs are:");
    addUsageLine("+ ");
    addUsageLine("+$ *Damping*: this is the envelope value at the border of the PSD. Micrographs ");
    addUsageLine("+with a high envelope value at border are either wrongly estimated strongly undersampled.");
    addUsageLine("+ ");
    addUsageLine("+$ *First zero average*: this is average in Angstroms of the first zero. ");
    addUsageLine("+Normally, this value should be between 4x and 10x the sampling rate in Angstroms.");
    addUsageLine("+ ");
    addUsageLine("+$ *First zero disagreement*: if the CTF has been estimated by two different methods ");
    addUsageLine("+(normally Xmipp and Ctffind), then this criterion measures the average disagreement ");
    addUsageLine("+in Angstroms between the first zero in the two estimates. Low disagreements are ");
    addUsageLine("+indicative of correct fit.");
    addUsageLine("+ ");
    addUsageLine("+$ *First zero ratio*: this measures the astigmatism of the CTF by computing the ratio ");
    addUsageLine("+between the largest and smallest axes of the first zero ellipse. Ratios close to 1 ");
    addUsageLine("+indicate no astigmatism.");
    addUsageLine("+ ");
    addUsageLine("+$ *Fitting score*: the CTF is computed by fitting a theoretical model to the experimentally observed PSD. ");
    addUsageLine("+This criterion is the fitting score. Smaller scores correspond to better fits.");
    addUsageLine("+ ");
    addUsageLine("+$ *Fitting correlation between zeros 1 and 3*: the region between the first and third zeroes ");
    addUsageLine("+is particularly important since it is where the Thon rings are most visible. ");
    addUsageLine("+This criterion reports the correlation between the experimental and theoretical PSDs ");
    addUsageLine("+within this region. High correlations indicate good fits.");
    addUsageLine("+ ");
    addUsageLine("+$ *PSD correlation at 90 degrees*: The PSD of non-astigmatic micrographs correlate well ");
    addUsageLine("+with itself after rotating the micrograph 90 degrees. This is so because non-astigmatic ");
    addUsageLine("+PSDs are circularly symmetrical, while astigmatic micrographs are elliptically symmetrical.");
    addUsageLine("+High correlation when rotating 90 degrees is an indicator of non-astigmatism.");
    addUsageLine("+This criterion is computed on the enhanced PSD. See [[ctf_enhance_psd_v3][ctf_enhance_psd]].");
    addUsageLine("+ ");
    addUsageLine("+$ *PSD radial integral*: this criterion reports the integral of the radially symmetrized PSD.");
    addUsageLine("+This criterion can highlight differences among the background noises of micrographs. ");
    addUsageLine("+This criterion is computed on the enhanced PSD. See [[ctf_enhance_psd_v3][ctf_enhance_psd]].");
    addUsageLine("+ ");
    addUsageLine("+$ *PSD variance*: the PSD is estimated by averaging different PSD local estimates in small regions of the micrograph. ");
    addUsageLine("+This criterion measures the variance of the different PSD local estimates. Untilted micrographs ");
    addUsageLine("+have equal defoci all over the micrograph, and therefore, the variance is due only to noise. ");
    addUsageLine("+However, tilted micrographs have an increased PSD variance since different regions of the micrograph ");
    addUsageLine("+have different defoci. Low variance of the PSD are indicative of non-tilted micrographs");
    addUsageLine("+ ");
    addUsageLine("+$ *PSD Principal Component 1 Variance*: when considering the local PSDs previously defined as vectors ");
    addUsageLine("+in a multidimensional space, we can compute the variance of their projection onto the first principal component axis. ");
    addUsageLine("+Low variance of this projection is indicative of a uniformity of local PSDs, i.e., this is another measure ");
    addUsageLine("+of the presence of tilt in the micrograph.");
    addUsageLine("+ ");
    addUsageLine("+$ *PSD !PCA Runs test*: when computing the projections onto the first principal component, as discussed in the previous criterion, ");
    addUsageLine("+one might expect that the sign of the projection is random for untilted micrographs. Micrographs with a marked ");
    addUsageLine("+non-random pattern of projections are indicative of tilted micrographs. The larger the value of this criterion, the less random the pattern is.");
    addParamsLine("   -i <selfile>              : Selfile with micrographs, it needs the columns image, psd and ctfmodel");
    addParamsLine("                             : This file is modified by the addition of the evaluation criteria");
    addParamsLine("==+ Enhancement filter parameters");
    addParamsLine("  [-f1 <freq_low=0.02>]      : Low freq. for band pass filtration, max 0.5");
    addParamsLine("  [-f2 <freq_high=0.2>]      : High freq. for band pass filtration, max 0.5");
    addParamsLine("  [-decay <freq_decay=0.02>] : Decay for the transition bands");
    addParamsLine("  [-m1 <mfreq_low=0.01>]     : Low freq. for mask, max 0.5");
    addParamsLine("  [-m2 <mfreq_high=0.45>]    : High freq. for mask, max 0.5");
}

/* Show -------------------------------------------------------------------- */
void ProgPSDSort::show() const
{
    std::cout << "Selfile:      " << fnSel << std::endl
    << "Filter w1:    " << filter_w1 << std::endl
    << "Filter w2:    " << filter_w2 << std::endl
    << "Filter decay: " << decay_width << std::endl
    << "Mask w1:      " << mask_w1 << std::endl
    << "Mask w2:      " << mask_w2 << std::endl;
}

/* Compute Correlation ----------------------------------------------------- */
double ProgPSDSort::evaluate(const FileName &fnMicrograph,
    const FileName &fnPSD, const FileName &fnCTF, const FileName &fnCTF2,
    PSDEvaluation &evaluation) const
{
    FileName fnRoot = fnMicrograph.withoutExtension();

    // Read input data
    Image<double> PSD;
    PSD.read(fnPSD);
    CTFDescription CTF1, CTF2;
    CTF1.read(fnCTF);
    CTF1.Produce_Side_Info();

    if (!fnCTF2.empty() && fnCTF2 != "NA")
    {

    	CTF2.read(fnCTF2);
    	CTF2.Produce_Side_Info();
    }

    // Enhance the PSD
    ProgCTFEnhancePSD enhancePSD;
    enhancePSD.filter_w1 = filter_w1;
    enhancePSD.filter_w2 = filter_w2;
    enhancePSD.decay_width = decay_width;
    enhancePSD.mask_w1 = mask_w1;
    enhancePSD.mask_w2 = mask_w2;
    enhancePSD.applyFilter(PSD());

    // Evaluate the radial integral
    PSD().setXmippOrigin();
    Matrix1D< int > center_of_rot(2);
    MultidimArray< double > radial_mean;
    MultidimArray<int> radial_count;
    radialAverage(PSD(),center_of_rot,radial_mean,radial_count);
    radial_mean.selfABS();
    radial_mean/=radial_mean.computeMax();
    evaluation.PSDradialIntegral=radial_mean.sum();

    // Rotate 90 degrees and compute correlation
    Image<double> PSDrotated;
    rotate(LINEAR,PSDrotated(),PSD(),90);
    evaluation.PSDcorrelation90=correlationIndex(PSD(), PSDrotated());

    // Get the fitting score
    MetaData MD;
    MD.read(fnCTF);
    size_t objId = MD.firstObject();
    MD.getValue(MDL_CTF_CRITERION_FITTINGSCORE,evaluation.fittingScore,objId);
    MD.getValue(MDL_CTF_CRITERION_FITTINGCORR13,evaluation.fittingCorr13,objId);
    MD.getValue(MDL_CTF_CRITERION_PSDVARIANCE,evaluation.PSDVariance,objId);
    MD.getValue(MDL_CTF_CRITERION_PSDPCA1VARIANCE,evaluation.PSDPC1Variance,objId);
    MD.getValue(MDL_CTF_CRITERION_PSDPCARUNSTEST,evaluation.PSDPCRunsTest,objId);

    // Explore the CTF
    Matrix1D<double> u(2), freqZero1(2), freqZero2(2);
    double wmax=0.5/CTF1.Tm;
    double maxModule=0, minModule=1e38, min;
    double N=0;
    evaluation.maxDampingAtBorder=0;
    evaluation.firstZeroDisagreement=-1;
    evaluation.firstZeroAvg=0;
    for (double alpha=0; alpha<=PI; alpha+=PI/180, N++)
    {
    	VECTOR_R2(u,cos(alpha),sin(alpha));

    	// Get the zero in the direction of u
    	CTF1.zero(1, u, freqZero1);
    	double module=1.0/freqZero1.module();
    	maxModule=XMIPP_MAX(maxModule,module);
    	minModule=XMIPP_MIN(minModule,module);
    	evaluation.firstZeroAvg+=module;
    	double wx=wmax*XX(u);
    	double wy=wmax*YY(u);
    	CTF1.precomputeValues(wx,wy);
    	double damping=CTF1.CTFdamping_at();
    	damping=damping*damping;
    	evaluation.maxDampingAtBorder=XMIPP_MAX(evaluation.maxDampingAtBorder,damping);
    	if (fnCTF2!="") {
        	CTF2.zero(1, u, freqZero2);
        	double module2=1.0/freqZero2.module();
        	double diff=ABS(module-module2);
        	evaluation.firstZeroDisagreement=XMIPP_MAX(evaluation.firstZeroDisagreement,diff);
    	}
    }
    evaluation.firstZeroAvg/=N;
    evaluation.firstZeroRatio=maxModule/minModule;

    // Evaluate micrograph normality
	ImageGeneric M;
	M.readMapped(fnMicrograph);
	double avg, stddev, minval, maxval;
	M().computeStats(avg, stddev, minval, maxval);
	Histogram1D hist;
	compute_hist(M(), hist, minval, maxval, 400);
	hist += 1;
	hist /= hist.sum();

	Histogram1D histGaussian;
	histGaussian.initZeros(hist);
	histGaussian.hmin=hist.hmin;
	histGaussian.hmax=hist.hmax;
	histGaussian.step_size=hist.step_size;
	histGaussian.istep_size=hist.istep_size;
	FOR_ALL_ELEMENTS_IN_ARRAY1D(histGaussian) {
		double x;
		hist.index2val(i, x);
		A1D_ELEM(histGaussian,i) = gaussian1D(x, stddev, avg);
	}
	evaluation.histogramNormality=0.5*(KLDistance(hist,histGaussian)+
			                           KLDistance(histGaussian,hist));
}

/* Run --------------------------------------------------------------------- */
void ProgPSDSort::run()
{
    MetaData SF(fnSel);
    PSDEvaluation evaluation;
    init_progress_bar(SF.size());
    int idx=0;
    FOR_ALL_OBJECTS_IN_METADATA(SF)
    {
        FileName fnMicrograph, fnPSD, fnCTF, fnCTF2;
        SF.getValue(MDL_MICROGRAPH,fnMicrograph,__iter.objId);
        SF.getValue(MDL_PSD,fnPSD,__iter.objId);
        SF.getValue(MDL_CTFMODEL,fnCTF,__iter.objId);
        if (SF.containsLabel(MDL_CTFMODEL2))
        	SF.getValue(MDL_CTFMODEL2,fnCTF2,__iter.objId);
        evaluate(fnMicrograph, fnPSD, fnCTF, fnCTF2, evaluation);
        SF.setValue(MDL_CTF_CRITERION_DAMPING,evaluation.maxDampingAtBorder,__iter.objId);
        SF.setValue(MDL_CTF_CRITERION_FIRSTZEROAVG,evaluation.firstZeroAvg,__iter.objId);
        if (evaluation.firstZeroDisagreement>0)
        	SF.setValue(MDL_CTF_CRITERION_FIRSTZERODISAGREEMENT,evaluation.firstZeroDisagreement,__iter.objId);
        SF.setValue(MDL_CTF_CRITERION_FIRSTZERORATIO,evaluation.firstZeroRatio,__iter.objId);
        SF.setValue(MDL_CTF_CRITERION_FITTINGSCORE,evaluation.fittingScore,__iter.objId);
        SF.setValue(MDL_CTF_CRITERION_FITTINGCORR13,evaluation.fittingCorr13,__iter.objId);
        SF.setValue(MDL_CTF_CRITERION_PSDCORRELATION90,evaluation.PSDcorrelation90,__iter.objId);
        SF.setValue(MDL_CTF_CRITERION_PSDRADIALINTEGRAL,evaluation.PSDradialIntegral,__iter.objId);
        SF.setValue(MDL_CTF_CRITERION_PSDVARIANCE,evaluation.PSDVariance,__iter.objId);
        SF.setValue(MDL_CTF_CRITERION_PSDPCA1VARIANCE,evaluation.PSDPC1Variance,__iter.objId);
        SF.setValue(MDL_CTF_CRITERION_PSDPCARUNSTEST,evaluation.PSDPCRunsTest,__iter.objId);
        SF.setValue(MDL_CTF_CRITERION_NORMALITY, evaluation.histogramNormality,__iter.objId);
        progress_bar(++idx);
    }
    SF.write(fnSel);
}
