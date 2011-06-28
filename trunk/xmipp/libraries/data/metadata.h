/***************************************************************************
 *
 * Authors:     J.R. Bilbao-Castro (jrbcast@ace.ual.es)
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

#ifndef METADATA_H
#define METADATA_H

#include <map>
#include <vector>
#include <iostream>
#include <iterator>
#include <sstream>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <sstream>
#include <strings.h>
#include "funcs.h"
#include "strings.h"
#include "metadata_sql.h"

/** @defgroup MetaData Metadata Stuff
 * @ingroup DataLibrary
 * @{
 */

static double zeroD=0.;
static double oneD=1.;
static bool  falseb=false;

#define BAD_OBJID 0
#define BAD_INDEX -1

/** Write mode
 */
typedef enum
{
    MD_OVERWRITE, //forget about the old file and overwrite it
    MD_APPEND,    //append a data_ at the file end or replace an existing one
} WriteModeMetaData;
/** Iterate over all elements in MetaData
 *
 * This macro is used to generate loops over all elements in the MetaData.
 * At each iteration the 'active object' is changed so you can perform
 * the set and get default method on the MetaData.
 *
 * @code
 * MetaData md;
 *   //...
 * FOR_ALL_OBJECTS_IN_METADATA(md)
 * {
 *     String imageFile;
 *     md.getValue(MDL_IMAGE, imageFile);
 *     std::cout << "Image file: " << imageFile << " ";
 * }
 * @endcode
 */

#define FOR_ALL_OBJECTS_IN_METADATA(__md) \
        for(MDIterator __iter(__md); __iter.hasNext(); __iter.moveNext())

/** Iterate over all elements of two MetaData at same time.
 *
 * This macro is useful to iterate over two MetaData with the same
 * number of elements and performs operations to elements in both of them.
 * At each iteration the 'active objects' in both MetaData are changed.
 *
 * @code
 * MetaData mdA, mdB, mdC;
 *  //Iterate over MetaData mdA and mdB
 *  //take image from the first and tilt angle from the second
 *  //and create a new MetaData.
 * FOR_ALL_OBJECTS_IN_METADATA2(mdA, mdB)
 * {
 *     String imageFile;
 *     double angle;
 *     mdA.getValue(MDL_IMAGE, imageFile,__iter.objId);
 *     mdB.getValue(MDL_ANGLE_TILT, angle,__iter.objId);
 *     size_t objId=mdC.addObject();
 *     mdC.setValue(MDL_IMAGE, imageFile,objId);
 *     mdC.setValue(MDL_ANGLE_TILT, angle,objId);
 * }
 * @endcode
 */

#define FOR_ALL_OBJECTS_IN_METADATA2(__md, __md2) \
        for(MDIterator __iter(__md), __iter2(__md2);\
             __iter.hasNext() && __iter2.hasNext(); \
             __iter.moveNext(), __iter2.moveNext())



class MDQuery;
class MDSql;
class MDIterator;
class MDValueGenerator;

/** Class to manage data files.
 *
 * The MetaData class manages all procedures related to
 * metadata. MetaData is intended to group toghether old
 * Xmipp specific files like Docfiles, Selfiles, etc..
 *
 */
class MetaData
{
protected:
    // Allows a fast search for pairs where the value is
    // a string, i.e. looking for filenames which is quite
    // usual
    std::map<String, size_t> fastStringSearch;
    MDLabel fastStringSearchLabel;

    String path; ///< A parameter stored on MetaData Files
    String comment; ///< A general comment for the MetaData file

    bool isColumnFormat; ///< Format for the file, column or row formatted
    int precision;
    /**Input file name
     * Where does this MetaData come from/go to be stored?
     */
    FileName inFile;

    /** What labels have been read from a docfile/metadata file
     * and/or will be stored on a new metadata file when "save" is
     * called
     **/
    std::vector<MDLabel> activeLabels;

    /** When reading a column formated file, if a label is found that
     * does not exists as a MDLabel, it is ignored. For further
     * file processing, such columns must be ignored and this structure
     * allows to do that
     **/
    std::vector<unsigned int> ignoreLabels;

    /** This variables should only be used by MDSql
     * for handling db status of metadata
     */
    /** The table id to do db operations */
    MDSql * myMDSql;

    /** Init, do some initializations tasks, used in constructors
     * @ingroup MetaDataConstructors
     */
    void init(const std::vector<MDLabel> *labelsVector = NULL);

