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

#include "image_base.h"

/*
 Based on rwSPIDER.h
 Header file for reading and writing SPIDER files
 Format: 3D image file format for the SPIDER package
 Author: Bernard Heymann
 Created: 19990410  Modified: 20010928
*/


#define SPIDERSIZE 1024 // Minimum size of the SPIDER header (variable)
///@defgroup Spider Spider File format
///@ingroup ImageFormats

/** Spider Header
  * @ingroup Spider
*/
struct SPIDERhead
{                    // file header for SPIDER data
    float nslice;    //  0      slices in volume (image = 1)
    float nrow;      //  1      rows per slice
    float irec;      //  2      # records in file (unused)
    float nhistrec;  //  3      (obsolete)
    float iform;     //  4      file type specifier
    float imami;     //  5      max/min flag (=1 if calculated)
    float fmax;      //  6      maximum
    float fmin;      //  7      minimum
    float av;        //  8      average
    float sig;       //  9      standard deviation (=-1 if not calculated)
    float ihist;     // 10      (obsolete)
    float nsam;      // 11      pixels per row
    float labrec;    // 12      # records in header
    float iangle;    // 13      flag: tilt angles filled
    float phi;       // 14      tilt angles
    float theta;     // 15
    float gamma;     // 16      (=psi)
    float xoff;      // 17      translation
    float yoff;      // 18
    float zoff;      // 19
    float scale;     // 20      scaling
    float labbyt;    // 21      # bytes in header
    float lenbyt;    // 22      record length in bytes (row length)
    float istack;    // 23      indicates stack of images
    float inuse;     // 24      indicates this image in stack is used (not used)
    float maxim;     // 25      max image in stack used
    float imgnum;    // 26      number of current image
    float unused[2]; // 27-28     (unused)
    float kangle;    // 29      flag: additional angles set
    float phi1;      // 30      additional angles
    float theta1;    // 31
    float psi1;      // 32
    float phi2;      // 33
    float theta2;    // 34
    float psi2;      // 35

    double fGeo_matrix[3][3]; // x9 = 72 bytes: Geometric info
    float fAngle1; // angle info

    float fr1;
    float fr2; // lift up cosine mask parameters

    /** Fraga 23/05/97  For Radon transforms **/
    float RTflag; // 1=RT, 2=FFT(RT)
    float Astart;
    float Aend;
    float Ainc;
    float Rsigma; // 4*7 = 28 bytes
    float Tstart;
    float Tend;
    float Tinc; // 4*3 = 12, 12+28 = 40B

    /** Sjors Scheres 17/12/04 **/
    float weight; // For Maximum-Likelihood refinement
    float flip;   // 0=no flipping operation (false), 1=flipping (true)

    char fNada2[576]; // empty 700-76-40=624-40-8= 576 bytes

    char cdat[12];   // 211-213   creation date
    char ctim[8];  // 214-215   creation time
    char ctit[160];  // 216-255   title
} ;

/************************************************************************
@Function: readSPIDER
@Description:
 Reading a SPIDER image file format.
@Algorithm:
 A 3D multi-image format used in electron microscopy.
 Header size:    1024 bytes (not same as data offset!).
 Data offset:    sizeof(float)*x_size*ceil(1024/x_size)
 File format extensions:   .spi
 Byte order determination: File type and third dimension values
        must be less than 256*256.
 Data type:      only float.
 Transform type:    Hermitian
        The x-dimension contains the x-size
        of the full transform
 A multi-image file has a global header followed by a header and data
 for each sub-image.
@Arguments:
 Bimage* p   the image structure.
 size_t select_img  image selection in multi-image file (-1 = all images).
@Returns:
 int     error code (<0 means failure).
**************************************************************************/
/** Spider Reader
  * @ingroup Spider
*/
#include "metadata_label.h"
#include <errno.h>

