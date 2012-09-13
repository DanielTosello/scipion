/***************************************************************************
 *
 * Authors:    Sjors Scheres           scheres@cnb.csic.es (2007)
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
#include "ml_tomo.h"

//#define DEBUG
// For blocking of threads
pthread_mutex_t mltomo_weightedsum_update_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mltomo_selfile_access_mutex = PTHREAD_MUTEX_INITIALIZER;

// Read arguments ==========================================================
void Prog_ml_tomo_prm::read(int argc, char **argv)
{
    // Generate new command line for restart procedure
    cline = "";
    int argc2 = 0;
    char ** argv2 = NULL;

    if (checkParameter(argc, argv, "-restart"))
    {
        REPORT_ERROR(1,"restart procedure temporarily de-activated.... sorry!");
        /*
           std::string comment;
           FileName fn_sel;
           DocFile DFi;
           DFi.read(getParameter(argc, argv, "-restart"));
           DFi.go_beginning();
           comment = (DFi.get_current_line()).get_text();
           if (strstr(comment.c_str(), "ml_tomo-logfile") == NULL)
           {
               std::cerr << "Error!! Docfile is not of ml_tomo-logfile type. " << std::endl;
               exit(1);
           }
           else
           {
               char *copy;
               int n = 0;
               int nmax = DFi.dataLineNo();
               SFr.reserve(nmax);
               copy = NULL;
               DFi.next();
               comment = " -frac " + DFi.name();
               fn_sel = DFi.name();
               fn_sel = fn_sel.without_extension() + "_restart.sel";
               comment += " -ref " + fn_sel;
               comment += (DFi.get_current_line()).get_text();
               DFi.next();
               cline = (DFi.get_current_line()).get_text();
               // Also read additional options upon restart
               for (int i=1; i <argc; i+=2)
               {
                   std::cerr<<"i= "<<i<<" argc= "<<argc<<" argv[i]"<<argv[i]<<std::endl;
                   if ((strcmp("-restart", argv[i]) != 0))
                   {
                       comment += " ";
                       comment += argv[i];
                       comment += " ";
                       comment += argv[i+1];
                       comment += " ";
                   }
               }
               comment = comment + cline;
               // regenerate command line
               generateCommandLine(comment, argc2, argv2, copy);
               // Read images names from restart file
               DFi.next();
               while (n < nmax)
               {
                   n++;
                   DFi.next();
                   if (DFi.get_current_line().Is_comment()) fn_sel = ((DFi.get_current_line()).get_text()).erase(0, 3);
                   SFr.insert(fn_sel, SelLine::ACTIVE);
                   DFi.adjust_to_data_line();
               }
               fn_sel = DFi.name();
               fn_sel = fn_sel.without_extension() + "_restart.sel";
               SFr.write(fn_sel);
               SFr.clear();
           }
           */
    }
    else
    {
        // no restart, just copy argc to argc2 and argv to argv2
        argc2 = argc;
        argv2 = argv;
        for (int i = 1; i < argc2; i++)
        {
            cline = cline + (std::string)argv2[i] + " ";
        }
    }

    // Read command line
    if (checkParameter(argc2, argv2, "-more_options"))
    {
        usage();
        extendedUsage();
    }
    nr_ref = textToInteger(getParameter(argc2, argv2, "-nref", "0"));
    fn_ref = getParameter(argc2, argv2, "-ref", "");
    fn_doc = getParameter(argc2, argv2, "-doc", "");
    fn_sel = getParameter(argc2, argv2, "-i");
    fn_root = getParameter(argc2, argv2, "-o", "mltomo");
    fn_sym = getParameter(argc2, argv2, "-sym", "c1");
    Niter = textToInteger(getParameter(argc2, argv2, "-iter", "25"));
    Niter2 = textToInteger(getParameter(argc2, argv2, "-impute_iter", "1"));
    istart = textToInteger(getParameter(argc2, argv2, "-istart", "1"));
    sigma_noise = textToFloat(getParameter(argc2, argv2, "-noise", "1"));
    sigma_offset = textToFloat(getParameter(argc2, argv2, "-offset", "3"));
    fn_frac = getParameter(argc2, argv2, "-frac", "");
    fix_fractions = checkParameter(argc2, argv2, "-fix_fractions");
    fix_sigma_offset = checkParameter(argc2, argv2, "-fix_sigma_offset");
    fix_sigma_noise = checkParameter(argc2, argv2, "-fix_sigma_noise");
    eps = textToFloat(getParameter(argc2, argv2, "-eps", "5e-5"));
    verb = textToInteger(getParameter(argc2, argv2, "-verb", "1"));
    no_SMALLANGLE = checkParameter(argc2, argv2, "-no_SMALLANGLE");
    do_keep_angles = checkParameter(argc2, argv2, "-keep_angles");
    do_perturb = checkParameter(argc2, argv2, "-perturb");
    pixel_size = textToFloat(getParameter(argc2, argv2, "-pixel_size", "1"));

    // Adjust power spectra
    do_adjust_spectra = checkParameter(argc2, argv2, "-adjust_spectra");

    // Low-pass filter
    do_filter = checkParameter(argc2, argv2, "-filter");
    do_ini_filter = checkParameter(argc2, argv2, "-ini_filter");

    // regularization
    reg0=textToFloat(getParameter(argc2, argv2, "-reg0", "0."));
    regF=textToFloat(getParameter(argc2, argv2, "-regF", "0."));
    reg_steps=textToInteger(getParameter(argc2, argv2, "-reg_steps", "5."));

    // ML (with/without) imputation, or maxCC
    do_ml = !checkParameter(argc2, argv2, "-maxCC");
    do_impute = !checkParameter(argc2, argv2, "-dont_impute");
    noimp_threshold = textToFloat(getParameter(argc2, argv2, "-noimp_threshold", "1."));

    // Angular sampling
    angular_sampling = textToFloat(getParameter(argc2, argv2, "-ang", "10"));
    psi_sampling = textToFloat(getParameter(argc2, argv2, "-psi_sampling", "-1"));
    tilt_range0 = textToFloat(getParameter(argc2, argv2, "-tilt0", "-91."));
    tilt_rangeF = textToFloat(getParameter(argc2, argv2, "-tiltF", "91."));
    ang_search = textToFloat(getParameter(argc2, argv2, "-ang_search", "-1."));
    do_limit_psirange = !checkParameter(argc2, argv2, "-dont_limit_psirange");
    limit_trans = textToFloat(getParameter(argc2, argv2, "-limit_trans", "-1."));

    // Skip rotation, only translate and classify
    dont_rotate = checkParameter(argc2, argv2, "-dont_rotate");
    // Skip rotation and translation, only classify
    dont_align = checkParameter(argc2, argv2, "-dont_align");
    // Skip rotation and translation and classification
    do_only_average = checkParameter(argc2, argv2, "-only_average");

    // For focussed classification (only in combination with dont_align)
    fn_mask = getParameter(argc2, argv2, "-mask", "");

    // Missing data structures
    fn_missing = getParameter(argc2, argv2, "-missing","");
    dim = textToInteger(getParameter(argc2, argv2, "-dim", "-1"));
    maxres = textToFloat(getParameter(argc2, argv2, "-maxres", "0.5"));

    // Hidden arguments
    trymindiff_factor = textToFloat(getParameter(argc2, argv2, "-trymindiff_factor", "0.9"));

    // Number of threads
    threads = textToInteger(getParameter(argc2, argv2, "-thr","1"));

}

// Show ====================================================================
void Prog_ml_tomo_prm::show()
{

    if (verb > 0)
    {
        // To screen
        std::cerr << " -----------------------------------------------------------------" << std::endl;
        std::cerr << " | Read more about this program in the following publication:    |" << std::endl;
        std::cerr << " |  Scheres ea.  (2009) Structure, 17, 1563-1572                 |" << std::endl;
        std::cerr << " |                                                               |" << std::endl;
        std::cerr << " |   *** Please cite it if this program is of use to you! ***    |" << std::endl;
        std::cerr << " -----------------------------------------------------------------" << std::endl;
        std::cerr << "--> Maximum-likelihood multi-reference refinement " << std::endl;
        std::cerr << "  Input images            : " << fn_sel << " (" << nr_exp_images << ")" << std::endl;
        if (fn_ref != "")
            std::cerr << "  Reference image(s)      : " << fn_ref << std::endl;
        else
            std::cerr << "  Number of references:   : " << nr_ref << std::endl;
        std::cerr << "  Output rootname         : " << fn_root << std::endl;
        if (!(dont_align || do_only_average || dont_rotate))
        {
            std::cerr << "  Angular sampling rate   : " << angular_sampling<< " degrees"<<std::endl;
            if (ang_search > 0.)
            {
                std::cerr << "  Local angular searches  : "<<ang_search<<" degrees"<<std::endl;
                if (!do_limit_psirange)
                    std::cerr << "                          : but with complete psi searches"<<std::endl;
            }
        }
        if (limit_trans >= 0.)
            std::cerr << "  Maximum allowed shifts  : "<<limit_trans<<" pixels"<<std::endl;
        std::cerr << "  Symmetry group          : " << fn_sym<<std::endl;
        std::cerr << "  Stopping criterium      : " << eps << std::endl;
        std::cerr << "  initial sigma noise     : " << sigma_noise << std::endl;
        std::cerr << "  initial sigma offset    : " << sigma_offset << std::endl;
        std::cerr << "  Maximum resolution      : " << maxres << " pix^-1"<< std::endl;
        std::cerr << "  Use images of size      : " << dim << std::endl;
        if (reg0 > 0.)
        {
            std::cerr << "  Regularization from     : "<<reg0<<" to "<<regF<<" in " <<reg_steps<<" steps"<<std::endl;
        }
        if (fn_missing!="")
        {
            std::cerr << "  Missing data info       : "<<fn_missing <<std::endl;
            if (do_impute && do_ml)
                std::cerr << "  Missing data treatment  : imputation "<<std::endl;
            else
                std::cerr << "  Missing data treatment  : conventional division "<<std::endl;
        }
        if (fn_frac != "")
            std::cerr << "  Initial model fractions : " << fn_frac << std::endl;

        if (dont_rotate)
        {
            std::cerr << "  -> Skip rotational searches, only translate and classify "<< std::endl;
        }
        if (dont_align)
        {
            std::cerr << "  -> Skip alignment, only classify "<< std::endl;
            if (do_mask)
                std::cerr << "  -> Classify within mask "<<fn_mask<< std::endl;
        }
        if (do_only_average)
        {
            std::cerr << "  -> Skip alignment and classification, only calculate weighted average "<<std::endl;
        }
        if (!do_ml)
        {
            std::cerr << "  -> Use constrained correlation coefficient instead of ML-imputation approach." << std::endl;
        }
        if (do_perturb)
        {
            std::cerr << "  -> Perturb angular sampling." << std::endl;
        }
        if (fix_fractions)
        {
            std::cerr << "  -> Do not update estimates of model fractions." << std::endl;
        }
        if (fix_sigma_offset)
        {
            std::cerr << "  -> Do not update sigma-estimate of origin offsets." << std::endl;
        }
        if (fix_sigma_noise)
        {
            std::cerr << "  -> Do not update sigma-estimate of noise." << std::endl;
        }
        if (do_adjust_spectra)
        {
            std::cerr << "  -> Adjust power spectra of the subtomograms in each tilt series"<<std::endl;
        }
        if (threads>1)
        {
            std::cerr << "  -> Using "<<threads<<" parallel threads"<<std::endl;
        }

        std::cerr << " -----------------------------------------------------------------" << std::endl;

    }

}

// Usage ===================================================================
void Prog_ml_tomo_prm::usage()
{
    //TODO!!
    std::cerr << "Usage:  ml_tomo [options] " << std::endl;
    std::cerr << "   -i <metadatafile>           : MetaData file with input images (and angles) \n";
    std::cerr << "   -nref <int>                 : Number of references to generate automatically (recommended)\n";
    std::cerr << "   OR -ref <metadatafile/image>      OR metadatafile with initial references/single reference image \n";
    std::cerr << " [ -o <rootname> ]             : Output rootname (default = \"mltomo\")\n";
    std::cerr << " [ -missing <metadatafile> ]   : MetaData file with missing data region definitions\n";
    std::cerr << " [ -ang <float=10> ]           : Angular sampling rate (in degrees)\n";
    std::cerr << " [ -ang_search <float> ]       : Angular search range around orientations from docfile \n";
    std::cerr << "                                    (by default, exhaustive searches are performed)\n";
    std::cerr << " [ -dont_limit_psirange ]      : Exhaustive psi searches when using -ang_search (only for c1 symmetry)\n";
    std::cerr << " [ -limit_trans <float=-1.> ]  : Maximum allowed shifts (negative value means no restriction)\n";
    std::cerr << " [ -reg0 <float=0.> ]          : Initial regularization parameters (in N/K^2) \n";
    std::cerr << " [ -regF <float=0.> ]          : Final regularization parameters (in N/K^2) \n";
    std::cerr << " [ -reg_steps <int=5> ]        : Number of iterations in which the regularization is changed from reg0 to regF\n";
    std::cerr << " [ -dont_rotate ]              : Keep orientations from docfile fixed, only translate and classify \n";
    std::cerr << " [ -dont_align ]               : Keep orientations and trabslations from docfile fixed, only classify \n";
    std::cerr << " [ -only_average ]             : Keep orientations and classes from docfile, only output weighted averages \n";

    std::cerr << " [ -sym <symgroup=c1> ]        : Symmetry group \n";
    std::cerr << " [ -iter <N=25> ]              : Number of iterations to perform \n";
    std::cerr << " [ -keep_angles ]              : Keep transformations from docfile (otherwise start from random)\n";
    std::cerr << " [ -perturb ]                  : Apply random perturbations to angular sampling in each iteration\n";
    std::cerr << " [ -dim <int> ]                : Use downscaled (in fourier space) images of this size \n";
    std::cerr << " [ -maxres <float=0.5> ]       : Maximum resolution (in pixel^-1) to use \n";
    std::cerr << " [ -thr <int=1> ]              : Number of shared-memory threads to use in parallel \n";
    std::cerr << " [ -more_options ]             : Show all possible input parameters \n";
}

// Extended usage ===================================================================
void Prog_ml_tomo_prm::extendedUsage()
{
    std::cerr << "Additional options: " << std::endl;
    std::cerr << " [ -impute_iter <int=1> ]      : Number of iterations for inner imputation loop \n";
    std::cerr << " [ -iter <int=100> ]           : Maximum number of iterations to perform \n";
    std::cerr << " [ -istart <int> ]             : number of initial iteration \n";
    std::cerr << " [ -noise <float=1> ]          : Expected standard deviation for pixel noise \n";
    std::cerr << " [ -offset <float=3> ]         : Expected standard deviation for origin offset [pix]\n";
    std::cerr << " [ -frac <docfile=\"\"> ]        : Docfile with expected model fractions (default: even distr.)\n";
    std::cerr << " [ -restart <logfile> ]        : restart a run with all parameters as in the logfile \n";
    std::cerr << " [ -fix_sigma_noise]           : Do not re-estimate the standard deviation in the pixel noise \n";
    std::cerr << " [ -fix_sigma_offset]          : Do not re-estimate the standard deviation in the origin offsets \n";
    std::cerr << " [ -fix_fractions]             : Do not re-estimate the model fractions \n";
    std::cerr << " [ -eps <float=5e-5> ]         : Stopping criterium \n";
    std::cerr << " [ -pixel_size <float=1> ]     : Pixel size (in Anstrom) for resolution in FSC plots \n";
    std::cerr << " [ -tilt0 <float=-91.> ]       : Limit tilt angle search from tilt0 to tiltF (in degrees) \n";
    std::cerr << " [ -tiltF <float=91.> ]        : Limit tilt angle search from tilt0 to tiltF (in degrees) \n";
    std::cerr << " [ -mask <maskfile> ]          : Mask particles; only valid in combination with -dont_align \n";
    std::cerr << " [ -maxCC ]                    : Use constrained cross-correlation and weighted averaging instead of ML \n";
    std::cerr << " [ -dont_impute ]              : Use weighted averaging, rather than imputation \n";
    std::cerr << " [ -noimp_threshold <float=1>] : Threshold to avoid division by zero for weighted averaging \n";
    std::cerr << " [ -adjust_sepctra ]           : Experimental: equalize power spectra of all tilt-series \n";
    std::cerr << std::endl;
    exit(1);
}

