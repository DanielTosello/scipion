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
#ifndef _PROG_MICROSCOPE_HH
#define _PROG_MICROSCOPE_HH

#include "fourier_filter.h"

#include <data/progs.h>

/**@defgroup MicroscopeProgram phantom_simulate_microscope (Microscope simulation)
   @ingroup ReconsLibrary */
//@{
/* Microscope Program Parameters ------------------------------------------- */
/** Parameter class for the project program */
class Prog_Microscope_Parameters: public Prog_parameters
{
public:
    /// Filename with the CTF
    FileName fn_ctf;
    /// Total noise power
    double   sigma;
    /// Low pass frequency before CTF
    double   low_pass_before_CTF;
    /// Filename with the root squared spectrum for noise after CTF
    bool     after_ctf_noise;
    /// Defocus change (%)
    double   defocus_change;
    /// True if this program is directly called from the command line
    bool command_line;
public:
    /// CTF
    ProgFourierFilter ctf;
    /// Low pass filter, if it is 0 no lowpass filter is applied
    ProgFourierFilter lowpass;
    /// After CTF noise root squared spectrum
    ProgFourierFilter after_ctf;
    /// Noise power before CTF
    double   sigma_before_CTF;
    /// Noise power after CTF
    double   sigma_after_CTF;
    /// Input image Xdim
    int Xdim;
    /// Input image Ydim
    int Ydim;
public:
    /** Read from a command line.
        An exception might be thrown by any of the internal conversions,
        this would mean that there is an error in the command line and you
        might show a usage message. */
    void read(int argc, char **argv);

    /** Usage message.
        This function shows the way of introducing these parameters. */
    void usage();

    /** Show parameters. */
    void show();

    /** Produce side information. */
    void produce_side_info();

    /** Apply to a single image. The image is modified.
        If the CTF is randomly selected then a new CTF is generated
        for each image */
    void apply(MultidimArray<double> &I);
};
//@}
#endif