int  ImageBase::readSPIDER(size_t select_img)
{
#undef DEBUG
    //#define DEBUG
#ifdef DEBUG
    printf("DEBUG readSPIDER: Reading Spider file\n");
#endif
#undef DEBUG

    SPIDERhead* header = new SPIDERhead;
    if ( fread( header, SPIDERSIZE, 1, fimg ) != 1 )
        REPORT_ERROR(ERR_IO_NOREAD,"rwSPIDER: cannot read Spider main header from file "+\
        		     filename + ". Error message: " + strerror(errno));

    swap = 0;

    // Determine byte order and swap bytes if from different-endian machine
    char*    b = (char *) header;
    int      i;
    int      extent = SPIDERSIZE - 180;  // exclude char bytes from swapping
    if ( ( fabs(header->nslice) > SWAPTRIG ) || ( fabs(header->iform) > SWAPTRIG ) ||
         ( fabs(header->nslice) < 1 ) )
    {
        swap = 1;
        for ( i=0; i<extent; i+=4 )
            swapbytes(b+i, 4);
    }

    if(header->labbyt != header->labrec*header->lenbyt)
        REPORT_ERROR(ERR_IO_NOTFILE,(std::string)"Invalid Spider file:  " + filename);

    offset = (size_t) header->labbyt;
    DataType datatype  = Float;

    MDMainHeader.setValue(MDL_MIN,(double)header->fmin);
    MDMainHeader.setValue(MDL_MAX,(double)header->fmax);
    MDMainHeader.setValue(MDL_AVG,(double)header->av);
    MDMainHeader.setValue(MDL_STDDEV,(double)header->sig);
    MDMainHeader.setValue(MDL_SAMPLINGRATEX,(double)header->scale);
    MDMainHeader.setValue(MDL_SAMPLINGRATEY,(double)header->scale);
    MDMainHeader.setValue(MDL_SAMPLINGRATEZ,(double)header->scale);
    MDMainHeader.setValue(MDL_DATATYPE,(int)datatype);

    bool isStack = ( header->istack > 0 );
    int _xDim,_yDim,_zDim;
    size_t _nDim, _nDimSet;
    _xDim = (int) header->nsam;
    _yDim = (int) header->nrow;
    _zDim = (int) header->nslice;
    _nDim = (isStack)? header->maxim : 1;

    replaceNsize = _nDim;

    /************
     * BELLOW HERE DO NOT USE HEADER BUT LOCAL VARIABLES
     */

    // Map the parameters, REad the whole object (-1) or a slide
    // Only handle stacks of images not of volumes
    if(!isStack)
        _nDimSet = 1;
    else
        _nDimSet = (select_img == ALL_IMAGES) ? _nDim : 1;

    setDimensions(_xDim, _yDim, _zDim, _nDimSet);

    //image is in stack? and set right initial and final image
    size_t header_size = offset;

    if ( isStack)
    {
        if ( select_img > _nDim )
            REPORT_ERROR(ERR_INDEX_OUTOFBOUNDS, formatString("readSpider (%s): Image number %lu exceeds stack size %lu" ,filename.c_str(),select_img, _nDim));
        offset += offset;
    }

    if (dataMode == HEADER || (dataMode == _HEADER_ALL && _nDimSet > 1)) // Stop reading if not necessary
    {
        delete header;
        return 0;
    }

    size_t datasize_n  = _xDim*_yDim*_zDim;
    size_t image_size  = header_size + datasize_n*sizeof(float);
    size_t pad         = (size_t) header->labbyt;
    size_t   imgStart = IMG_INDEX(select_img);
    size_t   imgEnd = (select_img != ALL_IMAGES) ? imgStart + 1 : _nDim;
    size_t   img_seek = header_size + imgStart * image_size;
    char*   hend;

    MD.clear();
    MD.resize(imgEnd - imgStart,MDL::emptyHeader);
    double daux;

    //std::cerr << formatString("DEBUG_JM: header_size: %10lu, datasize_n: %10lu, image_size: %10lu, imgStart: %10lu, img_seek: %10lu",
    //    header_size, datasize_n, image_size, imgStart, img_seek) <<std::endl;

    for (size_t n = 0, i = imgStart; i < imgEnd; ++i, ++n, img_seek += image_size )
    {
        if (fseek( fimg, img_seek, SEEK_SET ) != 0)//fseek return 0 on success
            REPORT_ERROR(ERR_IO, formatString("rwSPIDER: error seeking %lu for read image %lu", img_seek, i));

        // std::cerr << formatString("DEBUG_JM: rwSPIDER: seeking %lu for read image %lu", img_seek, i) <<std::endl;

        if(isStack)
        {
            if ( fread( header, SPIDERSIZE, 1, fimg ) != 1 )
                REPORT_ERROR(ERR_IO_NOREAD, formatString("rwSPIDER: cannot read Spider image %lu header", i));
            hend = (char *) header + extent;
            if ( swap )
                for ( b = (char *) header; b<hend; b+=4 )
                    swapbytes(b, 4);
        }
        if (dataMode == _HEADER_ALL || dataMode == _DATA_ALL)
        {
            daux = (double)header->xoff;
            MD[n].setValue(MDL_SHIFTX, daux);
            daux = (double)header->yoff;
            MD[n].setValue(MDL_SHIFTY, daux);
            daux = (double)header->zoff;
            MD[n].setValue(MDL_SHIFTZ, daux);
            daux = (double)header->phi;
            MD[n].setValue(MDL_ANGLEROT, daux);
            daux = (double)header->theta;
            MD[n].setValue(MDL_ANGLETILT, daux);
            daux = (double)header->gamma;
            MD[n].setValue(MDL_ANGLEPSI, daux);
            daux = (double)header->weight;
            MD[n].setValue(MDL_WEIGHT, daux);
            bool baux = (header->flip == 1);
            MD[n].setValue(MDL_FLIP, baux);
            daux = (double) header->scale;
            if (daux==0.)
                daux=1.0;
            MD[n].setValue(MDL_SCALE, daux);
        }
    }
    delete header;

    if (dataMode < DATA)   // Don't read  data if not necessary but read the header
        return 0;

#ifdef DEBUG

    std::cerr<<"DEBUG readSPIDER: header_size = "<<header_size<<" image_size = "<<image_size<<std::endl;
    std::cerr<<"DEBUG readSPIDER: select_img= "<<select_img<<" n= "<<Ndim<<" pad = "<<pad<<std::endl;
#endif
    //offset should point to the begin of the data
    readData(fimg, select_img, datatype, pad );

    return(0);
}
/************************************************************************
@Function: writeSPIDER
@Description:
 Writing a SPIDER image file format.
@Algorithm:
 A 3D image format used in electron microscopy.
@Arguments:
@Returns:
 int     error code (<0 means failure).
**************************************************************************/
/** Spider Writer
  * @ingroup Spider
*/
int  ImageBase::writeSPIDER(size_t select_img, bool isStack, int mode)
{
#undef DEBUG
    //#define DEBUG
#ifdef DEBUG
    printf("DEBUG writeSPIDER: Writing Spider file\n");
    printf("DEBUG writeSPIDER: File %s\n", filename.c_str());
#endif
#undef DEBUG

    // Check datatype compatibility
    DataType wDType = (isComplexT()) ? ComplexFloat : Float;

    if (mmapOnWrite)
    {
        MDMainHeader.setValue(MDL_DATATYPE,(int) wDType);
        if (!checkMmapT(wDType))
        {
            if (dataMode < DATA) // This means ImageGeneric wants to know which DataType must use in mapFile2Write
                return 0;
            else
                REPORT_ERROR(ERR_MMAP, "File datatype and image declaration not compatible with mmap.");
        }
        else
            dataMode = DATA;
    }

    int Xdim, Ydim, Zdim;
    size_t Ndim;
    getDimensions(Xdim, Ydim, Zdim, Ndim);

    size_t datasize, datasize_n;
    datasize_n = (size_t)Xdim*Ydim*Zdim;
    datasize = datasize_n * gettypesize(wDType);

    // Filling the main header
    float  lenbyt = gettypesize(wDType)*Xdim;  // Record length (in bytes)
    float  labrec = floor(SPIDERSIZE/lenbyt); // # header records
    if ( fmod(SPIDERSIZE,lenbyt) != 0 )
        labrec++;
    float  labbyt = labrec*lenbyt;   // Size of header in bytes
    offset = (size_t) labbyt;

    SPIDERhead* header = (SPIDERhead *) askMemory((int)labbyt*sizeof(char));

    // Map the parameters
    header->lenbyt = lenbyt;     // Record length (in bytes)
    header->labrec = labrec;     // # header records
    header->labbyt = labbyt;      // Size of header in bytes

    header->irec   = labrec + floor((datasize_n*gettypesize(wDType))/lenbyt + 0.999999); // Total # records
    header->nsam   = Xdim;
    header->nrow   = Ydim;
    header->nslice = Zdim;

    // If a transform, then the physical storage in x is only half+1
    size_t xstore  = Xdim;
    if ( transform == Hermitian )
    {
        xstore = Xdim * 0.5 + 1;
        header->nsam = 2*xstore;
    }

    //#define DEBUG
#ifdef DEBUG
    printf("DEBUG writeSPIDER: Size: %g %g %g %d %g\n",
           header->nsam,
           header->nrow,
           header->nslice,
           Ndim,
           header->maxim);
#endif
#undef DEBUG

    if ( Zdim < 2 )
    {
        // 2D image or 2D Fourier transform
        header->iform = ( transform == NoTransform ) ? 1 :  -12 + (int)header->nsam%2;
    }
    else
    {
        // 3D volume or 3D Fourier transform
        header->iform = ( transform == NoTransform )? 3 : -22 + (int)header->nsam%2;
    }
    double aux;
    bool baux;
    header->imami = 0;//never trust max/min


    if (!MDMainHeader.empty())
    {
        if(MDMainHeader.getValue(MDL_MIN, aux))
            header->fmin = (float)aux;
        if(MDMainHeader.getValue(MDL_MAX, aux))
            header->fmax = (float)aux;
        if(MDMainHeader.getValue(MDL_AVG, aux))
            header->av   = (float)aux;
        if(MDMainHeader.getValue(MDL_STDDEV, aux))
            header->sig  = (float)aux;

    }

    if (Ndim == 1 && mode != WRITE_APPEND && !isStack && !MD.empty())
    {
        if ((dataMode == _HEADER_ALL || dataMode == _DATA_ALL))
        {

            if(MD[0].getValue(MDL_SHIFTX,  aux))
                header->xoff  =(float)aux;
            if(MD[0].getValue(MDL_SHIFTY,  aux))
                header->yoff  =(float)aux;
            if(MD[0].getValue(MDL_SHIFTZ,  aux))
                header->zoff  =(float)aux;
            if(MD[0].getValue(MDL_ANGLEROT, aux))
                header->phi   =(float)aux;
            if(MD[0].getValue(MDL_ANGLETILT,aux))
                header->theta =(float)aux;
            if(MD[0].getValue(MDL_ANGLEPSI, aux))
                header->gamma =(float)aux;
            if(MD[0].getValue(MDL_WEIGHT,   aux))
                header->weight=(float)aux;
            if(MD[0].getValue(MDL_FLIP,    baux))
                header->flip  =(float)baux;
            if(MD[0].getValue(MDL_SCALE,    aux))
                header->scale  =(float)aux;
        }
        else
        {
            header->xoff = header->yoff = header->zoff =\
                                          header->phi = header->theta = header->gamma = header->weight = 0.;
            header->scale = 1.;
        }
    }

    //else end
    // Set time and date
    /*
    time_t timer;
    time ( &timer );
    tm* t = localtime(&timer);
    while ( t->tm_year > 100 )
        t->tm_year -= 100;
    sprintf(header->ctim, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    sprintf(header->cdat, "%02d-%02d-%02d", t->tm_mday, t->tm_mon, t->tm_year);
    */

#ifdef DEBUG

    printf("DEBUG writeSPIDER: Date and time: %s %s\n", header->cdat, header->ctim);
    printf("DEBUG writeSPIDER: Text label: %s\n", header->ctit);
    printf("DEBUG writeSPIDER: Header size: %g\n", header->labbyt);
    printf("DEBUG writeSPIDER: Header records and record length: %g %g\n", header->labrec, header->lenbyt);
    printf("DEBUG writeSPIDER: Data size: %ld\n", datasize);
    printf("DEBUG writeSPIDER: Data offset: %ld\n", offset);
    printf("DEBUG writeSPIDER: File %s\n", filename.c_str());
#endif


    // Only write mainheader when new file or number of images in stack changed
    bool writeMainHeaderReplace = false;
    header->maxim = replaceNsize;
    size_t newNsize = select_img + Ndim - 1;

    size_t imgStart = (mode == WRITE_APPEND)? replaceNsize : IMG_INDEX(select_img);
    if (Ndim > 1 || mode == WRITE_APPEND || isStack)
    {
        header->istack = 2;
        header->inuse =  1;

        if( mode == WRITE_APPEND )
        {
            writeMainHeaderReplace = true;
            header->maxim += Ndim;
        }
        else if( mode == WRITE_REPLACE && newNsize > replaceNsize)
        {
            writeMainHeaderReplace = true;
            header->maxim = newNsize;
            /* jcuenca 2011/04/11 */
        }
        else if(mode == WRITE_OVERWRITE)
        {
            writeMainHeaderReplace = true;
            header->maxim = Ndim;
        }
    }
    else
    {
        header->istack = 0;
        header->inuse = 0;
        header->maxim = 1;
        writeMainHeaderReplace=true;

    }

    //locking
    struct flock fl;

    fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
    fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
    fl.l_start  = 0;        /* Offset from l_whence         */
    fl.l_len    = 0;        /* length, 0 = to EOF           */
    fl.l_pid    = getpid(); /* our PID                      */

    /*
     * BLOCK HEADER IF NEEDED
     */
    fl.l_type   = F_WRLCK;
    fcntl(fileno(fimg), F_SETLKW, &fl); /* locked */

    // Write main header
    if( mode == WRITE_OVERWRITE ||
        mode == WRITE_APPEND ||
        writeMainHeaderReplace ||
        newNsize > replaceNsize) //header must change
        fwrite( header, offset, 1, fimg );

    // write single image if not stack
    if ( Ndim == 1 && !isStack)
    {
        if (mmapOnWrite)
        {
            mappedOffset = ftell(fimg);
            mappedSize = mappedOffset + datasize;
            fseek(fimg, datasize-1, SEEK_CUR);
            fputc(0, fimg);
        }
        else
            writeData(fimg, 0, wDType, datasize_n, CW_CAST);
    }
    else // Jump to the selected imgStart position
    {
        fseek( fimg,offset + (offset+datasize)*imgStart, SEEK_SET);

        //for ( size_t i=0; i<Ndim; i++ )
        std::vector<MDRow>::iterator it = MD.begin();

        for (size_t i = 0; i < Ndim; ++it, ++i)
        {
            // Write the individual image header
            if ( it != MD.end() && (dataMode == _HEADER_ALL || dataMode == _DATA_ALL))
            {
                if(it->getValue(MDL_SHIFTX,  aux))
                    header->xoff  =(float)aux;
                if(it->getValue(MDL_SHIFTY,  aux))
                    header->yoff  =(float)aux;
                if(it->getValue(MDL_SHIFTZ,  aux))
                    header->zoff  =(float)aux;
                if(it->getValue(MDL_ANGLEROT, aux))
                    header->phi   =(float)aux;
                if(it->getValue(MDL_ANGLETILT,aux))
                    header->theta =(float)aux;
                if(it->getValue(MDL_ANGLEPSI, aux))
                    header->gamma =(float)aux;
                if(it->getValue(MDL_WEIGHT,   aux))
                    header->weight=(float)aux;
                if(it->getValue(MDL_FLIP,    baux))
                    header->flip  =(float)baux;
                if(it->getValue(MDL_SCALE,    aux))
                    header->scale  =(float)aux;
            }
            else
            {
                header->xoff = header->yoff = header->zoff =\
                                              header->phi = header->theta = header->gamma = header->weight = 0.;
                header->scale = 1.;
            }
            //do not need to unlock because we are in the overwrite case
            fwrite( header, offset, 1, fimg );
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
                    writeData(fimg, i*datasize_n, wDType, datasize_n, CW_CAST);
            }
            else
                fseek(fimg, datasize, SEEK_CUR);
        }
    }
    //I guess I do not need to unlock since we are going to close the file
    fl.l_type   = F_UNLCK;
    fcntl(fileno(fimg), F_SETLK, &fl); /* unlocked */

    if (mmapOnWrite)
        mmapFile();

    freeMemory(header, (int)labbyt*sizeof(char));

    return(0);
}
