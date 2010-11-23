/***************************************************************************
 *
 * Authors:     Carlos Oscar S. Sorzano (coss@cnb.csic.es)
 *              Carlos Manzanares       (cmanzana@cnb.csic.es)
 *              Arun Kulshreshth        (arun_2000_iitd@yahoo.com)
 *              Enrique Recarte Llorens (erecallo@hotmail.com)
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

#ifndef __QT_WIDGET_MICROGRAPH_HH__
#define __QT_WIDGET_MICROGRAPH_HH__

#include <qwidget.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qmenubar.h>
#include <qscrollbar.h>
#include <qlabel.h>
#include <qlineedit.h>

#ifdef QT3_SUPPORT
#include <q3accel.h>
//Added by qt3to4:
#include <Q3PopupMenu>
#include <Q3VBoxLayout>
#else
#include <qaccel.h>
#endif

#include "image_micrograph.h"
#include "image_overview_micrograph.h"
#include "file_menu.h"

#include <data/mask.h>
#include <classification/naive_bayes.h>

#include <vector>

/* Forward declarations ---------------------------------------------------- */
class QtMainWidgetMark;
class QtImageMicrograph;
class QtPopupMenuMark;
class QtFiltersController;
class Micrograph;

/* Particle ---------------------------------------------------------------- */
class Particle
{
public:
    int x, y;             // position in micrograph
    int idx;              // Index of this particle within the micrograph
                          // list of coordinates
    char status;          // rejected=0, selected=1 or moved=2
    Matrix1D<double> vec; // vector of that particle
    double cost;          // Associated cost

    // Print
    friend std::ostream & operator << (std::ostream &_out, const Particle &_p);

    // Read
    void read(std::istream &_in, int _vec_size);
};

struct SAscendingParticleSort
{
    bool operator()(const Particle& rpStart, const Particle& rpEnd)
    {
        return rpStart.cost < rpEnd.cost;
    }
};

/* Classification model ---------------------------------------------------- */
class Classification_model
{
public:
    // Maximum number of elements per training class
    int __maxTrainingVectors;

    // Example vectors
    std::vector< std::vector< Particle > >        __training_particles;
    int                                           __classNo;
    int                                           __micrographs_number;
    std::vector<int>                              __micrographs_scanned;
    std::vector<int>                              __particles_picked;
    std::vector<int>                              __falsePositives;

public:
    NaiveBayes *                                  __bayesNet;
    EnsembleNaiveBayes *                          __bayesEnsembleNet;
    int                                           __bayesClassifier;

public:
    // Constructor
    Classification_model(int _classNo=3, int _maxTrainingVectors=20000)
    {
        __maxTrainingVectors=_maxTrainingVectors;
        init(_classNo);
    }

    // Clear
    void clear();

    // Is empy
    bool isEmpty()
    {
        return __micrographs_number==0;
    }

    // Initialize
    void init(int _classNo=3)
    {
        __classNo = _classNo;
        __training_particles.resize(__classNo);
        __micrographs_number = 0;
        __falsePositives.resize(0);
    }

    // Different additions
    void addMicrographScanned(int micrographScanned)
    {
        __micrographs_scanned.push_back(micrographScanned);
    }

    bool addParticleTraining(const Particle &p, int classIdx)
    {
    	if (__training_particles[classIdx].size()<__maxTrainingVectors)
    	{
    		__training_particles[classIdx].push_back(p);
    		return true;
    	}
    	return false;
    }

    void addParticlePicked(int particlePicked)
    {
        __particles_picked.push_back(particlePicked);
    }

    void addFalsePositives(int falsePositives)
    {
        __falsePositives.push_back(falsePositives);
    }

    void addMicrographItem()
    {
        __micrographs_number++;
    }

    // Import classification model
    void import_model(const Classification_model &_model);

    // Is a particle?
    int isParticle(const Matrix1D<double> &new_features, double &cost)
    {

        MultidimArray<double> features(new_features);

        int retval;
        if (__bayesClassifier==0)
        {
            if (__bayesNet->doInference(features,cost)==0)
                retval=1;
        }
        else
        {
            MultidimArray<int> votes;
            __bayesEnsembleNet->doInference(features,cost,votes);
            retval=votes(0);
        }
        return retval;
    }

    //init the naive bayesian network
    void initNaiveBayes(const std::vector < MultidimArray<double> >
                        &features, const Matrix1D<double> &probs,
                        int discreteLevels, double penalization);

