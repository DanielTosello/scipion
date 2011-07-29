/***************************************************************************
 * Authors:     Joaquin Oton (joton@cnb.csic.es)
 *
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

#include "xmipp_image_base.h"
#include "xmipp_image.h"

//This is needed for static memory allocation

void ImageBase::init()
{
    clearHeader();
    dataMode = DATA;
    if (isComplexT())
        transform = Standard;
    else
        transform = NoTransform;

    filename = "";
    offset = 0;
    swap = swapWrite = 0;
    replaceNsize=0;
    mmapOnRead = mmapOnWrite = false;
    mappedSize = 0;
    mFd    = NULL;
}

void ImageBase::clearHeader()
{
    MDMainHeader.clear();
    MD.clear();
    //Just to ensure there is an empty MDRow
    MD.push_back(MDMainHeader);
}

/** General read function
 */
int ImageBase::read(const FileName &name, DataMode datamode, size_t select_img,
                    bool mapData, int mode)
{
    if (!mapData)
        mode = WRITE_READONLY;

    hFile = openFile(name, mode);
    int err = _read(name, hFile, datamode, select_img, mapData);
    closeFile(hFile);

    return err;
}

int ImageBase::readMapped(const FileName &name, size_t select_img, int mode)
{
    read(name, HEADER);
    bool swap = this->swap > 0;
    return read(name, DATA, select_img, !swap, mode);
}

/** New mapped file */
void ImageBase::mapFile2Write(int Xdim, int Ydim, int Zdim, const FileName &_filename,
                              bool createTempFile, size_t select_img, bool isStack, int mode)
{
    mmapOnWrite = true;
    setDimensions(Xdim, Ydim, Zdim, 1); // Images with Ndim >1 cannot be mapped to image file
    MD.resize(1);
    filename = _filename;
    FileName fnToOpen;
    if (createTempFile)
    {
        tempFilename.initUniqueName("temp_XXXXXX");
        fnToOpen = tempFilename + ":" + _filename.getExtension();
    }
    else
        fnToOpen=_filename;

    /* If the filename is in stack or an image is selected, we will suppose
     * you want to write this, even if you have not set the flags to.
     */
    if ( (filename.isInStack() || select_img > ALL_IMAGES) && mode == WRITE_OVERWRITE)
    {
        isStack = true;
        mode = WRITE_REPLACE;
    }

    hFile = openFile(fnToOpen, mode);
    _write(filename, hFile, select_img, isStack, mode);
    closeFile(hFile);
}

/** General read function
 */
/** Macros for dont type */
#define GET_ROW()               MDRow row; md.getRow(row, objId)

#define READ_AND_RETURN()        ImageFHandler* hFile = openFile(name);\
                                  int err = _read(name, hFile, datamode, select_img); \
                                  applyGeo(row, only_apply_shifts, wrap); \
                                  closeFile(hFile); \
                                  return err

#define APPLY_GEO()        MDRow row; md.getRow(row, objId); \
                           applyGeo(row, only_apply_shifts, wrap) \

void ImageBase::applyGeo(const MetaData &md, size_t objId, bool only_apply_shifts, bool wrap)
{
    APPLY_GEO();
}

int ImageBase::readApplyGeo(const FileName &name, const MDRow &row, bool only_apply_shifts, DataMode datamode, size_t select_img, bool wrap)
{
    READ_AND_RETURN();
}

/** Read an image from metadata, filename is provided
*/
int ImageBase::readApplyGeo(const FileName &name, const MetaData &md, size_t objId, bool only_apply_shifts, DataMode datamode,
                            size_t select_img, bool wrap)
{
    GET_ROW();
    READ_AND_RETURN();
}

/** Read an image from metadata, filename is taken from MDL_IMAGE
 */
int ImageBase::readApplyGeo(const MetaData &md, size_t objId, bool only_apply_shifts, DataMode datamode, size_t select_img, bool wrap)
{
    GET_ROW();
    FileName name;
    row.getValue(MDL_IMAGE, name);
    READ_AND_RETURN();
}


