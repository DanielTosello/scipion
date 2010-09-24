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

#ifndef _FOURIER_FILTER_HH
#define _FOURIER_FILTER_HH

#include <data/ctf.h>
#include <data/multidim_array.h>
#include <data/fftw.h>
#include <data/program.h>

/**@defgroup FourierMasks Masks in Fourier space
   @ingroup ReconsLibrary */
//@{
/** Filter class for Fourier space.

   Example of use for highpass filtering
   @code
      ImageXmipp I("image.xmp");
      FourierMask Filter;
      Filter.FilterBand=HIGHPASS;
      Filter.w1=w_cutoff;
      Filter.raised_w=slope;
      I().setXmippOrigin();
      Filter.generate_mask(I()); // Should not be used with filters
                                 // other than WEDGE or CONE
      Filter.apply_mask_Space(I());
      I.write("filtered_image.xmp");
   @endcode
   
   For volumes you the mask is computed on the fly and
   in this way memory is saved (unless do_generate_3dmask == true).
*/
class ProgFourierFilter: public XmippMetadataProgram
{
public:
#define RAISED_COSINE 1
    /** Shape of the decay in the filter.
       Valid types are RAISED_COSINE. */
    int FilterShape;

#define LOWPASS       1
#define HIGHPASS      2
#define BANDPASS      3
#define STOPBAND      4
#define CTF           5
#define WEDGE         7
#define GAUSSIAN      8
#define CONE          9
#define CTFPOS       10
#define BFACTOR      11
#define REALGAUSSIAN 12
    /** Pass band. LOWPASS, HIGHPASS, BANDPASS, STOPBAND, CTF, CTFPOS,
       WEDGE, CONE, GAUSSIAN, FROM_FILE, REALGAUSSIAN, BFACTOR */
    int FilterBand;

    /** Cut frequency for Low and High pass filters, first freq for bandpass.
        Normalized to 1/2*/
    double w1;

    /** Second frequency for bandpass and stopband. Normalized to 1/2 */
    double w2;

    /** Pixels around the central frequency for the raised cosine */
    double raised_w;

    /** CTF parameters. */
    CTFDescription ctf;
    
    /** Correct phase before applying CTF */
    bool do_correct_phase;

    /** Flag to generate 3D mask */
    bool do_generate_3dmask;
protected:
    /** Define parameters */
    void defineParams();

    /** Read parameters from command line.
        If a CTF description file is provided it is read. */
    void readParams();

    /** Process one image */
    void processImage();

public:
    /** Empty constructor */
    ProgFourierFilter();

    /** Assignment */
    ProgFourierFilter & operator = (const ProgFourierFilter &F);

    /** Another function for assignment */
    void assign(const ProgFourierFilter &F);

    /** Clear */
    void clear();

    /** Show. */
    void show();

    /** Compute the mask value at a given frequency.
        The frequency must be normalized so that the maximum frequency
        in each direction is 0.5 */
    double maskValue(const Matrix1D<double> &w);

    /** Generate nD mask. */
    void generateMask(MultidimArray<double> &v);

    /** Apply mask in 3D. */
    void applyMaskSpace(MultidimArray<double> &v);

    /** Get the power of the nD mask. */
    double maskPower();
    
    /** Correct phase */
    void correctPhase();
public:
    // Auxiliary vector for representing frequency values
    Matrix1D<double> w;

    // Auxiliary mask for the filter in 3D
    MultidimArray<int> maskFourier;

    // Transformer
    FourierTransformer transformer;
};
//@}
#endif