    //init the naive bayesian network
    void initNaiveBayesEnsemble(const std::vector < MultidimArray<double> >
                                &features, const Matrix1D<double> &probs,
                                int discreteLevels, double penalization,
                                int numberOfClassifiers,
                                double samplingFeatures, double samplingIndividuals,
                                const std::string &newJudgeCombination);

    // Print
    friend std::ostream & operator << (std::ostream &_out,
                                       const Classification_model &_m);

    // Print Shape
    void printShape() const;

    // Read
    friend std::istream & operator >> (std::istream &_in,
                                       Classification_model &_m);
};

/* Automatic particle picking ---------------------------------------------- */
/** Class to perform the automatic particle picking */
class AutoParticlePicking
{
public:
    Micrograph                *__m;
    FileName                   __modelRootName;
    int                        __numThreads;
    bool                       __learn_particles_done;
    bool                       __autoselection_done;
    MaskProgram                __mask;
    Classification_model       __training_model;
    Classification_model       __training_loaded_model;
    Classification_model       __selection_model;
    Classification_model       __selection_model2;
    Classification_model       __selection_model3;
    int                        __auto_label;
    int                        __gray_bins;
    int                        __radial_bins;
    double                     __highpass_cutoff;
    double                     __penalization;
    float                      __minCost;
    int                        __piece_xsize;
    int                        __particle_radius;
    int                        __mask_size;
    int                        __min_distance_between_particles;
    int                        __output_scale;
    int                        __reduction; // Of the piece with respect
    // to the micrograph
    int                        __piece_overlap;
    int                        __scan_overlap;
    int                        __learn_overlap;
    int                        __classNo;
    std::vector<Particle>      __auto_candidates;
    std::vector<Particle>      __rejected_particles;
    bool                       __is_model_loaded;
    std::vector < MultidimArray<int> * >    __mask_classification;
    std::vector < MultidimArray<int> * >    __radial_val;
    std::vector < MultidimArray<double> * > __sector;
    std::vector < MultidimArray<double> * > __ring;
    std::vector < MultidimArray<int> * >    __Nsector;
public:
    /// Empty constructor
    AutoParticlePicking(Micrograph *_m);

    // Configure auto
    void configure_auto(int _ellipse_radius);

    // Set the number of threads
    void setNumThreads(int _numThreads);

    // Learn particles
    void learnParticles(int _ellipse_radius);

    // Build Selection model. The selection model is made up of the training
    // and the automatically selected particles.
    void buildSelectionModel();

    // Create mask for learning particles
    void createMask();

    // Classify mask
    void classifyMask();

    // Build vectors
    void buildVectors(std::vector<int> &_idx, Classification_model &_model);

    // Build vector from non particles
    void buildNegativeVectors(Classification_model &__model,
                              bool checkForPalsePostives);

    // Build classfication vector
    // x,y are in the coordinate system of the piece (that might be
    // a reduced version of a piece in the micrograph)
    // (0,0) is the top-left corner
    // Returns true if the vector is successfully built
    bool build_vector(const MultidimArray<int> &piece,
                      const MultidimArray<double> &original_piece,
                      int _x, int _y, Matrix1D<double> &_result);

    // Get a piece of the micrograph centered at position x,y (if possible)
    // the position of (x,y) in the piece is returned in (posx, posy)
    void get_centered_piece(MultidimArray<double> &piece,
                            int _x, int _y, int &_posx, int &_posy);

    // Get a piece whose top-left corner is at the desired position (if possible)
    // Returns true if the piece could be taken, and false if the whole
    // micrograph has been scanned. To scan the full micrograph,
    // Top and left should be initialized to 0,0 and this function should
    // succesive times. It returns the next top and left coordinates to
    // get the next piece along with the skips. The skips indicate which
    // part of the piece has been already scanned. This happens towards the
    // right and bottom boundaries of the micrograph, where the pieces
    // need to be shifted in order to fit with the required size.
    // The overlap parameter defines what piece overlap we want.
    bool get_corner_piece(MultidimArray<double> &piece,
                          int _top, int _left, int _skip_y,
                          int &_next_skip_x, int &_next_skip_y, int &_next_top,
                          int &_next_left, int overlap, bool copyPiece);