void ImageBase::write(const FileName &name, size_t select_img, bool isStack,
                      int mode, CastWriteMode castMode, int _swapWrite)
{
    const FileName &fname = (name.empty()) ? filename : name;

    if (mmapOnWrite && mappedSize > 0)
    {
        bool hasTempFile = !tempFilename.empty();
        if (hasTempFile && fname.isInStack())
            mmapOnWrite = !(mmapOnRead = true); // We change mmap mode from write to read to allow writing the image into a stack.
        else
        {
            if (_swapWrite > 0)
                REPORT_ERROR(ERR_ARG_INCORRECT, "Cannot swap endianess on writing if file is already mapped.");
            munmapFile();
            if (hasTempFile && std::rename(tempFilename.c_str(), fname.c_str()) != 0)
                REPORT_ERROR(ERR_IO, formatString("Error renaming file '%s' to '%s'.", tempFilename.c_str(), fname.c_str()));
            return;
        }
    }
    // Swap the endianess of the image file when writing
    swapWrite = _swapWrite;

    /* If the filename is in stack we will suppose you want to write this,
     * even if you have not set the flags to.
     */
    if ( fname.isInStack() && mode == WRITE_OVERWRITE)
    {
        isStack = true;
        mode = WRITE_REPLACE;
    }
    //    else if (!isStack && mode != WRITE_OVERWRITE)
    //        mode = WRITE_OVERWRITE;

    hFile = openFile(fname, mode);
    _write(fname, hFile, select_img, isStack, mode, castMode);
    closeFile(hFile);
}

void ImageBase::swapPage(char * page, size_t pageNrElements, DataType datatype, int swap)
{
    size_t datatypesize = gettypesize(datatype);
#ifdef DEBUG

    std::cerr<<"DEBUG swapPage: Swapping image data with swap= "
    << swap<<" datatypesize= "<<datatypesize
    << " pageNrElements " << pageNrElements
    << " datatype " << datatype
    <<std::endl;
    ;
#endif

    // Swap bytes if required
    if ( swap == 1 )
    {
        if ( datatype >= ComplexShort )
            datatypesize /= 2;
        for ( size_t i = 0; i < pageNrElements; i += datatypesize )
            swapbytes(page+i, datatypesize);
    }
    else if ( swap > 1 )
    {
        for (size_t i=0; i<pageNrElements; i+=swap )
            swapbytes(page+i, swap);
    }
}

/** Get Rot angle
    *
    * @code
    * std::cout << "First Euler angle " << I.rot() << std::endl;
    * @endcode
    */
double ImageBase::rot(const size_t n) const
{
    double dummy = 0;
    MD[n].getValue(MDL_ANGLEROT, dummy);
    return dummy;
}

/** Get Tilt angle
 *
 * @code
 * std::cout << "Second Euler angle " << I.tilt() << std::endl;
 * @endcode
 */
double ImageBase::tilt(const size_t n) const
{
    double dummy = 0;
    MD[n].getValue(MDL_ANGLETILT, dummy);
    return dummy;
}

/** Get Psi angle
 *
 * @code
 * std::cout << "Third Euler angle " << I.psi() << std::endl;
 * @endcode
 */
double ImageBase::psi(const size_t n) const
{
    double dummy = 0;
    MD[n].getValue(MDL_ANGLEPSI, dummy);
    return dummy;
}

/** Get Xoff
 *
 * @code
 * std::cout << "Origin offset in X " << I.Xoff() << std::endl;
 * @endcode
 */
double ImageBase::Xoff(const size_t n) const
{
    double dummy = 0;
    MD[n].getValue(MDL_SHIFTX, dummy);
    return dummy;
}

/** Get Yoff
 *
 * @code
 * std::cout << "Origin offset in Y " << I.Yoff() << std::endl;
 * @endcode
 */
double ImageBase::Yoff(const size_t n) const
{
    double dummy = 0;
    MD[n].getValue(MDL_SHIFTY, dummy);
    return dummy;
}