    /** Copy info variables from another metadata
     * @ingroup MetaDataConstructors
     */
    void copyInfo(const MetaData &md);

    /** Copy all data from another metadata
     * @ingroup MetaDataConstructors
     */
    void copyMetadata(const MetaData &md, bool copyObjects = true);

    /** This have the same logic of the public one,
     * but doesn't perform any range(wich implies do a size()) checks.
     */
    void _selectSplitPart(const MetaData &mdIn,
                          int n, int part, size_t mdSize,
                          const MDLabel sortLabel);

    /** This function is for generalizate the sets operations
     * of unionDistinct, intersection, substraction
     * wich can be expressed in terms of
     * ADD, SUBSTRACT of intersection part
     */
    void _setOperates(const MetaData &mdIn, const MDLabel label, SetOperation operation);
    void _setOperates(const MetaData &mdInLeft, const MetaData &mdInRight, const MDLabel label, SetOperation operation);
    /** clear data and table structure */
    void _clear(bool onlyData=false);

    /** Some private reading functions */
    void _readColumns(std::istream& is, std::vector<MDObject*> & columnValues,
                      const std::vector<MDLabel>* desiredLabels = NULL);
    void _readRows(std::istream& is, std::vector<MDObject*> & columnValues, bool useCommentAsImage);
    /** This function will be used to parse the rows data in START format
     * @param[out] columnValues MDRow with values to fill in
     * @param pchStar pointer to the position of '_loop' in memory
     * @param pEnd  pointer to the position of the next '_data' in memory
     */
    void _readRowsStar(std::vector<MDObject*> & columnValues, char * pchStart, char * pEnd);
    void _readRowFormat(std::istream& is);

public:
    /** @name Constructors
     *  @{
     */

    /** Empty Constructor.
     *
     * The MetaData is created with no data stored on it. You can fill in it programmatically
     * or by a later reading from a MetaData file or old Xmipp formatted type.
     * if labels vectors is passed this labels are created on metadata
     */
    MetaData();
    MetaData(const std::vector<MDLabel> *labelsVector);

    /** From File Constructor.
     *
     * The MetaData is created and data is read from provided FileName. Optionally, a vector
     * of labels can be provided to read just those required labels
     */
    MetaData(const FileName &fileName, const std::vector<MDLabel> *desiredLabels = NULL);

    /** Copy constructor
     *
     * Created a new metadata by copying all data from an existing MetaData object.
     */
    MetaData(const MetaData &md);

    /** Assignment operator
     *
     * Copies MetaData from an existing MetaData object.
     */
    MetaData& operator =(const MetaData &md);


    /** Destructor
     *
     * Frees all used memory and destroys object.
     */
    ~MetaData();

    /**Clear all data
     */
    void clear();
    /** @} */

    /** @name Getters and setters
     * @{
     */

    /**Get column format info.
     */
    bool getColumnFormat() const;

    /**Set precision (number of decimal digits) use by operator == when comparing
     * metadatas with double data. "2" is a good value for angles
     */
    bool setPrecission(int _precision)
    {
        precision= pow (10,_precision);
    }

    /** Set to false for row format (parameter files).
     *  set to true  for column format (this is the default) (docfiles)
     */
    void setColumnFormat(bool column);

    /** Check if the file (not the object) is in column format
     *  returns pointer do first two data_entries and firts loop
     */
    bool isColumnFormatFile(char * map, size_t mapSize,
                            char ** firstData,
                            char ** secondData,
                            char ** firstloop,
                            const char * blockName, size_t blockNameSize);
    /** Export medatada to xml file.
     *
     */
    void convertXML(FileName fn);

    /* Helper function to parse an MDObject and set its value.
     * The parsing will be from an input stream(istream)
     * and if parsing fails, an error will be raised
     */
    void _parseObject(std::istream &is, MDObject &object, size_t id = BAD_OBJID);

    /** Get Metadata labels for the block defined by start
     * and end loop pointers. Return pointer to newline after last label
     */
    char * _readColumnsStar(char * start,
                            char * end,
                            std::vector<MDObject*> & columnValues,
                            const std::vector<MDLabel>* desiredLabels, size_t id = BAD_OBJID);
    /**Get path.
     */
    String getPath() const ;

    /**Set Path.
     * the path will appear in first line
     */
    void setPath(const String &newPath = "");

