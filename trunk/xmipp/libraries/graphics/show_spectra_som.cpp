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

#include "show_spectra_som.h"

#include <qmessagebox.h>
#ifdef QT3_SUPPORT
//Added by qt3to4:
#include <Q3PopupMenu>
#endif

// Init/Clear -------------------------------------------------------------- */
void ShowSpectraSOM::init()
{
    Vdat        = NULL;
    SFcv        = NULL;
    SFcvs       = NULL;
    hisAssigned = NULL;
    cv_errors   = NULL;
    infStr      = "";
    ShowSpectra::init();
}

void ShowSpectraSOM::clear()
{
    if (Vdat        != NULL) delete    Vdat;
    if (SFcv        != NULL) delete [] SFcv;
    if (SFcvs       != NULL) delete [] SFcvs;
    if (hisAssigned != NULL) delete [] hisAssigned;
    if (cv_errors   != NULL) delete [] cv_errors;
    infStr = "";
    ShowSpectra::clear();
}

/* Initialize with a SOM file.---------------------------------------------- */
void ShowSpectraSOM::initWithFile(const FileName &_fn_root,
                                  const FileName &_fn_dat)
{
    init();
    readFile(_fn_root);

    std::ifstream fh_in(_fn_dat.c_str());
    if (!fh_in)
        REPORT_ERROR(ERR_IO_NOTOPEN, (std::string)"ShowSpectra::readFile: Cannot open" + _fn_dat);
    Vdat = new ClassicTrainingVectors(fh_in);
    fh_in.close();

    initTable();
    initRightclickMenubar();
    repaint();
}

/* Read a SOM -------------------------------------------------------------- */
void ShowSpectraSOM::readFile(const FileName &_fn_root,
                              double _minGray, double _maxGray)
{
    clear();
    fn              = _fn_root;
    setCaption(fn.c_str());
    readSOMFiles(_fn_root);
    readDatFile(_fn_root + ".cod");
}

void ShowSpectraSOM::readSOMFiles(const FileName &_fn_root)
{
    FileName fn_class, fn_his, fn_err, fn_inf;
    fn_class = _fn_root + ".vs";
    fn_his  = _fn_root + ".his";
    fn_err  = _fn_root + ".err";
    fn_inf  = _fn_root + ".inf";

    // Read histogram
    if (exists(fn_his))
    {
        std::ifstream fh_his(fn_his.c_str());
        if (fh_his)
        {
            ClassicTrainingVectors ts(0, true);
            fh_his >> ts;
            int imax = ts.size();
            hisAssigned = new std::string[imax];
            for (int i = 0; i < imax; i++)
                hisAssigned[i] = ts.theTargets[i];
        }
        fh_his.close();
    }

    // Read errors
    if (exists(fn_err))
    {
        std::ifstream fh_err(fn_err.c_str());
        if (fh_err)
        {
            ClassicTrainingVectors ts(0, true);
            fh_err >> ts;
            int imax = ts.size();
            cv_errors = new std::string[imax];
            for (int i = 0; i < imax; i++)
                cv_errors[i] = ts.theTargets[i];
        }
        fh_err.close();
    }

    // Read inf file
    if (exists(fn_inf))
    {
        std::ifstream fh_inf(fn_inf.c_str());
        if (fh_inf)
        {
            std::string line;
            getline(fh_inf, line);
            infStr = line.c_str();
            infStr += "\n";
            while (!fh_inf.eof())
            {
                getline(fh_inf, line);
                infStr += line.c_str();
                infStr += "\n";
            }
        }
        fh_inf.close();
    }

    // Read codevectors
    if (exists(fn_class))
    {
        std::ifstream fh_class(fn_class.c_str());
        if (fh_class)
        {
            std::string line, fn;
            int dim;
            std::string topol, neigh;
            fh_class >> dim >> topol >> NumCols >> NumRows >> neigh;
            listSize = NumCols * NumRows;
            if (listSize == 0)
                REPORT_ERROR(ERR_IO_SIZE, "ShowSpectraSOM::readFile: Input file is empty");
            getline(fh_class, line);

            if (infStr == "")
            {
                infStr = "Kohonen SOM algorithm\n\n";
                infStr += "Number of variables: ";
                line = integerToString(dim);
                infStr += line.c_str();
                infStr += "\n";
                infStr += "Horizontal dimension (Xdim): ";
                line = integerToString(NumCols);
                infStr += line.c_str();
                infStr += "\n";
                infStr += "Vertical dimension (Ydim): ";
                line = integerToString(NumRows);
                infStr += line.c_str();
                infStr += "\n";
                infStr += "Topology : ";
                infStr += topol.c_str();
                infStr += "\n";
                infStr += "Neighborhood function : ";
                infStr += neigh.c_str();
                infStr += "\n";
            }

            SFcv = new std::vector<std::string>[listSize];
            SFcvs = new std::vector<int>[listSize];
            int j = 0;
            while (!fh_class.eof())
            {
                int row, col;
                float tmp;
                fh_class >> col >> row >> tmp;
                getline(fh_class, line);
                int i = row * NumCols + col;
                SFcv[i].push_back(firstToken(line));
                SFcvs[i].push_back(j);
                j++;
            }
        }
        fh_class.close();
    }
}