/** Get Zoff
 *
 * @code
 * std::cout << "Origin offset in Z " << I.Zoff() << std::endl;
 * @endcode
 */
double ImageBase::Zoff(const size_t n) const
{
    double dummy = 0;
    MD[n].getValue(MDL_SHIFTZ, dummy);
    return dummy;
}

/** Get Weight
*
* @code
* std::cout << "weight= " << I.weight() << std::endl;
* @endcode
*/
double ImageBase::weight(const size_t n) const
{
    double dummy = 1;
    MD[n].getValue(MDL_WEIGHT, dummy);
    return dummy;
}

/** Get Scale factor
*
* @code
* std::cout << "scale= " << I.scale() << std::endl;
* @endcode
*/
double ImageBase::scale(const size_t n) const
{
    double dummy = 1;
    MD[n].getValue(MDL_SCALE, dummy);
    return dummy;
}


/** Get Flip
*
* @code
* std::cout << "flip= " << flip() << std::endl;
* @endcode
*/
bool ImageBase::flip(const size_t n) const
{
    bool dummy = false;
    MD[n].getValue(MDL_FLIP, dummy);
    return dummy;
}

/** Data type
    *
    * @code
    * std::cout << "datatype= " << dataType() << std::endl;
    * @endcode
    */
DataType ImageBase::dataType() const
{
    int dummy;
    MDMainHeader.getValue(MDL_DATATYPE, dummy);
    return (DataType)dummy;
}

/** Sampling RateX
*
* @code
* std::cout << "sampling= " << samplingRateX() << std::endl;
* @endcode
*/
double ImageBase::samplingRateX() const
{
    double dummy = 1.;
    MDMainHeader.getValue(MDL_SAMPLINGRATEX, dummy);
    return dummy;
}

/** Set Euler angles in image header
 */
void ImageBase::setEulerAngles(double rot, double tilt, double psi,
                               const size_t n)
{
    MD[n].setValue(MDL_ANGLEROT, rot);
    MD[n].setValue(MDL_ANGLETILT, tilt);
    MD[n].setValue(MDL_ANGLEPSI, psi);
}

/** Get Euler angles from image header
 */
void ImageBase::getEulerAngles(double &rot, double &tilt, double &psi,
                               const size_t n) const
{
    MD[n].getValue(MDL_ANGLEROT, rot);
    MD[n].getValue(MDL_ANGLETILT, tilt);
    MD[n].getValue(MDL_ANGLEPSI, psi);
}

/** Set origin offsets in image header
     */
void ImageBase::setShifts(double xoff, double yoff, double zoff, const size_t n)
{
    MD[n].setValue(MDL_SHIFTX, xoff);
    MD[n].setValue(MDL_SHIFTY, yoff);
    MD[n].setValue(MDL_SHIFTZ, zoff);
}
/** Get origin offsets from image header
  */
void ImageBase::getShifts(double &xoff, double &yoff, double &zoff, const size_t n) const
{
    MD[n].getValue(MDL_SHIFTX, xoff);
    MD[n].getValue(MDL_SHIFTY, yoff);
    MD[n].getValue(MDL_SHIFTZ, zoff);
}

/** Open file function
  * Open the image file and returns its file hander.
  */
