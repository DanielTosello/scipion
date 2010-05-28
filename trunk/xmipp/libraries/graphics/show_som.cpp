/***************************************************************************
 *
 * Authors:     Carlos Oscar S. Sorzano (coss@cnb.csic.es)
 *              Alberto Pascual (pascual@cnb.csic.es)
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * Part of this module has been developed by Lorenzo Zampighi and Nelson Tang
 * Dept. Physiology of the David Geffen School of Medistd::cine
 * Univ. of California, Los Angeles.
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

#include "show_som.h"
#include "show_2d.h"

#include <classification/training_vector.h>

#include <qmessagebox.h>

#ifdef QT3_SUPPORT
#include <q3filedialog.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3PopupMenu>
#else
#include <qfiledialog.h>
#endif

/* Init/Clear data --------------------------------------------------------- */
void ShowSOM::init()
{
    SFcv        = NULL;
    hisAssigned = NULL;
    cv_errors   = NULL;
    infStr      = "";
    ShowSel::init();
}

void ShowSOM::clear()
{
    if (SFcv        != NULL) delete [] SFcv;
    if (hisAssigned != NULL) delete [] hisAssigned;
    if (cv_errors   != NULL) delete [] cv_errors;
    infStr = "";
    ShowSel::clear();
}

/* Initialize with a SOM file.---------------------------------------------- */
void ShowSOM::initWithFile(const FileName &_fn_root)
{
    init();
    readFile(_fn_root);
    initTable();
    initRightclickMenubar();
    repaint();
}

/* Read a SOM -------------------------------------------------------------- */
void ShowSOM::readFile(const FileName &_fn_root,
                       double _minGray, double _maxGray)
{
    clear();
    fn              = _fn_root;
    setCaption(fn.c_str());
    readSOMFiles(_fn_root);
    readSelFile(_fn_root + ".sel");
}

void ShowSOM::readSOMFiles(const FileName &_fn_root)
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
            xmippCTVectors ts(0, true);
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
            xmippCTVectors ts(0, true);
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
                REPORT_ERROR(1, "ShowSOM::readFile: Input file is empty");
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

            SFcv = new MetaData[listSize];
            while (!fh_class.eof())
            {
                int row, col;
                float tmp;
                fh_class >> col >> row >> tmp;
                getline(fh_class, line);
                int i = row * NumCols + col;
                SFcv[i].addObject();
                SFcv[i].setValue(MDL_IMAGE, (std::string)firstToken(line));
                SFcv[i].setValue(MDL_ENABLED, 1);
			}
        }
        fh_class.close();
    }
}

/* Initialize right click menubar ------------------------------------------ */
void ShowSOM::initRightclickMenubar()
{
#ifdef QT3_SUPPORT
    menubar = new Q3PopupMenu();
    Q3PopupMenu * file = new Q3PopupMenu();
#else
    menubar = new QPopupMenu();
    QPopupMenu * file = new QPopupMenu();
#endif
    file->insertItem("Open...", this,  SLOT(GUIopenFile()));
    file->insertItem("Save assigned images in a sel file...",
                     this, SLOT(saveAssigned()), Qt::CTRL + Qt::Key_N);
    file->insertItem("Save assigned images in separate sel files...",
                     this, SLOT(saveAssignedSeparately()));
    menubar->insertItem("&File", file);

    // Options .............................................................
#ifdef QT3_SUPPORT
    options =  new Q3PopupMenu();
#else
    options = new QPopupMenu();
#endif
    setCommonOptionsRightclickMenubar();

    // What kind of labels
    mi_imgAsLabels = options->insertItem("Show Image Names as Labels", this,  SLOT(changeLabelsToImg()));
    mi_selAsLabels = options->insertItem("Show Sel status as Labels", this,  SLOT(changeLabelsToSel()));
    mi_hisAsLabels = options->insertItem("Show Histogram as Labels", this, SLOT(changeLabelsToHis()));
    mi_errAsLabels = options->insertItem("Show Quantization Errors as Labels", this, SLOT(changeLabelsToErr()));
    options->setItemEnabled(mi_imgAsLabels, true);
    options->setItemEnabled(mi_selAsLabels, true);
    options->setItemEnabled(mi_hisAsLabels, false);
    options->setItemEnabled(mi_errAsLabels, true);
    labeltype = Histogram_LABEL;
    options->insertSeparator();

    // Statistics
    options->insertItem("View average and SD of the Selected Codevectors",  this,  SLOT(showStats()));
    options->insertItem("View average and SD of the Assigned Images", this,  SLOT(showRepresentedStats()));
    options->insertItem("View average of the Assigned Images together", this, SLOT(showRepresentedAverageTogether()));
    options->insertItem("View assigned images", this,  SLOT(showRepresentedSel()), Qt::CTRL + Qt::Key_A);
    options->insertItem("View error Image", this,  SLOT(showErrorImage()), Qt::CTRL + Qt::Key_E);
    options->insertSeparator();
    options->insertItem("Show Algorithm Information", this,  SLOT(showAlgoInfo()), Qt::CTRL + Qt::Key_G);
    options->insertSeparator();

    // Insert options the menu
    menubar->insertItem("&Options", options);
    menubar->insertSeparator();

    // Inser Help and Quit
    insertGeneralItemsInRightclickMenubar();
}