    /**Get Header Comment.
     * the comment will appear in second line.
     */
    String getComment() const;

    /**Set Header Comment.
     * the comment will appear in second line
     */
    void setComment(const String &newComment = "No comment");

    /**Get metadata filename.
     */
    FileName getFilename() const;

    /**Set metadata filename.
     */
    void setFilename(const FileName &_filename);

    /**Get safe access to active labels.
     */
    std::vector<MDLabel> getActiveLabels() const;

    /**Get unsafe pointer to active labels.
     */
    std::vector<MDLabel> *getActiveLabelsAddress() const;

    /**Get maximum string length of column values.
    */
    int getMaxStringLength( const MDLabel thisLabel) const;

    /** @} */

    /** @name MetaData Manipulation
     * @{
     */


    /** Set the value of all objects in an specified column (both value and colum are specified in mdValueIn)
    */
    bool setValueCol(const MDObject &mdValueIn);

    /**Set the value of all objects in an specified column.
     * @code
     * MetaData md;
     * md.setValueCol(MDL_IMAGE, "images/image00011.xmp");
     * @endcode
     */

    template<class T>
    bool setValueCol(const MDLabel label, const T &valueIn)
    {
        return setValueCol(MDObject(label, valueIn));
    }

    /** Set the value for some label.
     * to the object that has id 'objectId'
     * or to 'activeObject' if is objectId=-1.
     * This is one of the most used functions to programatically
     * fill a metadata.
     * @code
     * MetaData md;
     * size_t id = md.addObject();
     * md.setValue(MDL_IMAGE, "images/image00011.xmp",id);
     * md.setValue(MDL_ANGLE_ROT, 0.,id);
     * @endcode
     */
    template<class T>
    bool setValue(const MDLabel label, const T &valueIn, size_t id)
    {
        return setValue(MDObject(label, valueIn), id);
    }
    //private:
    /** This functions are using MDObject for set real values
     * there is an explicit function signature
     * foreach type supported in Metadata.
     * This is done for some type checking of Metadata labels
     * and values
     */
    bool setValue(const MDObject &mdValueIn, size_t id);
    bool getValue(MDObject &mdValueOut, size_t id) const;
    /** Filename used in the read command, useful to write Error messages
     *
     */
    FileName eFilename;

    //public:

    /** Get the value of some label.
     * from the object that has id 'objectId'
     * or from 'activeObject' if objectId=-1.
     * @code
     * MetaData md;
     * md.read("images.xmd");
     * FileName imageFn;     *
     * FOR_ALL_OBJECTS_IN_METADATA(md)
     * {
     *      md.getValue(MDL_IMAGE, imageFn);
     *      std::out << "Image: " << imageFn);
     * }
     * @endcode
     */
    template<class T>
    bool getValue(const MDLabel label, T &valueOut, size_t id) const
    {
        MDObject mdValueOut(label);
        if (!getValue(mdValueOut, id))
            return false;
        mdValueOut.getValue(valueOut);
        return true;
    }

    /** Get all values of a column as a vector.
     */
    template<class T>
    void getColumnValues(const MDLabel label, std::vector<T> &valuesOut) const
    {
        T value;
        MDObject mdValueOut(label);
        std::vector<size_t> objectsId;
        findObjects(objectsId);
        size_t n = objectsId.size();
        valuesOut.resize(n);
        for (size_t i = 0; i < n; ++i)
        {
            getValue(mdValueOut, objectsId[i]);
            mdValueOut.getValue(value);
            valuesOut[i] = value;
        }

    }

    /** Get all values of an MetaData row of an specified objId*/
    bool getRow(MDRow &row, size_t id) const;

    /** Copy all the values in the input row in the current metadata*/
    void setRow(const MDRow &row, size_t id);

    /** Add a new Row and set values, return the objId of newly added object */
    size_t addRow(const MDRow &row);

    /** Set label values from string representation.
     */
    bool setValueFromStr(const MDLabel label, const String &value, size_t id);

    /** Get string representation from label value.
     */
    bool getStrFromValue(const MDLabel label, String &strOut, size_t id) const;

    /**Check whether the metadata is empty.
     */
    bool isEmpty() const;

    /**Number of objects contained in the metadata.
     */
    size_t size() const;

    /** Check whether a label is contained in metadata.
     */
    bool containsLabel(const MDLabel label) const;