ImageFHandler* ImageBase::openFile(const FileName &name, int mode) const
{
    if (name.empty())
        REPORT_ERROR(ERR_PARAM_INCORRECT, "ImageBase::openFile Cannot open an empty Filename.");

    ImageFHandler* hFile = new ImageFHandler;
    FileName fileName, headName = "";
    FileName ext_name = name.getFileFormat();

    // Remove image number
    size_t dump;
    name.decompose(dump, fileName);

    fileName = fileName.removeFileFormat();

    size_t found = fileName.find_first_of("%");
    if (found!=String::npos)
        fileName = fileName.substr(0, found) ;

    hFile->exist = exists(fileName);
    hFile->mode = mode;

    String wmChar;

    switch (mode)
    {
    case WRITE_READONLY:
        if (!hFile->exist)
            REPORT_ERROR(ERR_IO_NOTEXIST, formatString("Cannot read file '%s'. It doesn't exist", name.c_str()));
        wmChar = "r";
        break;
    case WRITE_OVERWRITE:
        wmChar = "w";
        break;
    case WRITE_APPEND:
    case WRITE_REPLACE:
        wmChar = (hFile->exist) ? "r+" : "w+";
        break;

    }

    if (ext_name.contains("tif"))
    {
        TIFFSetWarningHandler(NULL); // Switch off warning messages
        if ((hFile->tif = TIFFOpen(fileName.c_str(), wmChar.c_str())) == NULL)
            REPORT_ERROR(ERR_IO_NOTOPEN,"rwTIFF: There is a problem opening the TIFF file.");
        hFile->fimg = NULL;
        hFile->fhed = NULL;
    }
    else
    {
        hFile->tif = NULL;

        if (ext_name.contains("img") || ext_name.contains("hed"))
        {
            fileName = fileName.withoutExtension();
            headName = fileName.addExtension("hed");
            fileName = fileName.addExtension("img");
        }
        else if (ext_name.contains("raw"))
        {
            if (mode != WRITE_READONLY || exists(fileName.addExtension("inf")) )
            {
                headName = fileName.addExtension("inf");
                ext_name = "inf";
            }
            else
                ext_name = "raw";
        }
        else if (ext_name.contains("inf"))
        {
            headName = fileName;
            fileName = fileName.withoutExtension();
            ext_name = "inf";
        }

        // Open image file
        if ( (hFile->fimg = fopen(fileName.c_str(), wmChar.c_str())) == NULL )
        {
            if (errno == EACCES)
                REPORT_ERROR(ERR_IO_NOPERM,formatString("Image::openFile: permission denied when opening %s",fileName.c_str()));
            else
                REPORT_ERROR(ERR_IO_NOTOPEN,formatString("Image::openFile cannot open: %s", fileName.c_str()));
        }


        if (headName != "")
        {
            if ((hFile->fhed = fopen(headName.c_str(), wmChar.c_str()))  == NULL )
            {
                if (errno == EACCES)
                    REPORT_ERROR(ERR_IO_NOPERM,formatString("Image::openFile: permission denied when opening %s",headName.c_str()));
                else
                    REPORT_ERROR(ERR_IO_NOTOPEN,formatString("Image::openFile cannot open: %s",headName.c_str()));
            }

        }
        else
            hFile->fhed = NULL;

    }
    hFile->fileName = fileName;
    hFile->headName = headName;
    hFile->ext_name = ext_name;

    return hFile;
}

/** Close file function.
  * Close the image file according to its name and file handler.
  */
void ImageBase::closeFile(ImageFHandler* hFile) const
{
    FileName ext_name;
    FILE* fimg, *fhed;
    TIFF* tif;

    if (hFile != NULL)
    {
        ext_name = hFile->ext_name;
        fimg = hFile->fimg;
        fhed = hFile->fhed;
        tif  = hFile->tif;
    }
    else
    {
        ext_name = filename.getFileFormat();
        fimg = this->fimg;
        fhed = this->fhed;
        tif  = this->tif;
    }

    if (ext_name.contains("tif"))
        TIFFClose(tif);
    else
    {
        if (fclose(fimg) != 0 )
            REPORT_ERROR(ERR_IO_NOCLOSED,(String)"Can not close image file "+ filename);

        if (fhed != NULL &&  fclose(fhed) != 0 )
            REPORT_ERROR(ERR_IO_NOCLOSED,(String)"Can not close header file of "
                         + filename);
    }
    delete hFile;
}

/* Internal read image file method.
 */
