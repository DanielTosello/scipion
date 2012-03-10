/**************************************************************************
 *
 * Authors:      J.R. Bilbao-Castro (jrbcast@ace.ual.es)
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

#include <regex.h>
#include <algorithm>
#include "metadata.h"
#include "xmipp_image.h"
#include "xmipp_program_sql.h"

// Get the blocks available
void getBlocksInMetaDataFile(const FileName &inFile, StringVector& blockList)
{
    if (!inFile.isMetaData())
        return;

    MetaData MDaux(inFile);
    blockList.clear();
    //map file
    int fd;
    BUFFER_CREATE(bufferMap);
    mapFile(inFile, bufferMap.begin, bufferMap.size, fd);
    BUFFER_COPY(bufferMap, buffer);
    BLOCK_CREATE(block);
    String blockName;
    while (MDaux.nextBlock(buffer, block))
    {
        BLOCK_NAME(block, blockName);
        blockList.push_back(blockName);
    }

    unmapFile(bufferMap.begin, bufferMap.size, fd);
}

//-----Constructors and related functions ------------
void MetaData::_clear(bool onlyData)
{
    if (onlyData)
    {
        myMDSql->deleteObjects();
    }
    else
    {
        path.clear();
        comment.clear();
        fastStringSearch.clear();
        fastStringSearchLabel = MDL_UNDEFINED;

        activeLabels.clear();
        ignoreLabels.clear();
        _isColumnFormat = true;
        inFile = FileName();
        myMDSql->clearMd();
    }
    eFilename="";
}//close clear

void MetaData::clear()
{
    //_clear(true);
    init();
}

void MetaData::init(const std::vector<MDLabel> *labelsVector)
{
    _clear();
    if (labelsVector != NULL)
        this->activeLabels = *labelsVector;
    //Create table in database
    myMDSql->createMd();
    precision = 100;
    isMetadataFile = false;
}//close init

void MetaData::copyInfo(const MetaData &md)
{
    if (this == &md) //not sense to copy same metadata
        return;
    this->setComment(md.getComment());
    this->setPath(md.getPath());
    this->_isColumnFormat = md._isColumnFormat;
    this->inFile = md.inFile;
    this->fastStringSearchLabel = md.fastStringSearchLabel;
    this->activeLabels = md.activeLabels;
    this->ignoreLabels = md.ignoreLabels;
    this->isMetadataFile = md.isMetadataFile;

}//close copyInfo

void MetaData::copyMetadata(const MetaData &md, bool copyObjects)
{
    if (this == &md) //not sense to copy same metadata
        return;
    init(&(md.activeLabels));
    copyInfo(md);
    if (!md.activeLabels.empty())
    {
        if (copyObjects)
            md.myMDSql->copyObjects(this);
    }
    else
    {
        int n = md.size();
        for (int i = 0; i < n; i++)
            addObject();
    }
}

bool MetaData::setValue(const MDObject &mdValueIn, size_t id)
{
    if (id == BAD_OBJID)
    {
        REPORT_ERROR(ERR_MD_NOACTIVE, "setValue: please provide objId other than -1");
        exit(1);
    }
    //add label if not exists, this is checked in addlabel
    addLabel(mdValueIn.label);
    return myMDSql->setObjectValue(id, mdValueIn);
}

bool MetaData::setValueCol(const MDObject &mdValueIn)
{
    //add label if not exists, this is checked in addlabel
    addLabel(mdValueIn.label);
    return myMDSql->setObjectValue(mdValueIn);
}

bool MetaData::getValue(MDObject &mdValueOut, size_t id) const
{
    if (!containsLabel(mdValueOut.label))
        return false;

    if (id == BAD_OBJID)
        REPORT_ERROR(ERR_MD_NOACTIVE, "getValue: please provide objId other than -1");

    return myMDSql->getObjectValue(id, mdValueOut);
}

void MetaData::getColumnValues(const MDLabel label, std::vector<MDObject> &valuesOut) const
{
    MDObject mdValueOut(label);
    std::vector<size_t> objectsId;
    findObjects(objectsId);
    size_t n = objectsId.size();
    valuesOut.resize(n,mdValueOut);
    for (size_t i = 0; i < n; ++i)
    {
        getValue(mdValueOut, objectsId[i]);
        valuesOut[i] = mdValueOut;
    }
}

void MetaData::setColumnValues(const std::vector<MDObject> &valuesIn)
{
    bool addObjects=false;
    if (size()==0)
        addObjects=true;
    if (valuesIn.size()!=size() && !addObjects)
        REPORT_ERROR(ERR_MD_OBJECTNUMBER,"Input vector must be of the same size as the metadata");
    if (!addObjects)
    {
        size_t n=0;
        FOR_ALL_OBJECTS_IN_METADATA(*this)
        setValue(valuesIn[n++],__iter.objId);
    }
    else
    {
        size_t nmax=valuesIn.size();
        for (size_t n=0; n<nmax; ++n)
            setValue(valuesIn[n],addObject());
    }
}

bool MetaData::getRow(MDRow &row, size_t id) const
{
    row.clear();
    for (std::vector<MDLabel>::const_iterator it = activeLabels.begin(); it != activeLabels.end(); ++it)
    {
        MDObject obj(*it);
        if (!getValue(obj, id))
            return false;
        row.setValue(obj);
    }
    return true;
}

//TODO: could be improve in a query for update the entire row
#define SET_ROW_VALUES(row) \
    for (int i = 0; i < row._size; ++i){\
        const MDLabel &label = row.order[i];\
        if (row.containsLabel(label))\
            setValue(*(row.getObject(label)), id);}

void MetaData::setRow(const MDRow &row, size_t id)
{
    SET_ROW_VALUES(row);
}

size_t MetaData::addRow(const MDRow &row)
{
    size_t id = addObject();
    SET_ROW_VALUES(row);

    return id;
}

MetaData::MetaData()
{
    myMDSql = new MDSql(this);
    init(NULL);
}//close MetaData default Constructor

MetaData::MetaData(const std::vector<MDLabel> *labelsVector)
{
    myMDSql = new MDSql(this);
    init(labelsVector);
}//close MetaData default Constructor

MetaData::MetaData(const FileName &fileName, const std::vector<MDLabel> *desiredLabels)
{
    myMDSql = new MDSql(this);
    init(desiredLabels);
    read(fileName, desiredLabels);
}//close MetaData from file Constructor

MetaData::MetaData(const MetaData &md)
{
    myMDSql = new MDSql(this);
    copyMetadata(md);
}//close MetaData copy Constructor

MetaData& MetaData::operator =(const MetaData &md)
{
    copyMetadata(md);
    return *this;
}//close metadata operator =

MetaData::~MetaData()
{
    _clear();
    delete myMDSql;
}//close MetaData Destructor

//-------- Getters and Setters ----------

bool MetaData::isColumnFormat() const
{
    return _isColumnFormat;
}
/* Set to false for row format (parameter files)
 *  @ingroup GettersAndSetters
 *  set to true  for column format (this is the default) (docfiles)
 *
 */
