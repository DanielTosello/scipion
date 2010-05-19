/***************************************************************************
 *
 * Authors:     J.M. De la Rosa Trevin (jmdelarosa@cnb.csic.es)
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

#ifndef METADATALABEL_H
#define METADATALABEL_H

#include <map>
#include "strings.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include "funcs.h"

class MDLabelData;
class StaticInitialization;

enum MDLabel
{
    MDL_UNDEFINED = -1,
    MDL_FIRST_LABEL,
    MDL_ANGLEPSI2 = MDL_FIRST_LABEL, // Psi angle of an image (double)
    MDL_ANGLEPSI, // Psi angle of an image (double)
    MDL_ANGLEROT2, // Rotation angle of an image (double)
    MDL_ANGLEROT, // Rotation angle of an image (double)
    MDL_ANGLETILT2, // Tilting angle of an image (double)
    MDL_ANGLETILT, // Tilting angle of an image (double)
    MDL_AVG, //average value (double)
    MDL_AZIMUTALANGLE, //ctf definition azimutal angle
    MDL_BGMEAN, // Mean background value for an image
    MDL_BLOCK, // Current block number (for incremental EM)
    MDL_CELLX, // Cell location for crystals
    MDL_CELLY, // Cell location for crystals
    MDL_COMMENT, // A comment for this object /*** NOTE THIS IS A SPECIAL CASE AND SO IS TREATED ***/
    MDL_COST, // Cost for the image (double)
    MDL_COUNT, // Number of elements of a type (int) [this is a genereic type do not use to transfer information to another program]
    MDL_CTFINPUTPARAMS, // Parameters file for the CTF Model (std::string)
    MDL_CTFMODEL, // Name for the CTF Model (std::string)
    MDL_CTF_SAMPLING_RATE, // Sampling rate
    MDL_CTF_VOLTAGE, // Microscope voltage (kV)
    MDL_CTF_DEFOCUSU, // Defocus U (Angstroms)
    MDL_CTF_DEFOCUSV, // Defocus V (Angstroms)
    MDL_CTF_DEFOCUS_ANGLE, // Defocus angle (degrees)
    MDL_CTF_CS, // Spherical aberration
    MDL_CTF_CA, // Chromatic aberration
    MDL_CTF_ENERGY_LOSS, // Energy loss
    MDL_CTF_LENS_STABILITY, // Lens stability
    MDL_CTF_CONVERGENCE_CONE, // Convergence cone
    MDL_CTF_LONGITUDINAL_DISPLACEMENT, // Longitudinal displacement
    MDL_CTF_TRANSVERSAL_DISPLACEMENT, // Transversal displacemente
    MDL_CTF_Q0, // Inelastic absorption
    MDL_CTF_K, // CTF gain
    MDL_CTFBG_GAUSSIAN_K, // CTF Background parameter
    MDL_CTFBG_GAUSSIAN_SIGMAU, // CTF Background parameter
    MDL_CTFBG_GAUSSIAN_SIGMAV, // CTF Background parameter
    MDL_CTFBG_GAUSSIAN_CU, // CTF Background parameter
    MDL_CTFBG_GAUSSIAN_CV, // CTF Background parameter
    MDL_CTFBG_GAUSSIAN_ANGLE, // CTF Background parameter
    MDL_CTFBG_SQRT_K, // CTF Background parameter
    MDL_CTFBG_SQRT_U, // CTF Background parameter
    MDL_CTFBG_SQRT_V, // CTF Background parameter
    MDL_CTFBG_SQRT_ANGLE, // CTF Background parameter
    MDL_CTFBG_BASELINE, // CTF Background parameter
    MDL_CTFBG_GAUSSIAN2_K, // CTF Background parameter
    MDL_CTFBG_GAUSSIAN2_SIGMAU, // CTF Background parameter
    MDL_CTFBG_GAUSSIAN2_SIGMAV, // CTF Background parameter
    MDL_CTFBG_GAUSSIAN2_CU, // CTF Background parameter
    MDL_CTFBG_GAUSSIAN2_CV, // CTF Background parameter
    MDL_CTFBG_GAUSSIAN2_ANGLE, // CTF Background parameter
    MDL_DEFGROUP, // Defocus group
    MDL_DEFOCUSU, // microscope defocus U direction (double)
    MDL_DEFOCUSV, // microscope defocus V direction (double)
    MDL_DPR,//differential phase residual (double) add row only label at the end of the enum
    MDL_ENABLED, // Is this image enabled? (int [-1 or 1])
    MDL_FLIP, // Flip the image? (bool)
    MDL_FRC,//Fourier shell correlation (double)
    MDL_FRCRANDOMNOISE,//Fourier shell correlation noise (double)
    MDL_IMAGE_CLASS_COUNT, // Number of images assigned to the same class as this image
    MDL_IMAGE_CLASS_GROUP, // Name of the class group for this image (metadata with all the images assigned to that class)
    MDL_IMAGE_CLASS, // Name of the class representative for this image
    MDL_IMAGE, // Name of an image (std::string)
    MDL_IMAGE_ORIGINAL, // Name of an image from which MDL_IMAGE is coming from
    MDL_IMGMD, // Name of Metadata file for all images (string)
    MDL_INTSCALE, // Intensity scale for an image
    MDL_ITER, // Current iteration number (int)
    MDL_K,// //ctf definition K
    MDL_KSTEST, //KS-test statistics
    MDL_LL, // contribution of an image to log-likelihood value
    MDL_MASK, // Name of a mask associated to image
    MDL_MAXCC, // Cross-correlation for the image (double)
    MDL_MAX, //maximum value (double)
    MDL_MICROGRAPH, // Name of a micrograph (std::string)
    MDL_MIN, //minimum value (double)
    MDL_MIRRORFRAC, // Mirror fraction for a Maximum Likelihood model
    MDL_MISSINGREGION_NR, // Number of missing region in subtomogram
    MDL_MISSINGREGION_TYPE, // Type of missing region in subtomogram
    MDL_MODELFRAC, // Model fraction (alpha_k) for a Maximum Likelihood model
    MDL_NMA, // Normal mode displacements (vector double)
    MDL_OBJID, // object id (int)
    MDL_ORIGINX, // Origin for the image in the X axis (double)
    MDL_ORIGINY, // Origin for the image in the Y axis (double)
    MDL_ORIGINZ, // Origin for the image in the Z axis (double)
    MDL_PERIODOGRAM, // A periodogram's file name (std::string)
    MDL_PMAX, // Maximum value of normalized probability function (now called "Pmax/sumP") (double)
    MDL_Q0,//ctf definition Q0
    MDL_RANDOMSEED, // Seed for random number generator
    MDL_REF3D, // 3D Class to which the image belongs (int)
    MDL_REF, // Class to which the image belongs (int)
    MDL_REFMD, // Name of Metadata file for all references(string)
    MDL_RESOLUTIONFOURIER,//resolution in 1/A (double)
    MDL_RESOLUTIONREAL,//resolution in A (double)
    MDL_SAMPLINGRATE, // sampling rate (double)
    MDL_SAMPLINGRATEX, // sampling rate (double)
    MDL_SAMPLINGRATEY, // sampling rate (double)
    MDL_SAMPLINGRATEZ, // sampling rate (double)
    MDL_SCALE, // scaling factor for an image or volume (double)
    MDL_SELFILE, // Name of an image (std::string)
    MDL_SERIE, // A collection of micrographs, e.g. a tilt serie (std::string)
    MDL_SHIFTX, // Shift for the image in the X axis (double)
    MDL_SHIFTY, // Shift for the image in the Y axis (double)
    MDL_SHIFTZ, // Shift for the image in the Z axis (double)
    MDL_SHIFT_CRYSTALX, // Shift for the image in the X axis (double) for crystals
    MDL_SHIFT_CRYSTALY, // Shift for the image in the Y axis (double) for crystals
    MDL_SHIFT_CRYSTALZ, // Shift for the image in the Z axis (double) for crystals
    MDL_SIGMANOISE, // Standard deviation of the noise in ML model
    MDL_SIGMAOFFSET, // Standard deviation of the offsets in ML model
    MDL_SIGNALCHANGE, // Signal change for an image
    MDL_SPHERICALABERRATION, //ctf definition azimutal angle
    MDL_STDDEV, //stdandard deviation value (double)
    MDL_SUM, // Sum of elements of a given type (double) [this is a genereic type do not use to transfer information to another program]
    MDL_SUMWEIGHT, // Sum of all weights in ML model
    MDL_SYMNO, // Symmetry number for a projection (used in ART)
    MDL_TRANSFORMATIONMTRIX, // transformation matrix(vector double)
    MDL_VOLTAGE, // microscope voltage (double)
    MDL_WEIGHT, // Weight assigned to the image (double)
    MDL_WROBUST, // Weight of t-student distribution in robust Maximum likelihood
    MDL_XINT, // X component (int)
    MDL_X, // X component (double)
    MDL_YINT, // Y component (int)
    MDL_Y, // Y component (double)
    MDL_ZINT, // Z component (int)
    MDL_Z, // Z component (double)


    MDL_LAST_LABEL                       // **** NOTE ****: Do keep this label always at the end
    // it is here for looping purposes
};//close enum Label