int ImageBase::_read(const FileName &name, ImageFHandler* hFile, DataMode datamode, size_t select_img,
                     bool mapData)
{
    // Temporary Error to find old select_img == -1
    if (select_img == (size_t) -1)
        REPORT_ERROR(ERR_DEBUG_TEST, "To select all images use ALL_IMAGES macro, or FIRST_IMAGE macro.");

    int err = 0;
    dataMode = datamode;

    // If Image has been previously used with mmap, then close the previous file
    if (mappedSize != 0)
        munmapFile();

    // Check whether to map the data or not
    mmapOnRead = mapData;

    FileName ext_name = hFile->ext_name;
    fimg = hFile->fimg;
    fhed = hFile->fhed;
    tif  = hFile->tif;

    size_t image_num;
    name.decompose(image_num, filename);
    filename = name;
    dataFName = hFile->fileName;

    if (image_num != ALL_IMAGES)
        select_img = image_num;

#undef DEBUG
    //#define DEBUG
#ifdef DEBUG

    std::cerr << "READ\n" <<
    "name="<<name <<std::endl;
    std::cerr << "ext= "<<ext_name <<std::endl;
    std::cerr << " now reading: "<< filename <<" dataflag= "<<dataMode
    << " select_img "  << select_img << std::endl;
#endif
#undef DEBUG

    //Just clear the header before reading
    MDMainHeader.clear();
    //Set the file pointer at beginning
    if (fimg != NULL)
        fseek(fimg, 0, SEEK_SET);
    if (fhed != NULL)
        fseek(fhed, 0, SEEK_SET);

    if (ext_name.contains("spi") || ext_name.contains("xmp")  ||
        ext_name.contains("stk") || ext_name.contains("vol"))//mrc stack MUST go BEFORE plain MRC
        err = readSPIDER(select_img);
    else if (ext_name.contains("mrcs"))//mrc stack MUST go BEFORE plain MRC
        err = readMRC(select_img,true);
    else if (ext_name.contains("mrc"))//mrc
        err = readMRC(select_img,false);
    else if (ext_name.contains("img") || ext_name.contains("hed"))//
        err = readIMAGIC(select_img);//imagic is always an stack
    else if (ext_name.contains("ser"))//TIA
        err = readTIA(select_img,false);
    else if (ext_name.contains("dm3"))//DM3
        err = readDM3(select_img,false);
    else if (ext_name.contains("inf"))//RAW with INF file
        err = readINF(select_img,false);
    else if (ext_name.contains("raw"))//RAW without INF file
        err = readRAW(select_img,false);
    else if (ext_name.contains("tif"))//TIFF
        err = readTIFF(select_img,false);
    else if (ext_name.contains("spe"))//SPE
        err = readSPE(select_img,false);
    else
        err = readSPIDER(select_img);

    // Negative errors are bad.
    return err;
}

/* Internal write image file method.
 */