void MetaData::setColumnFormat(bool column)
{
    _isColumnFormat = column;
}
String MetaData::getPath()   const
{
    return path;
}

void MetaData::setPath(const String &newPath)
{
    const size_t length = 512;
    char _buffer[length];
    path = (newPath == "") ? String(getcwd(_buffer, length)) : newPath;
}

String MetaData::getComment() const
{
    return  comment;
}

void MetaData::setComment(const String &newComment)
{
    comment = newComment;
}

FileName MetaData::getFilename() const
{
    return inFile;
}

void MetaData::setFilename(const FileName &_fileName)
{
    inFile=_fileName;
}

std::vector<MDLabel> MetaData::getActiveLabels() const
{
    return activeLabels;
}

std::vector<MDLabel>* MetaData::getActiveLabelsAddress() const
{
    return (std::vector<MDLabel>*) (&activeLabels);
}

int MetaData::getMaxStringLength(const MDLabel thisLabel) const
{
    if (!containsLabel(thisLabel))
        return -1;

    return myMDSql->columnMaxLength(thisLabel);
}

bool MetaData::setValueFromStr(const MDLabel label, const String &value, size_t id)
{
    addLabel(label);

    if (id == BAD_OBJID)
    {
        REPORT_ERROR(ERR_MD_NOACTIVE, "setValue: please provide objId other than -1");
        exit(1);
    }
    MDObject mdValue(label);
    mdValue.fromString(value);
    return myMDSql->setObjectValue(id, mdValue);
}

bool MetaData::getStrFromValue(const MDLabel label, String &strOut, size_t id) const
{
    MDObject mdValueOut(label);
    if (!getValue(mdValueOut, id))
        return false;
    strOut = mdValueOut.toString();
    return true;
}

bool MetaData::isEmpty() const
{
    return size() == 0;
}

size_t MetaData::size() const
{
    std::vector<size_t> objects;
    myMDSql->selectObjects(objects);

    return objects.size();
}

bool MetaData::containsLabel(const MDLabel label) const
{
    return vectorContainsLabel(activeLabels, label);
}

bool MetaData::addLabel(const MDLabel label, int pos)
{
    if (containsLabel(label))
        return false;
    if (pos < 0 || pos >= activeLabels.size())
        activeLabels.push_back(label);
    else
        activeLabels.insert(activeLabels.begin() + pos, label);
    myMDSql->addColumn(label);
    return true;
}

bool MetaData::removeLabel(const MDLabel label)
{
    std::vector<MDLabel>::iterator location;
    location = std::find(activeLabels.begin(), activeLabels.end(), label);

    if (location == activeLabels.end())
        return false;

    activeLabels.erase(location);
    return true;
}

size_t MetaData::addObject()
{
    return (size_t)myMDSql->addRow();
}

void MetaData::importObject(const MetaData &md, const size_t id, bool doClear)
{
    MDValueEQ query(MDL_OBJID, id);
    md.myMDSql->copyObjects(this, &query);
}

void MetaData::importObjects(const MetaData &md, const std::vector<size_t> &objectsToAdd, bool doClear)
{
    init(&(md.activeLabels));
    copyInfo(md);
    int size = objectsToAdd.size();
    for (int i = 0; i < size; i++)
        importObject(md, objectsToAdd[i]);
}

void MetaData::importObjects(const MetaData &md, const MDQuery &query, bool doClear)
{
    if (doClear)
    {
        //Copy all structure and info from the other metadata
        init(&(md.activeLabels));
        copyInfo(md);
    }
    else
    {
        //If not clear, ensure that the have the same labels
        for (int i = 0; i < md.activeLabels.size(); i++)
            addLabel(md.activeLabels[i]);
    }
    md.myMDSql->copyObjects(this, &query);
}

bool MetaData::removeObject(size_t id)
{
    int removed = removeObjects(MDValueEQ(MDL_OBJID, id));
    return (removed > 0);
}

void MetaData::removeObjects(const std::vector<size_t> &toRemove)
{
    int size = toRemove.size();
    for (int i = 0; i < size; i++)
        removeObject(toRemove[i]);
}

int MetaData::removeObjects(const MDQuery &query)
{
    int removed = myMDSql->deleteObjects(&query);
    return removed;
}

int MetaData::removeObjects()
{
    int removed = myMDSql->deleteObjects();
    return removed;
}

void MetaData::addIndex(MDLabel label)
{
    std::vector<MDLabel> labels(1);
    labels[0]=label;
    addIndex(labels);
}
void MetaData::addIndex(const std::vector<MDLabel> desiredLabels)
{

    myMDSql->indexModify(desiredLabels, true);
}

void MetaData::removeIndex(MDLabel label)
{
    std::vector<MDLabel> labels(1);
    labels[0]=label;
    removeIndex(labels);
}

void MetaData::removeIndex(const std::vector<MDLabel> desiredLabels)
{
    myMDSql->indexModify(desiredLabels, false);
}


//----------Iteration functions -------------------

size_t MetaData::firstObject() const
{
    return myMDSql->firstRow();
}

size_t MetaData::firstObject(const MDQuery & query) const
{
    std::vector<size_t> ids;
    findObjects(ids, query);
    size_t id = ids.size() == 1 ? ids[0] : BAD_OBJID;
    return id;
}

size_t MetaData::lastObject() const
{
    return myMDSql->lastRow();
}

//-------------Search functions-------------------
void MetaData::findObjects(std::vector<size_t> &objectsOut, const MDQuery &query) const
{
    objectsOut.clear();
    myMDSql->selectObjects(objectsOut, &query);
}

void MetaData::findObjects(std::vector<size_t> &objectsOut, int limit) const
{
    objectsOut.clear();
    MDQuery query(limit);
    myMDSql->selectObjects(objectsOut, &query);
}

size_t MetaData::countObjects(const MDQuery &query)
{
    std::vector<size_t> objects;
    findObjects(objects, query);
    return objects.size();
}

bool MetaData::containsObject(size_t objectId)
{
    return containsObject(MDValueEQ(MDL_OBJID, objectId));
}

bool MetaData::containsObject(const MDQuery &query)
{
    std::vector<size_t> objects;
    findObjects(objects, query);
    return objects.size() > 0;
}

//--------------IO functions -----------------------
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