enum MDLabelType
{
    LABEL_INT, LABEL_BOOL, LABEL_DOUBLE, LABEL_FLOAT, LABEL_STRING, LABEL_VECTOR
};

class MDL
{
public:
    // This enum defines what MetaDataLabels this class can manage, if
    // you need a new one add it here and modify affected methods:
    //
    //  - static MDLabel codifyLabel( std::string strLabel );
    // - static std::string MDL::label2Str( MDLabel inputLabel );
    // - void writeValuesToFile( std::ofstream &outfile, MDLabel inputLabel );
    // - void addValue( std::string name, std::string value );
    //
    // Keep this special structure (using MDL_FIRSTLABEL and MDL_LAST_LABEL) so the
    // programmer can iterate through it like this:
    //
    //  for( MDLabel mdl = MDL_FIRST_LABEL ; mdl < MDL_LAST_LABEL ; MDLabel( mdl+1 ) )
    //

static MDLabel str2Label(const std::string &labelName);
static std::string label2Str(const MDLabel &label);

static bool isInt(const MDLabel &label);
static bool isBool(const MDLabel &label);
static bool isString(const MDLabel &label);
static bool isDouble(const MDLabel &label);
static bool isVector(const MDLabel &label);
static bool isValidLabel(const MDLabel &label);
static bool isValidLabel(const std::string &labelName);

private:
    static std::map<MDLabel, MDLabelData> data;
    static std::map<std::string, MDLabel> names;
    static StaticInitialization initialization; //Just for initialization