// Set up a lot of general stuff
// This side info is general, i.e. in parallel mode it is the same for
// all processors! (in contrast to produce_Side_info2)
void Prog_ml_tomo_prm::produceSideInfo()
{

    FileName                    fn_img, fn_tmp, fn_base, fn_tmp2;
    Image<double>              vol;
    MetaData                    MDtmp, MDpart;
    Matrix1D<double>          offsets(3), dum;
    MultidimArray<double>      Maux, Maux2;
    MultidimArray<std::complex<double> >  Faux;
    Matrix1D<int>               center(3);
    MultidimArray<int>          radial_count;
    double                      av, aux, Q0;
    int                         im, jm, xdim, ydim, zdim, ndim;

#ifdef  DEBUG

    std::cerr<<"Start produceSideInfo"<<std::endl;
#endif

    // For several random operations
    randomize_random_generator();

    // Read metadatafile with experimental images
    MDimg.read(fn_sel);
    MDimg.removeObjects(MDValueEqual(MDL_ENABLED, -1));
    nr_exp_images = SF.size();

    // Check whether MDimg contains orientations
    mdimg_contains_angles = (MDimg.valueExists(MDL_ANGLEROT) &&
    		MDimg.valueExists(MDL_ANGLETILT) &&
    		MDimg.valueExists(MDL_ANGLEPSI) &&
    		MDimg.valueExists(MDL_SHIFTX) &&
    		MDimg.valueExists(MDL_SHIFTY) &&
    		MDimg.valueExists(MDL_SHIFTZ));

    // Get original dimension
    ImgSize(MDimg, xdim, ydim, zdim, ndim);
    if (xdim != ydim || xdim != zdim)
        REPORT_ERROR(1,"ml_tomo ERROR%: only cubic volumes are allowed");
    oridim = xdim;

    // Downscaled dimension
    if (dim < 0)
        dim = oridim;
    if (dim > oridim)
        REPORT_ERROR(1,"Error: dim should be smaller than the size of the images");
    // Keep both dim and oridim even or uneven
    if (oridim%2 != dim%2)
        dim++;
    hdim = dim / 2;
    dim3 = dim * dim * dim;
    ddim3 = (double)dim3;
    scale_factor= (double)dim/(double)oridim;
    sigma_offset *= scale_factor;

    if (regF > reg0)
        REPORT_ERROR(1,"regF should be smaller than reg0!");
    reg_current = reg0;

    // Make real-space masks
    MultidimArray<int> int_mask(dim,dim,dim);
    int_mask.setXmippOrigin();
    real_mask.resize(dim, dim, dim);
    real_mask.setXmippOrigin();
    BinarySphericalMask(int_mask, hdim, INNER_MASK);
    FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(int_mask)
    {
        DIRECT_MULTIDIM_ELEM(real_mask,n) = (double)DIRECT_MULTIDIM_ELEM(int_mask,n);
    }
    real_omask.resize(real_mask);
    real_omask = 1. - real_mask;

    if (fn_mask=="")
    {
        do_mask = false;
    }
    else
    {
        if (!dont_align)
            REPORT_ERROR(1,"ERROR: option -mask is only valid in combination with -dont_align");
        Imask.read(fn_mask);
        if (Imask().computeMin() < 0. || Imask().computeMax() > 1.)
            REPORT_ERROR(1,"ERROR: mask should have values within the range [0,1]");
        Imask().setXmippOrigin();
        reScaleVolume(Imask(),true);
        // Remove any borders from the mask (to prevent problems rotating it later on)
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(int_mask)
        {
            DIRECT_MULTIDIM_ELEM(Imask(),n) *= (double)DIRECT_MULTIDIM_ELEM(int_mask,n);
        }
        nr_mask_pixels=Imask().sum();
    }

    // Set-up fourier-space mask
    MultidimArray<double> cosine_mask(dim,dim,dim);
    cosine_mask.setXmippOrigin();
    fourier_mask.resize(dim,dim,hdim+1);
    fourier_imask.resize(dim,dim,hdim+1);
    int r_maxres = XMIPP_MIN(hdim, FLOOR(maxres * dim));
    RaisedCosineMask(cosine_mask, r_maxres - 2, r_maxres, INNER_MASK);
    int yy, zz;
    int dimb = (dim - 1)/2;
    for ( int z=0, ii=0; z<dim; z++ )
    {
        if ( z > dimb )
            zz = z-dim;
        else
            zz = z;
        for ( int y=0; y<dim; y++ )
        {
            if ( y > dimb )
                yy = y-dim;
            else
                yy = y;
            for ( int xx=0; xx<hdim + 1; xx++, ii++ )
            {
                if (xx <= FINISHINGX(cosine_mask))
                {
                    DIRECT_MULTIDIM_ELEM(fourier_mask,ii) = VOL_ELEM(cosine_mask,xx,yy,zz);
                    if (VOL_ELEM(cosine_mask,xx,yy,zz) > 0.)
                        DIRECT_MULTIDIM_ELEM(fourier_imask,ii) = 1.;
                    else
                        DIRECT_MULTIDIM_ELEM(fourier_imask,ii) = 0.;
                }
            }
        }
    }
    // exclude origin voxel from fourier masks
    DIRECT_MULTIDIM_ELEM(fourier_mask,0) = 0.;
    DIRECT_MULTIDIM_ELEM(fourier_imask,0) = 0.;


    // Precalculate sampling
    do_sym=false;
    if (dont_align || do_only_average || dont_rotate)
    {
    	if (!mdimg_contains_angles)
    	    REPORT_ERROR(1,"Options -dont_align, -dont_rotate and -only_average require angle information in -i metadatafile");
        ang_search = -1.;
    }
    else
    {
        mysampling.SetSampling(angular_sampling);
        if (!mysampling.SL.isSymmetryGroup(fn_sym, symmetry, sym_order))
            REPORT_ERROR(3005, (std::string)"ml_refine3d::run Invalid symmetry" +  fn_sym);
        mysampling.SL.read_sym_file(fn_sym);
        // Check whether we are using symmetry
        if (mysampling.SL.SymsNo() > 0)
        {
            do_sym=true;
            if (verb > 0)
            {
                std::cerr<<"  --> Using symmetry version of the code: "<<std::endl;
                std::cerr<<"      [-psi, -tilt, -rot] is applied to the references, thereby "<<std::endl;
                std::cerr<<"      tilt and psi are sampled on an hexagonal lattice, " <<std::endl;
                std::cerr<<"      and rot is sampled linearly "<<std::endl;
                std::cerr<<"      Note that -dont_limit_psirange option is not allowed.... "<<std::endl;
            }
            if (!do_limit_psirange && ang_search > 0.)
                REPORT_ERROR(1,"ml_tomo: ERROR: exhaustive psi-angle search only allowed for C1 symmetry");
        }
        mysampling.fill_L_R_repository();
        // by default max_tilt= +91., min_tilt= -91.
        mysampling.Compute_sampling_points(false, // half sphere?
                                           tilt_rangeF,
                                           tilt_range0);
        mysampling.remove_redundant_points_exhaustive(symmetry,
                sym_order,
                false, // half sphere?
                0.75 * angular_sampling);
        if (psi_sampling < 0)
            psi_sampling = angular_sampling;
    }

    // Get number of references
    if (do_only_average)
    {
    	int refno;
    	nr_ref = 0;
    	FOR_ALL_OBJECTS_IN_METADATA(MDimg)
    	{
    		if (MDimg.getValue(MDL_REF, refno))
    		{
				nr_ref = XMIPP_MAX(refno, nr_ref);
    		}
    		else
    		{
    			nr_ref = 1;
    			break;
    		}
    	}
        fn_ref="tt";
        Niter=1;
        do_impute=false;
        do_ml=false;
    }
    else
    {
        if (fn_ref != "")
        {
            do_generate_refs = false;
            if (fn_ref.isMetaData())
            {
            	SFr.read(fn_ref);
            	nr_ref = SFr.size();
            }
            else
            {
                nr_ref = 1;
            }
        }
        else
            do_generate_refs = true;
    }

    // Read in docfile with information about the missing wedges
    nr_miss = 0;
    all_missing_info.clear();
    if (fn_missing=="")
    {
        do_missing = false;
    }
    else
    {
        do_missing = true;
        missing_info myinfo;
        MDmissing.read(fn_missing);
        nr_miss = 0;
        // Pre-calculate number of observed elements in this missing data structure
        FOR_ALL_OBJECTS_IN_METADATA(DFm)
        {
             double sum_observed_pixels=0.;
             MultidimArray<double> Mcomplete(dim,dim,dim);
             MultidimArray<double> Mmissing(dim,dim,hdim+1);
             Matrix2D<double> I(4,4);
             I.initIdentity();
             getMissingRegion(Mmissing, I, nr_miss);
             FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY3D(Mcomplete)
             {
                 if (j<XSIZE(Mmissing))
                     sum_observed_pixels +=  DIRECT_A3D_ELEM(Mmissing,k,i,j);
                 else
                     sum_observed_pixels +=  DIRECT_A3D_ELEM( Mmissing,(dim-k)%dim,(dim-i)%dim,dim-j);
             }
             miss_nr_pixels.push_back(sum_observed_pixels);
             nr_miss++;
        }
   }

    // Prepare power spectrum adjustment
    if (do_adjust_spectra)
    {
        if (!do_missing)
            REPORT_ERROR(1,"ERROR: -adjust_spectra requires -missing argument as well!");
        preparePowerSpectraAdjustment();
    }

}

// Adjust power spectra ====================================================
void Prog_ml_tomo_prm::preparePowerSpectraAdjustment()
{

    // If there is only one tilt series, then no power spectra adjustment is needed
    if (nr_miss == 1)
        return;

    if (!MDimg.valueExists(MDL_MISSING_REGION_NR))
        REPORT_ERROR(1,"ERROR: Image metadatafile should contain missingRegionNr for preparePowerSpectraAdjustment");

    // 1. Get the size of the smallest tilt series (group)
    Matrix1D<int > count_imgs(nr_miss);
    FileName fn_img;
    int smallest_group;
    count_imgs.initZeros();
    //FIXME: ----until here----
    SF.go_beginning();
    while (!SF.eof())
    {
        fn_img=SF.NextImg();
        if (fn_img=="")
            break;
        if (!DF.search_comment(fn_img))
        {
            std::cerr << "ERROR% "<<fn_img<<" not found in document file"<<std::endl;
            exit(0);
        }
        int missno = (int)(DF(7)) - 1;
        count_imgs(missno)++;
    }
    smallest_group=count_imgs.computeMin();
    std::cerr<<" Smallest group has "<<smallest_group<<" particles"<<std::endl;

    // 2. Get sums of aligned particles of random subsets of all groups with size of smallest group
    SelFile SFtmp;
    VolumeXmipp Itmp;
    std::vector< MultidimArray<double> > sumV, sumW;
    MultidimArray<double> V(dim,dim,dim), W(dim,dim,hdim+1), Mmissing(dim,dim,hdim+1);
    double my_rot, my_tilt, my_psi, my_xoff, my_yoff, my_zoff;
    Matrix1D<double> my_offsets(3);
    Matrix2D<double> my_A(4,4);
    int c = 0, cc = XMIPP_MAX(1, SF.ImgNo() / 60);
    count_imgs.initZeros();
    W.initZeros();
    V.initZeros();
    V.setXmippOrigin();
    for (int missno=0; missno < nr_miss; missno++)
    {
        sumV.push_back(V);
        sumW.push_back(W);
    }

    if (verb > 0)
    {
        std::cerr << "  Calculating average power spectra for power spectra adjustment"<<std::endl;
        init_progress_bar(SF.ImgNo());
    }

    SFtmp = SF.randomize();
    SFtmp.go_beginning();
    while (!SFtmp.eof())
    {
        fn_img=SFtmp.NextImg();
        if (fn_img=="")
            break;
        if (!DF.search_comment(fn_img))
        {
            std::cerr << "ERROR% "<<fn_img<<" not found in document file"<<std::endl;
            exit(0);
        }
        int missno = (int)(DF(7)) - 1;
        if (count_imgs(missno) < smallest_group)
        {
            count_imgs(missno)++;
            Itmp.read(fn_img);
            Itmp().setXmippOrigin();
            reScaleVolume(Itmp(),true);
            if (do_keep_angles || dont_align || dont_rotate || do_only_average)
            {
                // angles from docfile
                my_rot = DF(0);
                my_tilt = DF(1);
                my_psi = DF(2);
                my_offsets(0) = DF(3) * scale_factor;
                my_offsets(1) = DF(4) * scale_factor;
                my_offsets(2) = DF(5) * scale_factor;
                my_A = Euler_rotation3DMatrix(my_rot, my_tilt, my_psi);
                // TODO: Do this in one step!!
                Itmp().selfTranslate(my_offsets, DONT_WRAP);
                Itmp().selfApplyGeometry(my_A, IS_NOT_INV, DONT_WRAP);
            }
            else
            {
                my_A.initIdentity();
            }
            getMissingRegion(Mmissing, my_A, missno);
            sumV[missno] += Itmp();
            sumW[missno] += Mmissing;
        }
        c++;
        if (verb > 0 && c % cc == 0)
            progress_bar(c);
    }
    if (verb > 0)
        progress_bar(SFtmp.ImgNo());


    // 3. Calculate average amplitude spectra for all groups
    MultidimArray<std::complex<double> > Fimg;
    FileName                        fn_out;
    double                          aux, count;
    double                          *spectra_avg, *count_series, *count_avg;
    Matrix1D<double>                f(3);
    std::ofstream                   fh;
    int                             vsize=2*hdim+1;
    // Initialize spectra
    spectra_series = new double[nr_miss*vsize];
    spectra_avg = new double[vsize];
    count_series = new double[nr_miss*vsize];
    count_avg = new double[vsize];
    for (int i = 0; i < nr_miss*vsize; i++)
    {
        spectra_series[i] = 0.;
        count_series[i] = 0;
    }
    for (int i = 0; i < vsize; i++)
    {
        spectra_avg[i] = 0.;
        count_avg[i] = 0;
    }
    // Fill spectra
    for (int missno=0; missno < nr_miss; missno++)
    {
        transformer.FourierTransform(sumV[missno],Fimg,false);
        FOR_ALL_DIRECT_ELEMENTS_IN_MATRIX3D(Fimg)
        {
            count = dVkij(sumW[missno],k,i,j);
            if ( count > 0.)
            {
                FFT_IDX2DIGFREQ(j,dim,XX(f));
                FFT_IDX2DIGFREQ(i,dim,YY(f));
                FFT_IDX2DIGFREQ(k,dim,ZZ(f));
                double R = f.module();
                if (R>0.5)
                    continue;
                aux = abs( dVkij(Fimg,k,i,j) );
                int idx=ROUND(R*2*dim);
                spectra_series[missno*vsize + idx] += aux;
                spectra_avg[idx] += aux;
                count_series[missno*vsize + idx] += count;
                count_avg[idx] += count;
            }
        }
    }

    // Divide spectra_avg by all spectra_series for correction
    fn_out = fn_root+"_adjust_spectra.txt";
    fh.open((fn_out).c_str(), std::ios::out);
    if (!fh)
        REPORT_ERROR(1, (std::string)"Prog_ml_tomo_prm: Cannot write file: " + fn_out);

    fh<<"# dig. freq.   AVG  series 1-n... \n";
    for (int idx = 0; idx < vsize; idx++)
    {
        if (count_avg[idx]>0.)
            spectra_avg[idx] /= count_avg[idx];

        fh << (double)idx/(2*dim) <<" "<<1000.*spectra_avg[idx];
        for (int missno = 0; missno < nr_miss; missno++)
        {
            if (count_series[missno*vsize + idx] > 0.)
                spectra_series[missno*vsize + idx] /= count_series[missno*vsize + idx];
            fh <<" "<<1000.*spectra_series[missno*vsize + idx];

            if (spectra_series[missno*vsize + idx] > 0.)
                spectra_series[missno*vsize + idx] = spectra_avg[idx] / spectra_series[missno*vsize + idx];
        }
        fh<<"\n";
    }
    fh.close();

}