void MetaData::write(const FileName &_outFile, WriteModeMetaData mode) const
{
    String blockName;
    FileName outFile;

    blockName=_outFile.getBlockName();
    outFile = _outFile.removeBlockName();
    _write(outFile, blockName, mode);

}

void MetaData::_write(const FileName &outFile,const String &blockName, WriteModeMetaData mode) const
{
    if (outFile.hasImageExtension())
        REPORT_ERROR(ERR_IO,"Trying to write metadata with image extension");

    struct stat file_status;
    int fd;
    char *map;

    //check if file exists or not block name has been given
    //in our format no two identical data_xxx strings may exists
    if(mode==MD_OVERWRITE)
        ;
    else if (blockName.empty() || !outFile.exists())
        mode = MD_OVERWRITE;
    else
    {
        //does blockname exists?
        //remove it from file in this case
        // get length of file:
        if(stat(outFile.data(), &file_status) != 0)
            REPORT_ERROR(ERR_IO_NOPATH,"Metadata:write can not get filesize for file "+outFile);
        size_t size = file_status.st_size;
        if(size!=0)//size=0 for /dev/stderr
        {
            fd = open(outFile.data(),  O_RDWR, S_IREAD | S_IWRITE);
            if (fd == -1)
                REPORT_ERROR(ERR_IO_NOPATH,"Metadata:write can not read file named "+outFile);

            map = (char *) mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (map == MAP_FAILED)
                REPORT_ERROR(ERR_MEM_BADREQUEST,"Metadata:write can not map memory ");

            // Is this a START formatted FILE

            if(strncmp(map,"# XMIPP_STAR",12)!=0)
            {
                mode=MD_OVERWRITE;
            }
            else
            {
                //block name
                String _szBlockName = (String)("\ndata_") + blockName;
                size_t blockNameSize = _szBlockName.size();

                //search for the string
                char * target, * target2;
                target = (char *) _memmem(map, size, _szBlockName.data(), blockNameSize);
                if(target!=NULL)
                {
                    target2 = (char *) _memmem(target+1, size - (target - map), "\ndata_", 6);

                    if (target2==NULL)//truncate file at target
                        ftruncate(fd, target - map+1);
                    else//copy file from target2 to target and truncate
                    {
                        memmove(target,target2, (map + size) - target2);
                        ftruncate(fd, (target-map) + ((map + size) - target2) );
                    }
                }
            }
            if (munmap(map, size) == -1)
            {
                REPORT_ERROR(ERR_MEM_NOTDEALLOC,"metadata:write, Can not unmap memory");
            }
            close(fd);
        }
        else
            mode=MD_OVERWRITE;
    }

    std::ios_base::openmode openMode;
    if(mode==MD_OVERWRITE)
        openMode = std::ios_base::out;
    else if(mode=MD_APPEND)
        openMode = std::ios_base::app;
    std::ofstream ofs(outFile.data(), openMode);

    write(ofs, blockName, mode);
    ofs.close();
}

void MetaData::append(const FileName &outFile)
{
    if (outFile.exists())
    {
        std::ofstream ofs(outFile.data(), std::ios_base::app);
        _writeRows(ofs);
        ofs.close();
    }
    else
        write(outFile);
}

void MetaData::_writeRows(std::ostream &os) const
{

    FOR_ALL_OBJECTS_IN_METADATA(*this)
    {
        //std::cerr << "id: " << __iter.objId << std::endl;
        for (int i = 0; i < activeLabels.size(); i++)
        {
            if (activeLabels[i] != MDL_COMMENT)
            {
                MDObject mdValue(activeLabels[i]);
                os.width(1);
                myMDSql->getObjectValue(__iter.objId, mdValue);
                mdValue.toStream(os, true);
                os << " ";
            }
        }
        os << std::endl;
    }
}

void MetaData::print() const{
  write(std::cout);
}


void MetaData::write(std::ostream &os,const String &blockName, WriteModeMetaData mode ) const
{
    if(mode==MD_OVERWRITE)
        os << "# XMIPP_STAR_1 * "// << (isColumnFormat ? "column" : "row")
        << std::endl //write wich type of format (column or row) and the path
        << "# " << comment << std::endl;     //write md comment in the 2nd comment line of header
    //write data block
    String _szBlockName = (String)("data_") + blockName;

    if (_isColumnFormat)
    {
        //write md columns in 3rd comment line of the header
        os << _szBlockName << std::endl;
        os << "loop_" << std::endl;

        for (int i = 0; i < activeLabels.size(); i++)
        {
            if (activeLabels.at(i) != MDL_COMMENT)
            {
                os << " _" << MDL::label2Str(activeLabels.at(i)) << std::endl;
            }
        }

        _writeRows(os);
        //Put the activeObject to the first, if exists
    }
    else //rowFormat
    {
        os << _szBlockName << std::endl;

        // Get first object. In this case (row format) there is a single object
        size_t id = firstObject();

        if (id != BAD_OBJID)
        {
            int maxWidth=20;
            for (int i = 0; i < activeLabels.size(); i++)
            {
                if (activeLabels.at(i) != MDL_COMMENT)
                {
                    int w=MDL::label2Str(activeLabels.at(i)).length();
                    if (w>maxWidth)
                        maxWidth=w;
                }
            }

            for (int i = 0; i < activeLabels.size(); i++)
            {
                if (activeLabels[i] != MDL_COMMENT)
                {
                    MDObject mdValue(activeLabels[i]);
                    os << " _" << MDL::label2Str(activeLabels.at(i)) << " ";
                    myMDSql->getObjectValue(id, mdValue);
                    mdValue.toStream(os);
                    os << std::endl;
                }
            }
        }

    }
}//write

/* This function will read the posible columns from the file
 * and mark as MDL_UNDEFINED those who aren't valid labels
 * or those who appears in the IgnoreLabels vector
 * also set the activeLabels (for OLD doc files)
 */
void MetaData::_readColumns(std::istream& is, std::vector<MDObject*> & columnValues,
                            const std::vector<MDLabel>* desiredLabels)
{
    String token;
    MDLabel label;

    while (is >> token)
        if (token.find('(') == String::npos)
        {
            //label is not reconized, the MDValue will be created
            //with MDL_UNDEFINED, wich will be ignored while reading data
            label = MDL::str2Label(token);
            if (desiredLabels != NULL && !vectorContainsLabel(*desiredLabels, label))
                label = MDL_UNDEFINED; //ignore if not present in desiredLabels
            columnValues.push_back(new MDObject(label));
            if (label != MDL_UNDEFINED)
                addLabel(label);

        }
}

/* Helper function to parse an MDObject and set its value.
 * The parsing will be from an input stream(istream)
 * and if parsing fails, an error will be raised
 */
