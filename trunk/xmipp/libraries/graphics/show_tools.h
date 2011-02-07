/***************************************************************************
 *
 * Authors:      Alberto Pascual Montano (pascual@cnb.csic.es)
 *               Carlos Oscar S. Sorzano (coss@cnb.csic.es)
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

#ifndef _SHOWTOOLS_H
#define _SHOWTOOLS_H

#include <qwidget.h>
#include <qlabel.h>
#include <qscrollbar.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qradiobutton.h>

#ifdef QT3_SUPPORT
#include <q3textedit.h>
#else
#include <qtextedit.h>
#endif

#include <data/image.h>
#include <data/fft.h>

#include <vector>
#include <string>

/**@defgroup ShowTools ShowTools
   @ingroup GraphicsLibrary */
//@{

/**Scroll param class.
    This class opens a selfWindow for asking for the Scroll parameter.
    It emits a signal called new_value(float). (precision is the number of
    digits used by the mantise)

    An example of use of this class with a single parameter is
    @code
      // Create selfWindow
      ScrollParam* param_window;
      param_window = new ScrollParam(min, max, spacing, "Set spacing", "spacing",
         0, "new selfWindow", WDestructiveClose);

      // Connect its output to my input (set_spacing)
      connect( param_window, SIGNAL(new_value(float)),
               this,         SLOT(set_spacing(float)));

      // Show
      param_window->setFixedSize(250,150);
      param_window->show();
    @endcode

    With two parameters
    @code
      // Create selfWindow
      ScrollParam* param_window;
      std::vector<float> min; min.push_back(1); min.push_back(1);
      std::vector<float> max; max.push_back(N); max.push_back(N);
      std::vector<float> initial_value;
         initial_value.push_back(spacing);
         initial_value.push_back(x_tick_off);
      std::vector<char *> prm_name;
         prm_name.push_back("spacing");
         prm_name.push_back("Tick offset");
      param_window = new ScrollParam(min,max,initial_value,prm_name,
  "Set spacing", 0, "new selfWindow", WDestructiveClose,0);

      // Connect its output to my input (set_spacing)
      connect( param_window, SIGNAL(new_value(std::vector<float>)),
              this,          SLOT(set_spacing(std::vector<float>)) );

      // Show
      param_window->setFixedSize(200,175);
      param_window->show();
    @endcode
*/
class ScrollParam : public QWidget
{
    Q_OBJECT
public:
    /** Constructor for a single scroll.
        Provide the min_value, max_value, caption and initial_value.*/
    ScrollParam(float min, float max, float initial_value, char *prm_name,
                char *caption, QWidget *parent = 0,
                const char *name = 0, int wFlags = 0, int precision = 2);

    /** Constructor for several scrolls.
        Provide the min_value, max_value, caption and initial_value.*/
    ScrollParam(std::vector<float> &min, std::vector<float> &max,
                std::vector<float> &initial_value, std::vector<char *> &prm_name,
                char *caption, QWidget *parent = 0,
                const char *name = 0, int wFlags = 0, int precision = 2);

    /** Destructor. */
    ~ScrollParam();

    /** Init. */
    void init(std::vector<float> &min, std::vector<float> &max,
              std::vector<float> &initial_value, std::vector<char *> &prm_name,
              char *caption, int precision = 2);

    /** Get current values. */
    std::vector<float> getCurrentValues();
private:
    std::vector<float>     value;
    std::vector<QLabel *>  value_lab;   // label for the current value of the slider
    std::vector<QScrollBar *> scroll;   // sliders
    int       my_precision;
private slots:
    void scrollValueChanged(int);
    void slot_close_clicked();
    void slot_ok_clicked();
signals:
    /** Signal emitted when the value is changed*/
    void new_value(float);
    /** Signal emitted when the value is changed*/
    void new_value(std::vector<float>);
    /** Signal emitted when the close button is clicked */
    void signal_close_clicked();
    /** Signal emitted when the ok button is clicked */
    void signal_ok_clicked();
};

/**Exclusive param class.
    This class opens a selfWindow for asking for a exclusive parameter.
    It emits a signal called new_value(int) with the selected value

    An example of use of this class is
    @code
       std::vector<std::string> list_values;
       list_values.push_back("Option 1");
       list_values.push_back("Option 2");
      // Create selfWindow
      ExclusiveParam* param_window=
           new ExclusiveParam(list_values, parameter, "Set this exclusive parameter",
             0, "new selfWindow", WDestructiveClose);

      // Connect its output to my input (set_spacing)
      connect( param_window, SIGNAL(new_value(int)),
               this,         SLOT(set_spacing(int)));

      // Show
      param_window->setFixedSize(250,200);
      param_window->show();
    @endcode
*/
class ExclusiveParam : public QWidget
{
    Q_OBJECT
public:
    /** Constructor.
        Provide the min_value, max_value, caption and initial_value.*/
    ExclusiveParam(std::vector<std::string> &list_values, int initial_value,
                   char *caption, QWidget *parent = 0,
                   const char *name = 0, int wFlags = 0);
    ~ExclusiveParam();

private:
    int       value;
    std::vector< QRadioButton *> button;
private slots:
    void but_close_clicked();
    void but_ok_clicked();
    void exclusiveValueChanged();
signals:
    /** Signal emitted when the value is changed*/
    void new_value(int);
};

/** Image conversions */
//@{
/** Xmipp -> QImage.*/
void xmipp2Qt(Image<double>& _ximage, QImage &_qimage,
              int _minScale = 0, int _maxScale = 255, double _m = 0, double _M = 0);

/** Qimage -> Xmipp.*/
void Qt2xmipp(QImage &_qimage, Image<double> &_ximage);

/** Xmipp -> PixMap */
void xmipp2Pixmap(Image<double> &xmippImage, QPixmap* pixmap,
                  int _minScale = 0, int _maxScale = 255, double _m = 0, double _M = 0);
//@}

/** Miscellanea */
//@{
/** Return a QPixmap with the file provided.
    Function taken from qt/src/kernel/qpixmap.cpp for compatibility reasons. */
QPixmap xmipp_qPixmapFromMimeSource(const QString &abs_name);
//@}

//@}
#endif