void Prog_ml_tomo_prm::applyPowerSpectraAdjustment(MultidimArray<std::complex<double> > &M, int missno)
{
    Matrix1D<double> f(3);
    int vsize=2*hdim+1;
    FOR_ALL_DIRECT_ELEMENTS_IN_MATRIX3D(M)
    {
        FFT_IDX2DIGFREQ(j,dim,XX(f));
        FFT_IDX2DIGFREQ(i,dim,YY(f));
        FFT_IDX2DIGFREQ(k,dim,ZZ(f));
        double R =f.module();
        if (R>0.5)
            continue;
        int idx=ROUND(R*2*dim);
        dVkij(M,k,i,j) *= spectra_series[missno*vsize + idx];
    }

}

// Generate initial references =============================================
void Prog_ml_tomo_prm::generateInitialReferences()
{

    SelFile SFtmp;
    VolumeXmipp Iave1, Iave2, Itmp, Iout;
    MultidimArray<double> Msumwedge1, Msumwedge2, Mmissing;
    MultidimArray<std::complex<double> > Fave;
    std::vector<Matrix1D<double> > fsc;
    Matrix2D<double> my_A;
    Matrix1D<double> my_offsets(3);
    FileName fn_tmp;
    SelLine line;
    DocFile DF;
    int Nsub = ROUND((double)SF.ImgNo() / nr_ref);
    double my_rot, my_tilt, my_psi, my_xoff, my_yoff, my_zoff, resolution;
    DocLine DL;
    int missno, c = 0, cc = XMIPP_MAX(1, SF.ImgNo() / 60);

#ifdef  DEBUG

    std::cerr<<"Start generateInitialReferences"<<std::endl;
#endif

    if (verb > 0)
    {
        std::cerr << "  Generating initial references by averaging over random subsets" << std::endl;
        init_progress_bar(SF.ImgNo());
    }

    if (fn_doc!="")
    {
        DF.read(fn_doc);
    }

    fsc.clear();
    fsc.resize(nr_ref+1);
    Iave1().resize(dim,dim,dim);
    Iave1().setXmippOrigin();
    Iave2().resize(dim,dim,dim);
    Iave2().setXmippOrigin();
    Msumwedge1.resize(dim,dim,hdim+1);
    Msumwedge2.resize(dim,dim,hdim+1);
    // Make random subsets and calculate average images in random orientations
    // and correct using conventional division by the sum of the wedges
    SFtmp = SF.randomize();
    for (int refno = 0; refno < nr_ref; refno++)
    {
        Iave1().initZeros();
        Iave2().initZeros();
        Msumwedge1.initZeros();
        Msumwedge2.initZeros();
        SFtmp.go_beginning();
        SFtmp.jump_lines(Nsub*refno);
        if (refno == nr_ref - 1)
            Nsub = SFtmp.ImgNo() - refno * Nsub;
        for (int nn = 0; nn < Nsub; nn++)
        {
            fn_tmp=SFtmp.NextImg();
            if (do_keep_angles || do_missing || dont_align || dont_rotate)
            {
                // Going through the docfile again here is a bit dirty coding...
                // Now do nothing, leave DF pointer at relevant position and read below
                if (!DF.search_comment(fn_tmp))
                {
                    std::cerr << "ERROR% "<<fn_tmp<<" not found in document file"<<std::endl;
                    exit(0);
                }
            }
            Itmp.read(fn_tmp);
            Itmp().setXmippOrigin();
            reScaleVolume(Itmp(),true);
            if (do_keep_angles || dont_align || dont_rotate)
            {
                // angles from docfile
                my_rot = DF(0);
                my_tilt = DF(1);
                my_psi = DF(2);
                my_offsets(0) = DF(3) * scale_factor;
                my_offsets(1) = DF(4) * scale_factor;
                my_offsets(2) = DF(5) * scale_factor;
                Itmp().selfTranslate(my_offsets, DONT_WRAP);
            }
            else
            {
                // reset to random angles
                my_rot = 360. * rnd_unif(0., 1.);
                my_tilt= ACOSD((2.*rnd_unif(0., 1.) - 1));
                my_psi = 360. * rnd_unif(0., 1.);
            }
            my_A = Euler_rotation3DMatrix(my_rot, my_tilt, my_psi);
            Itmp().selfApplyGeometry(my_A, IS_NOT_INV, DONT_WRAP);

            int iran_fsc = ROUND(rnd_unif());
            if (iran_fsc==0)
                Iave1() += Itmp();
            else
                Iave2() += Itmp();
            if (do_missing)
            {
                missno = (int)(DF(7)) - 1;
                getMissingRegion(Mmissing, my_A, missno);
                if (iran_fsc==0)
                    Msumwedge1 += Mmissing;
                else
                    Msumwedge2 += Mmissing;
            }
            c++;
            if (verb > 0 && c % cc == 0)
                progress_bar(c);
        }

        // Calculate resolution
        calculateFsc(Iave1(), Iave2(), Msumwedge1, Msumwedge2,
                     fsc[0], fsc[refno+1], resolution);

        Iave1() += Iave2();
        Msumwedge1 += Msumwedge2;

        if (do_missing)
        {
            // 1. Correct for missing wedge by division of sumwedge
            transformer.FourierTransform(Iave1(),Fave,true);
            FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Fave)
            {
                if (DIRECT_MULTIDIM_ELEM(Msumwedge1,n) > noimp_threshold)
                {
                    DIRECT_MULTIDIM_ELEM(Fave,n) /=
                        DIRECT_MULTIDIM_ELEM(Msumwedge1,n);
                }
                else
                {
                    DIRECT_MULTIDIM_ELEM(Fave,n) = 0.;
                }
            }
            transformer.inverseFourierTransform(Fave,Iave1());
        }
        else
        {
            Iave1() /= (double)Nsub;
        }

        // Enforce fourier_mask, symmetry and omask
        if (do_ini_filter)
            postProcessVolume(Iave1, resolution);
        else
            postProcessVolume(Iave1);

        fn_tmp = fn_root + "_it";
        fn_tmp.compose(fn_tmp, 0, "");
        fn_tmp = fn_tmp + "_ref";
        fn_tmp.compose(fn_tmp, refno + 1, "");
        fn_tmp = fn_tmp + ".vol";
        Iout=Iave1;
        reScaleVolume(Iout(),false);
        Iout.write(fn_tmp);
        SFr.insert(fn_tmp, SelLine::ACTIVE);
        // Also write out average wedge of this reference
        fn_tmp = fn_root + "_it";
        fn_tmp.compose(fn_tmp, 0, "");
        fn_tmp = fn_tmp + "_wedge";
        fn_tmp.compose(fn_tmp, refno + 1, "");
        fn_tmp = fn_tmp + ".vol";
        Iout()=Msumwedge1;
        reScaleVolume(Iout(),false);
        Iout.write(fn_tmp);
    }
    if (verb > 0)
        progress_bar(SF.ImgNo());
    fn_ref = fn_root + "_it";
    fn_ref.compose(fn_ref, 0, "sel");
    SFr.write(fn_ref);

#ifdef  DEBUG

    std::cerr<<"Finished generateInitialReferences"<<std::endl;
#endif
}

// Read reference images to memory and initialize offset vectors
// This side info is NOT general, i.e. in parallel mode it is NOT the
// same for all processors! (in contrast to produce_Side_info)
void Prog_ml_tomo_prm::produceSideInfo2(int nr_vols)
{

    int                       c, idum;
    DocFile                   DF, DFsub;
    DocLine                   DL;
    FileName                  fn_tmp;
    VolumeXmipp               img, Vaux;
    std::vector<Matrix1D<double> > Vdm;

#ifdef  DEBUG

    std::cerr<<"Start produceSideInfo2"<<std::endl;
#endif

    // Store tomogram angles, offset vectors and missing wedge parameters
    imgs_missno.clear();
    imgs_optrefno.clear();
    imgs_optangno.clear();
    imgs_trymindiff.clear();
    imgs_optoffsets.clear();
    imgs_optpsi.clear();

    for (int imgno = 0; imgno < SF.ImgNo(); imgno++)
    {
        Matrix1D<double> dum(3);
        imgs_optrefno.push_back(0);
        imgs_optangno.push_back(0);
        imgs_optpsi.push_back(0.);
        imgs_trymindiff.push_back(-1.);
        if (do_missing)
            imgs_missno.push_back(-1);
        if (dont_align || dont_rotate || do_only_average)
            imgs_optoffsets.push_back(dum);
    }

    // Read in docfile with wedge info, optimal angles, etc.
    if (fn_doc!="")
    {
        DF.read(fn_doc);
        SF.go_beginning();
        int imgno = 0;
        DFsub.clear();
        while (!SF.eof())
        {
            fn_tmp=SF.NextImg();
            if (fn_tmp=="")
                break;
            DF.go_beginning();
            if (DF.search_comment(fn_tmp))
            {
                // Get missing wedge type
                if (do_missing)
                {
                    imgs_missno[imgno] = (int)(DF(7)) - 1;
                }
                // Generate a docfile from the (possible subset of) images in SF
                if (ang_search > 0. || dont_align || do_only_average || dont_rotate)
                {
                    DFsub.append_comment(fn_tmp);
                    DL=DF.get_current_line();
                    if (do_sym)
                    {
                        imgs_optpsi[imgno]=DL[0]; // for limited psi (now rot) searches
                        // Reverse rotation applied to the references!!
                        double daux=DL[0];
                        DL[0]=-DL[2];
                        DL[1]=-DL[1];
                        DL[2]=-daux;
                    }
                    else
                        imgs_optpsi[imgno]=DL[2]; // for limited psi searches
                    DFsub.append_line(DL);
                    imgs_optangno[imgno]=DFsub.dataLineNo()-1;
                    int aux = (int)DL[6] - 1;
                    if (aux < 0 || aux >= nr_ref)
                        imgs_optrefno[imgno]=0;
                    else
                        imgs_optrefno[imgno]=aux;
                    if (dont_align || dont_rotate || do_only_average)
                    {
                        imgs_optoffsets[imgno](0)=DL[3];
                        imgs_optoffsets[imgno](1)=DL[4];
                        imgs_optoffsets[imgno](2)=DL[5];
                    }
                }
            }
            else
            {
                std::cerr << "ERROR% "<<fn_tmp
                <<" not found in document file"
                <<std::endl;
                exit(0);
            }
            imgno++;
        }

        // Set up local searches
        if (ang_search > 0.)
        {
            mysampling.SetNeighborhoodRadius(ang_search);
            mysampling.fill_exp_data_projection_direction_by_L_R(DFsub);
            mysampling.remove_points_far_away_from_experimental_data();
            mysampling.compute_neighbors();
        }
    }

    // Calculate angular sampling
    // A. from docfile entries (DFsub) only
    if (dont_align || dont_rotate || do_only_average)
    {
        angle_info myinfo;
        all_angle_info.clear();
        nr_ang = 0;
        DFsub.go_first_data_line();
        while (!DFsub.eof())
        {
            DL=DFsub.get_current_line();
            if (do_sym)
            {
                myinfo.rot = -DL[2];
                myinfo.tilt = -DL[1];
                myinfo.psi = -DL[0];
            }
            else
            {
                myinfo.rot = DL[0];
                myinfo.tilt = DL[1];
                myinfo.psi = DL[2];
            }
            myinfo.A = Euler_rotation3DMatrix(myinfo.rot, myinfo.tilt, myinfo.psi);
            myinfo.direction = nr_ang;
            all_angle_info.push_back(myinfo);
            nr_ang ++;
            DFsub.next_data_line();
        }
    }
    // B. from mysampling
    else
    {
        int nr_psi = CEIL(360. / psi_sampling);
        double rot, tilt, psi;
        angle_info myinfo;
        all_angle_info.clear();
        nr_ang = 0;
        for (int i = 0; i < mysampling.no_redundant_sampling_points_angles.size(); i++)
        {
            rot=XX(mysampling.no_redundant_sampling_points_angles[i]);
            tilt=YY(mysampling.no_redundant_sampling_points_angles[i]);
            for (int ipsi = 0; ipsi < nr_psi; ipsi++)
            {
                psi = (double)(ipsi * 360. / nr_psi);
                if (!no_SMALLANGLE)
                    psi += SMALLANGLE;
                if (do_sym)
                {
                    // Inverse rotation because A_rot is being applied to the reference
                    // and rot, tilt and psi apply to the experimental subtomogram!
                    myinfo.rot = -psi;
                    myinfo.tilt = -tilt;
                    myinfo.psi = -rot;
                }
                else
                {
                    // Only in C1 we get away with reverse order rotations...
                    // This is ugly, but allows -dont_limit_psirange....
                    myinfo.rot = rot;
                    myinfo.tilt = tilt;
                    myinfo.psi = psi;
                }
                myinfo.A = Euler_rotation3DMatrix(myinfo.rot, myinfo.tilt, myinfo.psi);
                myinfo.direction = i;
                all_angle_info.push_back(myinfo);
                nr_ang ++;
            }
        }
    }

    // Copy all rot, tilr, psi and A into _ori equivalents
    for (int angno = 0; angno < nr_ang; angno++)
    {
        all_angle_info[angno].rot_ori = all_angle_info[angno].rot;
        all_angle_info[angno].tilt_ori = all_angle_info[angno].tilt;
        all_angle_info[angno].psi_ori = all_angle_info[angno].psi;
        all_angle_info[angno].A_ori = all_angle_info[angno].A;
    }


#ifdef DEBUG_SAMPLING
    DocFile DFt;
    for (int angno = 0; angno < nr_ang; angno++)
    {
        double rot=all_angle_info[angno].rot;
        double tilt=all_angle_info[angno].tilt;
        double psi=all_angle_info[angno].psi;
        DFt.append_angles(rot,tilt,psi,"rot","tilt","psi");
    }
    FileName fnt;
    fnt = fn_root + "_angles.doc";
    DFt.write(fnt);
#endif

    // Prepare reference images
    if (do_only_average)
    {
        img().initZeros(dim,dim,dim);
        img().setXmippOrigin();
        for (int refno = 0; refno < nr_ref; refno++)
        {
            Iref.push_back(img);
            Iold.push_back(img);
        }
    }
    else
    {
        // Read in all reference images in memory
        if (Is_VolumeXmipp(fn_ref))
        {
            SFr.reserve(1);
            SFr.insert(fn_ref);
        }
        else
        {
            SFr.read(fn_ref);
        }
        nr_ref = 0;
        SFr.go_beginning();
        while ((!SFr.eof()))
        {
            FileName fn_img=SFr.NextImg();
            img.read(fn_img);
            img().setXmippOrigin();
            if (do_mask)
            {
                img() *= Imask();
            }
            reScaleVolume(img(),true);

            // From now on we will assume that all references are omasked, so enforce this here
            maskSphericalAverageOutside(img());

            // Rotate some arbitrary (off-axis) angle and rotate back again to remove high frequencies
            // that will be affected by the interpolation due to rotation
            // This makes that the A2 values of the rotated references are much less sensitive to rotation
            img().selfApplyGeometry( Euler_rotation3DMatrix(32., 61., 53.), IS_NOT_INV,
                                     DONT_WRAP, DIRECT_MULTIDIM_ELEM(img(),0) );
            img().selfApplyGeometry( Euler_rotation3DMatrix(32., 61., 53.), IS_INV,
                                     DONT_WRAP, DIRECT_MULTIDIM_ELEM(img(),0) );
            Iref.push_back(img);
            Iold.push_back(img);
            nr_ref++;
        }
    }


    // Prepare prior alpha_k
    alpha_k.resize(nr_ref);
    if (fn_frac != "")
    {
        // read in model fractions if given on command line
        double sumfrac = 0.;
        DocFile DF;
        DocLine DL;
        DF.read(fn_frac);
        DF.go_first_data_line();
        for (int refno = 0; refno < nr_ref; refno++)
        {
            DL = DF.get_current_line();
            alpha_k(refno) = DL[0];
            sumfrac += alpha_k(refno);
            DF.next_data_line();
        }
        if (ABS(sumfrac - 1.) > 1e-3)
            if (verb > 0)
                std::cerr << " ->WARNING: Sum of all expected model fractions (" << sumfrac << ") is not one!" << std::endl;
        for (int refno = 0; refno < nr_ref; refno++)
        {
            alpha_k(refno) /= sumfrac;
        }
    }
    else
    {
        // Even distribution
        alpha_k.initConstant(1./(double)nr_ref);
    }


    // Regularization (do not regularize during restarts!)
    if (istart == 1)
        regularize(istart-1);

    //#define DEBUG_GENERAL
#ifdef DEBUG_GENERAL

    std::cerr<<"nr images= "<<SF.ImgNo()<<std::endl;
    std::cerr<<"do_generate_refs ="<<do_generate_refs<<std::endl;
    std::cerr<<"nr_ref= "<<nr_ref<<std::endl;
    std::cerr<<"nr_miss= "<<nr_miss<<std::endl;
    std::cerr<<"dim= "<<dim<<std::endl;
    std::cerr<<"Finished produceSideInfo"<<std::endl;
    std::cerr<<"nr_ang= "<<nr_ang<<std::endl;
    std::cerr<<"nr_psi= "<<nr_psi<<std::endl;
#endif

#ifdef DEBUG

    std::cerr<<"Finished produceSideInfo2"<<std::endl;
#endif

}

