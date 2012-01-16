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
/*
        Base on rwMRC.h
        Header file for reading and writing MRC files
        Format: 3D crystallographic image file format for the MRC package
        Author: Bernard Heymann
        Created: 19990321       Modified: 20030723
*/


#define MRCSIZE    1024 // Minimum size of the MRC header (when nsymbt = 0)

///@defgroup MRC MRC File format
///@ingroup ImageFormats

/** MRC Old Header
  * @ingroup MRC
*/
struct MRCheadold
{          // file header for MRC data
    int nx;              //  0   0       image size
    int ny;              //  1   4
    int nz;              //  2   8
    int mode;            //  3           0=uchar,1=short,2=float
    int nxStart;         //  4           unit cell offset
    int nyStart;         //  5
    int nzStart;         //  6
    int mx;              //  7           unit cell size in voxels
    int my;              //  8
    int mz;              //  9
    float a;             // 10   40      cell dimensions in A
    float b;             // 11
    float c;             // 12
    float alpha;         // 13           cell angles in degrees
    float beta;          // 14
    float gamma;         // 15
    int mapc;            // 16           column axis
    int mapr;            // 17           row axis
    int maps;            // 18           section axis
    float amin;          // 19           minimum density value
    float amax;          // 20   80      maximum density value
    float amean;         // 21           average density value
    int ispg;            // 22           space group number
    int nsymbt;          // 23           bytes used for sym. ops. table
    float extra[29];     // 24           user-defined info
    float xOrigin;       // 53           phase origin in pixels
    float yOrigin;       // 54
    int nlabl;           // 55           number of labels used
    char labels[10][80]; // 56-255       10 80-character labels
} ;

/** MRC Header
  * @ingroup MRC
*/
struct MRChead
{             // file header for MRC data
    int nx;              //  0   0       image size
    int ny;              //  1   4
    int nz;              //  2   8
    int mode;            //  3           0=char,1=short,2=float
    int nxStart;         //  4           unit cell offset
    int nyStart;         //  5
    int nzStart;         //  6
    int mx;              //  7           unit cell size in voxels
    int my;              //  8
    int mz;              //  9
    float a;             // 10   40      cell dimensions in A
    float b;             // 11
    float c;             // 12
    float alpha;         // 13           cell angles in degrees
    float beta;          // 14
    float gamma;         // 15
    int mapc;            // 16           column axis
    int mapr;            // 17           row axis
    int maps;            // 18           section axis
    float amin;          // 19           minimum density value
    float amax;          // 20   80      maximum density value
    float amean;         // 21           average density value
    int ispg;            // 22           space group number
    int nsymbt;          // 23           bytes used for sym. ops. table
    float extra[25];     // 24           user-defined info
    float xOrigin;       // 49           phase origin in pixels FIXME: is in pixels or [L] units?
    float yOrigin;       // 50
    float zOrigin;       // 51
    char map[4];         // 52       identifier for map file ("MAP ")
    char machst[4];      // 53           machine stamp
    float arms;          // 54       RMS deviation
    int nlabl;           // 55           number of labels used
    char labels[800];    // 56-255       10 80-character labels
} ;

