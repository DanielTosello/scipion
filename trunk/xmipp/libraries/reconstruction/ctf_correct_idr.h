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
#ifndef _PROG_IDR_ART_HH
#  define _PROG_IDR_ART_HH

#include <data/metadata.h>
#include <data/image.h>
#include "fourier_filter.h"

/**@defgroup IDR ctf_correct_idr (Iterative Data Refinement for CTF amplitude correction)
   @ingroup ReconsLibraryPrograms */
//@{
/* IDR Parameters ---------------------------------------------------------- */
/** IDR Parameters. */
class Prog_IDR_ART_Parameters
{
public:
    /// Reference volume
    FileName fn_vol;
    /// CTFdat file
    FileName fn_ctfdat;
    /// Relaxation factor
    double mu;
    /// Output filename root
    FileName fnRoot;
    
    /// This is running under MPI
    bool MPIversion;
    
    /// Number of processors
    int numberOfProcessors;

    /// MPIRank
    int MPIrank;

    /// Side Info: volume
    Image<double> V;
    /// Side Info: ctfdat
    MetaData ctfdat;
public:
    /// Empty constructor
    Prog_IDR_ART_Parameters();
    
    /// Read parameters from the command line
    void read(int argc, char **argv);

    /** Produce Side Information
        Exceptions are thrown if the number of images in the different
        selfiles do not match */
    void produce_side_info();

    /// Show parameters
    void show();

    /// Usage
    void Usage();

    /** IDR correction.
        This function process all images in the selfile.
    */
    void IDR_correction();
};
//@}
#endif