void Prog_ml_tomo_prm::perturbAngularSampling()
{

    Matrix2D< double > R, I(3,3);
    double ran1, ran2, ran3;
    I.initIdentity();

    // Note that each mpi process will have a different random perturbation!!
    ran1 = rnd_gaus(0., angular_sampling/3.);
    ran2 = rnd_gaus(0., angular_sampling/3.);
    ran3 = rnd_gaus(0., angular_sampling/3.);
    R = Euler_rotation3DMatrix(ran1, ran2, ran3);
    R.resize(3,3);

    //#define DEBUG_PERTURB
#ifdef DEBUG_PERTURB

    std::cerr<<" random perturbation= "<<ran1<<" "<<ran2<<" "<<ran3<<std::endl;
#endif

    for (int angno = 0; angno < nr_ang; angno++)
    {
        Euler_apply_transf(I, R,
                           all_angle_info[angno].rot_ori,
                           all_angle_info[angno].tilt_ori,
                           all_angle_info[angno].psi_ori,
                           all_angle_info[angno].rot,
                           all_angle_info[angno].tilt,
                           all_angle_info[angno].psi);
        all_angle_info[angno].A = Euler_rotation3DMatrix(all_angle_info[angno].rot,
                                  all_angle_info[angno].tilt,
                                  all_angle_info[angno].psi);
    }

}

void Prog_ml_tomo_prm::getMissingRegion(MultidimArray<double> &Mmissing,
                                        Matrix2D<double> A,
                                        const int missno)
{

    if (missno < 0)
    {
        std::cerr<<" BUG: missno < 0"<<std::endl;
        exit(1);
    }

    Matrix2D<double> Ainv = A.inv();
    double xp, yp, zp;
    double tg0_y=0., tgF_y=0., tg0_x=0., tgF_x=0., tg=0., limx0=0., limxF=0., limy0=0., limyF=0., lim=0.;
    bool do_limit_x, do_limit_y, do_cone, is_observed;
    Mmissing.resize(dim,dim,hdim+1);

    MDmissing.gotoFirstObject(MDValueEqual(MDL_MISSING_REGION_NR, missno));
    if (all_missing_info[missno].type==MISSING_WEDGE_Y)
    {
        do_limit_x = true;
        do_limit_y = false;
        do_cone    = false;
        // TODO: Make this more logical!!
        tg0_y = -tan(PI * (-90. - all_missing_info[missno].thyF) / 180.);
        tgF_y = -tan(PI * (90. - all_missing_info[missno].thy0) / 180.);
    }
    else if (all_missing_info[missno].type==MISSING_WEDGE_X)
    {
        do_limit_x = false;
        do_limit_y = true;
        do_cone    = false;
        tg0_x = -tan(PI * (-90. - all_missing_info[missno].thxF) / 180.);
        tgF_x = -tan(PI * (90. - all_missing_info[missno].thx0) / 180.);
    }
    else if (all_missing_info[missno].type==MISSING_PYRAMID)
    {
        do_limit_x = true;
        do_limit_y = true;
        do_cone    = false;
        tg0_y = -tan(PI * (-90. - all_missing_info[missno].thyF) / 180.);
        tgF_y = -tan(PI * (90. - all_missing_info[missno].thy0) / 180.);
        tg0_x = -tan(PI * (-90. - all_missing_info[missno].thxF) / 180.);
        tgF_x = -tan(PI * (90. - all_missing_info[missno].thx0) / 180.);
    }
    else if (all_missing_info[missno].type==MISSING_CONE)
    {
        do_limit_x = false;
        do_limit_y = false;
        do_cone    = true;
        tg = -tan(PI * (-90. - all_missing_info[missno].thy0) / 180.);
    }
    else
    {
        REPORT_ERROR(1,"bug: unrecognized type of missing region");
    }

    //#define DEBUG_WEDGE
#ifdef DEBUG_WEDGE
    std::cerr<<"do_limit_x= "<<do_limit_x<<std::endl;
    std::cerr<<"do_limit_y= "<<do_limit_y<<std::endl;
    std::cerr<<"do_cone= "<<do_cone<<std::endl;
    std::cerr<<"tg0_y= "<<tg0_y<<std::endl;
    std::cerr<<"tgF_y= "<<tgF_y<<std::endl;
    std::cerr<<"tg0_x= "<<tg0_x<<std::endl;
    std::cerr<<"tgF_x= "<<tgF_x<<std::endl;
    std::cerr<<"XMIPP_EQUAL_ACCURACY= "<<XMIPP_EQUAL_ACCURACY<<std::endl;
    std::cerr<<"Ainv= "<<Ainv<<" A= "<<A<<std::endl;
#endif

    int zz, yy;
    int dimb = (dim - 1)/2;
    for ( int z=0, ii=0; z<dim; z++ )
    {
        if ( z > dimb )
            zz = z-dim;
        else
            zz = z;
        for ( int y=0; y<dim; y++ )
        {
            if ( y > dimb )
                yy = y-dim;
            else
                yy = y;
            for ( int xx=0; xx<hdim + 1; xx++, ii++ )
            {

                double maskvalue= DIRECT_MULTIDIM_ELEM(fourier_imask,ii);
                if (maskvalue < XMIPP_EQUAL_ACCURACY)
                {
                    DIRECT_MULTIDIM_ELEM(Mmissing,ii) = 0.;
                }
                else
                {
                    // Rotate the wedge
                    xp = dMij(Ainv, 0, 0) * xx + dMij(Ainv, 0, 1) * yy + dMij(Ainv, 0, 2) * zz;
                    yp = dMij(Ainv, 1, 0) * xx + dMij(Ainv, 1, 1) * yy + dMij(Ainv, 1, 2) * zz;
                    zp = dMij(Ainv, 2, 0) * xx + dMij(Ainv, 2, 1) * yy + dMij(Ainv, 2, 2) * zz;

                    // Calculate the limits
                    if (do_cone)
                    {
                        lim = (tg * zp) * (tg * zp);
                        if (xp*xp + yp*yp >= lim)
                            is_observed = true;
                        else
                            is_observed = false;
                    }
                    else
                    {
                        is_observed = false; // for pyramid
                        if (do_limit_x)
                        {
                            limx0 = tg0_y * zp;
                            limxF = tgF_y * zp;
                            if (zp >= 0)
                            {
                                if (xp <= limx0 || xp >= limxF)
                                    is_observed = true;
                                else
                                    is_observed = false;
                            }
                            else
                            {
                                if (xp <= limxF || xp >= limx0)
                                    is_observed = true;
                                else
                                    is_observed = false;
                            }
                        }
                        if (do_limit_y && !is_observed)
                        {
                            limy0 = tg0_x * zp;
                            limyF = tgF_x * zp;
                            if (zp >= 0)
                            {
                                if (yp <= limy0 || yp >= limyF)
                                    is_observed = true;
                                else
                                    is_observed = false;
                            }
                            else
                            {
                                if (yp <= limyF || yp >= limy0)
                                    is_observed = true;
                                else
                                    is_observed = false;
                            }
                        }
                    }

                    if (is_observed)
                        DIRECT_MULTIDIM_ELEM(Mmissing,ii) = maskvalue;
                    else
                        DIRECT_MULTIDIM_ELEM(Mmissing,ii) = 0.;
                }
            }
        }
    }

#ifdef DEBUG_WEDGE
    VolumeXmipp test(dim,dim,dim), ori;
    ori()=Mmissing;
    ori.write("oriwedge.fft");
    test().initZeros();
    FOR_ALL_DIRECT_ELEMENTS_IN_MATRIX3D(test())
    if (j<hdim+1)
        DIRECT_VOL_ELEM(test(),k,i,j)=
            DIRECT_VOL_ELEM(ori(),k,i,j);
    else
        DIRECT_VOL_ELEM(test(),k,i,j)=
            DIRECT_VOL_ELEM(ori(),
                            (dim-k)%dim,
                            (dim-i)%dim,
                            dim-j);
    test.write("wedge.ftt");
    CenterFFT(test(),true);
    test.write("Fwedge.vol");
    test().setXmippOrigin();
    MultidimArray<int> ress(dim,dim,dim);
    ress.setXmippOrigin();
    BinaryWedgeMask(test(),all_missing_info[missno].thy0, all_missing_info[missno].thyF, A);
    test.write("Mwedge.vol");
#endif

}

// Calculate probability density function of all in-plane transformations phi
void Prog_ml_tomo_prm::calculatePdfTranslations()
{

#ifdef  DEBUG
    std::cerr<<"start calculatePdfTranslations"<<std::endl;
#endif

    double r2, pdfpix;
    P_phi.resize(dim, dim, dim);
    P_phi.setXmippOrigin();
    Mr2.resize(dim, dim, dim);
    Mr2.setXmippOrigin();

    FOR_ALL_ELEMENTS_IN_MATRIX3D(P_phi)
    {
        r2 = (double)(j * j + i * i + k * k);

        if (limit_trans >= 0. && r2 > limit_trans * limit_trans)
            VOL_ELEM(P_phi, k, i, j) = 0.;
        else
        {
            if (sigma_offset > XMIPP_EQUAL_ACCURACY)
            {
                pdfpix = exp(- r2 / (2. * sigma_offset * sigma_offset));
                pdfpix *= pow(2. * PI * sigma_offset * sigma_offset, -3./2.);
            }
            else
            {
                if (k== 0 && i == 0 && j == 0)
                    pdfpix = 1.;
                else
                    pdfpix = 0.;
            }
            VOL_ELEM(P_phi, k, i, j) = pdfpix;
            VOL_ELEM(Mr2, k, i, j) = (float)r2;
        }
    }

    // Re-normalize for limit_trans
    if (limit_trans >= 0.)
    {
        double sum = P_phi.sum();
        P_phi /= sum;
    }

    //#define  DEBUG_PDF_SHIFT
#ifdef DEBUG_PDF_SHIFT
    std::cerr<<" Sum of translation pdfs (should be one) = "<<P_phi.sum()<<std::endl;
    VolumeXmipp Vt;
    Vt()=P_phi;
    Vt.write("pdf.vol");
#endif

#ifdef  DEBUG

    std::cerr<<"finished calculatePdfTranslations"<<std::endl;
#endif

}

void Prog_ml_tomo_prm::maskSphericalAverageOutside(MultidimArray<double> &Min)
{
    double outside_density = 0., sumdd = 0.;
    FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(real_omask)
    {
        outside_density += DIRECT_MULTIDIM_ELEM(Min,n)*DIRECT_MULTIDIM_ELEM(real_omask,n);
        sumdd += DIRECT_MULTIDIM_ELEM(real_omask,n);
    }
    outside_density /= sumdd;

    FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(real_omask)
    {
        DIRECT_MULTIDIM_ELEM(Min,n) *= DIRECT_MULTIDIM_ELEM(real_mask,n);
        DIRECT_MULTIDIM_ELEM(Min,n) += (outside_density)*DIRECT_MULTIDIM_ELEM(real_omask,n);
    }


}


void Prog_ml_tomo_prm::reScaleVolume(MultidimArray<double> &Min, bool down_scale)
{
    MultidimArray<std::complex<double> > Fin;
    MultidimArray<double> Mout;
    XmippFftw local_transformer_in, local_transformer_out;
    int newdim;

    if (oridim==dim)
        return;
    else
    {
        if (down_scale)
            newdim=dim;
        else
            newdim=oridim;

        local_transformer_in.setReal(Min);
        local_transformer_in.FourierTransform();
        local_transformer_in.getCompleteFourier(Fin);
        Mout.resize(newdim,newdim,newdim);
        Mout.setXmippOrigin();
        local_transformer_out.setReal(Mout);

        CenterFFT(Fin,true);
        Fin.setXmippOrigin();
        Fin.window(STARTINGZ(Mout),STARTINGY(Mout),STARTINGX(Mout),
                   FINISHINGZ(Mout),FINISHINGY(Mout), FINISHINGX(Mout),0.);
        CenterFFT(Fin,false);
        local_transformer_out.setFromCompleteFourier(Fin);
        local_transformer_out.inverseFourierTransform();

        //#define DEBUG_RESCALE_VOLUME
#ifdef DEBUG_RESCALE_VOLUME

        VolumeXmipp Vt;
        Vt()=Min;
        Vt.write("Min.vol");
        Vt()=Mout;
        Vt.write("Mout.vol");
#endif

        Min = Mout;
    }

}

void Prog_ml_tomo_prm::postProcessVolume(VolumeXmipp &Vin, double resolution)
{

    MultidimArray<std::complex<double> > Faux;
#ifdef DEBUG

    std::cerr<<"start postProcessVolume"<<std::endl;
#endif

    // Fourier transform
    transformer.FourierTransform(Vin(),Faux,true);

    // Apply fourier mask
    FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Faux)
    {
        DIRECT_MULTIDIM_ELEM(Faux,n) *= DIRECT_MULTIDIM_ELEM(fourier_mask,n);
    }

    // Low-pass filter for given resolution
    if (resolution > 0.)
    {
        Matrix1D<double> w(3);
        double raised_w=0.02;
        double w1=resolution;
        double w2=w1+raised_w;

        FOR_ALL_DIRECT_ELEMENTS_IN_MATRIX3D(Faux)
        {
            FFT_IDX2DIGFREQ(k,ZSIZE(Vin()),ZZ(w));
            FFT_IDX2DIGFREQ(i,YSIZE(Vin()),YY(w));
            FFT_IDX2DIGFREQ(j,XSIZE(Vin()),XX(w));
            double absw=w.module();
            if (absw>w2)
                DIRECT_VOL_ELEM(Faux,k,i,j) *= 0.;
            else if (absw>w1 && absw<w2)
                DIRECT_VOL_ELEM(Faux,k,i,j) *= (1+cos(PI/raised_w*(absw-w1)))/2;
        }
    }

    // Inverse Fourier Transform
    transformer.inverseFourierTransform(Faux,Vin());

    // Spherical mask with average outside density
    maskSphericalAverageOutside(Vin());

    // Apply symmetry
    if (mysampling.SL.SymsNo() > 0)
    {
        // Note that for no-imputation this is not correct!
        // One would have to symmetrize the missing wedges and the sum of the images separately
        if (do_missing && !do_impute)
            std::cerr<<" WARNING: Symmetrization and dont_impute together are not implemented correctly...\n Proceed at your own risk"<<std::endl;

        VolumeXmipp Vaux, Vsym=Vin;
        Matrix2D<double> L(4, 4), R(4, 4);
        Matrix1D<double> sh(3);
        Vaux().initZeros(dim,dim,dim);
        Vaux().setXmippOrigin();
        for (int isym = 0; isym < mysampling.SL.SymsNo(); isym++)
        {
            mysampling.SL.get_matrices(isym, L, R);
            mysampling.SL.get_shift(isym, sh);
            R(3, 0) = sh(0) * dim;
            R(3, 1) = sh(1) * dim;
            R(3, 2) = sh(2) * dim;
            applyGeometry(Vaux(), R.transpose(), Vin(), IS_NOT_INV,
                          DONT_WRAP, DIRECT_MULTIDIM_ELEM(Vin(),0));
            Vsym() += Vaux();
        }
        Vsym()/=mysampling.SL.SymsNo()+1.;
        Vin = Vsym;
    }


