/***************************************************************************
 *
 * Authors:     Carlos Oscar S. Sorzano (coss@cnb.csic.es)
 *              Alberto Pascual (pascual@cnb.csic.es)
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

#ifndef SHOWSEL_H
#define SHOWSEL_H

#include "show_table.h"

#include <data/metadata.h>
#include <data/metadata_extension.h>
#include <data/fft.h>

#ifdef QT3_SUPPORT
//Added by qt3to4:
#include <QMouseEvent>
#endif

/**@defgroup ShowSelfile Show Collection of images in MetaData File
   @ingroup GraphicsLibrary */
//@{
/** Class to show multiple 2D images in metadata files.
    Example of use:
    @code
       ShowSel *showsel = new ShowSel;
       showsel->initWithFile(numRows, numCols, argv[i]);
       showsel->show();
    @endcode
*/
class ShowSel: public ShowTable
{
    Q_OBJECT;
public:
    typedef enum {
        Normal_mode,
        PSD_mode,
        CTF_mode
    } TLoadMode;

    // Show only active images (!= -1 in showsel). 1-> show active, 0-> show all
    bool        showonlyactive;
    bool        apply_geo;
    TLoadMode   load_mode;
protected:
    // Filenames within the Selfile
    FileName   *imgnames;
    // Sel file name. Used by the save routine
    FileName selfile_fn;
    // Selstatus of those files
    bool       *selstatus;
    // Kind of label: there are two types
    // SFLabel_LABEL  --> use the Selfile status as label
    // Filename_LABEL --> use the Filename       as label
    int         labeltype;
    // Valid types
#define SFLabel_LABEL  0
#define Filename_LABEL 1

    // Some menu items to enable/disable them
    int mi_Individual_norm, mi_Global_norm,
    mi_showLabel,       mi_hideLabel,
    mi_selAsLabels,     mi_imgAsLabels;

    // Assign CTF file: This is used for a single micrograph with pieces
    FileName fn_assign;

    // Assign CTF sel file: This is usef for a bunch of micrographs with
    // a single CTF each one
    FileName fn_assign_sel;

    // Reimplementation of the insert_in queue to account for
    // the deletion of unused cells
    void insert_content_in_queue(int i);

    /* Initialization.
       Sets labeltype = SFLabel_LABEL; imgnames, selstatus = NULL; and then
       calls to ShowTable::init() */
    virtual void init();
    /* Clear everything */
    virtual void clear();
    /* Configures the QTable widget */
    virtual void initTable();
    /* Compute the minimum and maximum pixel within selfile */
    virtual void compute_global_normalization_params();
    /* Write SelFile, opens a GUI for the file selection and checks if the
       output filename exists. If it does, it asks for confirmation.*/
    virtual void writeSelFile(MetaData &_SF, bool overwrite = false);
    /* Cell label depending on labeltype */
    virtual const char* cellLabel(int i) const;
    /* Initialize right click menubar */
    virtual void initRightclickMenubar();
    /* Set File in right click menu bar */
    void setFileRightclickMenubar();
    /* Set common options in right click menu bar */
    void setCommonOptionsRightclickMenubar();
    /* Produce Pixmap for cell i */
    virtual void producePixmapAt(int i);
    /* Open a new file. Old parameters must be discarded */
    void openNewFile(const FileName &);
    /* Read a Selfile.
       It performs some initializatrion and then calls to readSelFile*/
    virtual void readFile(const FileName &_fn,
                          double _minGray = 0, double _maxGray = 0);
    /* Really read a SelFile. */
    virtual void readSelFile(const FileName &_fn,
                             double _minGray = 0, double _maxGray = 0);
    /* Read a SelFile object */
    virtual void readObject(MetaData &SF,
                            double _minGray = 0, double _maxGray = 0);
    /* Update the label with the filename at cell i*/
    virtual void updateStatus(int i);
private slots:
    // For updating the status label
    virtual void contentsMouseMoveEvent(QMouseEvent *);
protected slots:
    // These slots are related with the right click menubar ---------------- */
    /* Reload all images */
    virtual void reloadAll();
    /* Unselect all cells */
    virtual void unSelectAll();
    /* Select all cells */
    virtual void SelectAll();
    /* Save current SelFile with marked projections as discarded */
    virtual void saveSelFileDiscarded();
    /* Save current SelFile with marked projections as active, and the
       rest as discarded */
    virtual void saveSelFileActive();
    /* Save marked projections in a new Selfile as Active */
    virtual void saveSelFileNew();
    /* Save marked projections in a new Selfile as Active */
    virtual void saveSelFileNewOverwrite();
    /* Show Average and Stddev images */
    virtual void showStats();
    /* Show statistics of the SelFile */
    virtual void showSelStats();
    /* Show this image separately */
    virtual void showThisImage();
    /* Show this image separately */
    virtual void recomputeCTFmodel();
    /* Show CTF model parameters */
    virtual void editCTFmodel();
    /* Change the global/individual normalization status */
    virtual void changeNormalize();
    /* Change the show/hide labels status */
    virtual void changeShowLabels();
    /* Change the kind of labels.*/
    virtual void changeLabels();
public:
    /** Empty constructor. */
    ShowSel();

    /** Read selfile for the first time.
        If you need to update the volume representation, use openNewFile(()*/
    virtual void initWithFile(int _numRows, int _numCols,
                              const FileName &_fn, double _minGray = 0, double _maxGray = 0);

    /** Read selfile for the first time changing the loading mode.*/
    void initWithFile(int _numRows, int _numCols,
                      const FileName &_fn, TLoadMode _load_mode);

    /** Initialize with a Selfile. */
    virtual void initWithObject(int _numRows, int _numCols,
                                MetaData &_SF, const char *_title);

    /** For CTF mode, set assign CTF file. */
    void setAssignCTFfile(const FileName &_fn_assign);

    /** For CTF mode, set assign CTF file. */
    void setAssignCTFselfile(const FileName &_fn_assign_sel);
};
//@}
#endif