    /** Add a new label to the metadata.
     * By default the label is added at the end,
     * if the position is specified and is between 0 and n-1
     * the new label is inserted at that position.
     */
    bool addLabel(const MDLabel label, int pos = -1);

    /** Remove a label from the metadata.
     */
    bool removeLabel(const MDLabel label);

    /** Adds a new, empty object to the objects map. If objectId == -1
     * the new ID will be that for the last object inserted + 1, else
     * the given objectId is used. If there is already an object whose
     * objectId == input objectId, just removes it and creates an empty
     * one
     */
    size_t addObject();

    /** Import objects from another metadata.
     * @code
     * //Import object 1000 from metadata B into metadata A
     * A.importObject(B, 1000);
     * //Import all objects with rotational angle greater that 60
     * A.importObjects(B, MDValuesGT(MDL_ANGLE_ROT, 60));
     * //Import all objects
     * A.importObjects(B);     *
     * @endcode
     */
    void importObject(const MetaData &md, const size_t id, bool doClear=true);
    void importObjects(const MetaData &md, const std::vector<size_t> &objectsToAdd, bool doClear=true);
    void importObjects(const MetaData &md, const MDQuery &query, bool doClear=true);

    /** Remove the object with this id.
    * Returns true if the object was removed or false if
    * the object did not exist
    */
    bool removeObject(size_t id);

    /** Removes the collection of objects of given vector id's
     * NOTE: The iterator will point to the first object after any of these
     * operations
     */
    void removeObjects(const std::vector<size_t> &toRemove);

    /** Removes objects from metadata.
     * return the number of deleted rows
     * if not query, all objectes are removed
     * Queries can be used in the same way
     * as in the importObjects function
     */
    int removeObjects(const MDQuery &query);
    int removeObjects();

    /** Add and remove indexes for fast search
     * in other labels, but insert are more expensive
     */
    void addIndex(MDLabel label);
    void removeIndex(MDLabel label);

    /** @} */

    /** @name Iteration functions
     * @{
     */

    /** Return the object id of the first element in metadata. */
    size_t firstObject() const;
    /** Return the object id of the first element result from query */
    size_t firstObject(const MDQuery&) const;

    /** Goto last metadata object.*/
    size_t lastObject() const;

    /** @name Search operations
     * @{
     */

    /** Find all objects that match a query.
     * if called without query, all objects are returned
     * if limit is provided only return a maximun of 'limit'
     */
    void findObjects(std::vector<size_t> &objectsOut, const MDQuery &query) const;
    void findObjects(std::vector<size_t> &objectsOut, int limit = -1) const;

    /**Count all objects that match a query.
     */
    size_t countObjects(const MDQuery &query);

    /** Find if the object with this id is present in the metadata
     */
    bool containsObject(size_t objectId);

    /**Check if exists at least one object that match query.
     */
    bool containsObject(const MDQuery &query);
    /** @} */

    /** @name I/O functions
     * @{
     */

    /** Write rows data to disk. */
    void _writeRows(std::ostream &os) const;

    /** Write metadata to disk.
     * This will write the metadata content to disk.
     */
    void _write(const FileName &outFile,const String & blockName="", WriteModeMetaData mode=MD_OVERWRITE) const;
    /** Write metadata to disk. Guess blockname from filename
     * @code
     * outFilename="first@md1.doc" -> filename = md1.doc, blockname = first
     * @endcode
     */
    void write(const FileName &outFile, WriteModeMetaData mode=MD_OVERWRITE) const;


    /** Write metadata to out stream
     */
    void write(std::ostream &os, const String & blockName="",WriteModeMetaData mode=MD_OVERWRITE) const;

    /** Append data lines to file.
     * This function can be used to add new data to
     * an existing metadata. Now should be used with
     * files with only one metadata, maybe can be extended later.
     * For now it will not check any compatibility beetween the
     * existent metadata and the new data to append.
     */
    void append(const FileName &outFile);

    /** Read data from file.
     */
    void _read(const FileName &inFile, const std::vector<MDLabel> *desiredLabels = NULL, const String & blockName="", bool decomposeStack=true);
    /** Read data from file. Guess the blockname from the filename
     * @code
     * inFilename="first@md1.doc" -> filename = md1.doc, blockname = first
     * @endcode
     */
    void read(const FileName &inFile, const std::vector<MDLabel> *desiredLabels = NULL, bool decomposeStack=true);
    /** @} */