#ifdef DEBUG
    std::cerr<<"finished postProcessVolume"<<std::endl;
#endif

}


// Calculate FT of each reference and calculate A2 =============
void Prog_ml_tomo_prm::precalculateA2(std::vector< VolumeXmippT<double> > &Iref)
{

#ifdef DEBUG
    std::cerr<<"start precalculateA2"<<std::endl;
    TimeStamp t0;
    time_config();
    annotate_time(&t0);
#endif

    double rot, tilt, psi, AA, stdAA, corr;
    Matrix2D<double>  A_rot_inv(4,4), I(4,4);
    MultidimArray<double> Maux(dim,dim,dim), Mmissing;
    MultidimArray<std::complex<double> > Faux, Faux2;

    A2.clear();
    corrA2.clear();
    I.initIdentity();
    Maux.setXmippOrigin();
    for (int refno = 0; refno < nr_ref; refno++)
    {
        // Calculate A2 for all different orientations
        for (int angno = 0; angno < nr_ang; angno++)
        {
            A_rot_inv = ((all_angle_info[angno]).A).inv();
            // use DONT_WRAP and put density of first element outside
            // i.e. assume volume has been processed with omask
            applyGeometry(Maux, A_rot_inv, Iref[refno](), IS_NOT_INV,
                          DONT_WRAP, DIRECT_MULTIDIM_ELEM(Iref[refno](),0));
            //#define DEBUG_PRECALC_A2_ROTATE
#ifdef DEBUG_PRECALC_A2_ROTATE

            VolumeXmipp Vt;
            Vt()=Maux;
            Vt.write("rot.vol");
            Vt()=Iref[refno]();
            Vt.write("ref.vol");
            std::cerr<<" angles= "<<(all_angle_info[angno]).rot<<" "<<(all_angle_info[angno]).tilt<<" "<<(all_angle_info[angno]).psi<<std::endl;
            std::cerr<<" Written volume rot.vol and ref.vol, press any key to continue ..."<<std::endl;
            char c;
            std::cin >> c;
#endif

            AA = Maux.sum2();
            if (angno==0)
                stdAA = AA;
            if (AA > 0)
                corr = sqrt(stdAA / AA);
            else
                corr = 1.;
            corrA2.push_back(corr);
            Maux *= corr;
            if (do_missing)
            {
                transformer.FourierTransform(Maux,Faux,false);
                // Save original copy of Faux in Faux2
                Faux2.resize(Faux);
                FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Faux)
                {
                    DIRECT_MULTIDIM_ELEM(Faux2,n) = DIRECT_MULTIDIM_ELEM(Faux,n);
                }
                for (int missno = 0; missno < nr_miss; missno++)
                {
                    getMissingRegion(Mmissing,I,missno);
                    FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Faux)
                    {
                        DIRECT_MULTIDIM_ELEM(Faux,n) = DIRECT_MULTIDIM_ELEM(Faux2,n) * DIRECT_MULTIDIM_ELEM(Mmissing,n);
                    }
                    transformer.inverseFourierTransform();
                    A2.push_back(Maux.sum2());
                    //#define DEBUG_PRECALC_A2
#ifdef DEBUG_PRECALC_A2

                    std::cerr<<"rot= "<<all_angle_info[angno].rot<<" tilt= "<<all_angle_info[angno].tilt<<" psi= "<<all_angle_info[angno].psi<<std::endl;
                    std::cerr<<"refno= "<<refno<<" angno= "<<angno<<" missno= "<<missno<<" A2= "<<Maux.sum2()<<" corrA2= "<<corr<<std::endl;
                    //#define DEBUG_PRECALC_A2_b
#ifdef DEBUG_PRECALC_A2_b

                    VolumeXmipp tt;
                    tt()=Maux;
                    tt.write("refrotwedge.vol");
                    std::cerr<<"press any key"<<std::endl;
                    char c;
                    std::cin >> c;
#endif
#endif

                }
            }
            else
            {
                A2.push_back(Maux.sum2());
#ifdef DEBUG_PRECALC_A2

                std::cerr<<"refno= "<<refno<<" angno= "<<angno<<" A2= "<<Maux.sum2()<<std::endl;
#endif

            }
        }
    }

#ifdef DEBUG
    std::cerr<<"finished precalculateA2"<<std::endl;
    print_elapsed_time(t0);
#endif
}

// Maximum Likelihood calculation for one image ============================================
// Integration over all translation, given  model and in-plane rotation
void Prog_ml_tomo_prm::expectationSingleImage(
    MultidimArray<double> &Mimg, int imgno, int missno, double old_psi,
    std::vector< VolumeXmippT<double> > &Iref,
    std::vector<MultidimArray<double> > &wsumimgs,
    std::vector<MultidimArray<double> > &wsumweds,
    double &wsum_sigma_noise, double &wsum_sigma_offset,
    Matrix1D<double> &sumw, double &LL, double &dLL, double &fracweight, double &sumfracweight,
    double &trymindiff, int &opt_refno, int &opt_angno, Matrix1D<double> &opt_offsets)
{

#ifdef DEBUG
    std::cerr<<"start expectationSingleImage"<<std::endl;
    TimeStamp t0, t1;
    time_config();
    annotate_time(&t0);
    annotate_time(&t1);
#endif

    MultidimArray<double> Maux, Maux2, Mweight, Mmissing, Mzero(dim,dim,hdim+1), Mzero2(dim,dim,dim);
    MultidimArray<std::complex<double> > Faux, Faux2(dim,dim,hdim+1), Fimg, Fimg0, Fimg_rot;
    std::vector<MultidimArray<double> > mysumimgs;
    std::vector<MultidimArray<double> > mysumweds;
    Matrix1D<double> refw;
    double sigma_noise2, aux, pdf, fracpdf, myA2, mycorrAA, myXi2, A2_plus_Xi2, myXA;
    double mind, diff, mindiff, my_mindiff;
    double my_sumweight, weight;
    double wsum_sc, wsum_sc2, wsum_offset;
    double wsum_corr, sum_refw, maxweight, my_maxweight;
    double rot, tilt, psi;
    int irot, irefmir, sigdim, xmax, ymax;
    int ioptpsi = 0, ioptlib = 0, ioptx = 0, iopty = 0, ioptz = 0, imax = 0;
    bool is_ok_trymindiff = false;
    int my_nr_ang, old_optangno = opt_angno, old_optrefno = opt_refno;
    std::vector<double> all_Xi2;
    Matrix2D<double> A_rot(4,4), I(4,4), A_rot_inv(4,4);
    bool is_a_neighbor, is_within_psirange=true;
    XmippFftw local_transformer;

    if (dont_align || dont_rotate)
        my_nr_ang=1;
    else
        my_nr_ang=nr_ang;

    // Only translations smaller than 6 sigma_offset are considered!
    // TODO: perhaps 3 sigma??
    I.initIdentity();
    sigdim = 2 * CEIL(sigma_offset * 6);
    sigdim++; // (to get uneven number)
    sigdim = XMIPP_MIN(dim, sigdim);
    // Setup matrices and constants
    Maux.resize(dim, dim, dim);
    Maux2.resize(dim, dim, dim);
    Maux.setXmippOrigin();
    Maux2.setXmippOrigin();
    if (dont_align)
        Mweight.initZeros(1,1,1);
    else
        Mweight.initZeros(sigdim, sigdim, sigdim);
    Mweight.setXmippOrigin();
    Mzero.initZeros();
    Mzero2.initZeros();
    Mzero2.setXmippOrigin();

    sigma_noise2 = sigma_noise * sigma_noise;

    Maux=Mimg;
    // Apply inverse rotation to the mask and apply
    if (do_mask)
    {
        if (!dont_align)
            REPORT_ERROR(1,"BUG: !dont_align and do_mask cannot coincide at this stage...");
        MultidimArray<double> Mmask;
        A_rot = (all_angle_info[opt_angno]).A;
        A_rot_inv = A_rot.inv();
        applyGeometry(Mmask, A_rot_inv, Imask(), IS_NOT_INV, DONT_WRAP, 0.);
        Maux *= Mmask;
    }
    // Calculate the unrotated Fourier transform with enforced wedge of Mimg (store in Fimg0)
    // also calculate myXi2;
    // Note that from here on local_transformer will act on Maux <-> Faux
    local_transformer.FourierTransform(Maux, Faux, false);
    if (do_missing)
    {
        if (do_adjust_spectra)
            applyPowerSpectraAdjustment(Faux,missno);

        // Enforce missing wedge
        getMissingRegion(Mmissing,I,missno);
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Faux)
        {
            DIRECT_MULTIDIM_ELEM(Faux,n) *= DIRECT_MULTIDIM_ELEM(Mmissing,n);
        }
    }
    Fimg0 = Faux;
    // BE CAREFUL: inverseFourierTransform messes up Faux!
    local_transformer.inverseFourierTransform();
    myXi2 = Maux.sum2();

    // To avoid numerical problems, subtract smallest difference from all differences.
    // That way: Pmax will be one and all other probabilities will be [0,1>
    // But to find mindiff I first have to loop over all hidden variables...
    // Fortunately, there is some flexibility as the reliable domain of the exp-functions goes from -700 to 700.
    // Therefore, make a first guess for mindiff (trymindiff) and check whether the difference
    // with the real mindiff is not larger than 500 (to be on the save side).
    // If that is the case: OK; if not: do the entire loop again.
    // The efficiency of this will depend on trymindiff.
    // First cycle use trymindiff = trymindiff_factor * 0.5 * Xi2
    // From then on: use mindiff from the previous iteration
    if (trymindiff < 0.)
        // 90% of Xi2 may be a good idea (factor half because 0.5*diff is calculated)
        trymindiff = trymindiff_factor * 0.5 * myXi2;
    int redo_counter = 0;
    while (!is_ok_trymindiff)
    {
        // Initialize mindiff, weighted sums and maxweights
        mindiff = 99.e99;
        wsum_corr = wsum_offset = wsum_sc = wsum_sc2 = 0.;
        maxweight = sum_refw = 0.;
        mysumimgs.clear();
        mysumweds.clear();
        for (int refno = 0; refno < nr_ref; refno++)
        {
            mysumimgs.push_back(Mzero2);
            if (do_missing)
                mysumweds.push_back(Mzero);
        }
        refw.initZeros(nr_ref);
        // The real stuff: now loop over all orientations, references and translations
        // Start the loop over all refno at old_optangno (=opt_angno from the previous iteration).
        // This will speed-up things because we will find Pmax probably right away,
        // and this will make the if-statement that checks SIGNIFICANT_WEIGHT_LOW
        // effective right from the start
        for (int aa = old_optangno; aa < old_optangno+my_nr_ang; aa++)
        {
            int angno = aa;
            if (angno >= nr_ang)
                angno -= nr_ang;

            // See whether this image is in the neighborhoood for this imgno
            if (ang_search > 0.)
            {
                is_a_neighbor = false;
                if (do_limit_psirange)
                {

                    // Only restrict psi range for directions away from the poles...
                    // Otherwise the is_a_neighbour construction may give errors:
                    // (-150, 0, 150) and (-100, 0, 150) would be considered as equal, since
                    // the distance between (-150,0) and (-100,0) is zero AND
                    // the distance between 150 and 150 is also zero...
                    if (ABS(realWRAP(all_angle_info[angno].tilt, -90., 90.)) < 1.1 * ang_search)
                        is_within_psirange=true;
                    else if ( (do_sym &&
                               ABS(realWRAP(old_psi - (all_angle_info[angno]).rot,-180.,180.)) <= ang_search)
                              || (!do_sym &&
                                  ABS(realWRAP(old_psi - (all_angle_info[angno]).psi,-180.,180.)) <= ang_search) )
                        is_within_psirange=true;
                    else
                        is_within_psirange=false;
                }

                if (!do_limit_psirange || is_within_psirange)
                {
                    for (int i = 0; i < mysampling.my_neighbors[imgno].size(); i++)
                    {
                        if (mysampling.my_neighbors[imgno][i] == (all_angle_info[angno]).direction)
                        {
                            is_a_neighbor = true;
                            break;
                        }
                    }
                }
            }
            else
            {
                is_a_neighbor = true;
            }

            // If it is in the neighborhood: proceed
            if (is_a_neighbor)
            {
                A_rot = (all_angle_info[angno]).A;
                A_rot_inv = A_rot.inv();

                // Start the loop over all refno at old_optrefno (=opt_refno from the previous iteration).
                // This will speed-up things because we will find Pmax probably right away,
                // and this will make the if-statement that checks SIGNIFICANT_WEIGHT_LOW
                // effective right from the start
                for (int rr = old_optrefno; rr < old_optrefno+nr_ref; rr++)
                {
                    int refno = rr;
                    if (refno >= nr_ref)
                        refno-= nr_ref;

                    fracpdf = alpha_k(refno)*(1./nr_ang);
                    // Now (inverse) rotate the reference and calculate its Fourier transform
                    // Use DONT_WRAP and assume map has been omasked
                    applyGeometry(Maux2, A_rot_inv, Iref[refno](), IS_NOT_INV,
                                  DONT_WRAP, DIRECT_MULTIDIM_ELEM(Iref[refno](),0));
                    mycorrAA = corrA2[refno*nr_ang + angno];
                    Maux = Maux2 * mycorrAA;
                    local_transformer.FourierTransform();
                    if (do_missing)
                        myA2 = A2[refno*nr_ang*nr_miss + angno*nr_miss + missno];
                    else
                        myA2 = A2[refno*nr_ang + angno];
                    A2_plus_Xi2 = 0.5 * ( myA2 + myXi2 );
                    // A. Backward FFT to calculate weights in real-space
                    FOR_ALL_DIRECT_ELEMENTS_IN_MATRIX3D(Faux)
                    {
                        dVkij(Faux,k,i,j) =
                            dVkij(Fimg0,k,i,j) *
                            conj(dVkij(Faux,k,i,j));
                    }
                    local_transformer.inverseFourierTransform();
                    CenterFFT(Maux, true);

                    // B. Calculate weights for each pixel within sigdim (Mweight)
                    my_sumweight = my_maxweight = 0.;
                    FOR_ALL_ELEMENTS_IN_MATRIX3D(Mweight)
                    {
                        myXA = VOL_ELEM(Maux, k, i, j) * ddim3;
                        diff = A2_plus_Xi2 - myXA;
                        mindiff = XMIPP_MIN(mindiff,diff);
#ifdef DEBUG_MINDIFF

                        if (mindiff < 0)
                        {
                            std::cerr<<"k= "<<k<<" j= "<<j<<" i= "<<i<<std::endl;
                            std::cerr<<"xaux="<<STARTINGX(Maux)<<" xw= "<<STARTINGX(Mweight)<<std::endl;
                            std::cerr<<"yaux="<<STARTINGY(Maux)<<" yw= "<<STARTINGY(Mweight)<<std::endl;
                            std::cerr<<"zaux="<<STARTINGZ(Maux)<<" zw= "<<STARTINGZ(Mweight)<<std::endl;
                            std::cerr<<"diff= "<<diff<<" A2_plus_Xi"<<std::endl;
                            std::cerr<<" mycorrAA= "<<mycorrAA<<" "<<std::endl;
                            std::cerr<<"debug mindiff= " <<mindiff<<" trymindiff= "<<trymindiff<< std::endl;
                            std::cerr<<"A2_plus_Xi2= "<<A2_plus_Xi2<<" myA2= "<<myA2<<" myXi2= "<<myXi2<<std::endl;
                            std::cerr.flush();
                            VolumeXmipp tt;
                            tt()=Maux;
                            tt.write("Maux.vol");
                            tt()=Mweight;
                            tt.write("Mweight.vol");
                            std::cerr<<"Ainv= "<<A_rot_inv<<std::endl;
                            std::cerr<<"A= "<<A_rot_inv.inv()<<std::endl;
                            exit(1);
                        }
#endif
                        pdf = fracpdf * VOL_ELEM(P_phi, k, i, j);
                        // Normal distribution
                        aux = (diff - trymindiff) / sigma_noise2;
                        // next line because of numerical precision of exp-function
                        if (aux > 1000.)
                            weight = 0.;
                        else
                            weight = exp(-aux) * pdf;
                        VOL_ELEM(Mweight, k, i, j) = weight;
                        // Accumulate sum weights for this (my) matrix
                        my_sumweight += weight;
                        // calculate weighted sum of (X-A)^2 for sigma_noise update
                        wsum_corr += weight * diff;
                        // calculated weighted sum of offsets as well
                        wsum_offset += weight * VOL_ELEM(Mr2, k, i, j);
                        // keep track of optimal parameters
                        my_maxweight = XMIPP_MAX(my_maxweight, weight);
                        if (weight > maxweight)
                        {
                            maxweight = weight;
                            ioptz = k;
                            iopty = i;
                            ioptx = j;
                            opt_angno = angno;
                            opt_refno = refno;
                        }

                    } // close for over all elements in Mweight

                    // C. only for significant settings, store weighted sums
                    if (my_maxweight > SIGNIFICANT_WEIGHT_LOW*maxweight )
                    {
                        //#define DEBUG_EXP_A2
#ifdef DEBUG_EXP_A2
                        std::cout<<" imgno= "<<imgno<<" refno= "<<refno<<" angno= "<<angno<<" A2= "<<myA2<<" <<Xi2= "<<myXi2<<" my_maxweight= "<<my_maxweight<<std::endl;
#endif

                        sum_refw += my_sumweight;
                        refw(refno) += my_sumweight;
                        // Back from smaller Mweight to original size of Maux
                        Maux.initZeros();
                        FOR_ALL_ELEMENTS_IN_MATRIX3D(Mweight)
                        {
                            VOL_ELEM(Maux, k, i, j) = VOL_ELEM(Mweight, k, i, j);
                        }
                        // Use forward FFT in convolution theorem again
                        CenterFFT(Maux, false);
                        local_transformer.FourierTransform();
                        FOR_ALL_DIRECT_ELEMENTS_IN_MATRIX3D(Faux)
                        {
                            dVkij(Faux, k, i, j) = conj(dVkij(Faux,k,i,j)) * dVkij(Fimg0,k,i,j);
                        }
                        local_transformer.inverseFourierTransform();
                        maskSphericalAverageOutside(Maux);
                        Maux.selfApplyGeometry(A_rot, IS_NOT_INV,
                                               DONT_WRAP, DIRECT_MULTIDIM_ELEM(Maux,0));
                        if (do_missing)
                        {
                            // Store sum of wedges!
                            getMissingRegion(Mmissing, A_rot, missno);
                            mysumweds[refno] += my_sumweight * Mmissing;

                            // Again enforce missing region to avoid filling it with artifacts from the rotation
                            local_transformer.FourierTransform();
                            FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Faux)
                            {
                                DIRECT_MULTIDIM_ELEM(Faux,n) *= DIRECT_MULTIDIM_ELEM(Mmissing,n);
                            }
                            local_transformer.inverseFourierTransform();
                        }
                        // Store sum of rotated images
                        mysumimgs[refno] += Maux * ddim3;
                    } // close if SIGNIFICANT_WEIGHT_LOW
                } // close for refno
            } // close if is_a_neighbor
        } // close for angno

        // Now check whether our trymindiff was OK.
        // The limit of the exp-function lies around
        // exp(700)=1.01423e+304, exp(800)=inf; exp(-700) = 9.85968e-305; exp(-800) = 0
        // Use 500 to be on the save side?
        if (ABS((mindiff - trymindiff) / sigma_noise2) > 500.)
        {

            //#define DEBUG_REDOCOUNTER
#ifdef DEBUG_REDOCOUNTER
            std::cerr<<"repeating mindiff "<<redo_counter<<"th time"<<std::endl;
            std::cerr<<"trymindiff= "<<trymindiff<<" mindiff= "<<mindiff<<std::endl;
            std::cerr<<"diff= "<<ABS((mindiff - trymindiff) / sigma_noise2)<<std::endl;
#endif
            // Re-do whole calculation now with the real mindiff
            trymindiff = mindiff;
            redo_counter++;
            // Never re-do more than once!
            if (redo_counter>1)
            {
                std::cerr<<"ml_tomo BUG% redo_counter > 1"<<std::endl;
                exit(1);
            }
        }
        else
        {
            is_ok_trymindiff = true;
            my_mindiff = trymindiff;
            trymindiff = mindiff;
        }
    }

    fracweight = maxweight / sum_refw;
    wsum_sc /= sum_refw;
    wsum_sc2 /= sum_refw;

    // Calculate remaining optimal parameters
    XX(opt_offsets) = -(double)ioptx;
    YY(opt_offsets) = -(double)iopty;
    ZZ(opt_offsets) = -(double)ioptz;

    // From here on lock threads
    pthread_mutex_lock( &mltomo_weightedsum_update_mutex );

    // Update all global weighted sums after division by sum_refw
    wsum_sigma_noise += (2 * wsum_corr / sum_refw);
    wsum_sigma_offset += (wsum_offset / sum_refw);
    sumfracweight += fracweight;
    // Randomly choose 0 or 1 for FSC calculation
    int iran_fsc = ROUND(rnd_unif());
    for (int refno = 0; refno < nr_ref; refno++)
    {
        sumw(refno) += refw(refno) / sum_refw;
        // Sum mysumimgs to the global weighted sum
        wsumimgs[iran_fsc*nr_ref + refno] += (mysumimgs[refno]) / sum_refw;
        if (do_missing)
        {
            wsumweds[iran_fsc*nr_ref + refno] += mysumweds[refno] / sum_refw;
        }
    }

    // 1st term: log(refw_i)
    // 2nd term: for subtracting mindiff
    // 3rd term: for (sqrt(2pi)*sigma_noise)^-1 term in formula (12) Sigworth (1998)
    // TODO: check this!!
    if (do_missing)

        dLL = log(sum_refw)
              - my_mindiff / sigma_noise2
              - miss_nr_pixels[missno] * log( sqrt(2. * PI * sigma_noise2));
    else
        dLL = log(sum_refw)
              - my_mindiff / sigma_noise2
              - ddim3 * log( sqrt(2. * PI * sigma_noise2));
    LL += dLL;

    pthread_mutex_unlock(  &mltomo_weightedsum_update_mutex );