// I/O prototypes
/** MRC Reader
  * @ingroup MRC
*/
int ImageBase::readMRC(size_t select_img, bool isStack)
{
#undef DEBUG
    //#define DEBUG
#ifdef DEBUG
    printf("DEBUG readMRC: Reading MRC file\n");
#endif

    MRChead* header = new MRChead;

    if ( fread( header, MRCSIZE, 1, fimg ) < 1 )
        return(-2);

    // Determine byte order and swap bytes if from little-endian machine
    if ( swap = (( abs( header->mode ) > SWAPTRIG ) || ( abs(header->nz) > SWAPTRIG )) )
    {
#ifdef DEBUG
        fprintf(stderr, "Warning: Swapping header byte order for 4-byte types\n");
#endif

        swapPage((char *) header, MRCSIZE - 800, Float); // MRCSIZE - 800 is to exclude labels from swapping
    }

    // Convert VAX floating point types if necessary
    if ( header->amin > header->amax )
        REPORT_ERROR(ERR_UNCLASSIFIED,"readMRC: amin > max: VAX floating point conversion unsupported");
    int _xDim,_yDim,_zDim;
    size_t _nDim;
    _xDim = header->nx;
    _yDim = header->ny;
    _zDim = header->nz;

    if(isStack)
    {
        if ( select_img > _zDim ) // When isStack slices in Z are supposed to be a stack of images
            REPORT_ERROR(ERR_INDEX_OUTOFBOUNDS, formatString("readMRC: %s Image number %lu exceeds stack size %lu", this->filename.c_str(), select_img, _zDim));

        _nDim = (select_img == ALL_IMAGES) ? (size_t) _zDim : 1;
        _zDim = 1;
    }
    else
        _nDim = 1;

    replaceNsize = _nDim;
    setDimensions(_xDim, _yDim, _zDim, _nDim);

    DataType datatype;
    switch ( header->mode % 5 )
    {
    case 0:
        datatype = UChar;
        break;
    case 1:
        datatype = Short;
        break;
    case 2:
        datatype = Float;
        break;
    case 3:
        datatype = ComplexShort;
        break;
    case 4:
        datatype = ComplexFloat;
        break;
    default:
        datatype = UChar;
        break;
    }
    offset = MRCSIZE + header->nsymbt;
    size_t datasize_n;
    datasize_n = _xDim*_yDim*_zDim;

    if ( header->mode%5 > 2 && header->mode%5 < 5 )
    {
        transform = CentHerm;
        fseek(fimg, 0, SEEK_END);
        if ( ftell(fimg) > offset + 0.8*datasize_n*gettypesize(datatype) )
            _xDim = (2 * (_xDim - 1));
        if ( header->mx%2 == 1 )
            _xDim += 1;     // Quick fix for odd x-size maps
        setDimensions(_xDim, _yDim, _zDim, _nDim);
    }

    MDMainHeader.setValue(MDL_MIN,(double)header->amin);
    MDMainHeader.setValue(MDL_MAX,(double)header->amax);
    MDMainHeader.setValue(MDL_AVG,(double)header->amean);
    MDMainHeader.setValue(MDL_STDDEV,(double)header->arms);
    MDMainHeader.setValue(MDL_DATATYPE,(int)datatype);

    if ( header->mx && header->a!=0)//ux
        MDMainHeader.setValue(MDL_SAMPLINGRATEX,(double)header->a/header->mx);
    if ( header->my && header->b!=0)//yx
        MDMainHeader.setValue(MDL_SAMPLINGRATEY,(double)header->b/header->my);
    if ( header->mz && header->c!=0)//zx
        MDMainHeader.setValue(MDL_SAMPLINGRATEZ,(double)header->c/header->mz);

    if (dataMode==HEADER || (dataMode == _HEADER_ALL && _nDim > 1)) // Stop reading if not necessary
    {
        delete header;
        return 0;
    }

    size_t   imgStart = IMG_INDEX(select_img);
    size_t   imgEnd = (select_img != ALL_IMAGES) ? imgStart + 1 : _nDim;

    MD.clear();
    MD.resize(imgEnd - imgStart,MDL::emptyHeader);

    /* As MRC does not support stacks, we use the geometry stored in the header
    for any image when we simulate the file is a stack.*/
    if (dataMode == _HEADER_ALL || dataMode == _DATA_ALL)
    {
        double aux;
        for ( size_t i = 0; i < imgEnd - imgStart; ++i )
        {
            MD[i].setValue(MDL_SHIFTX, (double) -header->nxStart);
            MD[i].setValue(MDL_SHIFTY, (double) -header->nyStart);
            MD[i].setValue(MDL_SHIFTZ, (double) -header->nzStart);

            // We include auto detection of MRC2000 or CCP4 style origin based on http://situs.biomachina.org/fmap.pdf
            if (header->xOrigin != 0)
                MD[i].setValue(MDL_ORIGINX, -header->xOrigin);
            else if (header->nxStart != 0 && MDMainHeader.getValue(MDL_SAMPLINGRATEX,aux))
                MD[i].setValue(MDL_ORIGINX, -header->nxStart/aux);

            if (header->yOrigin !=0)
                MD[i].setValue(MDL_ORIGINY, -header->yOrigin);
            else if(header->nyStart !=0 && MDMainHeader.getValue(MDL_SAMPLINGRATEY,aux))
                MD[i].setValue(MDL_ORIGINY, -header->nyStart/aux);

            if (header->zOrigin != 0)
                MD[i].setValue(MDL_ORIGINY, -header->zOrigin);
            else if(header->nzStart !=0 && MDMainHeader.getValue(MDL_SAMPLINGRATEZ,aux))
                MD[i].setValue(MDL_ORIGINZ, -header->nzStart/aux);
        }
    }
    //#define DEBUG
#ifdef DEBUG
    MDMainHeader.write("/dev/stderr");
    MD.write("/dev/stderr");
#endif

    delete header;

    if ( dataMode < DATA )   // Don't read the individual header and the data if not necessary
        return 0;

    readData(fimg, select_img, datatype, 0);

    return(0);
}

