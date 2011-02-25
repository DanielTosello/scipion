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

#ifndef MULTIDIM_ARRAY_GENERIC_H_
#define MULTIDIM_ARRAY_GENERIC_H_

#include "datatype.h"
#include "multidim_array.h"

/* Switch among different datatypes.
 *
 * This macro replicates the code for the different data type options.
 *
 *@code
 *
 *#define MYFUNC(type)  getSlice(k, *(MultidimArray<type>*)image, axis, n)
 *
 *SWITCHDATATYPE(datatype,MYFUNC)
 *
 *@endcode
 */
#define SWITCHDATATYPE(datatype,OP) \
    switch (datatype)\
        {\
        case Float:\
            OP(float);\
            break;\
        case UInt:\
            OP(unsigned int);\
            break;\
        case Int:\
            OP(int);\
            break;\
        case Short:\
            OP(short);\
            break;\
        case UShort:\
            OP(unsigned short);\
            break;\
        case SChar:\
            OP(char);\
            break;\
        case UChar:\
            OP(unsigned char);\
            break;\
        }

/// @addtogroup MultidimensionalArrays

//@{

/**
 * MultidimArrayGeneric class to handle arrays with independence of the data type
 */
class MultidimArrayGeneric
{
public:
    DataType       datatype;
    MultidimArrayBase *im;

protected:
    // Flag to allow destroy im data.
    bool destroyData;

public:

    /**
     * Constructor with pointer to array to be linked and datatype definition of
     * the linked array.
     */
    MultidimArrayGeneric(MultidimArrayBase* array, DataType _datatype);

    /* Get an aliasSlice of the selected slice from the multidimarray
     */
    MultidimArrayGeneric(MultidimArrayGeneric &mdim, int select_slice);

    /**
     * Destructor.
     */
    ~MultidimArrayGeneric();

    /* Initialize
     */
    void init();

    /* Clear the MultidimArrayBase and others
     */
    void clear();


    void setDatatype(DataType imgType);


    void aliasSlice(MultidimArrayGeneric &mdim, int select_slice);

    /**
     * Link the internal array base to a specific multidimarray object.
     */
    void link(MultidimArrayBase* array);

    /**
     *  Call the resize function of the linked array.
     */
    void resize(size_t Ndim, int Zdim, int Ydim, int Xdim, bool copy=true)
    {
        im->resize(Ndim,Zdim,Ydim,Xdim,copy);
    }

    /** Get a Window from the image*/
    void window(MultidimArrayGeneric &result, int z0, int y0, int x0,
                int zF, int yF, int xF,
                double init_value = 0.) const
    {
#define WINDOW(type) _window(*((MultidimArray<type>*)(result.im)), z0,y0,x0,zF,yF,xF,init_value)
        SWITCHDATATYPE(result.datatype,WINDOW);
#undef WINDOW

    }

    template <class T>
    void _window(MultidimArray<T> &result, int z0, int y0, int x0,
                 int zF, int yF, int xF,
                 double init_value = 0.) const
    {
#define WINDOW(type) ((MultidimArray<type>*)im)->window(result, z0,y0,x0,zF,yF,xF,(T)init_value)
        SWITCHDATATYPE(datatype,WINDOW);
#undef WINDOW

    }

    /**
     *  Copy a specific slice of the linked array.
     */
    template <typename T>
    void getSlice(int k, MultidimArray<T> &M, char axis = 'Z', size_t n = 0) const
    {
#define GETSLICE(type) ((MultidimArray<type>*) im)->getSlice(k, M, axis, n)

        SWITCHDATATYPE(datatype,GETSLICE);

#undef GETSLICE

    }

    /**
     *  Copy a specific slice of the linked array.
     */
    void getSlice(int k, MultidimArrayGeneric* M, char axis = 'Z', size_t n = 0) const
    {
#define GETSLICE(type) getSlice(k, *(MultidimArray<type>*)M->im, axis, n)

        SWITCHDATATYPE(M->datatype,GETSLICE);

#undef GETSLICE

    }

    /**
     *  Set a specific slice of the linked array.
     */
    template <typename T1>
    void setSlice(int k, const MultidimArray <T1>& v, size_t n = 0)
    {
#define SETSLICE(type) ((MultidimArray<type>*) im)->setSlice(k, v, n)

        SWITCHDATATYPE(datatype,SETSLICE);

#undef SETSLICE

    }

    /**
     * Set a specific slice of the linked array.
     */
    void setSlice(int k, const MultidimArrayGeneric* v, size_t n = 0)
    {
#define SETSLICE(type) setSlice(k,*(MultidimArray<type>*) v->im, n);
        SWITCHDATATYPE(v->datatype,SETSLICE);
#undef SETSLICE

    }

    /**
     * Get the dimensions of the linked array.
     */
    void getDimensions(int& Xdim, int& Ydim, int& Zdim, size_t &Ndim) const
    {
        im->getDimensions(Xdim,Ydim,Zdim,Ndim);
    }
    void getDimensions(int& Xdim, int& Ydim, int& Zdim) const
    {
        size_t Ndim;
        im->getDimensions(Xdim,Ydim,Zdim,Ndim);
    }

    /**
      * Get the dimensions of the linked array.
      */
    inline   void setXmippOrigin()
    {
        im->setXmippOrigin();
    }

    /** Compute average */
    double computeAvg() const
    {
        return im->computeAvg();
    }

    /** Get constant access */
    double operator()(size_t n, int k, int i, int j) const
    {
#define GETVALUE(type) return NZYX_ELEM(*(MultidimArray<type>*)im,n,k,i,j)
        SWITCHDATATYPE(datatype,GETVALUE);
#undef GETVALUE

    }

    /** Get array */
    MultidimArrayBase & operator()()
    {
        return *im;
    }

    /** Get array */
    const MultidimArrayBase & operator()() const
    {
        return *im;
    }

    /** Copy the image in MultidimarrayGeneric to a specific T MultidimArray
     */
    //    template <typename T>
    //    void getImage(MultidimArray<T> &M, size_t n = -1) const
    //    {
    //#define TYPECAST(type) typeCast(*(MultidimArray<type>*)(im), M, n);
    //        SWITCHDATATYPE(datatype, TYPECAST)
    //#undef TYPECAST

    //    }

    /** Copy in MultidimarrayGeneric an image from a specific T MultidimArray
     */
    //    template <typename T>
    //    void setImage(MultidimArray<T> &M, size_t n = -1)
    //    {
    //#define TYPECAST(type) typeCast(M, *(MultidimArray<type>*)(im), n);
    //        SWITCHDATATYPE(datatype, TYPECAST)
    //#undef TYPECAST
    //
    //    }

}
;
//@}

#endif /* MULTIDIM_ARRAY_GENERIC_H_ */
