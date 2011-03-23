/***************************************************************************
 *
 * Authors:    Sjors Scheres           scheres@cnb.csic.es (2004)
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

#ifndef _ML_REFINE3D_H
#define _ML_REFINE3D_H

#include "ml_align2d.h"
#include "mlf_align2d.h"
#include <data/sampling.h>
#include "recons.h"

//types of reconstructions to be used
#define RECONS_ART 0
#define RECONS_FOURIER 1



/**@defgroup Refine3d ml_refine3d (Maximum likelihood 3D refinement)
   @ingroup ReconsLibrary */
//@{
/** Refine3d parameters. */
class ProgMLRefine3D: public XmippProgram
{

public:
    // Filenames for input images, reference volumes, symmetry file and output rootname
    FileName fn_sel, fn_ref, fn_sym, fn_root, fn_solv, fn_iter, fn_symmask;
    // Metadata with reference volumes
    MetaData mdVol;
    // Number of volumes to refine
    int Nvols;
    // Iteration numbers
    int iter, istart, Niter;
    // Convergence check
    double eps;
    // Angular sampling interval (degree)
    double angular;
    // Type of reconstruction to use
    int recons_type;
    // Low-pass filter digital frequency
    double lowpass;
    // For user-provided tilt range
    double tilt_range0, tilt_rangeF;
    // Parameters of wlsart reconstruction
    double wlsart_lambda, wlsart_kappa;
    int wlsart_Niter;
    // Do not use a starting volume in wlsART reconstruction
    bool wlsart_no_start;
    // Threshold for flooding-like solvent mask
    double threshold_solvent;
    // Do Wang/Terwilliger-like probabilistic solvent flattening
    bool do_prob_solvent;
    // Do separate_object-like selection of largest connected volume in
    // binarized solvent mask
    bool do_deblob_solvent;
    // Dilate (binarized) solvent mask?
    int dilate_solvent;
    // Use MLF mode
    bool fourier_mode;
    // Flag to skip reconstruction
    bool skip_reconstruction;
    // Perturb angles of reference projections
    bool do_perturb;
    // sampling object
    Sampling mysampling;
    // Symmetry setup
    int symmetry, sym_order;
    // Number of reference projections per 3D model
    int nr_projections;

    //MPI related stuff
    int rank, size;

    // A pointer to the 2D alignment and classification program
    ML2DBaseProgram * ml2d;

private:
    ///Helper function to show to screen and file history
    void showToStream(std::ostream &out);

public:
    /// Empty constructor, call the constructor of ProgML2D
    /// with the ML3D flag set to true
    ProgMLRefine3D(bool fourier = false);
    /** Destructor */
    ~ProgMLRefine3D();
    /// Define the parameters accepted
    void defineParams();
    /// Read additional arguments for 3D-process from command line
    void readParams();
    /// Show
    void show();

    /// Create sampling for projecting volumes
    void createSampling();
    //Call produceSideInfo of ML2D and
    // Fill sampling and create DFlib
    virtual void produceSideInfo();
    virtual void produceSideInfo2();

    ///Provides implementation of the run function
    void run();

    /// Project the reference volumes in evenly sampled directions
    /// fill the metadata mdProj with the projections data
    void projectVolumes(MetaData &mdProj) ;

    /// (For mpi-version only:) calculate noise averages and write to disc
    void makeNoiseImages(std::vector<Image<double>  > &Iref) ;

    /// Create the program to be used for reconstruction of the volumes
    virtual ProgReconsBase * createReconsProgram();

    /// reconstruction by (weighted ART) or Fourier interpolation
    /// the metadata filename with volumes to reconstruct should be passed
    /// along with the base filename for reconstructed volumes
    void reconstructVolumes(const MetaData &mdProj, const FileName &outBase);

    /// Calculate 3D SSNR according to Unser ea. (2005)
    void calculate3DSSNR(MultidimArray<double> &spectral_signal);

    /** Copy reference volumes before start processing */
    virtual void copyVolumes();
    /** Update the metadata with reference volumes */
    void updateVolumesMetadata();



    /// Masking, filtering etc. of the volume
    virtual void postProcessVolumes();

    /// Convergency check
    bool checkConvergence() ;



    /**** DEPRECATED *****/
//    /// After reconstruction update reference volume selfile
//    void remakeSFvol(int iter, bool rewrite = false, bool include_noise = false);
//
//    /// Merge MLalign2D classification selfiles into volume classes
//    void concatenateSelfiles(int iter);
};
//@}
#endif
