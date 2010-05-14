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
#include "funcs.h"
#include "strings.h"
#include "metadata_container.h"
#include <time.h>
#include <stdio.h>
#include "cppsqlite3.h"
#include <sstream>

//empty vector used for inizialization

/// @defgroup MetaData Metadata management
/// @ingroup DataLibrary

/// @defgroup MetaDataClass Metadata class management
/// @ingroup MetaData

/** MetaData Manager.
 * @ingroup MetaDataClass
 *
 * The MetaData class manages all procedures related to
 * metadata. MetaData is intended to group toghether old
 * Xmipp specific files like Docfiles, Selfiles, etc..
 * 
 */
class MetaData
{
    std::map<long int, MetaDataContainer *> objects; ///< Effectively stores all metadata

    // Used by firstObject, nextObject and lastObject to keep a pointer
    // to the "active" object. This way when you call setValue without
    // an objectID, this one is chosen
    std::map<long int, MetaDataContainer *>::iterator objectsIterator;

    // Allows a fast search for pairs where the value is
    // a string, i.e. looking for filenames which is quite
    // usual
    std::map<std::string, long int> fastStringSearch;
    MetaDataLabel fastStringSearchLabel;

    std::string path; ///< A parameter stored on MetaData Files
    std::string comment; ///< A general comment for the MetaData file

    void read(std::ifstream *infile, std::vector<MetaDataLabel> * labelsVector);

    bool isColumnFormat; ///< Format for the file, column or row formatted

    /**Input file name
     * Where does this MetaData come from/go to be stored?
     */
    FileName inFile;

public:

    /// @defgroup MetaDataConstructors Constructors for MetaData objects
    /// @ingroup MetaDataClass

    /** Empty Constructor.
     * @ingroup MetaDataConstructors
     *
     * The MetaData is created with no data stored on it. You can fill in it programmatically
     * or by a later reading from a MetaData file or old Xmipp formatted type.
     */
    MetaData();

    /** From File Constructor.
     * @ingroup MetaDataConstructors
     *
     * The MetaData is created and data is read from provided fileName. Optionally, a vector
     * of labels can be provided to read just those required labels
     */
    MetaData(FileName fileName, std::vector<MetaDataLabel> * labelsVector =
                 NULL);

    /** Copy constructor
     * @ingroup MetaDataConstructors
     *
     * Created a new metadata by copying all data from an existing MetaData object.
     */
    MetaData(const MetaData & c);

    /** Assignment operator
     * @ingroup MetaDataConstructors
     *
     * Copies MetaData from an existing MetaData object.
     */
    MetaData& operator =(const MetaData &MD);

    /** union of  metadata objects, result in calling metadata object
     * union is a reserved word so I called this method union_
     */
    void union_(MetaData &MD, MetaDataLabel thisLabel=MDL_OBJID);

    /** merge of a metadata
     * This function reads another metadata and makes a union to this one
     */
    void merge(const FileName &fn);

    /** Aggregate modes */
    enum AggregateMode {
    	KEEP_OLD,
    	KEEP_NEW,
    	SUM
    };

    /** intersects two metadata objects, result in "calling" metadata
     */
    void intersection(MetaData & minuend, MetaData & ,
                      MetaDataLabel thisLabel);

    /** substract two metadata objects, result in "calling" metadata
     */
    void substraction(MetaData & minuend, MetaData & subtrahend,
                      MetaDataLabel thisLabel);

    /** Destructor
     * @ingroup MetaDataConstructors
     *
     * Frees all used memory and destroys object.
     */
    ~MetaData();

    /* get metadatacontainer for current metadata object
     *
     */
    MetaDataContainer * getObject(long int objectID = -1) const;

    /** Set to false for row format (parameter files)
     *  set to true  for column format (this is the default) (docfiles)
     *
     */
    void setColumnFormat(bool column);
    /**Get column format info.
     *
     */
    bool getColumnFormat() const
    {
        return isColumnFormat;
    }
    // Set a new pair/value for an specified object. If no objectID is given, that
    // pointed by the class iterator is used
    bool setValue(const std::string &name, const std::string &value,
                  long int objectID = -1);
    bool setValue(const std::string &name, const float &value,
                  long int objectID = -1)
    {
        std::cerr << "Do not use setValue with floats, use double"<< std::endl;
        std::cerr << "Floats are banned from metadata class"<< std::endl;
        exit(1);
    }
    bool setValue(const std::string &name, const char &value,
                  long int objectID = -1)
    {
        std::cerr << "Do not use setValue with char, use string"<< std::endl;
        std::cerr << "chars are banned from metadata class"<< std::endl;
        exit(1);
    }