/* Extract represented .---------------------------------------------------- */
void ShowSOM::extractRepresented(MetaData &SF_represented)
{
	for (int i = 0; i < listSize; i++)
    {
		if (cellMarks[i])
            SF_represented.unionDistinct(SFcv[i], MDL_IMAGE);
    }
}

void ShowSOM::saveAssigned()
{
    MetaData SFNew;
    extractRepresented(SFNew);
    if (SFNew.size() != 0) writeSelFile(SFNew);
    else QMessageBox::about(this, "Error!", "No images selected\n");
}

void ShowSOM::saveAssignedSeparately()
{
    QString basename;
    for (int i = 0; i < listSize; i++)
        if (cellMarks[i] && SFcv[i].size() > 0)
        {
            if (basename.isNull())
            {
                basename =
#ifdef QT3_SUPPORT
                    Q3FileDialog::getSaveFileName(QString::null, QString::null,
#else
                    QFileDialog::getSaveFileName(QString::null, QString::null,
#endif
                                                 this, "Save images",
                                                 "Enter base filename for sel files");
                if (basename.isEmpty())
                {
                    QMessageBox::about(this, "Warning!", "Saving aborted\n");
                    return;
                }
            }
            int r, c;
            IndextoPos(i, r, c);
            char buf[50];
            sprintf(buf, "_row%d_col%d", r + 1, c + 1);
            QString newfilename = basename + (QString) buf + ".sel";
            QFileInfo fi(newfilename);
            if (fi.exists() &&
                (QMessageBox::information(this, "File Exists",
                                          "File " + newfilename +
                                          " already exists. Overwrite?",
                                          "Yes", "No") != 0))
            {
                QMessageBox::about(this, "Warning!",
                                   "Skipping file " + newfilename + ".");
                continue;
            }
            SFcv[i].write(newfilename.ascii());
        }
    if (basename.isNull())
        QMessageBox::about(this, "Error!", "No images selected\n");
}



/* Change labels ----------------------------------------------------------- */
void ShowSOM::changeLabelsToImg()
{
    changeLabel(mi_imgAsLabels);
}
void ShowSOM::changeLabelsToSel()
{
    changeLabel(mi_selAsLabels);
}
void ShowSOM::changeLabelsToHis()
{
    changeLabel(mi_hisAsLabels);
}
void ShowSOM::changeLabelsToErr()
{
    changeLabel(mi_errAsLabels);
}
void ShowSOM::changeLabel(int _clicked_mi)
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

const char * ShowSOM::cellLabel(int i) const
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
void ShowSOM::showRepresentedStats()
{
    MetaData SFNew;
    extractRepresented(SFNew);
    if (SFNew.size()) ShowTable::showStats(SFNew, apply_geo);
    else QMessageBox::about(this, "Error!", "No images selected\n");
}

/* Show Average of the represented images together ------------------------- */
void ShowSOM::showRepresentedAverageTogether()
{
    unsigned int numMarked = 0;

    // Count how many are marked first
    for (int i = 0; i < listSize; i++)
    {
        if (cellMarks[i])
            numMarked++;
    }
    if (numMarked == 0)
    {
        QMessageBox::about(this, "Error!", "No images selected\n");
        return;
    }

    //std::vector <std::string> cell_labels;
    QPixmap *pixmap = new QPixmap[numMarked];
    if (!pixmap)
    {
        QMessageBox::about(this, "Error!", "Out of memory creating new pixmap!\n");
        return;
    }

    // Create a blank Selfile for the averages images.
    MetaData SFAvgs;

    // Go back through cells and add images to list
    for (int i = 0; i < listSize; i++)
    {
        if (cellMarks[i])
        {
            Image<double> _ave;
            if (SFcv[i].size())
            {
                Image<double> _sd;
                double _minPixel, _maxPixel;
                MetaData SFnew(SFcv[i]);
                get_statistics(SFnew, _ave, _sd, _minPixel, _maxPixel, apply_geo);
            }
            else
                _ave = Image<double>(projXdim, projYdim);

            // Write _ave to a temp file
            // Need to convert it to Image<double> because Selfile can't handle plain
            // Image files when it is being read back in.
            Image<double> xm_ave;
            xm_ave = _ave;
            int tempfd;
            std::string tmpImgfile = makeTempFile(tempfd);
            // Add that image file to the SelFile and save it
            xm_ave.write(tmpImgfile);
            SFAvgs.addObject();
            SFAvgs.setValue(MDL_IMAGE,tmpImgfile);
            SFAvgs.setValue(MDL_ENABLED,1);
            ::close(tempfd);
            /*
            int r, c;
            IndextoPos (i, r, c);
            char buf[20];
            sprintf (buf, "row %d col %d", r, c);
            cell_labels.push_back (buf);
            */
        }
    }

    if (SFAvgs.size())
    {
        ShowSel *showsel = new ShowSel;
        showsel->initWithObject(NumRows, NumCols, SFAvgs, "Averages of assigned images");
        //for (u = 0; u < numMarked; u++)
        //  v->setCellLabel (u, hisLabels[u], cell_labels[u].c_str());
        showsel->show();
    }
    else
        QMessageBox::about(this, "Error!", "No images selected\n");
}

/* Show assigned sel ------------------------------------------------------- */
void ShowSOM::showRepresentedSel()
{
    MetaData SFNew;
    extractRepresented(SFNew);
    if (SFNew.size())
    {
        ShowSel *showsel = new ShowSel;
        showsel->initWithObject(10, 10, SFNew, "Represented images");
        showsel->apply_geo = apply_geo;
        showsel->show();
    }
    else QMessageBox::about(this, "Error!", "No images selected\n");
}

/* Show error image -------------------------------------------------------- */
void ShowSOM::showErrorImage()
{
    int row = currentRow();
    int col = currentColumn();
    if (row < 0 || col < 0) return;
    int i = indexOf(row, col);

    MetaData SFNew;
    SFNew.unionDistinct(SFcv[i], MDL_IMAGE);
    if (SFNew.size())
    {
        // Compute the average of the images assigned to that cell
        Image<double> _ave, _sd;
        double _minPixel, _maxPixel;
        get_statistics(SFNew, _ave, _sd, _minPixel, _maxPixel, apply_geo);

        // Load the cell code vector
        Image<double> image, error_image;
        image.read(imgnames[i]);
        // Compute and show the error
        error_image() = _ave() - image();
        ImageViewer *w = new ImageViewer(&error_image, "Error image");
        w->show();
	}
    else
        QMessageBox::about(this, "Error!", "No images selected\n");
}

/* Show algorithm information ---------------------------------------------- */
void ShowSOM::showAlgoInfo()
{
    QMessageBox::about((QWidget*)this, "Algorithm Information", infStr);
}