    // Denoise, reject outliers and equalize histogram
    // Returns true, if successful. False if unsuccessful (skip this piece)
    // Usually, it is unsuccessful if the denoising fails to work because
    // some "weird" features of the piece
    bool prepare_piece(MultidimArray<double> &piece,
    				   MultidimArray<int> &ipiece,
                       MultidimArray<double> &original_piece);

    //To get the neighbours of the particle at position (x,y) in the micrograph
    // (with actual coordinates in the piece posx,posy)
    // and their positions in the piece image
    void find_neighbour(const MultidimArray<double> &piece,
                        std::vector<int> &_idx, int _index,
                        int _x, int _y,
                        int _posx, int _posy, MultidimArray<char> &_visited,
                        std::vector< Matrix1D<int> > &_nbr);

    // Automatically Select Particles
    // Returns the number of particles selected.
    int automaticallySelectParticles();

    // check if there are any particles in the actual scanning position
    bool anyParticle(int posx, int posy, int rect_size);

    // Given a current scanning position, this function returns
    // the next scanning position whithin the current piece.
    // The skips are given by get_corner_piece.
    // It returns true, if the next scanning position can be computed.
    // Otherwise, if the piece has been completely scanned, it returns false
    // Initialize _x,_y to 0,0 to scan the full piece (even if there are skips).
    // The overlap parameter defines what particle overlap we want.
    bool get_next_scanning_pos(
        const MultidimArray<double> &piece,
        int &_x, int &_y, int _skip_x, int _skip_y, int overlap);

    // Run over the list sorted by distances. If two particles are within
    // a given distance then either reject both or the one with largest distance
    // depending upon _reject_both. This function returns the number of particles
    // that are still candidates.
    int reject_within_distance(
        std::vector<Particle> &_Input, double _min_dist, bool _reject_both);

    // Refine the position of a particle within the current piece
    /*
    void refine_center(Particle &my_P);
    */

    // load models with a name
    void loadModels(const FileName &fn);

    // Save models
    void saveModels(const FileName &fn);

    // Save automatically selected particles
    void saveAutoParticles();

    // Get Features of the classification model
    void produceFeatures(const Classification_model &_model,
                         std::vector < MultidimArray<double> > &_features);

    // Get classes probabilities
    void produceClassesProbabilities(const Classification_model &_model,
                                     Matrix1D<double> &probabilities);

    // Get false positives automatically selected
    void getAutoFalsePositives(Classification_model &_training_model);

    // Get true positives automatically selected
    void getAutoTruePositives(Classification_model &_training_model);

    // Restrict selection
    void restrictSelection(float _cost);

    // Move particle.
    // The input index is the index of the moved particle in the micrograph list
    void move_particle(int _idx);

    // Delete particle.
    // The input index is the index of the moved particle in the micrograph list
    void delete_particle(int _idx);
};

// AutomaticallySelectThreadParams
struct AutomaticallySelectThreadParams
{
    AutoParticlePicking *autoPicking;
    int idThread;
};

// Automatically Select Particles thread
void * automaticallySelectParticlesThread(void *);

/* Widget for the micrograph ----------------------------------------------- */
class QtWidgetMicrograph : public QWidget
{
    Q_OBJECT

public:
    Micrograph                *__m;
    QtFiltersController       *__filtersController;
    int                        __activeFamily;
    QMenuBar                  *__menuBar;
    QtImageMicrograph         *__mImage;
    QtImageOverviewMicrograph *__mImageOverview;
#ifdef QT3_SUPPORT

    Q3VBoxLayout               *__gridLayout;
#else

    QVBoxLayout               *__gridLayout;
#endif

    QtFileMenu                *__file_menu;
    bool                       __tilted;
    int                        __mingray;
    int                        __maxgray;
    float                      __gamma;
    float                      __ellipse_radius;
    int                        __ellipse_type;
    AutoParticlePicking       *__autoPicking;
public:
    // Constructor
    QtWidgetMicrograph(QtMainWidgetMark *_mainWidget,
                       QtFiltersController *_f,
                       Micrograph *_m = NULL);
    ~QtWidgetMicrograph();

    // Open all windows
    void openAllWindows();

    // Set Micrograph
    void setMicrograph(Micrograph *_m);

    // Get Micrograph
    Micrograph *getMicrograph()
    {
        return(__m);
    }

    // Set this as tilted micrograph
    void setTilted()
    {
        __tilted = TRUE;
        __mImage->setTilted();
    }

    // Is tilted?
    bool isTilted()
    {
        return __tilted;
    }