#ifdef DEBUG

    std::cerr<<"finished expectationSingleImage"<<std::endl;
    print_elapsed_time(t0);
#endif
}


void Prog_ml_tomo_prm::maxConstrainedCorrSingleImage(
    MultidimArray<double> &Mimg, int imgno, int missno, double old_psi,
    std::vector<VolumeXmippT<double> > &Iref,
    std::vector<MultidimArray<double> > &wsumimgs,
    std::vector<MultidimArray<double> > &wsumweds,
    Matrix1D<double> &sumw, double &maxCC, double &sumCC,
    int &opt_refno, int &opt_angno, Matrix1D<double> &opt_offsets)
{
#ifdef DEBUG
    std::cerr<<"start maxConstrainedCorrSingleImage"<<std::endl;
    TimeStamp t0, t1;
    time_config();
    annotate_time(&t0);
    annotate_time(&t1);
#endif

    MultidimArray<double> Mimg0, Maux, Mref, Mmissing;
    MultidimArray<std::complex<double> > Faux, Fimg0, Fref;
    XmippFftw local_transformer;
    Matrix2D<double> A_rot(4,4), I(4,4), A_rot_inv(4,4);
    bool is_a_neighbor;
    double img_stddev, ref_stddev, corr, maxcorr=-9999.;
    int ioptx,iopty,ioptz;
    int my_nr_ang, old_optangno = opt_angno, old_optrefno = opt_refno;
    bool is_within_psirange=true;

    if (dont_align || dont_rotate)
        my_nr_ang=1;
    else
        my_nr_ang=nr_ang;

    I.initIdentity();
    Maux.resize(dim, dim, dim);
    Maux.setXmippOrigin();

    Maux=Mimg;
    // Apply inverse rotation to the mask and apply
    if (do_mask)
    {
        if (!dont_align)
            REPORT_ERROR(1,"BUG: !dont_align and do_mask cannot coincide at this stage...");
        MultidimArray<double> Mmask;
        A_rot = (all_angle_info[opt_angno]).A;
        A_rot_inv = A_rot.inv();
        applyGeometry(Mmask, A_rot_inv, Imask(), IS_NOT_INV, DONT_WRAP, 0.);
        Maux *= Mmask;
    }
    // Calculate the unrotated Fourier transform with enforced wedge of Mimg (store in Fimg0)
    // Note that from here on local_transformer will act on Maux <-> Faux
    local_transformer.FourierTransform(Maux, Faux, false);
    if (do_missing)
    {
        if (do_adjust_spectra)
            applyPowerSpectraAdjustment(Faux,missno);

        // Enforce missing wedge
        getMissingRegion(Mmissing,I,missno);
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Faux)
        {
            DIRECT_MULTIDIM_ELEM(Faux,n) *= DIRECT_MULTIDIM_ELEM(Mmissing,n);
        }
        // BE CAREFUL: inverseFourierTransform messes up Faux!!
        Fimg0 = Faux;
        local_transformer.inverseFourierTransform();
    }
    else
        Fimg0 = Faux;
    Mimg0 = Maux;

    if (do_only_average)
    {
        maxcorr = 1.;
        ioptz = 0;
        iopty = 0;
        ioptx = 0;
        opt_angno = old_optangno;
        opt_refno = old_optrefno;
    }
    else
    {

        // Calculate stddev of (wedge-inforced) image
        img_stddev = Mimg0.computeStddev();
        // Loop over all orientations
        for (int aa = old_optangno; aa < old_optangno+my_nr_ang; aa++)
        {
            int angno = aa;
            if (angno >= nr_ang)
                angno -= nr_ang;

            // See whether this image is in the neighborhoood for this imgno
            if (ang_search > 0.)
            {
                is_a_neighbor = false;
                if (do_limit_psirange)
                {
                    // Only restrict psi range for directions away from the poles...
                    // Otherwise the is_a_neighbour construction may give errors:
                    // (-150, 0, 150) and (-100, 0, 150) would be considered as equal, since
                    // the distance between (-150,0) and (-100,0) is zero AND
                    // the distance between 150 and 150 is also zero...
                    if (ABS(realWRAP(all_angle_info[angno].tilt, -90., 90.)) < 1.1 * ang_search)
                        is_within_psirange=true;
                    else if ( (do_sym &&
                               ABS(realWRAP(old_psi - (all_angle_info[angno]).rot,-180.,180.)) <= ang_search)
                              || (!do_sym &&
                                  ABS(realWRAP(old_psi - (all_angle_info[angno]).psi,-180.,180.)) <= ang_search) )
                        is_within_psirange=true;
                    else
                        is_within_psirange=false;
                }

                if (!do_limit_psirange || is_within_psirange)
                {
                    for (int i = 0; i < mysampling.my_neighbors[imgno].size(); i++)
                    {
                        if (mysampling.my_neighbors[imgno][i] == (all_angle_info[angno]).direction)
                        {
                            is_a_neighbor = true;
                            break;
                        }
                    }
                }
            }
            else
            {
                is_a_neighbor = true;
            }

            // If it is in the neighborhoood: proceed
            if (is_a_neighbor)
            {
                A_rot = (all_angle_info[angno]).A;
                A_rot_inv = A_rot.inv();

                // Loop over all references
                for (int rr = old_optrefno; rr < old_optrefno+nr_ref; rr++)
                {
                    int refno = rr;
                    if (refno >= nr_ref)
                        refno-= nr_ref;

                    // Now (inverse) rotate the reference and calculate its Fourier transform
                    // Use DONT_WRAP because the reference has been omasked
                    applyGeometry(Maux, A_rot_inv, Iref[refno](), IS_NOT_INV,
                                  DONT_WRAP, DIRECT_MULTIDIM_ELEM(Iref[refno](),0));
                    local_transformer.FourierTransform();
                    if (do_missing)
                    {
                        // Enforce wedge on the reference
                        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Faux)
                        {
                            DIRECT_MULTIDIM_ELEM(Faux,n) *= DIRECT_MULTIDIM_ELEM(Mmissing,n);
                        }
                        // BE CAREFUL! inverseFourierTransform messes up Faux
                        Fref = Faux;
                        local_transformer.inverseFourierTransform();
                    }
                    else
                        Fref = Faux;
                    // Calculate stddev of (wedge-inforced) reference
                    Mref = Maux;
                    ref_stddev = Mref.computeStddev();

                    // Calculate correlation matrix via backward FFT
                    FOR_ALL_DIRECT_ELEMENTS_IN_MATRIX3D(Faux)
                    {
                        dVkij(Faux,k,i,j) =
                            dVkij(Fimg0,k,i,j) *
                            conj(dVkij(Fref,k,i,j));
                    }
                    local_transformer.inverseFourierTransform();
                    CenterFFT(Maux, true);

                    if (dont_align)
                    {
                        corr = VOL_ELEM(Maux,0,0,0) / (img_stddev * ref_stddev);
                        if (corr > maxcorr)
                        {
                            maxcorr = corr;
                            ioptz = 0;
                            iopty = 0;
                            ioptx = 0;
                            opt_angno = angno;
                            opt_refno = refno;
                        }
                    }
                    else
                    {
                        FOR_ALL_ELEMENTS_IN_MATRIX3D(Maux)
                        {
                            corr = VOL_ELEM(Maux,k,i,j) / (img_stddev * ref_stddev);
                            if (corr > maxcorr)
                            {
                                maxcorr = corr;
                                ioptz = k;
                                iopty = i;
                                ioptx = j;
                                opt_angno = angno;
                                opt_refno = refno;
                            }
                        }
                    }
                }
            }
        }
    }

    // Now that we have optimal hidden parameters, update output
    XX(opt_offsets) = -(double)ioptx;
    YY(opt_offsets) = -(double)iopty;
    ZZ(opt_offsets) = -(double)ioptz;
    Mimg0.selfTranslate(opt_offsets, DONT_WRAP);
    A_rot = (all_angle_info[opt_angno]).A;
    maskSphericalAverageOutside(Mimg0);
    Mimg0.selfApplyGeometry(A_rot, IS_NOT_INV,
                            DONT_WRAP, DIRECT_MULTIDIM_ELEM(Mimg0,0));
    maxCC = maxcorr;

    if (do_missing)
    {
        // Store sum of wedges
        getMissingRegion(Mmissing, A_rot, missno);
        Maux = Mimg0;
        // Again enforce missing region to avoid filling it with artifacts from the rotation
        local_transformer.FourierTransform();
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Faux)
        {
            DIRECT_MULTIDIM_ELEM(Faux,n) *= DIRECT_MULTIDIM_ELEM(Mmissing,n);
        }
        local_transformer.inverseFourierTransform();
        Mimg0 = Maux;
    }

    // Randomly choose 0 or 1 for FSC calculation
    int iran_fsc = ROUND(rnd_unif());

    // From here on lock threads to add to sums
    pthread_mutex_lock( &mltomo_weightedsum_update_mutex );

    sumCC += maxCC;
    sumw(opt_refno)  += 1.;
    wsumimgs[iran_fsc*nr_ref + opt_refno] += Mimg0;
    if (do_missing)
        wsumweds[iran_fsc*nr_ref + opt_refno] += Mmissing;

    pthread_mutex_unlock(  &mltomo_weightedsum_update_mutex );