void MetaData::_parseObject(std::istream &is, MDObject &object, size_t id)
{
    object.fromStream(is);
    if (is.fail())
    {
        REPORT_ERROR(ERR_MD_BADLABEL, (String)"read: Error parsing data column, expecting " + MDL::label2Str(object.label));
    }
    else
        if (object.label != MDL_UNDEFINED)
            setValue(object, id);
}//end of function parseObject

//#define END_OF_LINE() ((char*) memchr (iter, '\n', end-iter+1))
#define END_OF_LINE() ((char*) memchr (iter, '\n', end-iter))

/* This function will read the posible columns from the file
 * and mark as MDL_UNDEFINED those who aren't valid labels
 * or those who appears in the IgnoreLabels vector
 * also set the activeLabels (for new STAR files)
 */
char * MetaData::_readColumnsStar(mdBlock &block,
                                  std::vector<MDObject*> & columnValues,
                                  const std::vector<MDLabel>* desiredLabels,
                                  bool addColumns,
                                  size_t id)
{
    char * end = block.end;
    char * newline = NULL;
    bool found_column;
    MDLabel label;
    char * iter = block.loop;
    if (!_isColumnFormat)
    {
        iter = block.begin;
        iter = END_OF_LINE() + 1; //this should point at first label, after data_XXX
    }

    do
    {
        found_column = false;
        while (iter[0] == '#') //Skip comment
            iter = END_OF_LINE() + 1;

        //trim spaces and newlines at the beginning
        while ( isspace(iter[0]))
            ++iter;

        if (iter < end && iter[0] == '_')
        {
            found_column = true;
            ++iter; //shift _
            std::stringstream ss;
            newline = END_OF_LINE();
            //Last label and no data needs this check
            if (newline == NULL)
            	newline = end;
            String s(iter, newline - iter);//get current line
            ss.str(s);//set the string of the stream
            //Take the first token wich is the label
            //if the label contain spaces will fail
            ss >> s; //get the first token, the label
            label = MDL::str2Label(s);
            if (label == MDL_UNDEFINED)
                std::cout << "WARNING: Ignoring unknown column: " + s << std::endl;
            else
                if (desiredLabels != NULL && !vectorContainsLabel(*desiredLabels, label))
                    label = MDL_UNDEFINED; //ignore if not present in desiredLabels
                else
                    addLabel(label);
            if (addColumns)
            {
                MDObject * _mdObject = new MDObject(label);
                columnValues.push_back(_mdObject);//add the value here with a char
                if(!_isColumnFormat)
                    _parseObject(ss, *_mdObject, id);
            }
            iter = newline + 1;//go to next line character
        }
    }
    while (found_column)
        ;

    // This condition fails for empty blocks
    // if (iter < block.end)
    if (iter <= block.end +1)
        block.loop = iter; //Move loop pointer to position of last found column
}

/* This function will be used to parse the rows data
 * having read the columns labels before and setting wich are desired
 * the useCommentAsImage is for compatibility with old DocFile format
 * where the image were in comments
 */
void MetaData::_readRows(std::istream& is, std::vector<MDObject*> & columnValues, bool useCommentAsImage)
{
    String line = "";
    while (!is.eof() && !is.fail())
    {
        //Move until the ';' or the first alphanumeric character
        while (is.peek() != ';' && isspace(is.peek()) && !is.eof())
            is.ignore(1);
        if (!is.eof())
        {
            if (is.peek() == ';')//is a comment
            {
                is.ignore(1); //ignore the ';'
                getline(is, line);
                trim(line);
            }
            else if (!isspace(is.peek()))
            {
                size_t id = addObject();
                if (line != "")//this is for old format files
                {
                    if (!useCommentAsImage)
                        setValue(MDL_COMMENT, line, id);
                    else
                        setValue(MDL_IMAGE, line, id);
                }
                int nCol = columnValues.size();
                for (int i = 0; i < nCol; ++i)
                    _parseObject(is, *(columnValues[i]), id);
            }
        }
    }
}

/* This function will be used to parse the rows data in START format
 */
void MetaData::_readRowsStar(mdBlock &block, std::vector<MDObject*> & columnValues)
{
    String line;
    std::stringstream ss;
    size_t nCol = columnValues.size();
    size_t id, n = block.end - block.loop;
    if (n==0)
        return;

    char * buffer = new char[n];
    memcpy(buffer, block.loop, n);
    char *iter = buffer, *end = iter + n, * newline = NULL;

    while (iter < end) //while there are data lines
    {
        //Assing \n position and check if NULL at the same time
        if (!(newline = END_OF_LINE()))
            newline = end;
        line.assign(iter, newline - iter);
        trim(line);
        if (!line.empty())
        {
            std::stringstream ss(line);
            id = addObject();
            for (int i = 0; i < nCol; ++i)
                _parseObject(ss, *(columnValues[i]), id);
        }
        iter = newline + 1; //go to next line
    }
    delete[] buffer;
}

/*This function will read the md data if is in row format */
void MetaData::_readRowFormat(std::istream& is)
{
    String line, token;
    MDLabel label;

    size_t objectID = addObject();

    // Read data and fill structures accordingly
    while (getline(is, line, '\n'))
    {
        if (line[0] == '#' || line[0] == '\0' || line[0] == ';')
            continue;

        // Parse labels
        std::stringstream os(line);

        os >> token;
        label = MDL::str2Label(token);
        MDObject value(label);
        os >> value;
        if (label != MDL_UNDEFINED)
            setValue(value, objectID);
    }
}
void MetaData::read(const FileName &_filename,
                    const std::vector<MDLabel> *desiredLabels,
                    bool decomposeStack)
{
    String BlockName;
    FileName filename;
    BlockName = _filename.getBlockName();
    //filename is global, so we can write the filename when reporting errors
    filename  = _filename.removeBlockName();
    _read(filename,desiredLabels,BlockName,decomposeStack);
    //_read calls clean so I cannot use eFilename as filename ROB
    // since eFilename is reset in clean
    eFilename = filename;
}

#define LINE_LENGTH 1024
void MetaData::readPlain(const FileName &inFile, const String &labelsString, const String &separator)
{

    clear();
    std::vector<MDLabel> labels;
    MDL::str2LabelVector(labelsString, labels);

    char lineBuffer[LINE_LENGTH];
    String line;
    std::ifstream is(inFile.data(), std::ios_base::in);
    size_t lineCounter = 0;
    size_t columnsNumber = labels.size();
    size_t objId;
    StringVector parts;

    while (is.getline(lineBuffer, LINE_LENGTH))
    {
        ++lineCounter;
        line.assign(lineBuffer);
        trim(line);
        if (line[0]=='#') // This is an old Xmipp comment
        	continue;
        if (!line.empty())
        {
            std::stringstream ss(line);
            objId = addObject();
            for (int i = 0; i < columnsNumber; ++i)
            {
                MDObject obj(labels[i]);
                _parseObject(ss, obj, objId);
                setValue(obj, objId);
            }
        }
    }
}