    static void addLabel(MDLabel label, MDLabelType type, std::string name, std::string name2 = "", std::string name3 = "");


    friend class StaticInitialization;
}
;//close class MLD definition

//Just an struct to store type and string alias
class MDLabelData
{
public:
    MDLabelType type;
    std::string str;
    //Default constructor
    MDLabelData()
    {
    }
    MDLabelData(MDLabelType t, std::string s)
    {
        type = t;
        str = s;
    }
};//close class MDLabelData c

//Just a class for static initialization
class StaticInitialization
{
private:
    StaticInitialization()
    {
        ///==== Add labels entries from here in the SAME ORDER as declared in ENUM ==========
        MDL::addLabel(MDL_ANGLEPSI2, LABEL_DOUBLE, "anglePsi2", "psi2");
        MDL::addLabel(MDL_ANGLEPSI, LABEL_DOUBLE, "anglePsi", "psi");
        MDL::addLabel(MDL_ANGLEROT2, LABEL_DOUBLE, "angleRot2", "rot2");
        MDL::addLabel(MDL_ANGLEROT, LABEL_DOUBLE, "angleRot", "rot");
        MDL::addLabel(MDL_ANGLETILT2, LABEL_DOUBLE, "angleTilt2", "tilt2");
        MDL::addLabel(MDL_ANGLETILT, LABEL_DOUBLE, "angleTilt", "tilt");
        MDL::addLabel(MDL_AVG, LABEL_DOUBLE, "avg");
        MDL::addLabel(MDL_AZIMUTALANGLE, LABEL_DOUBLE, "azimutalAngle");
        MDL::addLabel(MDL_BGMEAN, LABEL_DOUBLE, "bgMean");
        MDL::addLabel(MDL_BLOCK, LABEL_INT, "blockNumber");
        MDL::addLabel(MDL_CELLX, LABEL_INT, "cellX");
        MDL::addLabel(MDL_CELLY, LABEL_INT, "cellY");
        MDL::addLabel(MDL_COMMENT, LABEL_STRING, "comment");
        MDL::addLabel(MDL_COST, LABEL_DOUBLE, "cost");
        MDL::addLabel(MDL_COUNT, LABEL_INT, "count");
        MDL::addLabel(MDL_CTFINPUTPARAMS, LABEL_STRING, "CTFInputParams");
        MDL::addLabel(MDL_CTFMODEL, LABEL_STRING, "CTFModel");
        MDL::addLabel(MDL_CTF_SAMPLING_RATE, LABEL_DOUBLE, "CTF_Sampling_rate");
        MDL::addLabel(MDL_CTF_VOLTAGE, LABEL_DOUBLE, "CTF_Voltage");
        MDL::addLabel(MDL_CTF_DEFOCUSU, LABEL_DOUBLE, "CTF_Defocus_U");
        MDL::addLabel(MDL_CTF_DEFOCUSV, LABEL_DOUBLE, "CTF_Defocus_V");
        MDL::addLabel(MDL_CTF_DEFOCUS_ANGLE, LABEL_DOUBLE, "CTF_Defocus_angle");
        MDL::addLabel(MDL_CTF_CS, LABEL_DOUBLE, "CTF_Spherical_aberration");
        MDL::addLabel(MDL_CTF_CA, LABEL_DOUBLE, "CTF_Chromatic_aberration");
        MDL::addLabel(MDL_CTF_ENERGY_LOSS, LABEL_DOUBLE, "CTF_Energy_loss");
        MDL::addLabel(MDL_CTF_LENS_STABILITY, LABEL_DOUBLE, "CTF_Lens_stability");
        MDL::addLabel(MDL_CTF_CONVERGENCE_CONE, LABEL_DOUBLE, "CTF_Convergence_cone");
        MDL::addLabel(MDL_CTF_LONGITUDINAL_DISPLACEMENT, LABEL_DOUBLE, "CTF_Longitudinal_displacement");
        MDL::addLabel(MDL_CTF_TRANSVERSAL_DISPLACEMENT, LABEL_DOUBLE, "CTF_Transversal_displacement");
        MDL::addLabel(MDL_CTF_Q0, LABEL_DOUBLE, "CTF_Q0");
        MDL::addLabel(MDL_CTF_K, LABEL_DOUBLE, "CTF_K");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN_K, LABEL_DOUBLE, "CTFBG_Gaussian_K");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN_SIGMAU, LABEL_DOUBLE, "CTFBG_Gaussian_SigmaU");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN_SIGMAV, LABEL_DOUBLE, "CTFBG_Gaussian_SigmaU");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN_CU, LABEL_DOUBLE, "CTFBG_Gaussian_CU");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN_CV, LABEL_DOUBLE, "CTFBG_Gaussian_CV");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN_ANGLE, LABEL_DOUBLE, "CTFBG_Gaussian_Angle");
        MDL::addLabel(MDL_CTFBG_SQRT_K, LABEL_DOUBLE, "CTFBG_Sqrt_K");
        MDL::addLabel(MDL_CTFBG_SQRT_U, LABEL_DOUBLE, "CTFBG_Sqrt_U");
        MDL::addLabel(MDL_CTFBG_SQRT_V, LABEL_DOUBLE, "CTFBG_Sqrt_V");
        MDL::addLabel(MDL_CTFBG_SQRT_ANGLE, LABEL_DOUBLE, "CTFBG_Sqrt_Angle");
        MDL::addLabel(MDL_CTFBG_BASELINE, LABEL_DOUBLE, "CTFBG_Baseline");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN2_K, LABEL_DOUBLE, "CTFBG_Gaussian2_K");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN2_SIGMAU, LABEL_DOUBLE, "CTFBG_Gaussian2_SigmaU");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN2_SIGMAV, LABEL_DOUBLE, "CTFBG_Gaussian2_SigmaV");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN2_CU, LABEL_DOUBLE, "CTFBG_Gaussian2_CU");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN2_CV, LABEL_DOUBLE, "CTFBG_Gaussian2_CV");
        MDL::addLabel(MDL_CTFBG_GAUSSIAN2_ANGLE, LABEL_DOUBLE, "CTFBG_Gaussian2_Angle");
        MDL::addLabel(MDL_DEFGROUP, LABEL_INT, "defocusGroup");
        MDL::addLabel(MDL_DEFOCUSU, LABEL_DOUBLE, "defocusU");
        MDL::addLabel(MDL_DEFOCUSV, LABEL_DOUBLE, "defocusV");
        MDL::addLabel(MDL_DPR, LABEL_DOUBLE, "DPR");
        MDL::addLabel(MDL_ENABLED, LABEL_INT, "enabled");
        MDL::addLabel(MDL_FLIP, LABEL_BOOL, "flip", "Flip");
        MDL::addLabel(MDL_FRC, LABEL_DOUBLE, "FRC");
        MDL::addLabel(MDL_FRCRANDOMNOISE, LABEL_DOUBLE, "FRC_random_noise");
        MDL::addLabel(MDL_IMAGE_CLASS_COUNT, LABEL_INT, "class_count");
        MDL::addLabel(MDL_IMAGE_CLASS_GROUP, LABEL_STRING, "class_group");
        MDL::addLabel(MDL_IMAGE_CLASS, LABEL_STRING, "class_representative");
        MDL::addLabel(MDL_IMAGE, LABEL_STRING, "image");
        MDL::addLabel(MDL_IMAGE_ORIGINAL, LABEL_STRING, "original_image");
        MDL::addLabel(MDL_IMGMD, LABEL_STRING, "imageMetaData");
        MDL::addLabel(MDL_INTSCALE, LABEL_DOUBLE, "intScale");
        MDL::addLabel(MDL_ITER, LABEL_INT, "iterationNumber");
        MDL::addLabel(MDL_K, LABEL_DOUBLE, "K");
        MDL::addLabel(MDL_KSTEST, LABEL_DOUBLE, "KStest");
        MDL::addLabel(MDL_LL, LABEL_DOUBLE, "logLikelihood");
        MDL::addLabel(MDL_MASK, LABEL_STRING, "mask");
        MDL::addLabel(MDL_MAXCC, LABEL_DOUBLE, "maxCC");
        MDL::addLabel(MDL_MAX, LABEL_DOUBLE, "max");
        MDL::addLabel(MDL_MICROGRAPH, LABEL_STRING, "micrograph");
        MDL::addLabel(MDL_MIN, LABEL_DOUBLE, "min");
        MDL::addLabel(MDL_MIRRORFRAC, LABEL_DOUBLE, "mirrorFraction");
        MDL::addLabel(MDL_MISSINGREGION_NR, LABEL_INT, "missingRegionNumber");
        MDL::addLabel(MDL_MISSINGREGION_TYPE, LABEL_STRING, "missingRegionType");
        MDL::addLabel(MDL_MODELFRAC, LABEL_DOUBLE, "modelFraction");
        MDL::addLabel(MDL_NMA, LABEL_VECTOR, "NMADisplacements");
        MDL::addLabel(MDL_OBJID, LABEL_INT, "objId");
        MDL::addLabel(MDL_ORIGINX, LABEL_DOUBLE, "originX");
        MDL::addLabel(MDL_ORIGINY, LABEL_DOUBLE, "originY");
        MDL::addLabel(MDL_ORIGINZ, LABEL_DOUBLE, "originZ");
        MDL::addLabel(MDL_PERIODOGRAM, LABEL_STRING, "periodogram");
        MDL::addLabel(MDL_PMAX, LABEL_DOUBLE, "pMax", "Pmax", "sumP");
        MDL::addLabel(MDL_Q0, LABEL_DOUBLE, "Q0");
        MDL::addLabel(MDL_RANDOMSEED, LABEL_INT, "randomSeed");
        MDL::addLabel(MDL_REF3D, LABEL_INT, "ref3d");
        MDL::addLabel(MDL_REF, LABEL_INT, "ref", "Ref");
        MDL::addLabel(MDL_REFMD, LABEL_STRING, "referenceMetaData");
        MDL::addLabel(MDL_RESOLUTIONFOURIER, LABEL_DOUBLE, "Resol. [1/Ang]");
        MDL::addLabel(MDL_RESOLUTIONREAL, LABEL_DOUBLE, "Resol. [1/Ang]");
        MDL::addLabel(MDL_SAMPLINGRATE, LABEL_DOUBLE, "sampling_rate");
        MDL::addLabel(MDL_SAMPLINGRATEX, LABEL_DOUBLE, "sampling_rateX");
        MDL::addLabel(MDL_SAMPLINGRATEY, LABEL_DOUBLE, "sampling_rateY");
        MDL::addLabel(MDL_SAMPLINGRATEZ, LABEL_DOUBLE, "sampling_rateZ");
        MDL::addLabel(MDL_SCALE, LABEL_DOUBLE, "scale", "Scale");
        MDL::addLabel(MDL_SELFILE, LABEL_STRING, "selfile");
        MDL::addLabel(MDL_SERIE, LABEL_STRING, "serie");
        MDL::addLabel(MDL_SHIFTX, LABEL_DOUBLE, "shiftX", "Xoff");
        MDL::addLabel(MDL_SHIFTY, LABEL_DOUBLE, "shiftY", "Yoff");
        MDL::addLabel(MDL_SHIFTZ, LABEL_DOUBLE, "shiftZ", "Zoff");
        MDL::addLabel(MDL_SIGMANOISE, LABEL_DOUBLE, "sigmaNoise");
        MDL::addLabel(MDL_SIGMAOFFSET, LABEL_DOUBLE, "sigmaOffset");
        MDL::addLabel(MDL_SIGNALCHANGE, LABEL_DOUBLE, "signalChange");
        MDL::addLabel(MDL_SPHERICALABERRATION, LABEL_DOUBLE, "sphericalAberration");
        MDL::addLabel(MDL_STDDEV, LABEL_DOUBLE, "stddev");
        MDL::addLabel(MDL_SUM, LABEL_DOUBLE, "sum");
        MDL::addLabel(MDL_SUMWEIGHT, LABEL_DOUBLE, "sumWeight");
        MDL::addLabel(MDL_SYMNO, LABEL_INT, "symNo");
        MDL::addLabel(MDL_TRANSFORMATIONMTRIX, LABEL_VECTOR, "transMat");
        MDL::addLabel(MDL_VOLTAGE, LABEL_DOUBLE, "voltage");
        MDL::addLabel(MDL_WEIGHT, LABEL_DOUBLE, "weight", "Weight");
        MDL::addLabel(MDL_WROBUST, LABEL_DOUBLE, "wRobust");
        MDL::addLabel(MDL_XINT, LABEL_INT, "Xcoor", "<X position>");
        MDL::addLabel(MDL_X, LABEL_DOUBLE, "X");
        MDL::addLabel(MDL_YINT, LABEL_INT, "Ycoor", "<Y position>");
        MDL::addLabel(MDL_Y, LABEL_DOUBLE, "Y");
        MDL::addLabel(MDL_ZINT, LABEL_INT, "Zcoor");
        MDL::addLabel(MDL_Z, LABEL_DOUBLE, "Z");
    }

    ~StaticInitialization()
    {
    }
    friend class MDL;
};




#endif