    void readOldSelFile(std::ifstream *infile);
    void readOldDocFile(std::ifstream *infile,
                        std::vector<MetaDataLabel> * labelsVector);
    void read(FileName infile,
              std::vector<MetaDataLabel> * labelsVector = NULL);

    /**convert metafile to table in database
     *
     */
    void toDataBase(const FileName & DBname,
                    const std::string & tableName = "",
                    std::vector<MetaDataLabel> * labelsVector = NULL);
    /**Convert table from database in metadata
     *
     */
    void fromDataBase(const FileName & DBname, const std::string & tableName,
                      MetaDataLabel sortLabel=MDL_OBJID,
                      std::vector<MetaDataLabel> * labelsVector = NULL);

    /** What labels have been read from a docfile/metadata file
     *   and/or will be stored on a new metadata file when "save" is
     *   called
     **/
    std::vector<MetaDataLabel> activeLabels;

    /** When reading a column formated file, if a label is found that
     *   does not exists as a MetaDataLabel, it is ignored. For further
     *   file processing, such columns must be ignored and this structure
     *   allows to do that
     **/
    std::vector<unsigned int> ignoreLabels;

    /** Adds a new, empty object to the objects map. If objectID == -1
     *   the new ID will be that for the last object inserted + 1, else
     *   the given objectID is used. If there is already an object whose
     *   objectID == input objectID, just removes it and creates an empty
     *   one
     **/
    long int addObject(long int objectID = -1);

    // Possible error codes for the map
    enum errors
    {
        NO_OBJECTS_STORED = -1, // NOTE: Do not change this value (-1)
        NO_MORE_OBJECTS = -2,
        NO_OBJECT_FOUND = -3
    };

    long int firstObject();
    long int nextObject();
    long int lastObject();
    long int goToObject(long int objectID);

    void write(const std::string &fileName);

    bool isEmpty() const;

    void clear();

    void writeValueToString(std::string & result,
                            const std::string & inputLabel);
    /** Allows a fast search for pairs where the value is
     a string, i.e. looking for filenames which is quite
     usual.
     */
    long int fastSearch(MetaDataLabel name, std::string value, bool recompute =
                            false);

    /**Create new metadata with selected objectId
     *
     */
    void fillMetaData(MetaData &base, std::vector<long int> objectsToAdd);

    void combine(MetaData & other, MetaDataLabel thisLabel = MDL_UNDEFINED);
    void combineWithFiles(MetaDataLabel thisLabel);

    // Removes the collection of objects whose pair label/value is given
    // NOTE: The iterator will point to the first object after any of these
    // operations.
    void removeObjects(std::vector<long int> &toRemove);

    // Returns true if the object was removed or false if
    // the object did not exist
    bool removeObject(long int objectID);

    /**Set Path
     * will appear in first line
     */
    void setPath(std::string newPath = "");

    /**Set Header Comment
     * will appear in second line
     */
    void setComment(const std::string Comment = "");

    std::string getPath()   const ;
    std::string getComment() const;

    size_t size(void)
    {
        return objects.size();
    }
    /** Return metafile filename
     *
     */

    FileName getFilename()
    {
        return (inFile);
    }

    /*Detect if there is at least one entry with the given label-value pair
     * This can be much faster than 'countObjects' as it stops iterating once the first
     * object has been found.
     */
    template<class T>
    bool detectObjects(MetaDataLabel name, T value)
    {
        bool result = false;
        // Traverse all the structure looking for objects
        // that satisfy search criteria
        MetaDataContainer * aux;
        std::map<long int, MetaDataContainer *>::iterator It;
        for (It = objects.begin(); It != objects.end(); It++)
        {
            aux = It->second;
            if (aux->pairExists(name, value))
            {
                result = true;
                break;
            }
        }
        return result;
    }


    template<class T>
    std::vector<long int> findObjectsInRange(MetaDataLabel name, T minValue, T maxValue)
    {
        std::vector<long int> result;

        // Traverse all the structure looking for objects
        // that satisfy search criteria

        std::map<long int, MetaDataContainer *>::iterator It;

        MetaDataContainer * aux;

        for (It = objects.begin(); It != objects.end(); It++)
        {
            aux = It->second;

            if (aux->valueExists(name))
            {
                T value;
                aux->getValue(name, value);

                if (value >= minValue && value <= maxValue)
                {
                    result.push_back(It->first);
                }
            }
        }

        return result;
    }

    template<class T>
    std::vector<long int> findObjects(MetaDataLabel name,
                                      T value)
    {
        std::vector<long int> result;

        // Traverse all the structure looking for objects
        // that satisfy search criteria

        std::map<long int, MetaDataContainer *>::iterator It;

        MetaDataContainer * aux;

        for (It = objects.begin(); It != objects.end(); It++)
        {
            aux = It->second;

            if (aux->pairExists(name, value))
                result.push_back(It->first);
        }

        return result;
    }