void MetaData::addPlain(const FileName &inFile, const String &labelsString, const String &separator)
{
    MetaData md2;
    md2.readPlain(inFile, labelsString);
    merge(md2);
}

bool MetaData::existsBlock(const FileName &_inFile)
{
    String blockName;
    FileName outFile;

    blockName=_inFile.getBlockName();
    outFile = _inFile.removeBlockName();

    struct stat file_status;
    int fd;
    char *map;

    //check if file exists or not block name has been given
    //in our format no two identical data_xxx strings may exists

    if (blockName.empty() || !outFile.exists())
        return false;
    else
    {
        //does blockname exists?
        //remove it from file in this case
        // get length of file:
        if(stat(outFile.data(), &file_status) != 0)
            REPORT_ERROR(ERR_IO_NOPATH,"Metadata:existsBlock can not get filesize for file "+outFile);
        size_t size = file_status.st_size;
        if(size!=0)//size=0 for /dev/stderr
        {
            fd = open(outFile.data(),  O_RDWR, S_IREAD | S_IWRITE);
            if (fd == -1)
                REPORT_ERROR(ERR_IO_NOPATH,"Metadata:existsBlock can not read file named "+outFile);

            map = (char *) mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (map == MAP_FAILED)
                REPORT_ERROR(ERR_MEM_BADREQUEST,"Metadata:existsBlock can not map memory ");

            // Is this a START formatted FILE
            String _szBlockName = (String)("\ndata_") + blockName;
            size_t blockNameSize = _szBlockName.size();

            if (_memmem(map, size, _szBlockName.data(), blockNameSize) == NULL)
                return false;
            else
                return true;
        }
        if (munmap(map, size) == -1)
        {
            REPORT_ERROR(ERR_MEM_NOTDEALLOC,"metadata:write, Can not unmap memory");
        }
        close(fd);
    }
}
void MetaData::_read(const FileName &filename,
                     const std::vector<MDLabel> *desiredLabels,
                     const String & blockRegExp,
                     bool decomposeStack)
{
    //First try to open the file as a metadata
    _clear();
    myMDSql->createMd();
    _isColumnFormat = true;

    size_t id;

    if (!(isMetadataFile = filename.isMetaData()))//if not a metadata, try to read as image or stack
    {
        Image<char> image;
        image.read(filename, HEADER);
        if (image().ndim == 1 || !decomposeStack) //single image
        {
            id = addObject();
            setValue(MDL_IMAGE, filename, id);
            setValue(MDL_ENABLED, 1, id);
        }
        else //stack
        {
            FileName fnTemp;
            for (size_t i = 1; i <= image().ndim; ++i)
            {
                fnTemp.compose(i, filename);
                id = addObject();
                setValue(MDL_IMAGE, fnTemp, id);
                setValue(MDL_ENABLED, 1, id);
            }
        }
        return;
    }

    std::ifstream is(filename.data(), std::ios_base::in);
    std::stringstream ss;
    String line, token;
    std::vector<MDObject*> columnValues;

    getline(is, line); //get first line to identify the type of file

    if (is.fail())
    {
        REPORT_ERROR(ERR_IO_NOTEXIST, formatString("MetaData::read: File doesn't exists: %s", filename.c_str()) );
    }

    bool useCommentAsImage = false;
    this->inFile = filename;
    bool oldFormat=true;

    is.seekg(0, std::ios::beg);//reset the stream position to the beginning to start parsing

    if (line.find("XMIPP_STAR_1") != String::npos)
    {
        oldFormat=false;

        // Read comment
        is.ignore(256,'#');
        is.ignore(256,'#');
        getline(is, line);
        trim(line);
        setComment(line);

        //map file
        int fd;
        BUFFER_CREATE(bufferMap);
        mapFile(filename, bufferMap.begin, bufferMap.size, fd);

        BLOCK_CREATE(block);
        regex_t re;
        int rc = regcomp(&re, (blockRegExp+"$").c_str(), REG_EXTENDED|REG_NOSUB);
        if (blockRegExp.size() && rc != 0)
            REPORT_ERROR(ERR_ARG_INCORRECT, formatString("Pattern '%s' cannot be parsed: %s",
                         blockRegExp.c_str(), filename.c_str()));
        BUFFER_COPY(bufferMap, buffer);
        bool firstBlock = true;
        bool singleBlock = blockRegExp.find_first_of(".[*+")==String::npos;
        String blockName;

        while (nextBlock(buffer, block))
            //startingPoint, remainingSize, firstData, secondData, firstloop))
        {
            BLOCK_NAME(block, blockName);
            if (blockRegExp.size() == 0 || regexec(&re, blockName.c_str(), (size_t) 0, NULL, 0)==0)
            {
                //Read column labels from the datablock that starts at firstData
                //Label ends at firstloop
                if ((_isColumnFormat = (block.loop != NULL)))
                {
                    _readColumnsStar(block, columnValues, desiredLabels, firstBlock);
                    // If block is empty, makes block.loop and block.end equal
                    if(block.loop == (block.end + 1))
                        block.loop--;
                    _readRowsStar(block, columnValues);
                }
                else
                {
                    id = addObject();
                    _readColumnsStar(block, columnValues, desiredLabels, firstBlock, id);
                }
                firstBlock = false;

                if (singleBlock)
                    break;
            }
        }

        unmapFile(bufferMap.begin, bufferMap.size, fd);
        regfree(&re);
        if (firstBlock)
            REPORT_ERROR(ERR_MD_BADBLOCK, formatString("Block: '%s': %s",
                         blockRegExp.c_str(), filename.c_str()));
    }
    else if (line.find("Headerinfo columns:") != String::npos)
    {
        //This looks like an old DocFile, parse header
        std::cerr << "WARNING: ** You are using an old file format (DOCFILE) which is going "
        << "to be deprecated in next Xmipp release **" << std::endl;
        is.ignore(256, ':'); //ignore all until ':' to start parsing column labels
        getline(is, line);
        ss.str(line);
        columnValues.push_back(new MDObject(MDL_UNDEFINED));
        columnValues.push_back(new MDObject(MDL_UNDEFINED));

        addLabel(MDL_IMAGE);
        _readColumns(ss, columnValues, desiredLabels);
        useCommentAsImage = true;
    }
    else
    {
        std::cerr << "WARNING: ** You are using an old file format (SELFILE) which is going "
        << "to be deprecated in next Xmipp release **" << std::endl;
        //I will assume that is an old SelFile, so only need to add two columns
        columnValues.push_back(new MDObject(MDL_IMAGE));//addLabel(MDL_IMAGE);
        columnValues.push_back(new MDObject(MDL_ENABLED));//addLabel(MDL_ENABLED);
    }

    if (oldFormat)
        _readRows(is, columnValues, useCommentAsImage);

    //free memory of column values
    int nCols = columnValues.size();
    for (int i = 0; i < nCols; ++i)
        delete columnValues[i];
}