#ifdef DEBUG

    std::cerr<<"finished expectationSingleImage"<<std::endl;
    print_elapsed_time(t0);
#endif

}


void * threadMLTomoExpectationSingleImage( void * data )
{
    structThreadExpectationSingleImage * thread_data = (structThreadExpectationSingleImage *) data;

    // Variables from above
    int thread_id = thread_data->thread_id;
    int thread_num = thread_data->thread_num;
    Prog_ml_tomo_prm *prm = thread_data->prm;
    SelFile *SF = thread_data->SF;
    int *iter = thread_data->iter;
    double *wsum_sigma_noise = thread_data->wsum_sigma_noise;
    double *wsum_sigma_offset = thread_data->wsum_sigma_offset;
    double *sumfracweight = thread_data->sumfracweight;
    double *LL = thread_data->LL;
    std::vector<MultidimArray<double > > *wsumimgs = thread_data->wsumimgs;
    std::vector<MultidimArray<double > > *wsumweds = thread_data->wsumweds;
    std::vector< VolumeXmippT<double> > *Iref = thread_data->Iref;
    std::vector<Matrix1D<double > > *docfiledata = thread_data->docfiledata;
    Matrix1D<double> *sumw = thread_data->sumw;

    //#define DEBUG_THREAD
#ifdef DEBUG_THREAD

    std::cerr<<"start threadMLTomoExpectationSingleImage"<<std::endl;
#endif

    // Local variables
    VolumeXmipp img;
    FileName fn_img, fn_trans;
    std::vector<Matrix1D<double> > allref_offsets;
    Matrix1D<double> opt_offsets(3);
    float old_phi = -999., old_theta = -999., old_psi = -999.;
    double fracweight, maxweight2, trymindiff, dLL;
    int opt_refno, opt_angno, missno;

    // Calculate myFirst and myLast image for this thread
    int nn = (*SF).ImgNo();
    int remaining = nn % thread_num;
    int Npart = (int)(nn - remaining) / thread_num;
    int myFirst, myLast, myNum;
    if (thread_id < remaining)
    {
        myFirst = thread_id * (Npart + 1);
        myLast = myFirst + Npart;
    }
    else
    {
        myFirst = thread_id * Npart + remaining;
        myLast = myFirst + Npart - 1;
    }
    myNum = myLast - myFirst + 1;

    if (prm->verb > 0 && thread_id==0)
        init_progress_bar(myNum);

    // Loop over all images
    int cc = 0;
    for (int imgno = myFirst; imgno <= myLast; imgno++)
    {
        pthread_mutex_lock(  &mltomo_selfile_access_mutex );
        (*SF).go_beginning();
        (*SF).jump(imgno, SelLine::ACTIVE);
        fn_img = (*SF).get_current_file();
        if (fn_img=="")
            break;
        pthread_mutex_unlock(  &mltomo_selfile_access_mutex );

        img.read(fn_img);
        img().setXmippOrigin();

        if (prm->dont_align || prm->do_only_average)
        {
            img().selfTranslate(prm->imgs_optoffsets[imgno], DONT_WRAP);
        }

        prm->reScaleVolume(img(),true);

        if (prm->do_missing)
            missno = prm->imgs_missno[imgno];
        else
            missno = -1;

        //#define DEBUG_APPLYSPECTRA
#ifdef DEBUG_APPLYSPECTRA

        XmippFftw transf;
        VolumeXmipp Vt;
        MultidimArray<std::complex<double> > Faux;
        MultidimArray<double> Maux;
        Maux=img();
        transf.FourierTransform(Maux,Faux);
        prm->applyPowerSpectraAdjustment(Faux,missno);
        transf.inverseFourierTransform();
        Vt()=Maux;
        Vt.write(fn_img+".adjusted");
#else
        // These three parameters speed up expectationSingleImage
        trymindiff = prm->imgs_trymindiff[imgno];
        opt_refno = prm->imgs_optrefno[imgno];
        opt_angno = prm->imgs_optangno[imgno];
        old_psi = prm->imgs_optpsi[imgno];

        if (prm->do_ml)
        {
            // A. Use maximum likelihood approach
            (*prm).expectationSingleImage(img(), imgno, missno, old_psi, *Iref, *wsumimgs, *wsumweds,
                                          *wsum_sigma_noise, *wsum_sigma_offset,
                                          *sumw, *LL, dLL, fracweight, *sumfracweight,
                                          trymindiff, opt_refno, opt_angno, opt_offsets);
        }
        else
        {
            // B. Use constrained correlation coefficient approach
            (*prm).maxConstrainedCorrSingleImage(img(), imgno, missno, old_psi, *Iref,
                                                 *wsumimgs, *wsumweds, *sumw, fracweight, *sumfracweight,
                                                 opt_refno, opt_angno, opt_offsets);
        }

        // Store for next iteration
        prm->imgs_trymindiff[imgno] = trymindiff;
        prm->imgs_optrefno[imgno] = opt_refno;
        prm->imgs_optangno[imgno] = opt_angno;

        // Output docfile
        (*docfiledata)[imgno](0) = (prm->all_angle_info[opt_angno]).rot;     // rot
        (*docfiledata)[imgno](1) = (prm->all_angle_info[opt_angno]).tilt;    // tilt
        (*docfiledata)[imgno](2) = (prm->all_angle_info[opt_angno]).psi;     // psi
        if (prm->dont_align || prm->do_only_average)
        {
            (*docfiledata)[imgno](3) = prm->imgs_optoffsets[imgno](0);       // Xoff
            (*docfiledata)[imgno](4) = prm->imgs_optoffsets[imgno](1);       // Yoff
            (*docfiledata)[imgno](5) = prm->imgs_optoffsets[imgno](2);       // zoff
        }
        else
        {
            (*docfiledata)[imgno](3) = opt_offsets(0) / prm->scale_factor;   // Xoff
            (*docfiledata)[imgno](4) = opt_offsets(1) / prm->scale_factor;   // Yoff
            (*docfiledata)[imgno](5) = opt_offsets(2) / prm->scale_factor;   // Zoff
        }
        (*docfiledata)[imgno](6) = (double)(opt_refno + 1);   // Ref
        (*docfiledata)[imgno](7) = (double)(missno + 1);      // Wedge number

        (*docfiledata)[imgno](8) = fracweight;                // P_max/P_tot
        (*docfiledata)[imgno](9) = dLL;                       // log-likelihood
#endif

        if (prm->verb > 0 && thread_id==0)
            progress_bar(cc);
        cc++;
    }
    if (prm->verb > 0 && thread_id==0)
        progress_bar(myNum);

#ifdef DEBUG_THREAD

    std::cerr<<"finished threadMLTomoExpectationSingleImage"<<std::endl;
#endif

}

void Prog_ml_tomo_prm::expectation(
    SelFile &SF, std::vector< VolumeXmippT<double> > &Iref, int iter,
    double &LL, double &sumfracweight, DocFile &DFo,
    std::vector<MultidimArray<double> > &wsumimgs,
    std::vector<MultidimArray<double> > &wsumweds,
    double &wsum_sigma_noise, double &wsum_sigma_offset,
    Matrix1D<double> &sumw)
{

#ifdef DEBUG
    std::cerr<<"start expectation"<<std::endl;
#endif

    Matrix1D<double> dataline(MLTOMODATALINELENGTH);
    MultidimArray<double> Mzero(dim,dim,hdim+1), Mzero2(dim,dim,dim);
    std::vector<Matrix1D<double> > docfiledata;
    bool fill_real_space;
    int num_img_tot;

    // Perturb all angles
    // (Note that each mpi process will have a different random perturbation)
    if (do_perturb)
        perturbAngularSampling();

    if (do_ml)
    {
        // Precalculate A2-values for all references
        precalculateA2(Iref);

        // Pre-calculate pdf of all in-plane transformations
        calculatePdfTranslations();
    }

    // Initialize weighted sums
    LL = 0.;
    wsum_sigma_noise = 0.;
    wsum_sigma_offset = 0.;
    sumfracweight = 0.;
    sumw.initZeros(nr_ref);
    dataline.initZeros();
    for (int i = 0; i < SF.ImgNo(); i++)
        docfiledata.push_back(dataline);
    Mzero.initZeros();
    Mzero2.initZeros();
    Mzero2.setXmippOrigin();
    wsumimgs.clear();
    wsumweds.clear();
    for (int refno = 0; refno < 2*nr_ref; refno++)
    {
        wsumimgs.push_back(Mzero2);
        wsumweds.push_back(Mzero);
    }

    // Call threads to calculate the expectation of each image in the selfile
    pthread_t * th_ids = (pthread_t *)malloc( threads * sizeof( pthread_t));
    structThreadExpectationSingleImage * threads_d = (structThreadExpectationSingleImage *) malloc ( threads * sizeof( structThreadExpectationSingleImage ) );
    for( int c = 0 ; c < threads ; c++ )
    {
        threads_d[c].thread_id = c;
        threads_d[c].thread_num = threads;
        threads_d[c].prm = this;
        threads_d[c].SF=&SF;
        threads_d[c].iter=&iter;
        threads_d[c].wsum_sigma_noise=&wsum_sigma_noise;
        threads_d[c].wsum_sigma_offset=&wsum_sigma_offset;
        threads_d[c].sumfracweight=&sumfracweight;
        threads_d[c].LL=&LL;
        threads_d[c].wsumimgs=&wsumimgs;
        threads_d[c].wsumweds=&wsumweds;
        threads_d[c].Iref=&Iref;
        threads_d[c].docfiledata=&docfiledata;
        threads_d[c].sumw=&sumw;
        pthread_create( (th_ids+c), NULL, threadMLTomoExpectationSingleImage, (void *)(threads_d+c) );
    }


    // Wait for threads to finish and get joined DocFile
    for( int c = 0 ; c < threads ; c++ )
    {
        pthread_join(*(th_ids+c),NULL);
    }

    // Send back output in the form of a DocFile
    SF.go_beginning();
    for (int imgno = 0; imgno < SF.ImgNo(); imgno++)
    {
        DFo.append_comment(SF.NextImg());
        DFo.append_data_line(docfiledata[imgno]);
    }

#ifdef DEBUG
    std::cerr<<"finished expectation"<<std::endl;
#endif

}

// Update all model parameters
void Prog_ml_tomo_prm::maximization(std::vector<MultidimArray<double> > &wsumimgs,
                                    std::vector<MultidimArray<double> > &wsumweds,
                                    double &wsum_sigma_noise, double &wsum_sigma_offset,
                                    Matrix1D<double> &sumw, double &sumfracweight,
                                    double &sumw_allrefs, std::vector<Matrix1D<double> > &fsc,
                                    int iter)
{
#ifdef DEBUG
    std::cerr<<"started maximization"<<std::endl;
#endif

    Matrix1D<double> rmean_sigma2, rmean_signal2;
    Matrix1D<int> center(3), radial_count;
    MultidimArray<std::complex<double> > Faux(dim,dim,hdim+1), Fwsumimgs;
    MultidimArray<double> Maux, Msumallwedges(dim,dim,hdim+1);
    std::vector<double> refs_resol(nr_ref);
    VolumeXmipp Vaux;
    FileName fn_tmp;
    double rr, thresh, aux, sumw_allrefs2 = 0., avg_origin=0.;
    int c;

    // Calculate resolutions
    fsc.clear();
    fsc.resize(nr_ref + 1);
    for (int refno=0; refno<nr_ref; refno++)
    {
        calculateFsc(wsumimgs[refno], wsumimgs[nr_ref+refno],
                     wsumweds[refno], wsumweds[nr_ref+refno],
                     fsc[0], fsc[refno+1], refs_resol[refno]);
        // Restore original wsumimgs and wsumweds
        wsumimgs[refno] += wsumimgs[nr_ref + refno];
        wsumweds[refno] += wsumweds[nr_ref + refno];
    }

    // Update the reference images
    sumw_allrefs = 0.;
    Msumallwedges.initZeros();
    for (int refno=0;refno<nr_ref; refno++)
    {
        sumw_allrefs += sumw(refno);
        transformer.FourierTransform(wsumimgs[refno],Fwsumimgs,true);
        if (do_missing)
        {
            Msumallwedges += wsumweds[refno];
            // Only impute for ML-approach (maxCC approach uses division by sumwedge)
            if (do_ml && do_impute)
            {
                transformer.FourierTransform(Iref[refno](),Faux,true);
                if (sumw(refno) > 0.)
                {
                    // inner iteration: marginalize over missing data only
                    for (int iter2 = 0; iter2 < Niter2; iter2++)
                    {
                        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Faux)
                        {
                            // Impute old reference for missing pixels
                            DIRECT_MULTIDIM_ELEM(Faux,n) *=
                                (1. - DIRECT_MULTIDIM_ELEM(wsumweds[refno],n) / sumw(refno));
                            // And sum the weighted sum for observed pixels
                            DIRECT_MULTIDIM_ELEM(Faux,n) +=
                                (DIRECT_MULTIDIM_ELEM(Fwsumimgs,n) / sumw(refno));
                        }
                    }
                }
                // else do nothing (i.e. impute old reference completely
            }
            else
            {
                // no imputation: divide by number of times a pixel has been observed
                FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Faux)
                {
                    if (DIRECT_MULTIDIM_ELEM(wsumweds[refno],n) > noimp_threshold)
                    {
                        DIRECT_MULTIDIM_ELEM(Faux,n) =
                            DIRECT_MULTIDIM_ELEM(Fwsumimgs,n) /
                            DIRECT_MULTIDIM_ELEM(wsumweds[refno],n);
                        // TODO:  CHECK THE FOLLOWING LINE!!
                        DIRECT_MULTIDIM_ELEM(Faux,n) *= sumw(refno) / sumw(refno);
                    }
                    else
                    {
                        DIRECT_MULTIDIM_ELEM(Faux,n) = 0.;
                    }
                }
            }
        }
        else
        {
            // No missing data
            if (sumw(refno) > 0.)
            {
                // if no missing data: calculate straightforward average,
                FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(Faux)
                {
                    DIRECT_MULTIDIM_ELEM(Faux,n) =
                        DIRECT_MULTIDIM_ELEM(Fwsumimgs,n) / sumw(refno);
                }
            }
        }
        // Back to real space
        transformer.inverseFourierTransform(Faux,Iref[refno]());
    }

    // Average fracweight
    sumfracweight /= sumw_allrefs;

    // Update the model fractions
    if (!fix_fractions)
    {
        for (int refno=0;refno<nr_ref; refno++)
        {
            if (sumw(refno) > 0.)
                alpha_k(refno) = sumw(refno) / sumw_allrefs;
            else
                alpha_k(refno) = 0.;
        }
    }

    // Update sigma of the origin offsets
    if (!fix_sigma_offset)
    {
        sigma_offset = sqrt(wsum_sigma_offset / (2. * sumw_allrefs));
    }

    // Update the noise parameters
    if (!fix_sigma_noise)
    {
        if (do_missing)
        {
            double sum_complete_wedge = 0.;
            double sum_complete_fourier = 0.;
            MultidimArray<double> Mcomplete(dim,dim,dim);
            FOR_ALL_DIRECT_ELEMENTS_IN_MATRIX3D(Mcomplete)
            {
                if (j<XSIZE(Msumallwedges))
                {
                    sum_complete_wedge += DIRECT_VOL_ELEM(Msumallwedges,k,i,j);
                    sum_complete_fourier += DIRECT_VOL_ELEM(fourier_imask,k,i,j);
                }
                else
                {
                    sum_complete_wedge += DIRECT_VOL_ELEM(Msumallwedges,
                                                          (dim-k)%dim,(dim-i)%dim,dim-j);
                    sum_complete_fourier += DIRECT_VOL_ELEM(fourier_imask,
                                                            (dim-k)%dim,(dim-i)%dim,dim-j);
                }
            }
            //#define DEBUG_UPDATE_SIGMA
#ifdef DEBUG_UPDATE_SIGMA
            std::cerr<<" sum_complete_wedge= "<<sum_complete_wedge<<" = "<<100*sum_complete_wedge/(sumw_allrefs*sum_complete_fourier)<<"%"<<std::endl;
            std::cerr<<" ddim3= "<<ddim3<<std::endl;
            std::cerr<<" sum_complete_fourier= "<<sum_complete_fourier<<std::endl;
            std::cerr<<" sumw_allrefs= "<<sumw_allrefs<<std::endl;
            std::cerr<<" wsum_sigma_noise= "<<wsum_sigma_noise<<std::endl;
            std::cerr<<" sigma_noise_old= "<<sigma_noise<<std::endl;
            std::cerr<<" sigma_new_impute= "<<sqrt((sigma_noise*sigma_noise*(sumw_allrefs*sum_complete_fourier -  sum_complete_wedge  ) + wsum_sigma_noise)/(sumw_allrefs * sum_complete_fourier))<<std::endl;
            std::cerr<<" sigma_new_noimpute= "<<sqrt(wsum_sigma_noise / (sum_complete_wedge))<<std::endl;
            std::cerr<<" sigma_new_nomissing= "<<sqrt(wsum_sigma_noise / (sumw_allrefs * sum_complete_fourier))<<std::endl;
#endif

            if (do_impute)
            {
                for (int iter2 = 0; iter2 < Niter2; iter2++)
                {
                    sigma_noise *= sigma_noise;
                    sigma_noise *= sumw_allrefs*sum_complete_fourier - sum_complete_wedge ;
                    sigma_noise += wsum_sigma_noise;
                    sigma_noise =  sqrt(sigma_noise / (sumw_allrefs * sum_complete_fourier));
                }
            }
            else
            {
                sigma_noise = sqrt(wsum_sigma_noise / (sum_complete_wedge));
            }
        }
        else
        {
            sigma_noise = sqrt(wsum_sigma_noise / (sumw_allrefs * ddim3));
        }
        // Correct sigma_noise for number of pixels within the mask
        if (do_mask)
            sigma_noise *= ddim3/nr_mask_pixels;
    }

    // post-process reference volumes
    for (int refno=0; refno < nr_ref; refno++)
    {
        if (do_filter)
            postProcessVolume(Iref[refno], refs_resol[refno]);
        else
            postProcessVolume(Iref[refno]);
    }

    // Regularize
    regularize(iter);