void ImageBase::_write(const FileName &name, ImageFHandler* hFile, size_t select_img,
                       bool isStack, int mode, CastWriteMode castMode)
{

    // Temporary Error to find old select_img == -1
    if (select_img == (size_t) -1)
        REPORT_ERROR(ERR_DEBUG_TEST, "To select all images use ALL_IMAGES macro, or FIRST_IMAGE macro.");

    int err = 0;

    // if image is mapped to file then close the file and clear
    if (mmapOnWrite && mappedSize > 0)
    {
        munmapFile();
        return;
    }

    filename = name;
    dataFName = hFile->fileName;
    _exists = hFile->exist;
    fimg = hFile->fimg;
    fhed = hFile->fhed;
    tif  = hFile->tif;

    FileName ext_name = hFile->ext_name;

    size_t aux;
    FileName filNamePlusExt;
    name.decompose(aux, filNamePlusExt);

    if (select_img == ALL_IMAGES)
        select_img = aux;

    size_t found = filNamePlusExt.find_first_of("%");

    String imParam = "";

    if (found!=String::npos)
    {
        imParam =  filNamePlusExt.substr(found+1).c_str();
        filNamePlusExt = filNamePlusExt.substr(0, found) ;
    }

    found = filNamePlusExt.find_first_of(":");
    if ( found!=String::npos)
        filNamePlusExt   = filNamePlusExt.substr(0, found);


    //#define DEBUG
#ifdef DEBUG

    std::cerr << "write" <<std::endl;
    std::cerr<<"extension for write= "<<ext_name<<std::endl;
    std::cerr<<"filename= "<<filename<<std::endl;
    std::cerr<<"mode= "<<mode<<std::endl;
    std::cerr<<"isStack= "<<isStack<<std::endl;
    std::cerr<<"select_img= "<<select_img<<std::endl;
#endif
#undef DEBUG
    // Check that image is not empty
    if (getSize() < 1)
        REPORT_ERROR(ERR_MULTIDIM_EMPTY,"write Image ERROR: image is empty!");

    replaceNsize = 0;//reset replaceNsize in case image is reused
    if(isStack && select_img == ALL_IMAGES && mode == WRITE_REPLACE)
        REPORT_ERROR(ERR_VALUE_INCORRECT,"Please specify object to be replaced");
    else if (_exists && (mode == WRITE_REPLACE || mode == WRITE_APPEND))
    {
        // CHECK FOR INCONSISTENCIES BETWEEN data.xdim and x, etc???
        int Xdim, Ydim, Zdim, _Xdim, _Ydim, _Zdim;
        Xdim = Ydim = Zdim = _Xdim = _Ydim = _Zdim = 0;
        size_t Ndim, _Ndim;
        Ndim = _Ndim = 0;
        Image<char> auxI;
        auxI._read(filNamePlusExt, hFile, HEADER, ALL_IMAGES);

        this->getDimensions(Xdim, Ydim, Zdim, Ndim);
        auxI.getDimensions(_Xdim, _Ydim, _Zdim, _Ndim);

        replaceNsize = _Ndim;
        swapWrite = auxI.swap;

        if(Xdim != _Xdim ||
           Ydim != _Ydim ||
           Zdim != _Zdim)
        {
            std::cerr << "(x,y,z) " << Xdim << " " << Ydim << " " << Zdim << " "<< Ndim << std::endl;
            std::cerr << "(_x,_y,_z) " << _Xdim << " " << _Ydim << " " << _Zdim << " " <<_Ndim <<std::endl;
            REPORT_ERROR(ERR_MULTIDIM_SIZE,"write: target and source objects have different size");
        }
    }
    else if(!_exists && mode == WRITE_APPEND)
    {
        ;
    }
    else if (mode == WRITE_READONLY)//If new file we are in the WRITE_OVERWRITE mode
    {
        REPORT_ERROR(ERR_ARG_INCORRECT, formatString("File %s  opened in read-only mode. Cannot write.", name.c_str()));
    }
    /*
     * SELECT FORMAT
     */
    //Set the file pointer at beginning
    if (fimg != NULL)
        fseek(fimg, 0, SEEK_SET);
    if (fhed != NULL)
        fseek(fhed, 0, SEEK_SET);

    if(ext_name.contains("spi") || ext_name.contains("xmp") ||
       ext_name.contains("vol"))
        err = writeSPIDER(select_img,isStack,mode);
    else if (ext_name.contains("stk"))
        err = writeSPIDER(select_img,true,mode);
    else if (ext_name.contains("mrcs"))
        writeMRC(select_img,true,mode,imParam,castMode);
    else if (ext_name.contains("mrc"))
        writeMRC(select_img,false,mode,imParam,castMode);
    else if (ext_name.contains("img") || ext_name.contains("hed"))
        writeIMAGIC(select_img,mode,imParam,castMode);
    else if (ext_name.contains("dm3"))
        writeDM3(select_img,false,mode);
    else if (ext_name.contains("ser"))
        writeTIA(select_img,false,mode);
    else if (ext_name.contains("raw") || ext_name.contains("inf"))
        writeINF(select_img,false,mode,imParam,castMode);
    else if (ext_name.contains("tif"))
        writeTIFF(select_img,isStack,mode,imParam,castMode);
    else if (ext_name.contains("spe"))
        writeSPE(select_img,isStack,mode);
    else
        err = writeSPIDER(select_img,isStack,mode);

    if ( err < 0 )
    {
        std::cerr << " Filename = " << filename << " Extension= " << ext_name << std::endl;
        REPORT_ERROR(ERR_IO_NOWRITE, "Error writing file");
    }

    /* If initially the file did not existed, once the first image is written,
     * then the file exists
     */
    if (!_exists)
        hFile->exist = _exists = true;
}