void MetaData::merge(const MetaData &md2)
{
    if (size() != md2.size())
        REPORT_ERROR(ERR_MD, "Size of two metadatas should coincide for merging.");

    MDRow row;
    FOR_ALL_OBJECTS_IN_METADATA2(*this, md2)
    {
        md2.getRow(row, __iter2.objId);
        setRow(row, __iter.objId);
    }
}

void MetaData::fillExpand(MDLabel label)
{
    MDExpandGenerator generator;
    generator.label=label;
    generator.fill(*this);
}

void MetaData::fillConstant(MDLabel label, const String &value)
{
    MDConstGenerator generator(value);
    generator.label=label;
    generator.fill(*this);
}

void MetaData::aggregateSingle(MDObject &mdValueOut, AggregateOperation op,
                               MDLabel aggregateLabel)

{
    mdValueOut.setValue(myMDSql->aggregateSingleDouble(op,aggregateLabel));
}

void MetaData::aggregateSingleSizeT(MDObject &mdValueOut, AggregateOperation op,
                                    MDLabel aggregateLabel)

{
    mdValueOut.setValue(myMDSql->aggregateSingleSizeT(op,aggregateLabel));
}

void MetaData::aggregateSingleInt(MDObject &mdValueOut, AggregateOperation op,
                                  MDLabel aggregateLabel)

{
    size_t aux = myMDSql->aggregateSingleSizeT(op,aggregateLabel);
    int aux2 = (int) aux;
    mdValueOut.setValue(aux2);
}


bool MetaData::nextBlock(mdBuffer &buffer, mdBlock &block)
{
    BLOCK_INIT(block);
    if (buffer.size == 0)
        return false;
    // Search for data_ after a newline
    block.begin = BUFFER_FIND(buffer, "\ndata_", 6);

    if (block.begin) // data_ FOUND!!!
    {
        block.begin += 6; //Shift \ndata_
        size_t n = block.begin - buffer.begin;
        BUFFER_MOVE(buffer, n);
        //Search for the end of line
        char *newLine = BUFFER_FIND(buffer, "\n", 1);
        //Calculate lenght of block name, counting after data_
        block.nameSize = newLine - buffer.begin;
        //Search for next block if exists one
        //use assign and check if not NULL at same time
        if (!(block.end = BUFFER_FIND(buffer, "\ndata_", 6)))
            block.end = block.begin + buffer.size;
        block.loop = BUFFER_FIND(buffer, "\nloop_", 6);
        //If loop_ is not found or is found outside block
        //scope, the block is in column format
        if (block.loop)
        {
            if (block.loop < block.end)
                block.loop += 6; // Shift \nloop_
            else
                block.loop = NULL;
        }
        //Move buffer to end of block
        n = block.end - buffer.begin;
        BUFFER_MOVE(buffer, n);
        return true;
    }

    return false;
}

void MetaData::aggregate(const MetaData &mdIn, AggregateOperation op,
                         MDLabel aggregateLabel, MDLabel operateLabel, MDLabel resultLabel)
{
    std::vector<MDLabel> labels(2);
    std::vector<MDLabel> operateLabels(1);
    labels[0] = aggregateLabel;
    labels[1] = resultLabel;
    operateLabels[0]=operateLabel;
    init(&labels);
    std::vector<AggregateOperation> ops(1);
    ops[0] = op;
    mdIn.myMDSql->aggregateMd(this, ops, operateLabels);
}

void MetaData::aggregate(const MetaData &mdIn, const std::vector<AggregateOperation> &ops,
                         const std::vector<MDLabel> &operateLabels,
                         const std::vector<MDLabel> &resultLabels)
{
    if (resultLabels.size() - ops.size() != 1)
        REPORT_ERROR(ERR_MD, "Labels vectors should contain one element more than operations");
    init(&resultLabels);
    mdIn.myMDSql->aggregateMd(this, ops, operateLabels);
}

void MetaData::aggregateGroupBy(const MetaData &mdIn,
                                AggregateOperation op,
                                const std::vector<MDLabel> &groupByLabels,
                                MDLabel operateLabel,
                                MDLabel resultLabel)
{
    std::vector<MDLabel> labels;
    labels = groupByLabels;
    labels.push_back(resultLabel);
    init(&labels);
    mdIn.myMDSql->aggregateMdGroupBy(this, op, groupByLabels, operateLabel, resultLabel);
}

//-------------Set Operations ----------------------
void MetaData::_setOperates(const MetaData &mdIn, const MDLabel label, SetOperation operation)
{
    if (this == &mdIn) //not sense to operate on same metadata
        REPORT_ERROR(ERR_MD, "Couldn't perform this operation on input metadata");
    if (size() == 0 && mdIn.size() == 0)
        REPORT_ERROR(ERR_MD, "Couldn't perform this operation if both metadata are empty");
    //Add labels to be sure are present
    for (int i = 0; i < mdIn.activeLabels.size(); i++)
        addLabel(mdIn.activeLabels[i]);

    mdIn.myMDSql->setOperate(this, label, operation);
}

void MetaData::_setOperates(const MetaData &mdInLeft,
                            const MetaData &mdInRight,
                            const MDLabel labelLeft,
                            const MDLabel labelRight,
                            SetOperation operation)
{
    if (this == &mdInLeft || this == &mdInRight) //not sense to operate on same metadata
        REPORT_ERROR(ERR_MD, "Couldn't perform this operation on input metadata");
    //Add labels to be sure are present
    for (int i = 0; i < mdInLeft.activeLabels.size(); i++)
        addLabel(mdInLeft.activeLabels[i]);
    for (int i = 0; i < mdInRight.activeLabels.size(); i++)
        if(mdInRight.activeLabels[i]!=labelRight)
            addLabel(mdInRight.activeLabels[i]);

    myMDSql->setOperate(&mdInLeft, &mdInRight, labelLeft,labelRight, operation);
}

void MetaData::unionDistinct(const MetaData &mdIn, const MDLabel label)
{
    if(mdIn.isEmpty())
        return;
    _setOperates(mdIn, label, UNION_DISTINCT);
}

