/***************************************************************************
 *
 * Authors:     Carlos Oscar S. Sorzano (coss@cnb.uam.es)
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
#ifndef _PROG_PDBPHANTOM_HH
#  define _PROG_PDBPHANTOM_HH

#include "blobs.h"

/**@name PDB Phantom program */
//@{
/* PDB Phantom Program Parameters ------------------------------------------ */
/** Parameter class for the PDB Phantom program */
class Prog_PDBPhantom_Parameters
{
public:
    /** Sampling rate */
    double Ts;

    /** PDB file */
    FileName fn_pdb;

    /** Output fileroot */
    FileName fn_out;

    /** Blob */
    struct blobtype blob;

    /** Final size in pixels */
    int output_dim;
public:
    /** Empty constructor */
    Prog_PDBPhantom_Parameters();

    /** Read from a command line.
        An exception might be thrown by any of the internal conversions,
        this would mean that there is an error in the command line and you
        might show a usage message. */
    void read(int argc, char **argv);

    /** Usage message.
        This function shows the way of introducing this parameters. */
    void usage();

    /** Show parameters. */
    void show();

    /** Run. */
    void run();
public:
    /* Sampling rate used for highly sampled volumes */
    double highTs;

    /* periodic_table(i,0)=radius
       periodic_table(i,1)=atomic weight */
    matrix2D<double> periodic_table;

    // Protein geometry
    Matrix1D<double> center_of_mass, limit;

    /* Volume at a high sampling rate */
    VolumeXmipp Vhigh;

    /* Volume at a low sampling rate */
    VolumeXmipp Vlow;

    /* Blob properties at the high sampling rate */
    void blob_properties() const;

    /* Atom weight and radius */
    void atom_description(const string &_element, double &weight, double &radius)
    const;

    /* Protein geometry */
    void compute_protein_geometry();

    /* Create protein at a high sampling rate */
    void create_protein_at_high_sampling_rate();

    /* Create protein at a low sampling rate */
    void create_protein_at_low_sampling_rate();
};
//@}
#endif