#ifdef DEBUG

    std::cerr<<"finished maximization"<<std::endl;
#endif
}

void Prog_ml_tomo_prm::calculateFsc(MultidimArray<double> &M1, MultidimArray<double> &M2,
                                    MultidimArray<double> &W1, MultidimArray<double> &W2,
                                    Matrix1D<double> &freq, Matrix1D<double> &fsc,
                                    double &resolution)
{

    MultidimArray< std::complex< double > > FT1, FT2;
    XmippFftw transformer1, transformer2;
    transformer1.FourierTransform(M1, FT1, false);
    transformer2.FourierTransform(M2, FT2, false);

    int vsize = hdim + 1;
    Matrix1D<double> num, den1, den2, f(3);
    double w1, w2, R;
    freq.initZeros(vsize);
    fsc.initZeros(vsize);
    num.initZeros(vsize);
    den1.initZeros(vsize);
    den2.initZeros(vsize);

    FOR_ALL_DIRECT_ELEMENTS_IN_MATRIX3D(FT1)
    {
        FFT_IDX2DIGFREQ(j,XSIZE(M1),XX(f));
        FFT_IDX2DIGFREQ(i,YSIZE(M1),YY(f));
        FFT_IDX2DIGFREQ(k,ZSIZE(M1),ZZ(f));
        R=f.module();
        if (R>0.5)
            continue;
        int idx=ROUND(R*XSIZE(M1));
        if (do_missing)
        {
            w1 = dVkij(W1, k, i, j);
            w2 = dVkij(W2, k, i, j);
        }
        else
        {
            w1 = w2 = 1.;
        }
        std::complex<double> z1=w2*dVkij(FT1, k, i, j);
        std::complex<double> z2=w1*dVkij(FT2, k, i, j);
        double absz1=abs(z1);
        double absz2=abs(z2);
        num(idx)+=real(conj(z1) * (z2));
        den1(idx)+= absz1*absz1;
        den2(idx)+= absz2*absz2;
    }

    FOR_ALL_ELEMENTS_IN_MATRIX1D(freq)
    {
        freq(i) = (double) i / (XSIZE(M1) * pixel_size);
        if(num(i)!=0)
            fsc(i) = num(i)/sqrt(den1(i)*den2(i));
        else
            fsc(i) = 0;
    }

    // Find resolution limit acc. to FSC=0.5 criterion
    int idx;
    for (idx = 1; idx < XSIZE(freq); idx++)
        if (fsc(idx) < 0.5)
            break;
    idx = XMIPP_MAX(idx - 1, 1);
    resolution=freq(idx);

    //#define DEBUG_FSC
#ifdef DEBUG_FSC

    FOR_ALL_ELEMENTS_IN_MATRIX1D(freq)
    {
        std::cerr<<freq(i)<<" "<<fsc(i)<<std::endl;
    }
    std::cerr<<"resolution= "<<resolution<<std::endl;
#endif


}

// Apply regularization
bool Prog_ml_tomo_prm::regularize(int iter)
{

    // Update regularization constant in a linear manner
    reg_current = reg0 - (double)iter*(reg0-regF)/(double)reg_steps;
    reg_current = XMIPP_MAX(reg_current, regF);

    if (reg_current > 0.)
    {
#ifdef DEBUG
        std::cerr<<"start regularize"<<std::endl;
#endif
        // Write out original volumes (before regularization)
        FileName fnt;
        for (int refno = 0; refno < nr_ref; refno++)
        {
            fnt.compose(fn_root+"_it",iter,"");
            fnt.compose(fnt+"_oriref",refno+1,"vol");
            Iref[refno].write(fnt);
        }
        // Normalized regularization (in N/K)
        double reg_norm = reg_current * (double)nr_exp_images / (double)nr_ref;
        MultidimArray<std::complex<double> > Fref, Favg, Fzero(dim,dim,hdim+1);
        MultidimArray<double> Mavg, Mdiff;
        double sum_diff2=0.;

        //#define DEBUG_REGULARISE
#ifdef DEBUG_REGULARISE

        std::cerr<<"written out oriref volumes"<<std::endl;
#endif
        // Calculate  FT of average of all references
        // and sum of squared differences between all references
        for (int refno = 0; refno < nr_ref; refno++)
        {
            if (refno==0)
                Mavg = Iref[refno]();
            else
                Mavg += Iref[refno]();
            for (int refno2 = 0; refno2 < nr_ref; refno2++)
            {
                Mdiff = Iref[refno]() - Iref[refno2]();
                sum_diff2 += Mdiff.sum2();
            }
        }
        transformer.FourierTransform(Mavg,Favg,true);
        Favg *= reg_norm;
#ifdef DEBUG_REGULARISE

        std::cerr<<"calculated Favg"<<std::endl;
#endif

        // Update the regularized references
        for (int refno = 0; refno < nr_ref; refno++)
        {
            transformer.FourierTransform(Iref[refno](),Fref,true);
            double sumw = alpha_k(refno) * (double)nr_exp_images;
            // Fref = (sumw*Fref + reg_norm*Favg) /  (sumw + nr_ref * reg_norm)
#ifdef DEBUG_REGULARISE

            if (verb>0)
                std::cerr<<"refno= "<<refno<<" sumw = "<<sumw<<" reg_current= "<<reg_current<<" reg_norm= "<<reg_norm<<" Fref1= "<<DIRECT_MULTIDIM_ELEM(Fref,1) <<" Favg1= "<<DIRECT_MULTIDIM_ELEM(Favg,1)<<" (sumw + nr_ref * reg_norm)= "<<(sumw + nr_ref * reg_norm)<<std::endl;
#endif

            Fref *= sumw;
            Fref += Favg;
            Fref /= (sumw + nr_ref * reg_norm);
#ifdef DEBUG_REGULARISE

            if (verb>0)
                std::cerr<<" newFref1= "<<DIRECT_MULTIDIM_ELEM(Fref,1) <<std::endl;
#endif

            transformer.inverseFourierTransform(Fref,Iref[refno]());
        }

        // Update the regularized sigma_noise estimate
        if (!fix_sigma_noise)
        {
            double reg_sigma_noise2 = sigma_noise*sigma_noise*ddim3*(double)nr_exp_images;
#ifdef DEBUG_REGULARISE

            if (verb>0)
                std::cerr<<"reg_sigma_noise2= "<<reg_sigma_noise2<<" nr_exp_images="<<nr_exp_images<<" ddim3= "<<ddim3<<" sigma_noise= "<<sigma_noise<<" sum_diff2= "<<sum_diff2<<" reg_norm= "<<reg_norm<<std::endl;
#endif

            reg_sigma_noise2 += reg_norm * sum_diff2;
            sigma_noise = sqrt(reg_sigma_noise2/((double)nr_exp_images*ddim3));
#ifdef DEBUG_REGULARISE

            if (verb>0)
                std::cerr<<"new sigma_noise= "<<sigma_noise<<std::endl;
#endif

        }

#ifdef DEBUG
        std::cerr<<"finished regularize"<<std::endl;
#endif

    }
}
// Check convergence
bool Prog_ml_tomo_prm::checkConvergence(std::vector<double> &conv)
{

#ifdef DEBUG
    std::cerr<<"started checkConvergence"<<std::endl;
#endif

    bool converged = true;
    double convv;
    MultidimArray<double> Maux;

    Maux.resize(dim, dim, dim);
    Maux.setXmippOrigin();

    conv.clear();
    for (int refno=0;refno<nr_ref; refno++)
    {
        if (alpha_k(refno) > 0.)
        {
            Maux = Iold[refno]() * Iold[refno]();
            convv = 1. / (Maux.computeAvg());
            Maux = Iold[refno]() - Iref[refno]();
            Maux = Maux * Maux;
            convv *= Maux.computeAvg();
            conv.push_back(convv);
            if (convv > eps)
                converged = false;
        }
        else
        {
            conv.push_back(-1.);
        }
    }

#ifdef DEBUG
    std::cerr<<"finished checkConvergence"<<std::endl;
#endif

    return converged;
}

void Prog_ml_tomo_prm::writeOutputFiles(const int iter, DocFile &DFo,
                                        std::vector<MultidimArray<double> > &wsumweds,
                                        double &sumw_allrefs, double &LL, double &avefracweight,
                                        std::vector<double> &conv, std::vector<Matrix1D<double> > &fsc)
{

    FileName          fn_tmp, fn_base, fn_tmp2;
    Matrix1D<double>  fracline(2);
    SelFile           SFo, SFc;
    DocFile           DFl;
    std::string       comment;
    VolumeXmipp       Vt;

    DFl.clear();
    SFo.clear();
    SFc.clear();

    fn_base = fn_root;
    if (iter >= 0)
    {
        fn_base += "_it";
        fn_base.compose(fn_base, iter, "");
    }

    // Write out current reference images and fill sel & log-file
    for (int refno=0;refno<nr_ref; refno++)
    {
        fn_tmp = fn_base + "_ref";
        fn_tmp.compose(fn_tmp, refno + 1, "");
        fn_tmp = fn_tmp + ".vol";
        Vt=Iref[refno];
        reScaleVolume(Vt(),false);
        Vt.write(fn_tmp);
        SFo.insert(fn_tmp, SelLine::ACTIVE);
        fracline(0) = alpha_k(refno);
        fracline(1) = 1000 * conv[refno]; // Output 1000x the change for precision
        DFl.insert_comment(fn_tmp);
        DFl.insert_data_line(fracline);

        if (iter >= 1 && do_missing)
        {
            double sumw = alpha_k(refno)*sumw_allrefs;
            Vt().resize(dim,dim,dim);
            Vt().setXmippOrigin();
            Vt().initZeros();

            FOR_ALL_DIRECT_ELEMENTS_IN_MATRIX3D(Vt())
            if (j<hdim+1)
                DIRECT_VOL_ELEM(Vt(),k,i,j)=
                    DIRECT_VOL_ELEM(wsumweds[refno],k,i,j) / sumw_allrefs;
            else
                DIRECT_VOL_ELEM(Vt(),k,i,j)=
                    DIRECT_VOL_ELEM(wsumweds[refno],
                                    (dim-k)%dim,
                                    (dim-i)%dim,
                                    dim-j) / sumw_allrefs;

            CenterFFT(Vt(),true);
            reScaleVolume(Vt(),false);
            fn_tmp = fn_base + "_wedge";
            fn_tmp.compose(fn_tmp, refno + 1, "");
            fn_tmp = fn_tmp + ".vol";
            Vt.write(fn_tmp);
        }
    }

    // Write out FSC curves
    std::ofstream  fh;
    fn_tmp = fn_base + ".fsc";
    fh.open((fn_tmp).c_str(), std::ios::out);
    if (!fh)
        REPORT_ERROR(1, (std::string)"Prog_ml_tomo_prm: Cannot write file: " + fn_tmp);

    fh<<"# freq. FSC refno 1-n... \n";
    for (int idx = 1; idx < hdim + 1; idx++)
    {
        fh << fsc[0](idx) <<" ";
        for (int refno = 0; refno < nr_ref; refno++)
        {
            fh << fsc[refno+1](idx) <<" ";
        }
        fh<<"\n";
    }
    fh.close();

    // Write out sel & log-file
    fn_tmp = fn_base + ".sel";
    SFo.write(fn_tmp);

    if (iter >= 1)
    {
        DFl.go_beginning();
        comment = "ml_tomo-logfile: Number of images= " + floatToString(sumw_allrefs);
        comment += " LL= " + floatToString(LL, 15, 10) + " <Pmax/sumP>= " + floatToString(avefracweight, 10, 5);
        DFl.insert_comment(comment);
        comment = "-noise " + floatToString(sigma_noise, 15, 12) + " -offset " + floatToString(sigma_offset/scale_factor, 15, 12) + " -istart " + integerToString(iter + 1);
        DFl.insert_comment(comment);
        DFl.insert_comment(cline);
        DFl.insert_comment("columns: model fraction (1); 1000x signal change (2); resolution (3)");
        fn_tmp = fn_base + ".log";
        DFl.write(fn_tmp);

        // Write out docfile with optimal transformation & references
        fn_tmp = fn_base + ".doc";
        DFo.write(fn_tmp);

        // Also write out selfiles of all experimental images,
        // classified according to optimal reference image
        for (int refno = 0;refno < nr_ref; refno++)
        {
            DFo.go_beginning();
            SFo.clear();
            for (int n = 0; n < DFo.dataLineNo(); n++)
            {
                DFo.next();
                fn_tmp = ((DFo.get_current_line()).get_text()).erase(0, 3);
                DFo.adjust_to_data_line();
                if ((refno + 1) == (int)DFo(6))
                    SFo.insert(fn_tmp, SelLine::ACTIVE);
            }
            fn_tmp.compose(fn_base + "_ref", refno + 1, "sel");
            SFo.write(fn_tmp);
        }
    }

}