    /** Try to read a metadata from plain text with some columns.
     * Labels for each columns should be provided in an string separated by spaces.
     *  Return false if couldn't read
     */
    void readPlain(const FileName &inFile, const String &labelsString, const String &separator = " ");
    /** Same as readPlain, but instead of cleanning data, the
     * readed values will be added. If there are common columns in metadata
     * and the plain text, the lattest will be setted
     */
    void addPlain(const FileName &inFile, const String &labelsString, const String &separator=" ");

    /** @name Set Operations
     * @{
     */

    /** Aggregate metadata objects,
     * result in calling metadata object (except for aggregateSingle)
     * thisLabel label is used for aggregation, second. Valid operations are:
     *
     * MDL_AVG:  The avg function returns the average value of all  operationLabel within a group.
      The result of avg is always a floating point value as long as at there
      is at least one non-NULL input even if all inputs are integers.
       The result of avg is NULL if and only if there are no non-NULL inputs.

      AGGR_COUNT: The count function returns a count of the number of times that operationLabel is in a group.

      AGGR_MAX       The max aggregate function returns the maximum value of all values in the group.

      AGGR_MIN       The min aggregate function returns the minimum  value of all values in the group.

     AGGRL_SUM The total aggregate functions return sum of all values in the group.
     If there are no non-NULL input rows then returns 0.0.


     */

    void aggregate(const MetaData &mdIn, AggregateOperation op,
                   MDLabel aggregateLabel, MDLabel operateLabel, MDLabel resultLabel);
    void aggregate(const MetaData &mdIn, const std::vector<AggregateOperation> &ops,
                   const std::vector<MDLabel> &operateLabels, const std::vector<MDLabel> &resultLabels);
    void aggregateSingle(MDObject &mdValueOut, AggregateOperation op,
                         MDLabel aggregateLabel);
    /** Union of elements in two Metadatas, without duplicating.
     * Result in calling metadata object
     * union is a reserved word so I called this method unionDistinct
     */
    void unionDistinct(const MetaData &mdIn, const MDLabel label=MDL_OBJID);

    /** Union of all elements in two Metadata, duplicating common elements.
     * Result in calling metadata object
     * Repetion are allowed
     */
    void unionAll(const MetaData &mdIn);

    /** Merge of two Metadata.
     * This function reads another metadata and add all columns values.
     * The size of the two Metadatas should be the same. If there are
     * common columns, the values in md2 will be setted.
     */
    void merge(const MetaData &md2);

    /** Intersects two Metadatas.
     * Result in "calling" Metadata
     */
    void intersection(const MetaData &mdIn, const MDLabel label);

    /** Substract two Metadatas.
     * Result in "calling" metadata
     */
    void subtraction(const MetaData &mdIn, const MDLabel label);

    /** Join two Metadatas
     * Result in "calling" metadata
     */
    void join(const MetaData &mdInLeft, const MetaData &mdInRight, const MDLabel label, JoinType type=LEFT);

    /** Basic operations on columns data.
     * Mainly perform replacements on string values and
     * basic algebraic operations on numerical ones.
     */
    void operate(const String &expression);

    /** Randomize a metadata.
     * MDin is input and the "randomized"
     * result will be in the "calling" Metadata.
    */
    void randomize(MetaData &MDin);

    /**Remove duplicate entries for attribute in label
     *
     */
    void removeDuplicates(MetaData &MDin);

    /*
    * Sort a Metadata by a label.
    * Sort the content of MDin comparing
    * the label supplied, the result will
    * be in the "calling" MetaData.
    */
    void sort(MetaData &MDin, const MDLabel sortLabel, bool asc=true);

    /*
    * Sort a Metadata by a label.
    * Sort the content of MDin comparing
    * the label supplied, the result will
    * be in the "calling" MetaData.
    * If the input label is a vector field,
    * you may supply label:col, to sort by that column,
    * e.g., NMADisplacements:0
    */
    void sort(MetaData &MDin, const String &sortLabel, bool asc=true);

    /** Split Metadata in several Metadatas.
     * The Metadata will be divided in 'n'
     * almost equally parts and the result will
     * be a vector of Metadatas. The "calling"
     * Metadata will no be modified.
     * @code
     *   // Divide the images metadata in 10 metadatas.
     *   std::vector<MetaData> imagesGroups;
     *   imageMD.split(10, imagesGroups);
     * @endcode
     */
    void split(int n, std::vector<MetaData> &results,
               const MDLabel sortLabel=MDL_OBJID);