void MetaData::unionAll(const MetaData &mdIn)
{
    if(mdIn.isEmpty())
        return;
    _setOperates(mdIn, MDL_UNDEFINED, UNION);//label not needed for unionAll operation
}


void MetaData::intersection(const MetaData &mdIn, const MDLabel label)
{
    if(mdIn.isEmpty())
        clear();
    else
        _setOperates(mdIn, label, INTERSECTION);
}

void MetaData::removeDuplicates(MetaData &MDin)
{
    if(MDin.isEmpty())
        return;
    _setOperates(MDin, MDL_UNDEFINED, REMOVE_DUPLICATE);
}

void MetaData::removeDisabled()
{
    if (containsLabel(MDL_ENABLED))
        removeObjects(MDValueLE(MDL_ENABLED, 0)); // Remove values -1 and 0 on MDL_ENABLED label
}

void MetaData::subtraction(const MetaData &mdIn, const MDLabel label)
{
    if(mdIn.isEmpty())
        return;
    _setOperates(mdIn, label, SUBSTRACTION);
}

void MetaData::join(const MetaData &mdInLeft, const MetaData &mdInRight, const MDLabel label, JoinType type)
{
    join(mdInLeft, mdInRight, label, label, type);
}

void MetaData::join(const MetaData &mdInLeft, const MetaData &mdInRight, const MDLabel labelLeft,
                    const MDLabel labelRight, JoinType type)
{
    clear();
    _setOperates(mdInLeft, mdInRight, labelLeft,labelRight, (SetOperation)type);
}

void MetaData::operate(const String &expression)
{
    if (!myMDSql->operate(expression))
        REPORT_ERROR(ERR_MD, "MetaData::operate: error doing operation");
}

void MetaData::replace(const MDLabel label, const String &oldStr, const String &newStr)
{
    const char * labelStr = MDL::label2Str(label).c_str();
    String expression = formatString("%s=replace(%s,'%s', '%s')",
                                     labelStr, labelStr, oldStr.c_str(), newStr.c_str());
    if (!myMDSql->operate(expression))
        REPORT_ERROR(ERR_MD, "MetaData::replace: error doing operation");
}

void MetaData::randomize(MetaData &MDin)
{
    static bool randomized = false;
    if (!randomized)
    {
        srand ( time(NULL) );
        randomized = true;
    }
    std::vector<size_t> objects;
    MDin.myMDSql->selectObjects(objects);
    std::random_shuffle(objects.begin(), objects.end());
    importObjects(MDin, objects);
}

void MetaData::sort(MetaData &MDin, const MDLabel sortLabel,bool asc, int limit, int offset)
{
    if (MDin.containsLabel(sortLabel))
    {
        init(&(MDin.activeLabels));
        copyInfo(MDin);
        MDQuery query(limit, offset, sortLabel,asc);
        MDin.myMDSql->copyObjects(this, &query);
    }
    else
        *this=MDin;
}

void MetaData::sort(MetaData &MDin, const String &sortLabel,bool asc, int limit, int offset)
{
    // Check if the label has semicolon
    int ipos=sortLabel.find(':');
    MDLabelType type = MDL::labelType(sortLabel);
    if (ipos!=String::npos || type == LABEL_VECTOR || type == LABEL_VECTOR_LONG)
    {
        if(limit != -1 || offset != 0)
            REPORT_ERROR(ERR_ARG_INCORRECT,"Limit and Offset are not implemented for vector sorting.");

        MDLabel label;
        int column;
        if (ipos!=String::npos)
        {
            // Check that the label is a vector field
            std::vector< String > results;
            splitString(sortLabel,":",results);
            column=textToInteger(results[1]);
            MDLabelType type = MDL::labelType(results[0]);
            if (type != LABEL_VECTOR || type != LABEL_VECTOR_LONG)
                REPORT_ERROR(ERR_ARG_INCORRECT,"Column specifications cannot be used with non-vector labels");
            label = MDL::str2Label(results[0]);
        }
        else
        {
            label = MDL::str2Label(sortLabel);
            column = 0;
        }

        // Get the column values
        MultidimArray<double> v;
        v.resizeNoCopy(MDin.size());
        std::vector<double> vectorValues;
        int i=0;
        FOR_ALL_OBJECTS_IN_METADATA(MDin)
        {
            MDin.getValue(label,vectorValues,__iter.objId);
            if (column>=vectorValues.size())
                REPORT_ERROR(ERR_MULTIDIM_SIZE,"Trying to access to inexistent column in vector");
            DIRECT_A1D_ELEM(v,i)=vectorValues[column];
            i++;
        }

        // Sort
        MultidimArray<int> idx;
        v.indexSort(idx);

        // Construct output Metadata
        init(&(MDin.activeLabels));
        copyInfo(MDin);
        size_t id;
        FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY1D(idx)
        {
            MDRow row;
            MDin.getRow(row,DIRECT_A1D_ELEM(idx,i));
            id = addObject();
            setRow(row, id);
        }
    }
    else
        sort(MDin, MDL::str2Label(sortLabel),asc, limit, offset);
}

void MetaData::split(int n, std::vector<MetaData> &results, const MDLabel sortLabel)
{
    size_t mdSize = size();
    if (n > mdSize)
        REPORT_ERROR(ERR_MD, "MetaData::split: Couldn't split a metadata in more parts than its size");

    results.clear();
    results.resize(n);
    for (int i = 0; i < n; i++)
    {
        MetaData &md = results.at(i);
        md._selectSplitPart(*this, n, i, mdSize, sortLabel);
    }
}

void MetaData::_selectSplitPart(const MetaData &mdIn,
                                int n, int part, size_t mdSize,
                                const MDLabel sortLabel)
{
    size_t first, last, n_images;
    n_images = divide_equally(mdSize, n, part, first, last);
    init(&(mdIn.activeLabels));
    copyInfo(mdIn);
    mdIn.myMDSql->copyObjects(this, new MDQuery(n_images, first, sortLabel));
}

void MetaData::selectSplitPart(const MetaData &mdIn, int n, int part, const MDLabel sortLabel)
{
    size_t mdSize = mdIn.size();
    if (n > mdSize)
        REPORT_ERROR(ERR_MD, "selectSplitPart: Couldn't split a metadata in more parts than its size");
    if (part < 0 || part >= n)
        REPORT_ERROR(ERR_MD, "selectSplitPart: 'part' should be between 0 and n-1");
    _selectSplitPart(mdIn, n, part, mdSize, sortLabel);

}