/* Initialize right click menubar ------------------------------------------ */
void ShowSpectraSOM::initRightclickMenubar()
{
#ifdef QT3_SUPPORT
    menubar = new Q3PopupMenu();
    Q3PopupMenu * file = new Q3PopupMenu();
#else
    menubar = new QPopupMenu();
    QPopupMenu* file = new QPopupMenu();
#endif

    file->insertItem("Open...", this,  SLOT(GUIopenFile()));
    file->insertItem("Save assigned images in a sel file...",
                     this, SLOT(saveAssigned()), Qt::CTRL + Qt::Key_N);
    menubar->insertItem("&File", file);

    // Options .............................................................
#ifdef QT3_SUPPORT
    options =  new Q3PopupMenu();
#else
    options = new QPopupMenu();
#endif
    setCommonOptionsRightclickMenubar();

    setCommonSpectraOptionsRightclickMenubar();

    // What kind of labels
    mi_imgAsLabels = options->insertItem("Show Image Names as Labels", this,  SLOT(changeLabelsToImg()));
    mi_selAsLabels = options->insertItem("Show Sel status as Labels", this,  SLOT(changeLabelsToSel()));
    mi_hisAsLabels = options->insertItem("Show Histogram as Labels", this, SLOT(changeLabelsToHis()));
    mi_errAsLabels = options->insertItem("Show Errors as Labels", this, SLOT(changeLabelsToErr()));
    options->setItemEnabled(mi_imgAsLabels, true);
    options->setItemEnabled(mi_selAsLabels, true);
    options->setItemEnabled(mi_hisAsLabels, false);
    options->setItemEnabled(mi_errAsLabels, true);
    labeltype = Histogram_LABEL;
    options->insertSeparator();

    // Statistics
    options->insertItem("View average and SD Represented Spectra",  this,  SLOT(showRepresentedSpectraStats()));
    options->insertItem("View average and SD Represented Images", this,  SLOT(showRepresentedImagesStats()));
    options->insertItem("View Represented Spectra", this,  SLOT(showRepresentedSpectra()));
    options->insertItem("View Represented Images", this,  SLOT(showRepresentedSel()));
    options->insertItem("View error spectrum", this,  SLOT(showErrorSpectrum()), Qt::CTRL + Qt::Key_E);
    options->insertSeparator();
    options->insertItem("Show Algorithm Information", this,  SLOT(showAlgoInfo()), Qt::CTRL + Qt::Key_G);
    options->insertSeparator();

    // Insert options the menu
    menubar->insertItem("&Options", options);
    menubar->insertSeparator();

    // Inser Help and Quit
    insertGeneralItemsInRightclickMenubar();
}

/* Extract represented.----------------------------------------------------- */
void ShowSpectraSOM::extractRepresented(MetaData &SF_represented)
{
    for (int i = 0; i < listSize; i++)
        if (cellMarks[i])
        {
            int jmax = SFcv[i].size();
            for (int j = 0; j < jmax; j++)
            {
            	SF_represented.addObject();
            	SF_represented.setValue(MDL_IMAGE, (SFcv[i])[j]);
            	SF_represented.setValue(MDL_ENABLED, 1);
            }
        }
}

void ShowSpectraSOM::extractRepresented(ClassicTrainingVectors &_v_represented)
{
    // Count the number of represented vectors
    int counter = 0;
    for (int i = 0; i < listSize; i++)
        if (cellMarks[i]) counter += SFcvs[i].size();

    // Resize the represented vectors
    _v_represented.theItems.resize(counter);
    _v_represented.theTargets.resize(counter);

    // Now copy the items indicated by SFcvs
    long myIndex = 0;
    for (long i = 0; i < listSize; i++)
        if (cellMarks[i])
        {
            int jmax = SFcvs[i].size();
            for (int j = 0; j < jmax; j++)
            {
                _v_represented.theItems[myIndex] = Vdat->theItems[(SFcvs[i])[j]];
                _v_represented.theTargets[myIndex] = Vdat->theTargets[(SFcvs[i])[j]];
                myIndex++;
            }
        }
}

/* Save assigned images ---------------------------------------------------- */
void ShowSpectraSOM::saveAssigned()
{
    MetaData SFNew;
    extractRepresented(SFNew);
    if (SFNew.size()) writeSelFile(SFNew);
    else QMessageBox::about(this, "Error!", "No images selected\n");
}