    /** Take a part from MetaData.
     * This function is equivallent to divide
     * the input MetaData in n parts and take one.
     * The result will be in "calling" MetaData.
     */
    void selectSplitPart(const MetaData &mdIn,
                         int n, int part,
                         const MDLabel sortLabel=MDL_OBJID);

    /** Select some part from Metadata.
     * Select elements from input Metadata
     * at some starting position
     * if the numberOfObjects is -1, all objects
     * will be returned from startPosition to the end.
    */
    void selectPart (const MetaData &mdIn, size_t startPosition, size_t numberOfObjects,
                     const MDLabel sortLabel=MDL_OBJID);

    /** Makes filenames with absolute paths
    *
    */
    void makeAbsPath(const MDLabel label=MDL_IMAGE);

    /** @} */
    friend class MDSql;
    friend class MDIterator;
    /** Expand Metadata with metadata pointed by label
     * Given a metadata md1, with a column containing the name of another column metdata file mdxx
     * add the columns in mdxx to md1
     */
    void fillExpand(MDLabel label);
    /** 'is equal to' (equality).*/
    bool operator==(const MetaData& op) const;



}
;//class MetaData

/** print metadata
 *
 */
std::ostream& operator<<(std::ostream& o, const MetaData & mD);


////////////////////////////// MetaData Iterator ////////////////////////////
/** Iterates over metadatas */
class MDIterator
{
protected:
    size_t * objects;
    size_t size;

    /** Internal function to initialize the iterator */
    void init(const MetaData &md, const MDQuery * pQuery=NULL);
public:

    /** Empty constructor */
    MDIterator();
    /** Empty constructor, creates an iterator from metadata */
    MDIterator(const MetaData &md);
    /** Same as before but iterating over a query */
    MDIterator(const MetaData &md, const MDQuery &query);
    /** Destructor */
    ~MDIterator();

    /** This is the object ID in the metadata, usually starts at 1 */
    size_t objId;
    /** This is the index of the object, starts at 0 */
    size_t objIndex;
    /** Function to move to next element.
     * return false if there aren't more elements to iterate.
     */
    bool moveNext();
    /** Function to check if exist next element
     */
    bool hasNext();
}
;//class MDIterator

////////////////////////////// MetaData Value Generator ////////////////////////
/** Class to generate values for columns of a metadata*/
class MDValueGenerator
{
public:
    MDLabel label; //label to which generate values

    /* Method to be implemented in concrete generators */
    virtual bool fillValue(MetaData &md, size_t objId) = 0;
    /* Fill whole metadata */
    bool fill(MetaData &md);
}
;//end of class MDValueGenerator

///////// Some concrete generators ////////////////
typedef enum { GTOR_UNIFORM, GTOR_GAUSSIAN, GTOR_STUDENT } RandMode;

/** MDGenerator to generate random values on columns */
class MDRandGenerator: public MDValueGenerator
{
protected:
    double op1, op2, op3;
    RandMode mode;

    inline double getRandValue();
public:
    MDRandGenerator(double op1, double op2, const String &mode, double op3=0.);
    bool fillValue(MetaData &md, size_t objId);
}
;//end of class MDRandGenerator

/** Class to fill columns with constant values */
class MDConstGenerator: public MDValueGenerator
{
public:
    String value;

    MDConstGenerator(const String &value);
    bool fillValue(MetaData &md, size_t objId);
}
;//end of class MDConstGenerator

/** Class to fill columns with another metadata in row format */
class MDExpandGenerator: public MDValueGenerator
{
public:
    MetaData expMd;
    FileName fn;
    MDRow row;

    bool fillValue(MetaData &md, size_t objId);
}
;//end of class MDExpandGenerator

/** Class to fill columns with a lineal serie */
class MDLinealGenerator: public MDValueGenerator
{
public:
    double initValue, step;
    size_t counter;

    MDLinealGenerator(double initial, double step);
    bool fillValue(MetaData &md, size_t objId);
}
;//end of class MDExpandGenerator
/** Convert string to write mode metadata enum.
 *
 */
WriteModeMetaData metadataModeConvert (String mode);


/** @} */

#endif