void MetaData::selectPart(const MetaData &mdIn, size_t startPosition, size_t numberOfObjects,
                          const MDLabel sortLabel)
{
    size_t mdSize = mdIn.size();
    if (startPosition < 0 || startPosition >= mdSize)
        REPORT_ERROR(ERR_MD, "selectPart: 'startPosition' should be between 0 and size()-1");
    init(&(mdIn.activeLabels));
    copyInfo(mdIn);
    mdIn.myMDSql->copyObjects(this, new MDQuery(numberOfObjects, startPosition, sortLabel));
}

void MetaData::makeAbsPath(const MDLabel label)
{

    String aux_string;
    String aux_string_path;
    char buffer[1024];

    getcwd(buffer, 1023);
    String path_str(buffer);
    path_str += "/";
    getValue(label, aux_string, firstObject());

    if (aux_string[0] == '/')
        return;

    FileName auxFile;
    FOR_ALL_OBJECTS_IN_METADATA(*this)
    {
        aux_string_path = path_str;
        getValue(label, auxFile, __iter.objId);

        if (auxFile.isInStack())
        {
            size_t id = auxFile.find('@',0);
            auxFile.insert(id+1,aux_string_path);
            setValue(label, auxFile, __iter.objId);
        }
        else
        {
            auxFile.addPrefix(aux_string_path);
            setValue(label, auxFile, __iter.objId);
        }
    }
}

void MetaData::convertXML(FileName fn)
{

    std::ofstream ofs(fn.data(), std::ios_base::out);

    size_t size = activeLabels.size();
    ofs <<  "<" << fn << ">"<< std::endl;
    FOR_ALL_OBJECTS_IN_METADATA(*this)
    {
        ofs <<  "<ROW ";
        for (size_t i = 0; i < size; i++)
        {
            if (activeLabels[i] != MDL_COMMENT)
            {
                ofs << MDL::label2Str(activeLabels[i]) << "=\"";
                MDObject mdValue(activeLabels[i]);
                //ofs.width(1);
                myMDSql->getObjectValue(__iter.objId, mdValue);
                mdValue.toStream(ofs, true);
                ofs << "\" ";
            }
        }
        ofs <<  " />" << std::endl;
    }
    ofs <<  "</" << fn << ">"<< std::endl;
}

bool MetaData::operator==(const MetaData& op) const
{
    return myMDSql->equals(*(op.myMDSql));
}

std::ostream& operator<<(std::ostream& o, const MetaData & mD)
{
    mD.write(o);
    return o;
}


void MDIterator::init(const MetaData &md, const MDQuery * pQuery)
{
    std::vector<size_t> objectsVector;
    md.myMDSql->selectObjects(objectsVector, pQuery);
    objects = NULL;
    objId = BAD_OBJID;
    objIndex = BAD_INDEX;
    size = objectsVector.size();

    if (size > 0)
    {
        objects = new size_t[size];
        size_t * objPtr = &(objectsVector[0]);
        memcpy(objects, objPtr, sizeof(size_t) * size);//copy objects id's
        objIndex = 0;
        objId = objects[0];
    }
}

MDIterator::MDIterator()
{
    objects = NULL;
    objId = BAD_OBJID;
    objIndex = BAD_INDEX;
    size = 0;
}

MDIterator::MDIterator(const MetaData &md)
{
    init(md);
}

MDIterator::MDIterator(const MetaData &md, const MDQuery &query)
{
    init(md, &query);
}

MDIterator::~MDIterator()
{
    delete [] objects;
}

bool MDIterator::moveNext()
{
    if (objects == NULL)
        return false;

    if (++objIndex < size)
    {
        objId = objects[objIndex];
        return true;
    }
    objId = BAD_OBJID;
    objIndex = BAD_INDEX;
    return false;

}
bool MDIterator::hasNext()
{
    return (objects != NULL && objIndex < size);
}

//////////// Generators implementations
inline double MDRandGenerator::getRandValue()
{
    switch (mode)
    {
    case GTOR_UNIFORM:
        return rnd_unif(op1, op2);
    case GTOR_GAUSSIAN:
        return rnd_gaus(op1, op2);
    case GTOR_STUDENT:
        return rnd_student_t(op3, op1, op2);
    }
}
MDRandGenerator::MDRandGenerator(double op1, double op2, const String &mode, double op3)
{
    static bool randomized = false;

    if (!randomized)//initialize random seed just once
    {
        randomize_random_generator();
        randomized = true;
    }
    this->op1 = op1;
    this->op2 = op2;
    this->op3 = op3;
    if (mode == "uniform")
        this->mode = GTOR_UNIFORM;
    else if (mode == "gaussian")
        this->mode = GTOR_GAUSSIAN;
    else if (mode == "student")
        this->mode = GTOR_STUDENT;
    else
        REPORT_ERROR(ERR_PARAM_INCORRECT, formatString("Unknown random type '%s'", mode.c_str()));

}

bool MDRandGenerator::fillValue(MetaData &md, size_t objId)
{
    double aux = getRandValue();
    md.setValue(label, aux, objId);
}

MDConstGenerator::MDConstGenerator(const String &value)
{
    this->value = value;
}
bool MDConstGenerator::fillValue(MetaData &md, size_t objId)
{
    md.setValueFromStr(label, value, objId);
}

MDLinealGenerator::MDLinealGenerator(double initial, double step)
{
    this->initValue = initial;
    this->step = step;
    counter = 0;
}

bool MDLinealGenerator::fillValue(MetaData &md, size_t objId)
{
    double value = initValue + step * counter++;
    if (MDL::isInt(label))
        md.setValue(label, ROUND(value), objId);
    else
        md.setValue(label, value, objId);
}

WriteModeMetaData metadataModeConvert (String mode)
{
    toLower(mode);
    if (mode.npos != mode.find("overwrite"))
        return MD_OVERWRITE;
    if (mode.npos != mode.find("append"))
        return MD_APPEND;
    REPORT_ERROR(ERR_ARG_INCORRECT,"metadataModeConvert: Invalid mode");
}

/* Class to generate values for columns of a metadata*/
bool MDValueGenerator::fill(MetaData &md)
{
    FOR_ALL_OBJECTS_IN_METADATA(md)
    {
        fillValue(md, __iter.objId);
    }
}

/* Class to fill columns with another metadata in row format */
bool MDExpandGenerator::fillValue(MetaData &md, size_t objId)
{
    if (md.getValue(label, fn, objId))
    {
        expMd.read(fn);
        if (expMd.isColumnFormat() || expMd.isEmpty())
            REPORT_ERROR(ERR_VALUE_INCORRECT, "Only can expand non empty and row formated metadatas");
        expMd.getRow(row, expMd.firstObject());
        md.setRow(row, objId);
    }
    else
        REPORT_ERROR(ERR_MD_BADLABEL, formatString("Can't expand missing label '%s'", MDL::label2Str(label).c_str()));
}