/** Show image properties
      */
std::ostream& operator<<(std::ostream& o, const ImageBase& I)
{
    o << std::endl;
    o << "Filename       : " << I.filename << std::endl;
    o << "Image type     : ";
    if (I.isComplex())
        o << "Fourier-space image" << std::endl;
    else
        o << "Real-space image" << std::endl;

    o << "Endianess      : ";
    if (I.swap^IsLittleEndian())
        o << "Little"  << std::endl;
    else
        o << "Big" << std::endl;

    o << "Reversed       : ";
    if (I.swap)
        o << "True"  << std::endl;
    else
        o << "False" << std::endl;

    o << "Data type      : ";
    switch (I.dataType())
    {
    case Unknown_Type:
        o << "Undefined data type";
        break;
    case UChar:
        o << "Unsigned character or byte type (UInt8)";
        break;
    case SChar:
        o << "Signed character (Int8)";
        break;
    case UShort:
        o << "Unsigned short integer (UInt16)";
        break;
    case Short:
        o << "Signed short integer (Int16)";
        break;
    case UInt:
        o << "Unsigned integer (UInt32)";
        break;
    case Int:
        o << "Signed integer (Int32)";
        break;
    case Long:
        o << "Signed integer (4 or 8 byte, depending on system)";
        break;
    case Float:
        o << "Floating point (4-byte)";
        break;
    case Double:
        o << "Double precision floating point (8-byte)";
        break;
    case ComplexShort:
        o << "Complex two-byte integer (4-byte)";
        break;
    case ComplexInt:
        o << "Complex integer (8-byte)";
        break;
    case ComplexFloat:
        o << "Complex floating point (8-byte)";
        break;
    case ComplexDouble:
        o << "Complex floating point (16-byte)";
        break;
    case Bool:
        o << "Boolean (1-byte?)";
        break;
    }
    o << std::endl;

    int xdim, ydim, zdim;
    size_t ndim;
    I.getDimensions(xdim, ydim, zdim, ndim);
    o << "Dimensions     : " << ndim << " x " << zdim << " x " << ydim << " x " << xdim;
    o << "  ((N)Objects x (Z)Slices x (Y)Rows x (X)Columns)" << std::endl;
    o << "Data offset    : " << I.offset << std::endl;

    std::stringstream oGeo;

    if (I.individualContainsLabel(MDL_ANGLEROT))
    {
        oGeo << "Euler angles   : " << std::endl;
        oGeo << "                 Phi   (rotation around Z axis)                  = " << I.rot() << std::endl;
        oGeo << "                 Theta (tilt, second rotation around new Y axis) = " << I.tilt() << std::endl;
        oGeo << "                 Psi   (third rotation around new Z axis)        = " << I.psi() << std::endl;
    }
    if (I.individualContainsLabel(MDL_SHIFTX))
    {
        oGeo << "Origin Offsets : " << std::endl;
        oGeo << "                 Xoff  (origin offset in X-direction) = " << I.Xoff() << std::endl;
        oGeo << "                 Yoff  (origin offset in Y-direction) = " << I.Yoff() << std::endl;
        oGeo << "                 Zoff  (origin offset in Z-direction) = " << I.Zoff() << std::endl;
    }
    if (I.individualContainsLabel(MDL_SCALE))
        oGeo << "Scale          : " <<I.scale() << std::endl;
    if (I.individualContainsLabel(MDL_WEIGHT))
        oGeo << "Weight         : " << I.weight() << std::endl;
    if (I.individualContainsLabel(MDL_FLIP))
        oGeo << "Flip           : " << I.flip() << std::endl;

    if (!oGeo.str().empty())
        o << "--- Geometry ---" << std::endl << oGeo.str();

    return o;
}