/* Change labels ----------------------------------------------------------- */
void ShowSpectraSOM::changeLabelsToImg()
{
    changeLabel(mi_imgAsLabels);
}
void ShowSpectraSOM::changeLabelsToSel()
{
    changeLabel(mi_selAsLabels);
}
void ShowSpectraSOM::changeLabelsToHis()
{
    changeLabel(mi_hisAsLabels);
}
void ShowSpectraSOM::changeLabelsToErr()
{
    changeLabel(mi_errAsLabels);
}
void ShowSpectraSOM::changeLabel(int _clicked_mi)
{
    options->setItemEnabled(mi_imgAsLabels, true);
    options->setItemEnabled(mi_selAsLabels, true);
    options->setItemEnabled(mi_hisAsLabels, true);
    options->setItemEnabled(mi_errAsLabels, true);
    options->setItemEnabled(_clicked_mi,    false);
    if (_clicked_mi == mi_imgAsLabels) labeltype = Filename_LABEL;
    else if (_clicked_mi == mi_selAsLabels) labeltype = SFLabel_LABEL;
    else if (_clicked_mi == mi_hisAsLabels) labeltype = Histogram_LABEL;
    else if (_clicked_mi == mi_errAsLabels) labeltype = Err_LABEL;
    repaintContents();
}

const char * ShowSpectraSOM::cellLabel(int i) const
{
    if (options->isItemEnabled(mi_showLabel)) return NULL;
    switch (labeltype)
    {
    case SFLabel_LABEL:
        return (selstatus[i]) ? "1" : "-1";
    case Filename_LABEL:
        return imgnames[i].c_str();
    case Histogram_LABEL:
        return hisAssigned[i].c_str();
    case Err_LABEL:
        return cv_errors[i].c_str();
    }
}

/* Show Average and SD of the represented images --------------------------- */
void ShowSpectraSOM::showRepresentedImagesStats()
{
    MetaData SFNew;
    extractRepresented(SFNew);
    if (SFNew.size()) ShowTable::showStats(SFNew, apply_geo);
    else QMessageBox::about(this, "Error!", "No images selected\n");
}

void ShowSpectraSOM::showRepresentedSpectraStats()
{
    // Extract represented vectors
    ClassicTrainingVectors V_represented(0, true);
    extractRepresented(V_represented);
    // Get Avg and SD
    ClassicTrainingVectors *myVect = new ClassicTrainingVectors(0, true);
    *myVect = V_represented.getStatVector();
    // Represent
    ShowSpectra *myST = new ShowSpectra;
    myST->initWithVectors(1, 2, myVect, "Average and SD of represented spectra");
    myST->show();
}

/* Show assigned sel ------------------------------------------------------- */
void ShowSpectraSOM::showRepresentedSel()
{
    MetaData SFNew;
    extractRepresented(SFNew);
    if (SFNew.size())
    {
        ShowSel *showsel = new ShowSel;
        showsel->initWithObject(10, 10, SFNew, "Represented images");
        showsel->show();
    }
    else QMessageBox::about(this, "Error!", "No images selected\n");
}

/* Show assigned spectra --------------------------------------------------- */
void ShowSpectraSOM::showRepresentedSpectra()
{
    ClassicTrainingVectors *V_represented = new ClassicTrainingVectors(0, true);
    extractRepresented(*V_represented);
    ShowSpectra *myST = new ShowSpectra;
    myST->initWithVectors(6, 6, V_represented, "Represented spectra");
    myST->show();
}

/* Show error spectrum ----------------------------------------------------- */
void ShowSpectraSOM::showErrorSpectrum()
{
    // Extract represented vectors
    ClassicTrainingVectors V_represented(0, true);
    int row = currentRow();
    int col = currentColumn();
    if (row < 0 || col < 0) return;
    int i = indexOf(row, col);
    int represented_images = SFcvs[i].size();
    if (represented_images == 0)
    {
        QMessageBox::about(this,
                           "Error!", "This vector does not represent any spectrum\n");
        return;
    }

    V_represented.theItems.resize(represented_images);
    V_represented.theTargets.resize(represented_images);
    for (int j = 0; j < represented_images; j++)
    {
        V_represented.theItems[j] = Vdat->theItems[(SFcvs[i])[j]];
        V_represented.theTargets[j] = Vdat->theTargets[(SFcvs[i])[j]];
    }

    // Get Avg
    ClassicTrainingVectors *myVect = new ClassicTrainingVectors(0, true);
    *myVect = V_represented.getStatVector();
    myVect->deleteRow(1);
    myVect->theTargets[0] = "Error spectrum";

    // Compute the difference with the code vector
    int jmax = myVect->theItems[0].size();
    for (int j = 0; j < jmax; j++)
        myVect->theItems[0][j] -= V->theItems[i][j];

    // Represent
    ShowSpectra *myST = new ShowSpectra;
    myST->initWithVectors(1, 1, myVect, "Error spectrum of the represented spectra");
    myST->show();
}

/* Show algorithm information ---------------------------------------------- */
void ShowSpectraSOM::showAlgoInfo()
{
    QMessageBox::about((QWidget*)this, "Algorithm Information", infStr);
}
