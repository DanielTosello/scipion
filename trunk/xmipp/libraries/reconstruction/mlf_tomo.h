/***************************************************************************
 *
 * Authors:    Sjors Scheres           scheres@cnb.uam.es (2004)
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
 *  e-mail address 'xmipp@cnb.uam.es'                                  
 ***************************************************************************/
#include <data/fft.h>
#include <data/args.h>
#include <data/funcs.h>
#include <data/selfile.h>
#include <data/docfile.h>
#include <data/image.h>
#include <data/filters.h>
#include <data/mask.h>
#include <data/geometry.h>
#include "symmetrize.h"
#include <vector>

typedef struct Wedgelist 
{
    int num; 
    double th0; 
    double thF;
} wedgelist;

#define SIGNIFICANT_WEIGHT_LOW 1e-4

/**@name mlf_tomo */
//@{
/** mlf_tomo parameters. */
class Prog_mlf_tomo_prm {

public:
  /** Filenames reference selfile/image, fraction docfile & output rootname */
  FileName fn_ref,fn_root,fn_frac,fn_sig,fn_doc,fn_misalign;
  FileName fn_wlist,fn_sym,fn_solv,fn_solv2, fn_mask, fn_wed;
  /** sigma-value for origin offsets */
  double sigma_offset;
  /** Vector containing estimated fraction for each model */
  vector<double> alpha_k;
  /** Flag whether to fix estimates for model fractions */
  bool fix_fractions;
  /** Flag whether to fix estimate for sigma of noise */
  bool fix_sigma_noise;
  /** Flag whether to fix estimate for sigma of origin offsets */
  bool fix_sigma_offset;
  /** Starting iteration */
  int istart;
  /** Number of iterations to be performed */
  int Niter;
  /** dimension of the images  */
  int dim;
  /** Number of steps to sample rot, tilt, psi and limited translations */
  int nr_rot, nr_rot_tilt, nr_tilt, nr_psi, nr_trans;
  /** Offsets for limited translations */
  vector<matrix1D<double> > Vtrans;
  /** Range and step size for angular searches */
  double rot0,  rotF,  rot_step;
  double tilt0, tiltF, tilt_step;
  double psi0,  psiF,  psi_step;
  /** Number of reference images */
  int nr_ref;
  /** Verbose level:
      2: gives debug statements
      1: gives progress bar (=default)
      0: gives no output to screen at all */
  int verb;
  // SelFile images (images, reference and sumwedges set)
  SelFile SF, SFr, SFw;
    // Vector for volumes to hold the real and imag parts of Fref and
    // the sum of all wedges for each reference
    vector<vector <matrix3D<double> > > Fref_trans;
  // For all tomograms: angles, offsets and wedge parameters
  vector<double> img_rot,img_tilt,img_psi,img_xoff,img_yoff,img_zoff,img_th0,img_thF,img_wednr;
  // Matrices for calculating PDF of (in-plane) translations
  matrix3D<double> pdf_trans, Mr2;
  /** Number for which limited translation is zero */
  int zero_trans;
  /** Vector for Fourier-space sigma2-values  */
  vector<double> Vsigma2;
  /** number of different wedges */
  int nr_wedge;
  matrix3D<double> smooth_edge_mask, corr_mask;
  matrix3D<int> outside_mask, mask, resmask, resmask2;
  /* wedgelist */
  vector<wedgelist> wedges;
  /* Symmetry information */
  SymList SL;
  /* store as double: dim x dim x dim */
  double dim3,dim3eff;
  /* for smoothing */
  double theta, theta_step, theta0;
  /* also for smoothing */
  int nr_img;
  /* pointers for the different wedges in Fourier Space */
  vector<vector<int> > pointer,pointer_mis,pointer_i,pointer_j,pointer_k,pointer_resol;
  /* Number of elements in pointers for the different ctf-matrices */
  vector<int> nr_pointer,nr_pointer_mis;
  /* High/low resolution limits */
  double highres,lowres;
  /* Integer high/low resolution limits*/
  int resol_max, resol_min;
  /* resolution pointer */
  matrix3D<int> Mresol;
  /* Maximum number of pixels to search offsets */
  double search_shift;
    /* Vector for nonzero pixels of each wedge */
    vector<vector<double> > nonzero_pixels;

  //// DEBUGGING
  bool debug;

public:

    /// Read arguments from command line
    void read(int argc, char **argv);
  
    /// Show
    void show();

    /// Usage
    void usage();

    /// Extended Usage
    void extended_usage();

    /// splitted SF-INdependent side-info calculations
    void produce_Side_info();

    /// splitted SF-dependent side-info calculations
    void produce_Side_info2();

    /// Update all reciprocal space pointers
    void update_pointers();

    /// initial sigma2-estimation for fourier-mode
    void estimate_initial_sigma2();

    /// Calculate prior probability density funtion of the translations
    void calculate_pdf_trans();

    /// parameters using fourier-space likelihood functions
    void MLF_integrate(matrix3D<double> Mimg, matrix2D<double> A_img, int iwedge,  
		       vector<matrix3D<double> > &wsum_Fimgs,
		       vector<matrix3D<double> > &wsum_Fweds,
		       vector<double> &wsum_sigma2, double &wsum_sigma_offset, 
		       vector<double> &sumw, double &LL, double &fracweight, 
		       int &opt_refno, double &opt_rot, double &opt_tilt, double &opt_psi,
		       double &opt_xoff, double &opt_yoff, double &opt_zoff);

    /// Integrate over all experimental images
    void sum_over_all_images(SelFile &SF,
			     vector<matrix3D<double> > &wsum_Fimgs,
			     vector<matrix3D<double> > &wsum_Fweds, 
			     vector<double> &sum_nonzero_pixels,
			     vector<double> &wsum_sigma2, double &wsum_sigma_offset, 
			     vector<double> &sumw, double &LL, 
			     double &sumcorr, DocFile &DFo); 

    /// Update all model parameters
    void update_parameters(vector<matrix3D<double> > &wsum_Fimgs,
			   vector<matrix3D<double> > &wsum_Fweds, 
			   vector<double> &sum_nonzero_pixels,
			   vector<double> &wsum_sigma2, double &wsum_sigma_offset, 
			   vector<double> &sumw, double &sumcorr, 
			   double &sumw_allrefs, int iter);

    // Post-processing on (real-space) reference maps
    void post_process_references(vector<matrix3D<double> > &Mref);

    // Solvent flattening
    void solvent_flattening(vector<matrix3D<double> > &Mref, FileName &fn_solvent);
    
    /// Write out reference images, selfile and logfile
    void write_output_files(const int iter, SelFile &SF, DocFile &DF,
			    vector<matrix3D<double> > &Mref,
			    double &sumw_allrefs, vector<double> &sumw, 
			    double &LL, double &avecorr);


};    
//@}