    // Set Automatic Particle Picking
    void setAutoParticlePicking(AutoParticlePicking *_autoPicking);

    // Set Automatic Particle Picking
    AutoParticlePicking * getAutoParticlePicking() const;

    // Get filters controller
    QtFiltersController *getFiltersController()
    {
        return(__filtersController);
    }

    // Get active family
    int activeFamily()
    {
        return(__activeFamily);
    }

    // Get overview
    QtImageOverviewMicrograph *overview()
    {
        return(__mImageOverview);
    }

    // Get Image
    QtImageMicrograph *image()
    {
        return(__mImage);
    }

    // Get Filemenu
    QtFileMenu *file_menu()
    {
        return __file_menu;
    }

    // Add menu item
    void addMenuItem(const char *_msg, const QtPopupMenuMark *_item)
    {
#ifdef QT3_SUPPORT
        __menuBar->insertItem(_msg, (Q3PopupMenu*)_item);
#else

        __menuBar->insertItem(_msg, (QPopupMenu*) _item);
#endif

    }

    // Draw axis
    void draw_axis(double _ang)
    {
        __mImageOverview->enableAxis();
        __mImageOverview->draw_axis(_ang);
    }

    // Open menu.
    // Add your menus to this function
    void openMenus();

    // Change contrast
    void changeContrast(int _mingray, int _maxgray, float _gamma);

    // Change mark type
    void changeMarkType(int _type);

    // Change circle radius
    void changeCircleRadius(float _circle_radius);

    // Repaint
    void repaint();

    // Add family.
    // The family label is returned
    int add_family(std::vector<Particle> &_list,
                   const std::string &_family_name);

    // Learn particles
    void learnParticles();

    // Automatically Select Particles
    void automaticallySelectParticles();

    // Save models
    void saveModels(bool askFilename);

public slots:
    void slotActiveFamily(int _f);
    void slotAddFamily(const char *_familyName);
    void slotDeleteMarkOther(int _coord);
    void slotRepaint()
    {
        repaint();
    }
    void slotDrawEllipse(int _x, int _y, int _f);
    void slotDrawLastEllipse(int _x, int _y, int _f);
    void slotQuit();
    void slotChangeContrast();
    void slotChangeCrop();
    void slotChangeCircleRadius();
    void slotRestrictSelection(float _cost);
signals:
    void signalActiveFamily(int _f);
    void signalAddFamily(const char *_familyName);
    void signalRepaint();
};

/** Class to adjust contrast
*/
class AdjustContrastWidget : public QWidget
{
    Q_OBJECT
public:
    /** Constructor */
    AdjustContrastWidget(int min, int max, float gamma,
                         QtWidgetMicrograph *_qtwidgetmicrograph,
                         QWidget *parent = 0, const char *name = 0, int wflags = 0);
private:
    QtWidgetMicrograph *__qtwidgetmicrograph;
    QScrollBar        *__scroll_min;
    QScrollBar        *__scroll_max;
    QScrollBar        *__scroll_gamma;
    QLabel            *__label_min;
    QLabel            *__label_max;
    QLabel            *__label_gamma;
private slots:
    void scrollValueChanged(int);
};

/** Class to adjust contrast
*/
class CropWidget : public QWidget
{
    Q_OBJECT
public:
    /** Constructor */
    CropWidget(QtWidgetMicrograph *_qtwidgetmicrograph,
               QWidget *parent = 0, const char *name = 0, int wflags = 0);

    /** Destructor */
    ~CropWidget();
private:
    QtWidgetMicrograph      *__qtwidgetmicrograph;
    std::vector < QScrollBar * >  __scroll;
    std::vector < QLabel * >      __label;
    QLineEdit               *__outputNameLineEdit;
private slots:
    void scrollValueChanged(int);
    void accept();
    void cancel();
signals:
    void new_value(std::vector<int>);
};

/** Class to adjust circle radius
*/
class AdjustCircleRadiustWidget : public QWidget
{
    Q_OBJECT
public:
    /** Constructor */
    AdjustCircleRadiustWidget(int min, int max, int start_with,
                              QtWidgetMicrograph *_qtwidgetmicrograph,
                              QWidget *parent = 0, const char *name = 0, int wflags = 0);
private:
    QtWidgetMicrograph *__qtwidgetmicrograph;
    QScrollBar        *__scroll_radius;
    QLabel            *__label_radius;

private slots:
    void scrollValueChanged(int);
};
#endif