    template<class T>
    long int goToFirstObject(MetaDataLabel name, T value)
    {
        // Traverse all the structure looking for objects
        // that satisfy search criteria

        std::map<long int, MetaDataContainer *>::iterator It;

        MetaDataContainer * aux;

        if (!objects.empty())
        {
            for (It = objects.begin(); It != objects.end(); It++)

            {
                aux = It->second;

                if (aux->pairExists(name, value))
                {
                    objectsIterator = It;
                    return It->first;
                }
            }
            return NO_OBJECT_FOUND;
        }
        else
            return NO_OBJECTS_STORED;
    }

    template<class T>
    void removeObjects(MetaDataLabel name, const T &value)
    {
        std::vector<long int> toRemove = findObjects(name, value);
        removeObjects(toRemove);
    }

    /**Count number of objects that satisfy a given label,entry pair
     */
    template<class T>
    long int countObjects(MetaDataLabel name, const T &value)
    {
        return ((findObjects(name, value)).size());
    }

    template<class T>
    bool getValue(MetaDataLabel name, T &value,
                  long int objectID = -1) const
    {
        MetaDataContainer * aux = getObject(objectID);
        if (!aux->valueExists(name))
        {
            return false;
        }
        else
        {
            aux->getValue(name, value);
        }
        return true;
    }

    template<class T>
    bool setValue(MetaDataLabel name,const T &value, long int objectID=-1)
    {
        long int auxID;
        if (!objects.empty() && MetaDataContainer::isValidLabel(name))
        {
            if (objectID == -1)
            {
                auxID = objectsIterator->first;
            }
            else
            {
                auxID = objectID;
            }

            MetaDataContainer * aux = objects[auxID];

            // Check whether label is correct (belongs to the enum in the metadata_container header
            // and whether it is present in the activeLabels vector. If not, add it to all the other
            // objects with default values
            std::vector<MetaDataLabel>::iterator location;
            std::map<long int, MetaDataContainer *>::iterator It;
            location = std::find(activeLabels.begin(), activeLabels.end(), name);
            if (location == activeLabels.end())
            {
                activeLabels.push_back(name);
                // Add this label to the rest of the objects in this class
                for (It = objects.begin(); It != objects.end(); It++)
                {
                    if (It->second != aux)
                    {
                        (It->second)->addValue(name, T());
                    }
                }
            }
            aux->addValue(name, value);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool valueExists(MetaDataLabel name)
    {
        return (objectsIterator->second)->valueExists(name);
    }

    /**Add object with metadata label name in the range given by minvalue and maxvalue
     * This template function may be accessed from swig
     *
     * */
    template<class T>
    friend void addObjectsInRangeSwig(MetaData &MDin, MetaData &MDout,
                                      MetaDataLabel name, T minValue, T maxValue)
    {
        MDout.fillMetaData(MDin, MDin.findObjectsInRange(name, minValue, maxValue));
    }
    template<class T>
    friend bool setValueSwig(MetaData &MDout,MetaDataLabel name, T &value, long int objectID=-1)
    {
        MDout.setValue( name, value, objectID);
    }
    template<class T>
    friend bool getValueSwig(MetaData &MDout,MetaDataLabel name, T &value, long int objectID=-1)
    {
        MDout.getValue( name,  value, objectID);
    }
    /*
     * Randomize this metadata, MDin is input
     */
    void randomize(MetaData &MDin);
    /*
     * Sort this metadata, by label
     * dirty implementation using sqlite
     */

    void sort(MetaData & MDin, MetaDataLabel sortlabel);

    /** Split metadata into two random halves
     *
     */
    void split_in_two(MetaData &SF1, MetaData &SF2,MetaDataLabel sortlabel=MDL_UNDEFINED);

    /** Fill metadata with N entries from MD starting at start
     *
     */
    void fillWithNextNObjects (MetaData &MD, long int start, long int numberObjects);

    /** Returns the maximum length of string file in the metadatafile.
     * @ingroup MetaDataInfo
     *
     */

    int MaxStringLength( MetaDataLabel thisLabel);

};
/** For all objects.
 @code
 FOR_ALL_OBJECTS_IN_METADATA(metadata) {
 double rot; DF.getValue( MDL_ANGLEROT, rot);
 }
 @endcode
 */
//Write for partial metadatas- read use NULL
//better sort
//error in metadata_split when there is only one comment
//COMMENT

#define FOR_ALL_OBJECTS_IN_METADATA(kkkk_metadata) \
        for(long int kkkk = (kkkk_metadata).firstObject(); \
             kkkk != MetaData::NO_MORE_OBJECTS; \
             kkkk=(kkkk_metadata).nextObject())
#endif