/** MRC Writer
  * @ingroup MRC
*/
int ImageBase::writeMRC(size_t select_img, bool isStack, int mode, const String &bitDepth, bool adjust)
{
    MRChead*  header = (MRChead *) askMemory(sizeof(MRChead));

    // Cast T to datatype
    DataType wDType,myTypeID = myT();
    CastWriteMode castMode;

    if (bitDepth == "")
    {
        castMode = CW_CAST;
        switch(myTypeID)
        {
        case Double:
        case Float:
        case Int:
        case UInt:
            wDType = Float;
            header->mode = 2;
            break;
        case Short:
        case UShort:
            wDType = Short;
            header->mode = 1;
            break;
        case SChar:
        case UChar:
            castMode = CW_CONVERT;
            wDType = UChar;
            header->mode = 0;
            break;
        case ComplexFloat:
        case ComplexDouble:
            wDType = ComplexFloat;
            header->mode = 4;
            break;
        default:
            wDType = Unknown_Type;
            REPORT_ERROR(ERR_TYPE_INCORRECT,(std::string)"ERROR: Unsupported data type by MRC format.");
        }
    }
    else //Convert to other data type
    {
        // Default Value
        wDType = (bitDepth == "default") ? Float : datatypeRAW(bitDepth);

        switch (wDType)
        {
        case Float:
            header->mode = 2;
            break;
        case UChar:
            header->mode = 0;
            break;
        case Short:
            header->mode = 1;
            break;
        case ComplexFloat:
            header->mode = 4;
            break;
        default:
            REPORT_ERROR(ERR_TYPE_INCORRECT,"ERROR: incorrect MRC bits depth value.");
        }
        castMode = (adjust)? CW_ADJUST : CW_CONVERT;
    }

    if (mmapOnWrite)
    {
        MDMainHeader.setValue(MDL_DATATYPE,(int) wDType);
        if (!checkMmapT(wDType))
        {
            if (dataMode < DATA && castMode == CW_CAST) // This means ImageGeneric wants to know which DataType must use in mapFile2Write
                return 0;
            else
                REPORT_ERROR(ERR_MMAP, "File datatype and image declaration not compatible with mmap.");
        }
        else
            dataMode = DATA;
    }


    /*
         if ( transform != NoTransform )
             img_convert_fourier(p, CentHerm);
     */

    // Map the parameters
    strncpy(header->map, "MAP ", 4);
    // FIXME TO BE DONE WITH rwCCP4!!
    //set_CCP4_machine_stamp(header->machst);
    char* machine_stamp;
    machine_stamp = (char *)(header->machst);
    if(IsLittleEndian())
    {
        machine_stamp[0] = 68;
        machine_stamp[1] = 65;
    }
    else
    {
        machine_stamp[0] = machine_stamp[1] = 17;
    }
    //                    case LittleVAX:
    //                        machine_stamp[0] = 34;
    //                        machine_stamp[1] = 65;
    //                        break;

    int Xdim, Ydim, Zdim;
    size_t Ndim;
    getDimensions(Xdim, Ydim, Zdim, Ndim);

    header->a = header->mx =header->nx = Xdim;
    header->b = header->my =header->ny = Ydim;
    header->c = header->mz =header->nz = Zdim;

    if ( transform == CentHerm )
        header->nx = Xdim/2 + 1;        // If a transform, physical storage is nx/2 + 1

    header->alpha = 90.;
    header->beta  = 90.;
    header->gamma = 90.;

    //Set this to zero till we decide if we want to update it
    //    header->mx = 0;//(int) (ua/ux + 0.5);
    //    header->my = 0;//(int) (ub/uy + 0.5);
    //    header->mz = 0;//(int) (uc/uz + 0.5);
    header->mapc = 1;
    header->mapr = 2;
    header->maps = 3;
    double aux,aux2;

    //    header->a = 0.;// ua;
    //    header->b = 0.;// ub;
    //    header->c = 0.;// uc;

    if (!MDMainHeader.empty())
    {
        if(MDMainHeader.getValue(MDL_MIN, aux))
            header->amin  = (float)aux;
        if(MDMainHeader.getValue(MDL_MAX, aux))
            header->amax  = (float)aux;
        if(MDMainHeader.getValue(MDL_AVG, aux))
            header->amean = (float)aux;
        if(MDMainHeader.getValue(MDL_STDDEV, aux))
            header->arms  = (float)aux;

        if ((dataMode == _HEADER_ALL || dataMode == _DATA_ALL))
        {
            if(MD[0].getValue(MDL_SHIFTX, aux))
                header->nxStart = (int)-(aux-0.5);
            if(MD[0].getValue(MDL_ORIGINX, aux) &&
               MDMainHeader.getValue(MDL_SAMPLINGRATEX,aux2))//header is init to zero
                header->xOrigin = (float)(aux*aux2);
            if (MD[0].getValue(MDL_SHIFTY, aux))
                header->nyStart = (int)-(aux-0.5);
            if (MD[0].getValue(MDL_ORIGINY, aux) &&
                MDMainHeader.getValue(MDL_SAMPLINGRATEY,aux2))//header is init to zero
                header->yOrigin = (float)(aux*aux2);
            if (MD[0].getValue(MDL_SHIFTZ, aux))
                header->nzStart = (int)-(aux-0.5);
            if (MD[0].getValue(MDL_ORIGINZ, aux) &&
                MDMainHeader.getValue(MDL_SAMPLINGRATEZ,aux2))//header is init to zero
                header->zOrigin = (float)(aux*aux2);
        }
        else
            header->nxStart = header->xOrigin = header->nyStart = \
                                                header->yOrigin = header->nzStart = header->zOrigin = 0;
    }

    header->nsymbt = 0;
    header->nlabl = 10; // or zero?
    //strncpy(header->labels, p->label.c_str(), 799);

    offset = MRCSIZE + header->nsymbt;
    size_t datasize, datasize_n;
    datasize_n = Xdim*Ydim*Zdim;
    datasize = datasize_n * gettypesize(wDType);

    //#define DEBUG
#ifdef DEBUG

    printf("DEBUG rwMRC: Offset = %ld,  Datasize_n = %ld\n", offset, datasize_n);
#endif

    size_t imgStart = 0;

    if (isStack)
    {
        imgStart = IMG_INDEX(select_img);
        header->nz = replaceNsize;


        if( mode == WRITE_APPEND )
        {
            imgStart = replaceNsize;
            header->nz = replaceNsize + Ndim;
        }
        else if( mode == WRITE_REPLACE && select_img + Ndim - 1 > replaceNsize)
        {
            header->nz = select_img + Ndim - 1;
        }
        else if (Ndim > replaceNsize)
            header->nz = Ndim;
    }

    //locking
    struct flock fl;

    fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
    fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
    fl.l_start  = 0;        /* Offset from l_whence         */
    fl.l_len    = 0;        /* length, 0 = to EOF           */
    fl.l_pid    = getpid(); /* our PID                      */


    //BLOCK
    fl.l_type   = F_WRLCK;
    fcntl(fileno(fimg), F_SETLKW, &fl); /* locked */

    // Write header when needed
    if(!isStack || replaceNsize < header->nz)
    {
        if ( swapWrite )
            swapPage((char *) header, MRCSIZE - 800, Float);
        fwrite( header, MRCSIZE, 1, fimg );
    }
    freeMemory(header, sizeof(MRChead) );

    // Jump to the selected imgStart position
    fseek( fimg,offset + (datasize)*imgStart, SEEK_SET);

    size_t imgEnd = (isStack)? Ndim : 1;

    for ( size_t i = 0; i < imgEnd; i++ )
    {
        // If to also write the image data or jump its size
        if (dataMode >= DATA)
        {
            if (mmapOnWrite && Ndim == 1) // Can map one image at a time only
            {
                mappedOffset = ftell(fimg);
                mappedSize = mappedOffset + datasize;
                fseek(fimg, datasize-1, SEEK_CUR);
                fputc(0, fimg);
            }
            else
                writeData(fimg, i*datasize_n, wDType, datasize_n, castMode);
        }
        else
            fseek(fimg, datasize, SEEK_CUR);
    }

    // Unlock the file
    fl.l_type   = F_UNLCK;
    fcntl(fileno(fimg), F_SETLK, &fl); /* unlocked */

    if (mmapOnWrite)
        mmapFile();

    return(0);
}
