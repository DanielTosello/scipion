/***************************************************************************
 *
 * Authors:     Carlos Oscar S. Sorzano (coss@cnb.csic.es)
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

#ifndef MULTIDIM_ARRAY_H
#define MULTIDIM_ARRAY_H

#include <typeinfo>

#include "funcs.h"
#include "error.h"
#include "args.h"
#include "matrix1d.h"
#include "matrix2d.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


extern int bestPrecision(float F, int _width);
extern std::string floatToString(float F, int _width, int _prec);

/// @defgroup MultidimensionalArrays Multidimensional Arrays
/// @ingroup Arrays

/** @defgroup MultidimArraysSpeedUp Speed up macros
 * @ingroup MultidimensionalArrays
 *
 * This macros are defined to allow high speed in critical parts of your
 * program. They shouldn't be used systematically as usually there is no
 * checking on the correctness of the operation you are performing. Speed comes
 * from three facts: first, they are macros and no function call is performed
 * (although most of the critical functions are inline functions), there is no
 * checking on the correctness of the operation (it could be wrong and you are
 * not warned of it), and destination vectors are not returned saving time in
 * the copy constructor and in the creation/destruction of temporary vectors.
 */

/// @defgroup MultidimArraysSizeShape Size and shape
/// @ingroup MultidimArraysSpeedUp

/** Returns the first X valid logical index
 * @ingroup MultidimArraysSizeShape
 */
#define STARTINGX(v) ((v).xinit)

/** Returns the last X valid logical index
 * @ingroup MultidimArraysSizeShape
 */
#define FINISHINGX(v) ((v).xinit + (v).xdim - 1)

/** Returns the first Y valid logical index
 * @ingroup MultidimArraysSizeShape
 */
#define STARTINGY(v) ((v).yinit)

/** Returns the last Y valid logical index
 * @ingroup MultidimArraysSizeShape
 */
#define FINISHINGY(v) ((v).yinit + (v).ydim - 1)

/** Returns the first Z valid logical index
 * @ingroup MultidimArraysSizeShape
 */
#define STARTINGZ(v) ((v).zinit)

/** Returns the last Z valid logical index
 * @ingroup MultidimArraysSizeShape
 */
#define FINISHINGZ(v) ((v).zinit + (v).zdim - 1)

/** Access to X dimension (size)
 * @ingroup MultidimArraysSizeShape
 */
#define XSIZE(v) ((v).xdim)

/** Access to Y dimension (size)
 * @ingroup MultidimArraysSizeShape
 */
#define YSIZE(v) ((v).ydim)

/** Access to Z dimension (size)
 * @ingroup MultidimArraysSizeShape
 */
#define ZSIZE(v) ((v).zdim)

/** Access to N dimension (size)
 * @ingroup MultidimArraysSizeShape
 */
#define NSIZE(v) ((v).ndim)

/** Access to XY dimension (Ysize*Xsize)
 * @ingroup MultidimArraysSizeShape
 */
#define YXSIZE(v) ((v).yxdim)

/** Access to XYZ dimension (Zsize*Ysize*Xsize)
 * @ingroup MultidimArraysSizeShape
 */
#define ZYXSIZE(v) ((v).zyxdim)

/// FIXME MULTIDIM_SIZE will disappear
/** Access to XYZ dimension (Zsize*Ysize*Xsize)
 * @ingroup MultidimArraysSizeShape
 */
#define MULTIDIM_SIZE(v) ((v).nzyxdim)

/** Access to XYZN dimension (Nsize*Zsize*Ysize*Xsize)
 * @ingroup MultidimArraysSizeShape
 */
#define NZYXSIZE(v) ((v).nzyxdim)

/** Array access.
 * @ingroup MultidimArraysSizeShape
 *
 * This macro gives you access to the array (T **)
 */
#ifndef MULTIDIM_ARRAY
#define MULTIDIM_ARRAY(v) ((v).data)
#endif

/** Access to a direct element.
 * @ingroup MultidimArraysSizeShape.
 * v is the array, l is the image, k is the slice, i is the Y index and j is the X index.
 * i and j) within the slice.
 */
#define DIRECT_NZYX_ELEM(v, l, k, i, j) ((v).data[(l)*ZYXSIZE(v)+(k)*YXSIZE(v)+((i)*XSIZE(v))+(j)])

/** Multidim element: Logical access.
 * @ingroup MultidimArraysSizeShape.

 */
#define NZYX_ELEM(v, l, k, i, j)  \
    DIRECT_NZYX_ELEM((v), (l), (k) - STARTINGZ(v), (i) - STARTINGY(v), (j) - STARTINGX(v))

/** Access to a direct element.
 * @ingroup MultidimArraysSizeShape.
 * v is the array, k is the slice and n is the number of the pixel (combined
 * i and j) within the slice.
 */
#define DIRECT_MULTIDIM_ELEM(v,n) ((v).data[(n)])

/** For all direct elements in the array
 * @ingroup MultidimArraysSizeShape
 *
 * This macro is used to generate loops for the array in an easy manner. It
 * defines an internal index 'n' which goes over the slices and 'n' that
 * goes over the pixels in each slice.
 *
 * @code
 * FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(v)
 * {
 *     std::cout << DIRECT_MULTIDIM_ELEM(v,n) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(v) \
    for (unsigned long int n=0; n<NZYXSIZE(v); ++n)

/** For all direct elements in the array
 * @ingroup MultidimArraysSizeShape
 *
 * This macro is used to generate loops for the array in an easy
 * manner. It defines internal indexes 'l', 'k','i' and 'j' which
 * ranges over the n volume using its physical definition.
 *
 * @code
 * FOR_ALL_DIRECT_NZYX_ELEMENTS_IN_MULTIDIMARRAY(v)
 * {
 *     std::cout << DIRECT_NZYX_ELEM(v,l, k, i, j) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_DIRECT_NZYX_ELEMENTS_IN_MULTIDIMARRAY(V) \
    for (size_t l=0; l<NSIZE(V); l++) \
        for (size_t k=0; k<ZSIZE(V); k++) \
            for (size_t i=0; i<YSIZE(V); i++)      \
                for (size_t j=0; j<XSIZE(V); j++)

/** For all direct elements in the array
 * @ingroup MultidimArraysSizeShape
 *
 * This macro is used to generate loops for the array in an easy
 * manner. It defines internal indexes 'l', 'k','i' and 'j' which
 * ranges over the n volume using its logical definition.
 *
 * @code
 * FOR_ALL_NZYX_ELEMENTS_IN_MULTIDIMARRAY(v)
 * {
 *     std::cout << NZYX_ELEM(v,l, k, i, j) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_NZYX_ELEMENTS_IN_MULTIDIMARRAY(V) \
    for (int l=0; l<NSIZE(V); l++) \
        for (int k=STARTINGZ(V); k<=FINISHINGZ(V); k++) \
            for (int i=STARTINGY(V); i<=FINISHINGY(V); i++)     \
                for (int j=STARTINGX(V); j<=FINISHINGX(V); j++)

/** For all direct elements in the array, pointer version
 * @ingroup MultidimArraysSizeShape
 *
 * This macro is used to generate loops for the array in an easy manner. It
 * defines an internal index 'k' which goes over the slices and 'n' that
 * goes over the pixels in each slice. Each element can be accessed through
 * an external pointer called ptr.
 *
 * @code
 * T* ptr=NULL;
 * unsigned long int n;
 * FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(v,n,ptr)
 * {
 *     std::cout << *ptr << " ";
 * }
 * @endcode
 */
#define FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(v,n,ptr) \
    for ((n)=0, (ptr)=(v).data; (n)<NZYXSIZE(v); ++(n), ++(ptr))


/** Access to a direct element.
 * @ingroup MultidimArraySizeShape
 * v is the array, k is the slice (Z), i is the Y index and j is the X index.
 */
#define DIRECT_A3D_ELEM(v,k,i,j) ((v).data[(k)*YXSIZE(v)+((i)*XSIZE(v))+(j)])

/** A short alias for the previous function.
 * @ingroup MultidimArraySizeShape
 *
 */
#define dAkij(V, k, i, j) DIRECT_A3D_ELEM(V, k, i, j)

/** Volume element: Logical access.
 * @ingroup MultidimArraySizeShape
 *
 * @code
 * A3D_ELEM(V, -1, -2, 1) = 1;
 * val = A3D_ELEM(V, -1, -2, 1);
 * @endcode
 */
#define A3D_ELEM(V, k, i, j) \
    DIRECT_A3D_ELEM((V),(k) - STARTINGZ(V), (i) - STARTINGY(V), (j) - STARTINGX(V))

/** For all elements in the array.
 * @ingroup MultidimArraySizeShape
 *
 * This macro is used to generate loops for the volume in an easy way. It
 * defines internal indexes 'k','i' and 'j' which ranges the volume using its
 * mathematical definition (ie, logical access).
 *
 * @code
 * FOR_ALL_ELEMENTS_IN_ARRAY3D(V)
 * {
 *     std::cout << V(k, i, j) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_ELEMENTS_IN_ARRAY3D(V) \
    for (int k=STARTINGZ(V); k<=FINISHINGZ(V); k++) \
        for (int i=STARTINGY(V); i<=FINISHINGY(V); i++) \
            for (int j=STARTINGX(V); j<=FINISHINGX(V); j++)

/** For all elements in common.
 * @ingroup MultidimArraySizeShape
 *
 * This macro is used to generate loops for all the elements logically in common
 * between two volumes in an easy manner. Then k, i and j (locally defined)
 * range from
 *
 * MAX(STARTINGZ(V1),STARTINGZ(V2)) to MIN(FINISHINGZ(V1),FINISHINGZ(V2)),
 * MAX(STARTINGY(V1),STARTINGY(V2)) to MIN(FINISHINGY(V1),FINISHINGY(V2)),
 * MAX(STARTINGX(V1),STARTINGX(V2)) to MIN(FINISHINGX(V1),FINISHINGX(V2))
 *
 * (included limits) respectively. You need to define SPEED_UP_temps.
 *
 * @code
 * SPEED_UP_temps;
 * MultidimArray< double > V1(10, 10, 10), V2(20, 20, 20);
 * V1.setXmippOrigin();
 * V2.setXmippOrigin();
 *
 * FOR_ALL_ELEMENTS_IN_COMMON_IN_ARRAY3D(V1, V2)
 * {
 *    // ...
 * }
 * @endcode
 */
#define FOR_ALL_ELEMENTS_IN_COMMON_IN_ARRAY3D(V1, V2) \
    ispduptmp0 = XMIPP_MAX(STARTINGZ(V1), STARTINGZ(V2)); \
    ispduptmp1 = XMIPP_MIN(FINISHINGZ(V1),FINISHINGZ(V2)); \
    ispduptmp2 = XMIPP_MAX(STARTINGY(V1), STARTINGY(V2)); \
    ispduptmp3 = XMIPP_MIN(FINISHINGY(V1),FINISHINGY(V2)); \
    ispduptmp4 = XMIPP_MAX(STARTINGX(V1), STARTINGX(V2)); \
    ispduptmp5 = XMIPP_MIN(FINISHINGX(V1),FINISHINGX(V2)); \
    for (int k=ispduptmp0; k<=ispduptmp1; k++) \
        for (int i=ispduptmp2; i<=ispduptmp3; i++) \
            for (int j=ispduptmp4; j<=ispduptmp5; j++)

/** For all direct elements in the array.
 * @ingroup VolumeSizeShape
 *
 * This macro is used to generate loops for the volume in an easy way. It
 * defines internal indexes 'k','i' and 'j' which ranges the volume using its
 * physical definition.
 *
 * @code
 * FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY3D(V)
 * {
 *     std::cout << DIRECT_A3D_ELEM(m, k, i, j) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY3D(V) \
    for (size_t k=0; k<ZSIZE(V); k++) \
        for (size_t i=0; i<YSIZE(V); i++) \
            for (size_t j=0; j<XSIZE(V); j++)


/** Access to a direct element of a matrix.
 * @ingroup MatricesSizeShape
 * v is the array, i and j define the element v_ij.
 *
 * Be careful because this is physical access, usually matrices follow the C
 * convention of starting index==0 (X and Y). This function should not be used
 * as it goes against the vector library philosophy unless you explicitly want
 * to access directly to any value in the matrix without taking into account its
 * logical position
 *
 * @code
 * DIRECT_A2D_ELEM(m, 0, 0) = 1;
 * val = DIRECT_A2D_ELEM(m, 0, 0);
 * @endcode

 */
#define DIRECT_A2D_ELEM(v,i,j) ((v).data[(i)*(v).xdim+(j)])

/** Short alias for DIRECT_A2D_ELEM
 * @ingroup MatricesSizeShape
 */
#define dAij(M, i, j) DIRECT_A2D_ELEM(M, i, j)

/** Matrix element: Logical access
 * @ingroup MatricesSizeShape
 *
 * @code
 * A2D_ELEM(m, -2, 1) = 1;
 * val = A2D_ELEM(m, -2, 1);
 * @endcode
 */
#define A2D_ELEM(v, i, j) \
    DIRECT_A2D_ELEM(v, (i) - STARTINGY(v), (j) - STARTINGX(v))

/** TRUE if both arrays have the same shape
 * @ingroup MatricesSizeShape
 *
 * Two arrays have the same shape if they have the same size and the same
 * starting point. Be aware that this is a macro which simplifies to a boolean.
 */
#define SAME_SHAPE2D(v1, v2) \
    (XSIZE(v1) == XSIZE(v2) && \
     YSIZE(v1) == YSIZE(v2) && \
     STARTINGX(v1) == STARTINGX(v2) && \
     STARTINGY(v1) == STARTINGY(v2))

/** For all elements in the array
 * @ingroup MatricesSizeShape
 *
 * This macro is used to generate loops for the matrix in an easy way. It
 * defines internal indexes 'i' and 'j' which ranges the matrix using its
 * mathematical definition (ie, logical access).
 *
 * @code
 * FOR_ALL_ELEMENTS_IN_ARRAY2D(m)
 * {
 *     std::cout << m(i, j) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_ELEMENTS_IN_ARRAY2D(m) \
    for (int i=STARTINGY(m); i<=FINISHINGY(m); i++) \
        for (int j=STARTINGX(m); j<=FINISHINGX(m); j++)

/** For all elements in common
 * @ingroup MatricesSizeShape
 *
 * This macro is used to generate loops for all the elements logically in common
 * between two images in an easy manner. Then i and j (locally defined) range
 * from MAX(STARTINGY(V1), STARTINGY(V2)) to MIN(FINISHINGY(V1),
 * FINISHINGY(V2)), MAX(STARTINGX(V1), STARTINGX(V2)) to MIN(FINISHINGX(V1),
 * FINISHINGX(V2)) (included limits) respectively. You need to define
 * SPEED_UP_temps.
 *
 * @code
 * MultidimArray< double > m1(10, 10), m2(20, 20);
 * m1.setXmippOrigin();
 * m2.setXmippOrigin();
 *
 * FOR_ALL_ELEMENTS_IN_COMMON_IN_ARRAY2D(m1, m2)
 * {
 *     ...
 * }
 * @endcode
 */
#define FOR_ALL_ELEMENTS_IN_COMMON_IN_ARRAY2D(m1, m2) \
    ispduptmp2 = XMIPP_MAX(STARTINGY(m1), STARTINGY(m2)); \
    ispduptmp3 = XMIPP_MIN(FINISHINGY(m1), FINISHINGY(m2)); \
    ispduptmp4 = XMIPP_MAX(STARTINGX(m1), STARTINGX(m2)); \
    ispduptmp5 = XMIPP_MIN(FINISHINGX(m1), FINISHINGX(m2)); \
    for (int i=ispduptmp2; i<=ispduptmp3; i++) \
        for (int j=ispduptmp4; j<=ispduptmp5; j++)

/** For all elements in the array between corners.
 * @ingroup MultidimArraySizeShape
 *
 * This macro is used to generate loops for a volume in an easy manner. Then
 *  ZZ(r), YY(r) and XX(r) range from
 *
 * (int) ZZ(corner1) to (int)ZZ(corner2),
 * (int) YY(corner1) to (int)YY(corner2),
 * (int) XX(corner1) to (int) XX(corner2) (included limits) respectively.
 *
 * Notice that corner1 and corner2 need only be MultidimArray.
 *
 * @code
 * MultidimArray< double > corner1(3), corner2(3), r(3);
 * XX(corner1) = -1; XX(corner2) = 1;
 * YY(corner1) = -2; YY(corner2) = 2;
 * ZZ(corner1) = -3; ZZ(corner2) = 3;
 *
 * FOR_ALL_ELEMENTS_IN_ARRAY3D_BETWEEN(corner1, corner2)
 * {
 *     std::cout << v(r) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_ELEMENTS_IN_ARRAY3D_BETWEEN(corner1, corner2) \
    for (ZZ(r)=ZZ((corner1)); ZZ(r)<=ZZ((corner2)); ZZ(r)++) \
        for (YY(r)=YY((corner1)); YY(r)<=YY((corner2)); YY(r)++) \
            for (XX(r)=XX((corner1)); XX(r)<=XX((corner2)); XX(r)++)


/** For all elements in the array between corners
 * @ingroup MatricesSizeShape
 *
 * This macro is used to generate loops for a matrix in an easy manner. It needs
 * an externally defined MultidimArray< double > r(2). Then YY(r) and XX(r) range
 * from (int) YY(corner1) to (int)YY(corner2), (int) XX(corner1) to (int)
 * XX(corner2) (included limits) respectively. Notice that corner1 and corner2
 * need only be MultidimArray.
 *
 * @code
 * MultidimArray< double > corner1(2), corner2(2);
 * MultidimArray< int > r(2);
 * XX(corner1) = -1;
 * XX(corner2) = 1;
 * YY(corner1) = -2;
 * YY(corner2) = 2;
 *
 * FOR_ALL_ELEMENTS_IN_ARRAY2D_BETWEEN(corner1, corner2)
 * {
 *     std::cout << v(r) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_ELEMENTS_IN_ARRAY2D_BETWEEN(corner1, corner2) \
    for (YY(r)=YY((corner1)); YY(r)<=YY((corner2)); YY(r)++) \
        for (XX(r)=XX((corner1)); XX(r)<=XX((corner2)); XX(r)++)

/** For all elements in the array between corners
 * @ingroup MultidimArraySizeShape
 *
 * This macro is used to generate loops for a vector in an easy manner. It needs
 * an externally defined MultidimArray< double > r(1). Then XX(r) ranges from
 * (int) XX(corner1) to (int) XX(corner2) (included limits) (notice that corner1
 * and corner2 need only to be MultidimArray).
 *
 * @code
 * MultidimArray< double > corner1(1), corner2(1), r(1);
 * XX(corner1) = -1;
 * XX(corner2) = 1;
 * FOR_ALL_ELEMENTS_IN_ARRAY1D_BETWEEN(corner1, corner2)
 * {
 *     std::cout << v(XX(r)) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_ELEMENTS_IN_ARRAY1D_BETWEEN(corner1, corner2) \
    for (XX(r)=(int) XX((corner1)); XX(r)<=(int) XX((corner2)); XX(r)++)

/** For all elements in the array, accessed physically
 * @ingroup MatricesSizeShape
 *
 * This macro is used to generate loops for the matrix in an easy way using
 * physical indexes. It defines internal indexes 'i' and 'j' which ranges the
 * matrix using its physical definition.
 *
 * @code
 * FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(m)
 * {
 *     std::cout << DIRECT_A2D_ELEM(m, i, j) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(m) \
    for (size_t i=0; i<YSIZE(m); i++) \
        for (size_t j=0; j<XSIZE(m); j++)

/** Vector element: Physical access
 * @ingroup MultidimArraySizeShape
 *
 * Be careful because this is physical access, usually vectors follow the C
 * convention of starting index==0. This function should not be used as it goes
 * against the vector library philosophy unless you explicitly want to access
 * directly to any value in the vector without taking into account its logical
 * position.
 *
 * @code
 * DIRECT_A1D_ELEM(v, 0) = 1;
 * val = DIRECT_A1D_ELEM(v, 0);
 * @endcode
 */
#define DIRECT_A1D_ELEM(v, i) ((v).data[(i)])

/** A short alias to previous function
 * @ingroup MultidimArraySizeShape
 */
#define dAi(v, i) DIRECT_A1D_ELEM(v, i)

/** Vector element: Logical access
 * @ingroup MultidimArraySizeShape
 *
 * @code
 * A1D_ELEM(v, -2) = 1;
 * val = A1D_ELEM(v, -2);
 * @endcode
 */
#define A1D_ELEM(v, i) DIRECT_A1D_ELEM(v, (i) - ((v).xinit))

/** For all elements in the array
 * @ingroup MultidimArraySizeShape
 *
 * This macro is used to generate loops for the vector in an easy manner. It
 * defines an internal index 'i' which ranges the vector using its mathematical
 * definition (ie, logical access).
 *
 * @code
 * FOR_ALL_ELEMENTS_IN_ARRAY1D(v)
 * {
 *     std::cout << v(i) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_ELEMENTS_IN_ARRAY1D(v) \
    for (int i=STARTINGX(v); i<=FINISHINGX(v); i++)

/** For all elements in common
 * @ingroup MultidimArraySizeShape
 *
 * This macro is used to generate loops for all the elements logically in common
 * between two vectors in an easy manner. Then i (locally defined) ranges from
 * MAX(STARTINGX(V1), STARTINGX(V2)) to MIN(FINISHINGX(V1), FINISHINGX(V2))
 * (included limits) respectively. You need to define SPEED_UP_temps.
 *
 * @code
 * MultidimArray< double > v1(10), v2(20);
 * v1.setXmippOrigin();
 * v2.setXmippOrigin();
 *
 * FOR_ALL_ELEMENTS_IN_COMMON_IN_ARRAY1D(v1, v2)
 * {
 *     ...
 * }
 * @endcode
 */
#define FOR_ALL_ELEMENTS_IN_COMMON_IN_ARRAY1D(v1, v2) \
    ispduptmp4 = XMIPP_MAX(STARTINGX(v1), STARTINGX(v2)); \
    ispduptmp5 = XMIPP_MIN(FINISHINGX(v1), FINISHINGX(v2)); \
    for (int i=ispduptmp4; i<=ispduptmp5; i++)

/** For all elements in the array, accessed physically
 * @ingroup MultidimArraySizeShape
 *
 * This macro is used to generate loops for the vector in an easy way using
 * physical indexes. It defines internal the index 'i' which ranges the vector
 * using its physical definition.
 *
 * @code
 * FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY1D(v)
 * {
 *     std::cout << DIRECT_A2D_ELEM(v, i) << " ";
 * }
 * @endcode
 */
#define FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY1D(v) \
    for (size_t i=0; i<v.xdim; i++)



// Forward declarations ====================================================
template<typename T>
class MultidimArray;

template<typename T>
void coreArrayByScalar(const MultidimArray<T>& op1, const T& op2,
                       MultidimArray<T>& result, char operation);

template<typename T>
void coreScalarByArray(const T& op1, const MultidimArray<T>& op2,
                       MultidimArray<T>& result, char operation);

template<typename T>
void coreArrayByArray(const MultidimArray<T>& op1, const MultidimArray<T>& op2,
                      MultidimArray<T>& result, char operation);

/* MultidimArray aliases
 * @ingroup MultidimensionalArrays
 * Just for making explicit for what dimensionality these MultidimArrays are meant
 * No check on dimensions or whatsoever is performed..
 * This is just for clarity in the code
 */
#define MultidimArray1D MultidimArray
#define MultidimArray2D MultidimArray
#define MultidimArray3D MultidimArray


/** Template class for Xmipp arrays.
  * @ingroup MultidimensionalArrays
  * This class provides physical and logical access.
*/
template<typename T>
class MultidimArray
{
public:
    /* The array itself.
       The array is always a 3D array (Z,Y,X). For vectors the size of the array
       is (1,1,X) and for matrices (1,Y,X). The pixel (i,j) (y,x) is at the
       position data[i*Xdim+j] or data[y*Xdim+x]
    */
    T* data;

    // Destroy data
    bool destroyData;

    // Number of images
    unsigned long ndim;

    // Number of elements in Z
    int zdim;

    // Number of elements in Y
    int ydim;

    // Number of elements in X
    int xdim;

    // Number of elements in YX
    unsigned long int yxdim;

    // Number of elements in ZYX
    unsigned long int zyxdim;

    // Number of elements in NZYX
    unsigned long int nzyxdim;

    // Z init
    int zinit;

    // Y init
    int yinit;

    // X init
    int xinit;

    //Alloc memory or map to a file
    bool     mmapOn;
    FileName   mapFile;
    int     mFd;

public:
    /// @defgroup MultidimArrayConstructors Constructors
    /// @ingroup MultidimensionalArrays
    /** Empty constructor.
     * @ingroup MultidimArrayConstructors
     * The empty constructor creates an array with no memory associated,
     * size=0.
     */
    MultidimArray()
    {
        coreInit();
    }

    /** Size constructor with 4D size.
     * @ingroup MultidimArrayConstructors
     * The Size constructor creates an array with memory associated,
     * and fills it with zeros.
     */
    MultidimArray(unsigned long int Ndim, int Zdim, int Ydim, int Xdim)
    {
        coreInit();
        resize(Ndim, Zdim, Ydim, Xdim);
    }

    /** Size constructor with 3D size.
     * @ingroup MultidimArrayConstructors
     * The Size constructor creates an array with memory associated,
     * and fills it with zeros.
     */
    MultidimArray(int Zdim, int Ydim, int Xdim)
    {
        coreInit();
        resize(1, Zdim, Ydim, Xdim);
    }

    /** Size constructor with 2D size.
     * @ingroup MultidimArrayConstructors
     * The Size constructor creates an array with memory associated,
     * and fills it with zeros.
     */
    MultidimArray(int Ydim, int Xdim)
    {
        coreInit();
        resize(1, 1, Ydim, Xdim);
    }

    /** Size constructor with 1D size.
     * @ingroup MultidimArrayConstructors
     * The Size constructor creates an array with memory associated,
     * and fills it with zeros.
     */
    MultidimArray(int Xdim)
    {
        coreInit();
        resize(1, 1, 1, Xdim);
    }

    /** Copy constructor
     * @ingroup MultidimArrayConstructors
     *
     * The created volume is a perfect copy of the input array but with a
     * different memory assignment.
     *
     * @code
     * MultidimArray< double > V2(V1);
     * @endcode
     */
    MultidimArray(const MultidimArray<T>& V)
    {
        coreInit();
        *this = V;
    }

    /** Copy constructor from a Matrix1D.
     * @ingroup MultidimArrayConstructors
     * The Size constructor creates an array with memory associated,
     * and fills it with zeros.
     */
    MultidimArray(const Matrix1D<T>& V)
    {
        coreInit();
        resize(1, 1, 1, V.size());
        for (int i = 0; i < V.size(); i++)
            (*this)(i) = V(i);
    }

    /** Constructor from vector 1D
     * @ingroup MultidimArrayConstructors
     * This will create a MultidimArray 1D
     * the size and elements will be copied from
     * the std::vector
     */
    MultidimArray(const std::vector<T> &vector)
    {
        coreInit();
        resize(1, 1, 1, vector.size());
        for (int i = 0; i < vector.size(); i++)
            (*this)(i) = vector[i];
    }

    /** Destructor.
     * @ingroup MultidimArrayConstructors
     */
    ~MultidimArray()
    {
        coreDeallocate();
    }

    /** Clear.
     * @ingroup MultidimArrayConstructors
     */
    void clear()
    {
        coreDeallocate();
        coreInit();
    }

    /// @defgroup MultidimArrayCore Core memory operations for MultidimArrays
    /// @ingroup MultidimensionalArrays
    /** Core init.
     * @ingroup MultidimArrayCore
     * Initialize everything to 0
     */
    void coreInit()
    {
        xdim=yxdim=zyxdim=nzyxdim=0;
        ydim=zdim=ndim=1;
        zinit=yinit=xinit=0;
        data=NULL;
        destroyData=true;
        mmapOn = false;
    }

    /** Core allocate with dimensions.
     * @ingroup MultidimArrayCore
     */
    void coreAllocate(unsigned long int _ndim, int _zdim, int _ydim, int _xdim)
    {
        if (_ndim <= 0 || _zdim <= 0 || _ydim<=0 || _xdim<=0)
        {
            clear();
            return;
        }
        if(data!=NULL)
            REPORT_ERROR(1001, "do not allocate space for an image if you have not deallocate it first");

        ndim=_ndim;
        zdim=_zdim;
        ydim=_ydim;
        xdim=_xdim;
        yxdim=ydim*xdim;
        zyxdim=zdim*yxdim;
        nzyxdim=ndim*zyxdim;

        coreAllocate();
    }

    /** Core allocate without dimensions.
     * @ingroup MultidimArrayCore
     *
     * It is supposed the dimensions are set previously with setXdim(x), setYdim(y)
     * setZdim(z), setNdim(n) or with setDimensions(Xdim, Ydim, Zdim, Ndim);
     *
     */
    void coreAllocate()
    {
        if(data!=NULL)
            REPORT_ERROR(1001, "do not allocate space for an image if you have not deallocate it first");
        if (nzyxdim < 0)
        {
            REPORT_ERROR(1,"coreAllocate:Cannot allocate a negative number of bytes");
        }

        if (mmapOn)
        {
            mapFile.init_random(8);
            mapFile = mapFile.add_extension("tmp");


            if ( ( mFd = open(mapFile.c_str(),  O_RDWR | O_CREAT | O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) ) == -1 )
                REPORT_ERROR(20,"MultidimArray::coreAllocate: Error creating map file.");

            if ((lseek(mFd, nzyxdim*sizeof(T), SEEK_SET) == -1)|| (::write(mFd,"",1) == -1))// Use of :: to call write from global space due to confict with multidimarray::write
            {
                close(mFd);
                REPORT_ERROR(20,"MultidimArray::coreAllocate: Error 'stretching' the map file.");
            }

            if ( (data = (T*) mmap(0,nzyxdim*sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, mFd, 0)) == (void*) -1 )
                REPORT_ERROR(21,"MultidimArray::coreAllocate: mmap failed.");
        }
        else
        {
            data = new T [nzyxdim];
            if (data == NULL)
                REPORT_ERROR(1001, "Allocate: No space left");
        }
    }
    /** Core allocate without dimensions.
     * @ingroup MultidimArrayCore
     *
     * It is supposed the dimensions are set previously with setXdim(x), setYdim(y)
     * setZdim(z), setNdim(n) or with setDimensions(Xdim, Ydim, Zdim, Ndim);
     *
     */
    void coreAllocateReuse()
    {
        if(data!=NULL)
            return;
        if (nzyxdim < 0)
        {
            REPORT_ERROR(1,"coreAllocateReuse:Cannot allocate a negative number of bytes");
        }

        if (mmapOn)
        {
            mapFile.init_random(8);
            mapFile = mapFile.add_extension("tmp");

            if ( ( mFd = open(mapFile.c_str(),  O_RDWR | O_CREAT | O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) ) == -1 )
                REPORT_ERROR(20,"MultidimArray::coreAllocateReuse: Error creating map file.");
            if ((lseek(mFd, nzyxdim*sizeof(T), SEEK_SET) == -1) || (::write(mFd,"",1) == -1))// Use of :: to call write from global space due to confict with multidimarray::write
            {
                close(mFd);
                REPORT_ERROR(20,"MultidimArray::coreAllocateReuse: Error 'stretching' the map file.");
            }

            if ( (data = (T*) mmap(0,nzyxdim*sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, mFd, 0)) == (void*) -1 )
                REPORT_ERROR(21,"MultidimArray::coreAllocateReuse: mmap failed.");
        }
        else
            data = new T [nzyxdim];
        if (data == NULL)
            REPORT_ERROR(1001, "Allocate: No space left");
    }


    /** Sets mmap.
     * @ingroup MultidimArrayMmap
     *
     * Sets on/off mmap flag to allocate memory in a file.
     *
     */
    void setMmap(bool mmap)
    {
        mmapOn = mmap;
    }

    /** Sets new 4D dimensions.
      * @ingroup MultidimArraySize
      *
      *  Note that the dataArray is NOT resized. This should be done separately with coreAllocate()
      *
      */
    void setDimensions(int Xdim, int Ydim, int Zdim, unsigned long int Ndim)
    {
        ndim=Ndim;
        zdim=Zdim;
        ydim=Ydim;
        xdim=Xdim;
        yxdim=ydim*xdim;
        zyxdim=zdim*yxdim;
        nzyxdim=ndim*zyxdim;
    }

    /** Sets new N dimension.
     * @ingroup MultidimArraySize
     *
     *  Note that the dataArray is NOT resized. This should be done separately with coreAllocate()
     *
     */
    void setNdim(int Ndim)
    {
        ndim = Ndim;
        nzyxdim=ndim*zyxdim;
    }

    /** Sets new Z dimension.
     * @ingroup MultidimArraySize
     *
     *  Note that the dataArray is NOT resized. This should be done separately with coreAllocate()
     *
     */
    void setZdim(int Zdim)
    {
        zdim = Zdim;
        zyxdim=zdim*yxdim;
        nzyxdim=ndim*zyxdim;
    }

    /** Sets new Y dimension.
     * @ingroup MultidimArraySize
     *
     *  Note that the dataArray is NOT resized. This should be done separately with coreAllocate()
     *
     */
    void setYdim(int Ydim)
    {
        ydim = Ydim;
        yxdim=ydim*xdim;
        zyxdim=zdim*yxdim;
        nzyxdim=ndim*zyxdim;
    }

    /** Sets new X dimension.
      * @ingroup MultidimArraySize
      *
      *  Note that the dataArray is NOT resized. This should be done separately with coreAllocate()
      *
      */
    void setXdim(int Xdim)
    {
        xdim = Xdim;
        yxdim=ydim*xdim;
        zyxdim=zdim*yxdim;
        nzyxdim=ndim*zyxdim;
    }


    /** Core deallocate.
     * @ingroup MultidimArrayCore
     * Free all data.
     */
    void coreDeallocate()
    {
        if (data != NULL && destroyData)
        {
            if (mmapOn)
            {
                munmap(data,nzyxdim*sizeof(T));
                close(mFd);
                remove(mapFile.c_str());
            }
            else
                delete[] data;
        }
        data=NULL;
    }

    /// @defgroup MultidimSize Size
    /// @ingroup MultidimensionalArrays

    /** Copy the shape parameters
     * @ingroup MultidimSize
     *
     */
    void copyShape(const MultidimArray<T> &m)
    {
        ndim=m.ndim;
        zdim=m.zdim;
        ydim=m.ydim;
        xdim=m.xdim;
        yxdim=m.yxdim;
        zyxdim=m.zyxdim;
        nzyxdim=m.nzyxdim;
        zinit=m.zinit;
        yinit=m.yinit;
        xinit=m.xinit;
    }

    /** Alias a multidimarray.
     * @ingroup MultidimSize
     *
     * Treat the multidimarray as if it were a volume. The data is not copied
     * into new memory, but a pointer to the multidimarray is copied.
     * You should not make any operation on this volume such that the
     * memory locations are changed
     */
    void alias(const MultidimArray<T> &m)
    {
        copyShape(m);
        this->data=m.data;
        this->destroyData=false;
    }

    /** Resize to a given size
     * @ingroup MultidimSize
     *
     * This function resize the actual array to the given size. The origin is
     * not modified. If the actual array is larger than the pattern then the
     * values outside the new size are lost, if it is smaller then 0's are
     * added. An exception is thrown if there is no memory.
     *
     * @code
     * V1.resize(3, 3, 2);
     * @endcode
     */
    void resize(unsigned long int Ndim, int Zdim, int Ydim, int Xdim)
    {
        if (Xdim == XSIZE(*this) && Ydim == YSIZE(*this) &&
            Zdim == ZSIZE(*this) && Ndim == NSIZE(*this) && data !=NULL)
            return;

        if (Xdim <= 0 || Ydim <= 0 || Zdim <= 0 || Ndim <= 0)
        {
            clear();
            return;
        }

        // data can be NULL while xdim etc are set to non-zero values
        // (This can happen for reading of images...)
        // In that case, initialize data to zeros.
        if (NZYXSIZE(*this) > 0 && data == NULL)
        {
            coreAllocate();
            return;
        }

        // Ask for memory
        size_t YXdim=Ydim*Xdim;
        size_t ZYXdim=Zdim*YXdim;
        size_t NZYXdim=Ndim*ZYXdim;
        int    new_mFd;
        FileName   newMapFile;

        T * new_data;

        try
        {
            if (mmapOn)
            {
                newMapFile.init_random(8);
                newMapFile = newMapFile.add_extension("tmp");

                if ( ( new_mFd = open(newMapFile.c_str(),  O_RDWR | O_CREAT | O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) ) == -1 )
                    REPORT_ERROR(20,"MultidimArray::resize: Error creating map file.");
                if ((lseek(new_mFd, NZYXdim*sizeof(T)-1, SEEK_SET) == -1) || (::write(new_mFd,"",1) == -1))
                {
                    close(new_mFd);
                    REPORT_ERROR(20,"MultidimArray::resize: Error 'stretching' the map file.");
                }

                if ( (new_data = (T*) mmap(0,NZYXdim*sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, new_mFd, 0)) == (void*) -1 )
                    REPORT_ERROR(21,"MultidimArray::resize: mmap failed.");
            }
            else
                new_data = new T [NZYXdim];
        }
        catch (std::bad_alloc &)
        {
            REPORT_ERROR(1001, "Allocate: No space left");
        }

        // Copy needed elements, fill with 0 if necessary
        for (unsigned long int l = 0; l < Ndim; l++)
            for (int k = 0; k < Zdim; k++)
                for (int i = 0; i < Ydim; i++)
                    for (int j = 0; j < Xdim; j++)
                    {
                        T val;
                        if (k >= ZSIZE(*this))
                            val = 0;
                        else if (i >= YSIZE(*this))
                            val = 0;
                        else if (j >= XSIZE(*this))
                            val = 0;
                        else
                            val = DIRECT_A3D_ELEM(*this, k, i, j);
                        new_data[l*ZYXdim + k*YXdim+i*Xdim+j] = val;
                    }

        // deallocate old vector
        coreDeallocate();

        // assign *this vector to the newly created
        data = new_data;
        ndim = Ndim;
        xdim = Xdim;
        ydim = Ydim;
        zdim = Zdim;
        yxdim = Ydim * Xdim;
        zyxdim = Zdim * yxdim;
        nzyxdim = Ndim * zyxdim;
        mFd =new_mFd;
        mapFile = newMapFile;

    }

    /** Resize a single 3D image
     * @ingroup MultidimSize
     *
     * This function assumes n is 1
     * @code
     * V1.resize(3, 3, 2);
     * @endcode
     */
    void resize(int Zdim, int Ydim, int Xdim)
    {
        resize(1, Zdim, Ydim, Xdim);
    }

    /** Resize a single 2D image
     * @ingroup MultidimSize
     *
     * This function assumes n and z are 1
     * @code
     * V1.resize(3, 2);
     * @endcode
     */
    void resize(int Ydim, int Xdim)
    {
        resize(1, 1, Ydim, Xdim);
    }

    /** Resize a single 1D image
     * @ingroup MultidimSize
     *
     * This function assumes n and z and y are 1
     * @code
     * V1.resize(2);
     * @endcode
     */
    void resize(int Xdim)
    {
        resize(1, 1, 1, Xdim);
    }

    /** Resize according to a pattern.
     * @ingroup MultidimSize
     *
     * This function resize the actual array to the same size and origin
     * as the input pattern. If the actual array is larger than the pattern
     * then the trailing values are lost, if it is smaller then 0's are
     * added at the end
     *
     * @code
     * v2.resize(v1);
     * // v2 has got now the same structure as v1
     * @endcode
     */
    template<typename T1>
    void resize(const MultidimArray<T1> &v)
    {
        if (NSIZE(*this) != NSIZE(v) || XSIZE(*this) != XSIZE(v) ||
            YSIZE(*this) != YSIZE(v) || ZSIZE(*this) != ZSIZE(v) || data==NULL)
            resize(NSIZE(v), ZSIZE(v), YSIZE(v), XSIZE(v));

        STARTINGX(*this) = STARTINGX(v);
        STARTINGY(*this) = STARTINGY(v);
        STARTINGZ(*this) = STARTINGZ(v);
    }

    /** Returns the multidimArray N,Z, Y and X dimensions.
     * @ingroup MultidimSize
     *
     * @code
     * V.getDimensions(Xdim, Ydim, Zdim, Ndim);
     * @endcode
     */
    void getDimensions(int& Xdim, int& Ydim, int& Zdim, unsigned long int &Ndim) const
    {
        Xdim = XSIZE(*this);
        Ydim = YSIZE(*this);
        Zdim = ZSIZE(*this);
        Ndim = NSIZE(*this);
    }

    /** Returns the total size of the multidimArray
     * @ingroup MultidimSize
     *
     * @code
     * if (V.getSize() > 1) ...
     * @endcode
     */
    unsigned long int getSize() const
    {
        return NZYXSIZE(*this);
    }

    /** Returns the multidimArray dimension.
     * @ingroup MultidimSize
     *
     * @code
     * int dim = V.getDim();
     * @endcode
     */
    int getDim() const
    {
        if (NZYXSIZE(*this) < 1)
            return 0;
        if (ZSIZE(*this) > 1)
            return 3;
        if (YSIZE(*this) > 1)
            return 2;
        else
            return 1;
    }

    /** Check dimension.
     * @ingroup MultidimSize
     *
     * returns true if the dimension is equal to the argument and false otherwise
     * It also prints an error message in the latter case.
     */
#define checkDimension(dim) checkDimensionWithDebug(dim,__FILE__,__LINE__)
    void checkDimensionWithDebug(int dim, const char *file, int line) const
    {
        if (getDim() != dim)
        {
            std::cerr<<" Check for dimension: "  << dim <<std::endl;
            std::cerr << "MultidimArray shape: ";
            printShape(std::cerr);
            std::cerr << std::endl;
            std::cerr << "Check called from file "<<file<<" line "<<line<<std::endl;
            exit(1);
        }
    }

    /** Get size.
     * @ingroup MultidimSize
     *
     * Returns the size of the object in a 4D vector. If the object is a matrix
     * or a vector, then the higher order dimensions will be set to 1, ie,
     * (Xdim, 1, 1) or (Xdim, Ydim, 1).
     *
     * This function is not ported to Python.
     */
    void getSize(int* size) const
    {
        size[0] = xdim;
        size[1] = ydim;
        size[2] = zdim;
        size[3] = ndim;
    }
    /** Generic window routine (dim independent)
     * @ingroup MultidimSize
     *
     * This function will call to 3D,2D or 1D specific window routines
     */
    void window(int n0,int z0, int y0, int x0,
                int nF,int zF, int yF, int xF,
                T init_value = 0, unsigned long n = 0)
    {
        if (this->ndim >1)
        {
            REPORT_ERROR(1,"stack windowing not implemented");
        }
        if (this->zdim >1)
        {//call 3Dwindow
            window( z0,  y0,  x0,
                    zF,  yF,  xF,
                    init_value ,n );
        }
        else if (this->ydim >1)
        {//call 2Dwindow
            window( y0,  x0,
                    yF,  xF,
                    init_value ,  n );

        }
        else if (this->xdim >1)
        {//call 1Dwindow
            window( x0,
                    xF,
                    init_value = 0,  n );
        }
    }
    /** Put a 3D window to the nth volume
     * @ingroup MultidimSize
     *
     * The volume is windowed within the two positions given to this function.
     * Indexes always refer to logical indexes. If a position is outside the
     * actual matrix range then the matrix is padded init_value until the
     * new position is reached. In the following example suppose that m1
     * is the following and that the origin is (-1,-1,-1).
     *
     * @code
     * slice 0
     * [01 02 03          [
     *  04 05 06           04 05 06 0
     *  07 08 09]          07 08 09 0]
     *
     * ----->
     *
     * slice 1
     * [11 12 13          [
     *  14 15 16           14 15 16 0
     *  17 18 19]          17 18 19 0]
     * @endcode
     *
     * @code
     * V1.window(0, 0, -1, 1, 1, 2);
     * @endcode
     */
    void window(int z0, int y0, int x0, int zF, int yF, int xF, T init_value = 0, unsigned long n = 0)
    {
        MultidimArray<T> result(1, zF - z0 + 1, yF - y0 + 1, xF - x0 + 1);
        result.zinit = z0;
        result.yinit = y0;
        result.xinit = x0;

        for (int k = z0; k <= zF; k++)
            for (int i = y0; i <= yF; i++)
                for (int j = x0; j <= xF; j++)
                    if ((k >= STARTINGZ(*this) && k <= FINISHINGZ(*this)) &&
                        (i >= STARTINGY(*this) && i <= FINISHINGY(*this)) &&
                        (j >= STARTINGX(*this) && j <= FINISHINGX(*this)))
                        A3D_ELEM(result, k, i, j) = NZYX_ELEM(*this, n, k, i, j);
                    else
                        A3D_ELEM(result, k, i, j) = init_value;

        *this = result;
    }

    /** Put a 2D window to the nth matrix
     * @ingroup MultidimSize
     *
     * The matrix is windowed within the two positions given to this function.
     * Indexes always refer to logical indexes. If a position is outside the
     * actual matrix range then the matrix is padded with init_value until the
     * new position is reached. In the following examples suppose that m1 is the
     * following and that the origin is (-1,-1).
     *
     * @code
     *      [1 2 3               [1 2 3 0
     * m1 =  4 5 6    --->  m1 =  4 5 6 0
     *       7 8 9]               7 8 9 0]
     *
     * @endcode
     *
     * @code
     * m1.window(-1, -1, 1, 2);
     * @endcode
     */
    void window(int y0, int x0, int yF, int xF, T init_value = 0, unsigned long n = 0)
    {
        MultidimArray<T> result(1, 1, yF - y0 + 1, xF - x0 + 1);
        STARTINGY(result) = y0;
        STARTINGX(result) = x0;

        FOR_ALL_ELEMENTS_IN_ARRAY2D(result)
        if (j >= STARTINGX(*this) && j <= FINISHINGX(*this) &&
            i >= STARTINGY(*this) && i <= FINISHINGY(*this))
            A2D_ELEM(result, i, j) = NZYX_ELEM(*this, n, 0, i, j);
        else
            A2D_ELEM(result, i, j) = init_value;

        *this = result;
    }

    /** Put a 1D window to the nth vector
     * @ingroup MultidimSize
     *
     * The vector is windowed within the two indexes given to this function.
     * Indexes always refer to logical indexes. If an index is outside the
     * actual vector range then the vector is padded winit_value. In the
     * following examples suppose that v1=[-2 -1 0 1 2] and that the origin is
     * -2.
     *
     * @code
     * v1.window(-1, 2); // v1=[-1 0 1 2]; v1.startingX() == -1
     *
     * v1.window(-3, 1); // v1=[0 -2 -1 0 1]; v1.startingX() == -3
     * @endcode
     */
    void window(int x0, int xF, T init_value = 0, unsigned long n = 0)
    {
        MultidimArray<T> result(xF - x0 + 1);
        STARTINGX(result) = x0;

        for (int j = x0; j <= xF; j++)
            if (j >= STARTINGX(*this) && j <= FINISHINGX(*this))
                A1D_ELEM(result, j) = NZYX_ELEM(*this, n, 0, 0, j);
            else
                A1D_ELEM(result, j) = init_value;

        *this = result;
    }

    /** Print shape of multidimensional array.
     * @ingroup MultidimSize
     *
     * This function shows the size, starting and finishing indexes of the
     * given array. No end of line is printed neither at the beginning nor
     * the end.
     *
     * @code
     * v.printShape();
     *
     * std::ofstream fh;
     * ...;
     * v.printShape(fh);
     * @endcode
     */
    void printShape(std::ostream& out = std::cout) const
    {
        if (NSIZE(*this) > 1)
            out << " Number of images = "<<NSIZE(*this);

        int dim = getDim();
        if (dim == 3)
            out<< " Size(Z,Y,X): " << ZSIZE(*this) << "x" << YSIZE(*this) << "x" << XSIZE(*this)
            << " k=[" << STARTINGZ(*this) << ".." << FINISHINGZ(*this) << "]"
            << " i=[" << STARTINGY(*this) << ".." << FINISHINGY(*this) << "]"
            << " j=[" << STARTINGX(*this) << ".." << FINISHINGX(*this) << "]";
        else if (dim == 2)
            out<< " Size(Y,X): " << YSIZE(*this) << "x" << XSIZE(*this)
            << " i=[" << STARTINGY(*this) << ".." << FINISHINGY(*this) << "]"
            << " j=[" << STARTINGX(*this) << ".." << FINISHINGX(*this) << "]";
        else if (dim == 1)
            out<< " Size(X): " << XSIZE(*this)
            << " j=[" << STARTINGX(*this) << ".." << FINISHINGX(*this) << "]";
        else
            out << " Empty MultidimArray!";
        out<<"\n";

    }

    /** Same shape.
     * @ingroup MultidimSize
     *
     * Returns true if this object has got the same shape (origin and size)
     * than the argument
     */
    template <typename T1>
    bool sameShape(const MultidimArray<T1>& op) const
    {
        return (NSIZE(*this) == NSIZE(op) &&
                XSIZE(*this) == XSIZE(op) &&
                YSIZE(*this) == YSIZE(op) &&
                ZSIZE(*this) == ZSIZE(op) &&
                STARTINGX(*this) == STARTINGX(op) &&
                STARTINGY(*this) == STARTINGY(op) &&
                STARTINGZ(*this) == STARTINGZ(op));
    }


    /** Outside for 3D matrices
     * @ingroup MultidimSize
     *
     * TRUE if the logical index given is outside the definition region of this
     * array.
     */
    bool outside(int k, int i, int j) const
    {
        return (j < STARTINGX(*this) || j > FINISHINGX(*this) ||
                i < STARTINGY(*this) || i > FINISHINGY(*this) ||
                k < STARTINGZ(*this) || k > FINISHINGZ(*this));
    }

    /** Outside for 2D matrices
     * @ingroup MultidimSize
     *
     * TRUE if the logical index given is outside the definition region of this
     * array.
     */
    bool outside(int i, int j) const
    {
        return (j < STARTINGX(*this) || j > FINISHINGX(*this) ||
                i < STARTINGY(*this) || i > FINISHINGY(*this));
    }

    /** Outside for 1D matrices
     * @ingroup MultidimSize
     *
     * TRUE if the logical index given is outside the definition region of this
     * array.
     */
    bool outside(int i) const
    {
        return (i < STARTINGX(*this) || i > FINISHINGX(*this));
    }

    /** Outside
     * @ingroup MultidimSize
     *
     * TRUE if the logical index given is outside the definition region of this
     * array.
     */
    bool outside(const Matrix1D<double> &r) const
    {
        if (r.size() < 1)
        {
            REPORT_ERROR(1, "Outside: index vector has not got enough components");
        }
        else if (r.size()==1)
        {
            return (XX(r) < STARTINGX(*this) || XX(r) > FINISHINGX(*this));
        }
        else if (r.size()==2)
        {
            return (XX(r) < STARTINGX(*this) || XX(r) > FINISHINGX(*this) ||
                    YY(r) < STARTINGY(*this) || YY(r) > FINISHINGY(*this));
        }
        else if (r.size()==3)
        {
            return (XX(r) < STARTINGX(*this) || XX(r) > FINISHINGX(*this) ||
                    YY(r) < STARTINGY(*this) || YY(r) > FINISHINGY(*this) ||
                    ZZ(r) < STARTINGZ(*this) || ZZ(r) > FINISHINGZ(*this));
        }
        else
            REPORT_ERROR(2,"Outside: index vector has too many components");
    }


    ///defgroup MultidimMemory Access to the pixel values
    /** Volume element access via double vector.
     * @ingroup MultidimMemory
     *
     * Returns the value of a matrix logical position, but this time the
     * element position is determined by a R3 vector. The elements can be used
     * either by value or by reference. An exception is thrown if the index is
     * outside the logical range. Pay attention in the following example that
     * we are accessing the same element as in the previous function but, now
     * we have to give first the X position because we are building first a
     * vector of the form (x,y,z).
     *
     * @code
     * V(vectorR3(1, -2, 0)) = 1;
     * val = V(vectorR3(1, -2, 0));
     * @endcode
     */
    T& operator()(Matrix1D< double >& v) const
    {
        v.resize(3);
        return A3D_ELEM((*this), ROUND(ZZ(v)), ROUND(YY(v)), ROUND(XX(v)));
    }

    /** Volume element access via integer vector.
     * @ingroup MultidimMemory
     */
    T& operator()(Matrix1D< int >& v) const
    {
        v.resize(3);
        return A3D_ELEM((*this), ZZ(v), YY(v), XX(v));
    }

    /** 4D element access via index.
    * @ingroup MultidimMemory
    *
    * Returns the value of a matrix logical position. In our example we could
    * access from v(0, 0,-2,-1) to v(0, 1,2,1). The elements can be used either by
    * value or by reference. An exception is thrown if the index is outside
    * the logical range. Be careful that the argument order is (Z,Y,X).
    *
    * @code
    * V(0, 0, -2, 1) = 1;
    * val = V(0, 0, -2, 1);
    * @endcode
    */
    T& operator()(unsigned long n, int k, int i, int j) const
    {
        return NZYX_ELEM(*this, n, k, i, j);
    }

    /** 3D element access via index.
    * @ingroup MultidimMemory
    *
    * Returns the value of a matrix logical position. In our example we could
    * access from v(0,-2,-1) to v(1,2,1). The elements can be used either by
    * value or by reference. An exception is thrown if the index is outside
    * the logical range. Be careful that the argument order is (Z,Y,X).
    *
    * @code
    * V(0, -2, 1) = 1;
    * val = V(0, -2, 1);
    * @endcode
    */
    T& operator()(int k, int i, int j) const
    {
        return A3D_ELEM(*this, k, i, j);
    }

    /** 3D element access via index (getVoxel).
    * @ingroup MultidimMemory
    *
    * Same function as operator() but with a name
    *
    */
    T getVoxel(int k, int i, int j) const
    {
        return A3D_ELEM(*this, k, i, j);
    }

    /** 3D element access via index (setVoxel).
    * @ingroup MultidimMemory
    *
    * Same function as operator() but with a name
    *
    */
    void setVoxel(int k, int i, int j, T newval)
    {
        A3D_ELEM(*this, k, i, j)=newval;
    }

    /** Matrix element access via index
     * @ingroup MultidimMemory
     *
     * Returns the value of a matrix logical position. In our example we could
     * access from v(-2,-1) to v(2,1). The elements can be used either by value
     * or by reference. An exception is thrown if the index is outside the
     * logical range. The first argument is the Y position and the second the X
     * position.
     *
     * @code
     * m(-2, 1) = 1;
     * val = m(-2, 1);
     * @endcode
     */
    T& operator()(int i, int j) const
    {
        return A2D_ELEM(*this, i, j);
    }

    /** Vector element access
     * @ingroup MultidimMemory
     *
     * Returns the value of a vector logical position. In our example we could
     * access from v(-2) to v(2). The elements can be used either by value or by
     * reference. An exception is thrown if the index is outside the logical
     * range.
     *
     * @code
     * v(-2) = 1;
     * val = v(-2);
     * @endcode
     */
    T& operator()(int i) const
    {
        return A1D_ELEM(*this, i);
    }


    /** Get a single 1,2 or 3D image from a multi-image array
     *  @ingroup MultidimMemory
     *
     * This function extracts a single-image array from a multi-image one.
     * @code
     * V.getImage(0, m);
     * @endcode
     */
    void getImage(unsigned long n, MultidimArray<T>& M) const
    {
        if (XSIZE(*this) == 0)
        {
            M.clear();
            return;
        }

        if (n > NSIZE(*this))
            REPORT_ERROR(1," Multidimarray getImage: n larger than NSIZE");

        M.resize(1, ZSIZE(*this), YSIZE(*this), XSIZE(*this));
        FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY3D(M)
        DIRECT_A2D_ELEM(M, i, j) = DIRECT_NZYX_ELEM(*this, n, k, i, j);

        STARTINGX(M) = STARTINGX(*this);
        STARTINGY(M) = STARTINGY(*this);
        STARTINGZ(M) = STARTINGZ(*this);

    }

    /** 2D Slice access for reading.
     * @ingroup MultidimMemory
     *
     * This function returns a slice (a 2D matrix) corresponding to the choosen
     * slice inside the nth 3D matrix, the numbering of the slices is also logical not
     * physical. This function differs from the previous one in that this one
     * cuts and assign in a single step instead of in two steps, as in
     * the previous example.
     *
     * @code
     * V.slice(0, m);
     * @endcode
     */
    void getSlice(int k, MultidimArray<T>& M, char axis = 'Z', unsigned long n = 0) const
    {
        if (XSIZE(*this) == 0)
        {
            M.clear();
            return;
        }

        switch (axis)
        {
        case 'Z':
            if (k < STARTINGZ(*this) || k > FINISHINGZ(*this))
                REPORT_ERROR(1203,
                             "Slice: Multidim subscript (k) out of range");

            k = k - STARTINGZ(*this);
            M.resize(1, 1, YSIZE(*this), XSIZE(*this));
            FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(M)
            DIRECT_A2D_ELEM(M, i, j) = DIRECT_NZYX_ELEM(*this, n, k, i, j);
            STARTINGX(M) = STARTINGX(*this);
            STARTINGY(M) = STARTINGY(*this);
            break;
        case 'Y':
            if (k < STARTINGY(*this) || k > FINISHINGY(*this))
                REPORT_ERROR(1203,
                             "Slice: Multidim subscript (i) out of range");

            k = k - STARTINGY(*this);
            M.resize(ZSIZE(*this), XSIZE(*this));
            FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(M)
            DIRECT_A2D_ELEM(M, i, j) = DIRECT_NZYX_ELEM(*this, n, i, k, j);
            STARTINGX(M) = STARTINGX(*this);
            STARTINGY(M) = STARTINGZ(*this);
            break;
        case 'X':
            if (k < STARTINGX(*this) || k > FINISHINGX(*this))
                REPORT_ERROR(1203,
                             "Slice: Multidim subscript (j) out of range");

            k = k - STARTINGX(*this);
            M.resize(ZSIZE(*this), YSIZE(*this));
            FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(M)
            DIRECT_A2D_ELEM(M, i, j) = DIRECT_NZYX_ELEM(*this, n, i, j, k);
            STARTINGX(M) = STARTINGY(*this);
            STARTINGY(M) = STARTINGZ(*this);
            break;
        default:
            REPORT_ERROR(1205,
                         (std::string) "Slice: not supported axis " + axis);
        }
    }

    /** Slice access for writing.
     * @ingroup MultidimMemory
     *
     * This function sets a 2D matrix corresponding to the choosen slice inside the nth
     * volume, the numbering of the slices is also logical not physical.
     *
     * @code
     * // Copies slice 0 in slice 1
     * V.setSlice(1, (V.slice(0)));
     * @endcode
     */
    void setSlice(int k, const MultidimArray<T>& v, unsigned long n = 0)
    {
        if (xdim == 0)
            return;

        if (k < STARTINGZ(*this) || k > FINISHINGZ(*this))
            REPORT_ERROR(1203,
                         "setSlice: MultidimArray subscript (k) out of range");

        if (v.rowNumber() != YSIZE(*this) || v.colNumber() != XSIZE(*this))
            REPORT_ERROR(1202,
                         "setSlice: MultidimArray dimensions different from the matrix ones");

        k = k - STARTINGZ(*this);

        FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(v)
        DIRECT_NZYX_ELEM(*this, n, k, i, j) = DIRECT_A2D_ELEM(v, i, j);
    }

    /** Returns Y dimension.
     * @ingroup MultidimMemory
     */
    int rowNumber() const
    {
        return ydim;
    }

    /** Returns X dimension.
     * @ingroup MultidimMemory
     */
    int colNumber() const
    {
        return xdim;
    }


    /** Get Column
     * @ingroup MultidimMemory
     *
     * This function returns a column vector corresponding to the
     * choosen column.
     *
     * @code
     * std::vector< double > v;
     * m.getCol(-1, v);
     * @endcode
     */
    void getCol(int j, MultidimArray<T>& v) const
    {
        if (xdim == 0 || ydim == 0)
        {
            v.clear();
            return;
        }

        if (j < 0 || j >= xdim)
            REPORT_ERROR(1103,"getCol: Matrix subscript (j) greater than matrix dimension");

        v.resize(ydim);
        for (int i = 0; i < ydim; i++)
            v(i) = (*this)(i, j);

    }

    /** Set Column
     * @ingroup MultidimMemory
     *
     * This function sets a column vector corresponding to the choosen column
     * inside matrix.
     *
     * @code
     * m.setCol(0, (m.row(1)).transpose()); // Copies row 1 in column 0
     * @endcode
     */
    void setCol(int j, const MultidimArray<T>& v)
    {
        if (xdim == 0 || ydim == 0)
            REPORT_ERROR(1, "setCol: Target matrix is empty");

        if (j < 0 || j>= xdim)
            REPORT_ERROR(1103, "setCol: Matrix subscript (j) out of range");

        if (v.xdim != ydim)
            REPORT_ERROR(1102,
                         "setCol: Vector dimension different from matrix one");

        for (int i = 0; i < ydim; i++)
            (*this)(i, j) = v(i);
    }

    /** Get row
     * @ingroup MultidimMemory
     *
     * This function returns a row vector corresponding to the choosen
     * row inside the nth 2D matrix, the numbering of the rows is also
     * logical not physical.
     *
     * @code
     * std::vector< double > v;
     * m.getRow(-2, v);
     * @endcode
     */
    void getRow(int i, MultidimArray<T>& v) const
    {
        if (xdim == 0 || ydim == 0)
        {
            v.clear();
            return;
        }

        if (i < 0 || i >= ydim)
            REPORT_ERROR(1103, "getRow: Matrix subscript (i) greater than matrix dimension");

        v.resize(xdim);
        for (int j = 0; j < xdim; j++)
            v(j) = (*this)(i, j);

    }

    /** Set Row
     * @ingroup MatricesMemory
     *
     * This function sets a row vector corresponding to the choosen row in the 2D Matrix
     *
     * @code
     * m.setRow(-2, m.row(1)); // Copies row 1 in row -2
     * @endcode
     */
    void setRow(int i, const MultidimArray<T>& v)
    {
        if (xdim == 0 || ydim == 0)
            REPORT_ERROR(1, "setRow: Target matrix is empty");

        if (i < 0 || i >= ydim)
            REPORT_ERROR(1103, "setRow: Matrix subscript (i) out of range");

        if (v.xdim != xdim)
            REPORT_ERROR(1102,
                         "setRow: Vector dimension different from matrix one");

        for (int j = 0; j < xdim; j++)
            (*this)(i, j) = v(j);
    }

    /** 3D Logical to physical index translation.
     * @ingroup MultidimArrayMemory
     *
     * This function returns the physical position of a logical one.
     *
     * @code
     * m.toPhysical(k_log, i_log, j_log, k_phys, i_phys, j_phys);
     * @endcode
     */
    void toPhysical(int k_log, int i_log, int j_log,
                    int& k_phys, int& i_phys, int& j_phys) const
    {
        k_phys = k_log - STARTINGZ(*this);
        i_phys = i_log - STARTINGY(*this);
        j_phys = j_log - STARTINGX(*this);
    }

    /** 3D Physical to logical index translation.
     * @ingroup MultidimArrayMemory
     *
     * This function returns the logical position of a physical one.
     *
     * @code
     * m.toLogical(i_phys, j_phys, i_log, j_log);
     * @endcode
     */
    void toLogical(int k_phys, int i_phys, int j_phys,
                   int& k_log, int& i_log, int& j_log) const
    {
        k_log = k_phys + STARTINGZ(*this);
        i_log = i_phys + STARTINGY(*this);
        j_log = j_phys + STARTINGX(*this);
    }

    /** 2D Logical to physical index translation
     * @ingroup MultidimMemory
     *
     * This function returns the physical position of a logical one.
     *
     * @code
     * m.toPhysical(i_log, j_log, i_phys, j_phys);
     * @endcode
     */
    void toPhysical(int i_log, int j_log, int& i_phys, int& j_phys) const
    {
        i_phys = i_log - STARTINGY(*this);
        j_phys = j_log - STARTINGX(*this);
    }

    /** 2D Physical to logical index translation
     * @ingroup MultidimMemory
     *
     * This function returns the logical position of a physical one.
     *
     * @code
     * m.toLogical(i_phys, j_phys, i_log, j_log);
     * @endcode
     */
    void toLogical(int i_phys, int j_phys, int &i_log, int& j_log) const
    {
        i_log = i_phys + STARTINGY(*this);
        j_log = j_phys + STARTINGX(*this);
    }

    /** 1D Logical to physical index translation
     * @ingroup MultidimMemory
     *
     * This function returns the physical position of a logical one.
     *
     * @code
     * v.toPhysical(i_log, i_phys);
     * @endcode
     */
    void toPhysical(int i_log, int& i_phys) const
    {
        i_phys = i_log - STARTINGX(*this);
    }

    /** 1D Physical to logical index translation.
     * @ingroup MultidimMemory
     *
     * This function returns the logical position of a physical one.
     *
     * @code
     * v.toLogical(i_phys, i_log);
     * @endcode
     */
    void toLogical(int i_phys, int& i_log) const
    {
        i_log = i_phys + STARTINGX(*this);
    }

    /** Interpolates the value of the nth 3D matrix M at the point (x,y,z).
     * @ingroup MultidimMemory
     *
     * (x,y,z) are in logical coordinates.
     */
    T interpolatedElement3D(double x, double y, double z, T outside_value = (T) 0, int n = 0)
    {
        int x0 = FLOOR(x);
        double fx = x - x0;
        int x1 = x0 + 1;

        int y0 = FLOOR(y);
        double fy = y - y0;
        int y1 = y0 + 1;

        int z0 = FLOOR(z);
        double fz = z - z0;
        int z1 = z0 + 1;

        T d000 = (outside(z0, y0, x0)) ? outside_value : NZYX_ELEM(*this, n, z0, y0, x0);
        T d001 = (outside(z0, y0, x1)) ? outside_value : NZYX_ELEM(*this, n, z0, y0, x1);
        T d010 = (outside(z0, y1, x0)) ? outside_value : NZYX_ELEM(*this, n, z0, y1, x0);
        T d011 = (outside(z0, y1, x1)) ? outside_value : NZYX_ELEM(*this, n, z0, y1, x1);
        T d100 = (outside(z1, y0, x0)) ? outside_value : NZYX_ELEM(*this, n, z1, y0, x0);
        T d101 = (outside(z1, y0, x1)) ? outside_value : NZYX_ELEM(*this, n, z1, y0, x1);
        T d110 = (outside(z1, y1, x0)) ? outside_value : NZYX_ELEM(*this, n, z1, y1, x0);
        T d111 = (outside(z1, y1, x1)) ? outside_value : NZYX_ELEM(*this, n, z1, y1, x1);

        double dx00 = LIN_INTERP(fx, (double) d000, (double) d001);
        double dx01 = LIN_INTERP(fx, (double) d100, (double) d101);
        double dx10 = LIN_INTERP(fx, (double) d010, (double) d011);
        double dx11 = LIN_INTERP(fx, (double) d110, (double) d111);
        double dxy0 = LIN_INTERP(fy, (double) dx00, (double) dx10);
        double dxy1 = LIN_INTERP(fy, (double) dx01, (double) dx11);

        return (T) LIN_INTERP(fz, dxy0, dxy1);
    }

    /** Interpolates the value of the nth 2D matrix M at the point (x,y)
     * @ingroup MultidimMemory
     *
     * Bilinear interpolation. (x,y) are in logical coordinates.
     */
    inline T interpolatedElement2D(double x, double y, T outside_value = (T) 0, int n = 0) const
    {
        int x0 = FLOOR(x);
        double fx = x - x0;
        int x1 = x0 + 1;
        int y0 = FLOOR(y);
        double fy = y - y0;
        int y1 = y0 + 1;

        T d00 = outside(y0, x0) ? outside_value : NZYX_ELEM(*this, n, 0, y0, x0);
        T d10 = outside(y1, x0) ? outside_value : NZYX_ELEM(*this, n, 0, y1, x0);
        T d11 = outside(y1, x1) ? outside_value : NZYX_ELEM(*this, n, 0, y1, x1);
        T d01 = outside(y0, x1) ? outside_value : NZYX_ELEM(*this, n, 0, y0, x1);

        double d0 = (T) LIN_INTERP(fx, (double) d00, (double) d01);
        double d1 = (T) LIN_INTERP(fx, (double) d10, (double) d11);
        return (T) LIN_INTERP(fy, d0, d1);
    }

    /** Interpolates the value of the nth 3D matrix M at the point (x,y,z) knowing
     * that this image is a set of B-spline coefficients.
     * @ingroup MultidimMemory
     *
     * (x,y,z) are in logical coordinates.
     */
    T interpolatedElementBSpline3D(double x, double y, double z,
                                   int SplineDegree = 3, unsigned long n = 0)
    {
        int SplineDegree_1 = SplineDegree - 1;

        // Logical to physical
        z -= STARTINGZ(*this);
        y -= STARTINGY(*this);
        x -= STARTINGX(*this);

        int lmax = XSIZE(*this);
        int mmax = YSIZE(*this);
        int nmax = ZSIZE(*this);

        int l1 = CEIL(x - SplineDegree_1);
        int l2 = l1 + SplineDegree;

        int m1 = CEIL(y - SplineDegree_1);
        int m2 = m1 + SplineDegree;

        int n1 = CEIL(z - SplineDegree_1);
        int n2 = n1 + SplineDegree;

        double zyxsum = 0.0;
        for (int nn = n1; nn <= n2; nn++)
        {
            int equivalent_nn=nn;
            if      (nn<0)
                equivalent_nn=-nn-1;
            else if (nn>=ZSIZE(*this))
                equivalent_nn=2*ZSIZE(*this)-nn-1;
            double yxsum = 0.0;
            for (int m = m1; m <= m2; m++)
            {
                int equivalent_m=m;
                if      (m<0)
                    equivalent_m=-m-1;
                else if (m>=YSIZE(*this))
                    equivalent_m=2*YSIZE(*this)-m-1;
                double xsum = 0.0;
                for (int l = l1; l <= l2; l++)
                {
                    double xminusl = x - (double) l;
                    int equivalent_l=l;
                    if      (l<0)
                        equivalent_l=-l-1;
                    else if (l>=XSIZE(*this))
                        equivalent_l=2*XSIZE(*this)-l-1;
                    double Coeff = (double) DIRECT_NZYX_ELEM(*this, n,
                                   equivalent_nn,equivalent_m,equivalent_l);
                    switch (SplineDegree)
                    {
                    case 2:
                        xsum += Coeff * Bspline02(xminusl);
                        break;
                    case 3:
                        xsum += Coeff * Bspline03(xminusl);
                        break;
                    case 4:
                        xsum += Coeff * Bspline04(xminusl);
                        break;
                    case 5:
                        xsum += Coeff * Bspline05(xminusl);
                        break;
                    case 6:
                        xsum += Coeff * Bspline06(xminusl);
                        break;
                    case 7:
                        xsum += Coeff * Bspline07(xminusl);
                        break;
                    case 8:
                        xsum += Coeff * Bspline08(xminusl);
                        break;
                    case 9:
                        xsum += Coeff * Bspline09(xminusl);
                        break;
                    }
                }

                double yminusm = y - (double) m;
                switch (SplineDegree)
                {
                case 2:
                    yxsum += xsum * Bspline02(yminusm);
                    break;
                case 3:
                    yxsum += xsum * Bspline03(yminusm);
                    break;
                case 4:
                    yxsum += xsum * Bspline04(yminusm);
                    break;
                case 5:
                    yxsum += xsum * Bspline05(yminusm);
                    break;
                case 6:
                    yxsum += xsum * Bspline06(yminusm);
                    break;
                case 7:
                    yxsum += xsum * Bspline07(yminusm);
                    break;
                case 8:
                    yxsum += xsum * Bspline08(yminusm);
                    break;
                case 9:
                    yxsum += xsum * Bspline09(yminusm);
                    break;
                }
            }

            double zminusn = z - (double) nn;
            switch (SplineDegree)
            {
            case 2:
                zyxsum += yxsum * Bspline02(zminusn);
                break;
            case 3:
                zyxsum += yxsum * Bspline03(zminusn);
                break;
            case 4:
                zyxsum += yxsum * Bspline04(zminusn);
                break;
            case 5:
                zyxsum += yxsum * Bspline05(zminusn);
                break;
            case 6:
                zyxsum += yxsum * Bspline06(zminusn);
                break;
            case 7:
                zyxsum += yxsum * Bspline07(zminusn);
                break;
            case 8:
                zyxsum += yxsum * Bspline08(zminusn);
                break;
            case 9:
                zyxsum += yxsum * Bspline09(zminusn);
                break;
            }
        }

        return (T) zyxsum;
    }

    /** Interpolates the value of the nth 2D matrix M at the point (x,y) knowing
     * that this image is a set of B-spline coefficients
     * @ingroup MultidimMemory
     *
     * (x,y) are in logical coordinates
     *
     * To interpolate using splines you must first produce the Bspline
     * coefficients. An example to interpolate an image at (0.5,0.5) using
     * splines would be:
     *
     * @code
     * MultidimArray< double > Bspline_coeffs;
     * myImage.produceSplineCoefficients(Bspline_coeffs, 3);
     * interpolated_value = Bspline_coeffs.interpolatedElementBSpline(0.5,
     * 0.5,3);
     * @endcode
     */
    inline T interpolatedElementBSpline2D(double x, double y, int SplineDegree = 3,
                                          unsigned long n = 0) const
    {
        int SplineDegree_1 = SplineDegree - 1;

        // Logical to physical
        y -= STARTINGY(*this);
        x -= STARTINGX(*this);

        int lmax = XSIZE(*this);
        int mmax = YSIZE(*this);
        int l1 = CEIL(x - SplineDegree_1);
        int l2 = l1 + SplineDegree;
        int m1 = CEIL(y - SplineDegree_1);
        int m2 = m1 + SplineDegree;

        double columns = 0.0;
        for (int m = m1; m <= m2; m++)
        {
            int equivalent_m=m;
            if      (m<0)
                equivalent_m=-m-1;
            else if (m>=YSIZE(*this))
                equivalent_m=2*YSIZE(*this)-m-1;
            int row_m = XSIZE(*this) * equivalent_m;
            double rows = 0.0;
            for (int l = l1; l <= l2; l++)
            {
                double xminusl = x - (double) l;
                int equivalent_l=l;
                if      (l<0)
                    equivalent_l=-l-1;
                else if (l>=XSIZE(*this))
                    equivalent_l=2*XSIZE(*this)-l-1;
                double Coeff = (double) DIRECT_NZYX_ELEM(*this, n, 0, equivalent_m,equivalent_l);
                switch (SplineDegree)
                {
                case 2:
                    rows += Coeff * Bspline02(xminusl);
                    break;

                case 3:
                    rows += Coeff * Bspline03(xminusl);
                    break;

                case 4:
                    rows += Coeff * Bspline04(xminusl);
                    break;

                case 5:
                    rows += Coeff * Bspline05(xminusl);
                    break;

                case 6:
                    rows += Coeff * Bspline06(xminusl);
                    break;

                case 7:
                    rows += Coeff * Bspline07(xminusl);
                    break;

                case 8:
                    rows += Coeff * Bspline08(xminusl);
                    break;

                case 9:
                    rows += Coeff * Bspline09(xminusl);
                    break;
                }
            }

            double yminusm = y - (double) m;
            switch (SplineDegree)
            {
            case 2:
                columns += rows * Bspline02(yminusm);
                break;

            case 3:
                columns += rows * Bspline03(yminusm);
                break;

            case 4:
                columns += rows * Bspline04(yminusm);
                break;

            case 5:
                columns += rows * Bspline05(yminusm);
                break;

            case 6:
                columns += rows * Bspline06(yminusm);
                break;

            case 7:
                columns += rows * Bspline07(yminusm);
                break;

            case 8:
                columns += rows * Bspline08(yminusm);
                break;

            case 9:
                columns += rows * Bspline09(yminusm);
                break;
            }
        }
        return (T) columns;
    }

    /** Interpolates the value of the nth 1D vector M at the point (x) knowing
     * that this vector is a set of B-spline coefficients
     * @ingroup VectorsMemory
     *
     * (x) is in logical coordinates
     *
     * To interpolate using splines you must first produce the Bspline
     * coefficients. An example to interpolate a vector at (0.5) using
     * splines would be:
     *
     * @code
     * MultidimArray< double > Bspline_coeffs;
     * myVector.produceSplineCoefficients(Bspline_coeffs, 3);
     * interpolated_value = Bspline_coeffs.interpolatedElementBSpline(0.5,3);
     * @endcode
     */
    T interpolatedElementBSpline1D(double x, int SplineDegree = 3, unsigned long n = 0) const
    {
        int SplineDegree_1 = SplineDegree - 1;

        // Logical to physical
        x -= STARTINGX(*this);

        int lmax = XSIZE(*this);
        int l1 = CEIL(x - SplineDegree_1);
        int l2 = l1 + SplineDegree;

        double sum = 0.0;
        for (int l = l1; l <= l2; l++)
        {
            double xminusl = x - (double) l;
            int equivalent_l=l;
            if      (l<0)
                equivalent_l=-l-1;
            else if (l>=XSIZE(*this))
                equivalent_l=2*XSIZE(*this)-l-1;
            double Coeff = (double) DIRECT_NZYX_ELEM(*this, n, 0, 0, equivalent_l);
            switch (SplineDegree)
            {
            case 2:
                sum += Coeff * Bspline02(xminusl);
                break;

            case 3:
                sum += Coeff * Bspline03(xminusl);
                break;

            case 4:
                sum += Coeff * Bspline04(xminusl);
                break;

            case 5:
                sum += Coeff * Bspline05(xminusl);
                break;

            case 6:
                sum += Coeff * Bspline06(xminusl);
                break;

            case 7:
                sum += Coeff * Bspline07(xminusl);
                break;

            case 8:
                sum += Coeff * Bspline08(xminusl);
                break;

            case 9:
                sum += Coeff * Bspline09(xminusl);
                break;
            }
        }
        return (T) sum;
    }

    /** Set logical origin in Xmipp fashion.
     * @ingroup MultidimSize
     *
     * This function adjust the starting points in the array such that the
     * center of the array is defined in the Xmipp fashion.
     *
     * @code
     * V.setXmippOrigin();
     * @endcode
     */
    void setXmippOrigin()
    {
        zinit = FIRST_XMIPP_INDEX(zdim);
        yinit = FIRST_XMIPP_INDEX(ydim);
        xinit = FIRST_XMIPP_INDEX(xdim);
    }


    /** Move origin to.
      * @ingroup MultidimSize
      *
      * This function adjust logical indexes such that the Xmipp origin of the
      * array moves to the specified position. For instance, an array whose x
      * indexes go from -1 to 1, if we move the origin to 4, then the x indexes
      * go from 3 to 5. This is very useful for convolution operations where you
      * only need to move the logical starting of the array.
      *
      */
    void moveOriginTo(int k, int i, int j)
    {
        zinit = k + FIRST_XMIPP_INDEX(zdim);
        yinit = i + FIRST_XMIPP_INDEX(ydim);
        xinit = j + FIRST_XMIPP_INDEX(xdim);
    }

    /** Move origin to.
      * @ingroup MultidimSize
      *
      * This function adjust logical indexes such that the Xmipp origin of the
      * array moves to the specified position. For instance, an array whose x
      * indexes go from -1 to 1, if we move the origin to 4, then the x indexes
      * go from 3 to 5. This is very useful for convolution operations where you
      * only need to move the logical starting of the array.
      *
      */
    void moveOriginTo(int i, int j)
    {
        yinit = i + FIRST_XMIPP_INDEX(ydim);
        xinit = j + FIRST_XMIPP_INDEX(xdim);
    }

    /** Returns the first valid logical Z index.
      * @ingroup MultidimSize
      */
    int startingZ() const
    {
        return zinit;
    }

    /** Returns the last valid logical Z index.
     * @ingroup MultidimSize
     */
    int finishingZ() const
    {
        return zinit + zdim - 1;
    }

    /** Returns the first valid logical Y index.
     * @ingroup MultidimSize
     */
    int startingY() const
    {
        return yinit;
    }

    /** Returns the last valid logical Y index.
     * @ingroup MultidimSize
     */
    int finishingY() const
    {
        return yinit + ydim - 1;
    }

    /** Returns the first valid logical X index.
     * @ingroup MultidimSize
     */
    int startingX() const
    {
        return xinit;
    }

    /** Returns the last valid logical X index.
     * @ingroup MultidimSize
     */
    int finishingX() const
    {
        return xinit + xdim - 1;
    }

    /** Produce a 3D array suitable for working with Numerical Recipes.
     * @ingroup MultidimSize
     *
     * This function must be used only as a preparation for routines which need
     * that the first physical index is 1 and not 0 as it usually is in C. New
     * memory is needed to hold the new double pointer array.
     */
    T*** adaptForNumericalRecipes3D(unsigned long n = 0) const
    {
        T*** m = NULL;
        ask_Tvolume(m, 1, ZSIZE(*this), 1, YSIZE(*this), 1, XSIZE(*this));

        FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY3D(*this)
        m[k+1][i+1][j+1] = DIRECT_NZYX_ELEM(*this, n, k, i, j);

        return m;
    }

    /** Kill a 3D array produced for numerical recipes.
     * @ingroup MultidimSize
     */
    void killAdaptationForNumericalRecipes3D(T*** m) const
    {
        free_Tvolume(m, 1, ZSIZE(*this), 1, YSIZE(*this), 1, XSIZE(*this));
    }

    /** Produce a 2D array suitable for working with Numerical Recipes
     * @ingroup MatricesSize
     *
     * This function must be used only as a preparation for routines which need
     * that the first physical index is 1 and not 0 as it usually is in C. New
     * memory is needed to hold the new double pointer array.
     */
    T** adaptForNumericalRecipes2D(unsigned long n = 0) const
    {
        T** m = NULL;
        ask_Tmatrix(m, 1, YSIZE(*this), 1, XSIZE(*this));

        FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY2D(*this)
        m[i+1][j+1] = DIRECT_NZYX_ELEM(*this, n, 0, i, j);

        return m;
    }

    /** Produce a 1D pointer suitable for working with Numerical Recipes (2)
     * @ingroup MultidimSize
     *
     * This function meets the same goal as the one before, however this one
     * work with 2D arrays as a single pointer. The first element of the array
     * is pointed by result[1*Xdim+1], and in general result[i*Xdim+j]
     */
    T* adaptForNumericalRecipes22D() const
    {
        return MULTIDIM_ARRAY(*this) - 1 - XSIZE(*this);
    }

    /** Load 2D array from numerical recipes result.
     * @ingroup MatricesSize
     */
    void loadFromNumericalRecipes2D(T** m, int Ydim, int Xdim)
    {
        resize(Ydim, Xdim);

        for (int i = 1; i <= Ydim; i++)
            for (int j = 1; j <= Xdim; j++)
                (*this)(i - 1, j - 1) = m[i][j];
    }

    /** Kill a 2D array produced for numerical recipes
     * @ingroup MatricesSize
     *
     * The allocated memory is freed.
     */
    void killAdaptationForNumericalRecipes2D(T** m) const
    {
        free_Tmatrix(m, 1, YSIZE(*this), 1, XSIZE(*this));
    }

    /** Kill a 2D array produced for numerical recipes, 2.
     * @ingroup MatricesSize
     *
     * Nothing needs to be done.
     */
    void killAdaptationForNumericalRecipes22D(T** m) const
        {}

    /** Produce a 1D array suitable for working with Numerical Recipes
     * @ingroup MultidimSize
     *
     * This function must be used only as a preparation for routines which need
     * that the first physical index is 1 and not 0 as it usually is in C. In
     * fact the vector provided for Numerical recipes is exactly this same one
     * but with the indexes changed.
     *
     * This function is not ported to Python.
     */
    T* adaptForNumericalRecipes1D() const
    {
        return MULTIDIM_ARRAY(*this) - 1;
    }

    /** Kill a 1D array produced for Numerical Recipes.
     * @ingroup MultidimSize
     *
     * Nothing needs to be done in fact.
     *
     * This function is not ported to Python.
     */
    void killAdaptationForNumericalRecipes1D(T* m) const
        {}

    /// @defgroup Statistics Statistics functions
    /// @ingroup MultidimensionalArrays
    /** Print statistics in current line.
     * @ingroup Statistics
     *
     * No end of line character is written after this print out.
     *
     * @code
     * a.computeStats();
     * std::cout << "Statistics of variable a ";
     * a.printStats();
     * std::cout << std::endl;
     * @endcode
     */
    void printStats(std::ostream& out = std::cout) const
    {
        T minval, maxval;
        double avgval, devval;

        computeStats(avgval, devval, minval, maxval);

        out.setf(std::ios::showpoint);
        int old_prec = out.precision(7);

        out << " min= ";
        out.width(9);
        out << minval;
        out << " max= ";
        out.width(9);
        out << maxval;
        out << " avg= ";
        out.width(9);
        out << avgval;
        out << " dev= ";
        out.width(9);
        out << devval;

        out.precision(old_prec);
    }

    /** Maximum of the values in the array.
     * @ingroup Statistics
     *
     * The returned value is of the same type as the type of the array.
     */
    T computeMax() const
    {
        if (NZYXSIZE(*this) <= 0)
            return static_cast< T >(0);

        T maxval = data[0];

        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        if (*ptr > maxval)
            maxval = *ptr;

        return maxval;
    }

    /** Minimum of the values in the array.
     * @ingroup Statistics
     *
     * The returned value is of the same type as the type of the array.
     */
    T computeMin() const
    {
        if (NZYXSIZE(*this) <= 0)
            return static_cast< T >(0);

        T minval = data[0];

        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        if (*ptr < minval)
            minval = *ptr;

        return minval;
    }

    /** 4D Indices for the minimum element.
     * @ingroup Statistics
     *
     * This function returns the index of the minimum element of an array.
     * array(l,k,i,j). Returns -1 if the array is empty
     */
    void minIndex(int &lmin, int& kmin, int& imin, int& jmin) const
    {
        if (XSIZE(*this) == 0)
        {
            lmin = kmin = imin = jmin = -1;
            return;
        }

        kmin = STARTINGZ(*this);
        imin = STARTINGY(*this);
        jmin = STARTINGX(*this);
        lmin = 0;
        T minval = NZYX_ELEM(*this, lmin, kmin, imin, jmin);


        FOR_ALL_NZYX_ELEMENTS_IN_MULTIDIMARRAY(*this)
        if (NZYX_ELEM(*this, l, k, i, j) > minval)
        {
            minval = NZYX_ELEM(*this, l, k, i, j);
            lmin = l;
            kmin = k;
            imin = i;
            jmin = j;
        }

    }

    /** 3D Indices for the minimum element.
     * @ingroup Statistics
     *
     * This function just calls to the 4D function
     */
    void minIndex(int& kmin, int& imin, int& jmin) const
    {
        int zeroInt=0;
        minIndex(zeroInt,kmin,imin,jmin);
    }

    /** 2D Indices for the minimum element.
     * @ingroup Statistics
     *
     * This function just calls to the 4D function
     */
    void minIndex(int& imin, int& jmin) const
    {
        int zeroInt=0;
        minIndex(zeroInt,zeroInt,imin,jmin);
    }

    /** 1D Indices for the minimum element.
     * @ingroup Statistics
     *
     * This function just calls to the 4D function
     */
    void minIndex(int& jmin) const
    {
        int zeroInt=0;
        minIndex(zeroInt,zeroInt,zeroInt,jmin);
    }

    /** 4D Indices for the maximum element.
     * @ingroup Statistics
     *
     * This function returns the index of the maximum element of an array.
     * array(l,k,i,j). Returns -1 if the array is empty
     */
    void maxIndex(int &lmax, int& kmax, int& imax, int& jmax) const
    {
        if (XSIZE(*this) == 0)
        {
            lmax = kmax = imax = jmax = -1;
            return;
        }

        kmax = STARTINGZ(*this);
        imax = STARTINGY(*this);
        jmax = STARTINGX(*this);
        lmax = 0;
        T maxval = NZYX_ELEM(*this, lmax, kmax, imax, jmax);

        FOR_ALL_NZYX_ELEMENTS_IN_MULTIDIMARRAY(*this)
        if (NZYX_ELEM(*this, l, k, i, j) > maxval)
        {
            maxval = NZYX_ELEM(*this, l, k, i, j);
            lmax = l;
            kmax = k;
            imax = i;
            jmax = j;
        }
    }

    /** 3D Indices for the maximum element.
     * @ingroup Statistics
     *
     * This function just calls to the 4D function
     */
    void maxIndex(int& kmax, int& imax, int& jmax) const
    {
        int dum;
        maxIndex(dum, kmax, imax, jmax);
    }

    /** 2D Indices for the maximum element.
     * @ingroup Statistics
     *
     * This function just calls to the 4D function
     */
    void maxIndex(int& imax, int& jmax) const
    {
        int dum;
        maxIndex(dum, dum, imax, jmax);
    }

    /** 1D Indices for the maximum element.
     * @ingroup Statistics
     *
     * This function just calls to the 4D function
     */
    void maxIndex(int& jmax) const
    {
        int dum;
        maxIndex(dum, dum, dum, jmax);
    }

    /** Minimum and maximum of the values in the array.
     * @ingroup Statistics
     *
     * As doubles.
     */
    void computeDoubleMinMax(double& minval, double& maxval) const
    {
        if (NZYXSIZE(*this) <= 0)
            return;

        minval = maxval = static_cast< double >(data[0]);

        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        {
            if (*ptr < minval)
                minval = static_cast< double >(*ptr);

            if (*ptr > maxval)
                maxval = static_cast< double >(*ptr);
        }
    }

    /** Average of the values in the array.
     * @ingroup Statistics
     *
     * The returned value is always double, independently of the type of the
     * array.
     */
    double computeAvg() const
    {
        if (NZYXSIZE(*this) <= 0)
            return 0;

        double sum = 0;

        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        sum += static_cast< double >(*ptr);

        return sum / NZYXSIZE(*this);
    }

    /** Standard deviation of the values in the array.
     * @ingroup Statistics
     *
     * Be careful that the standard deviation and NOT the variance is returned.
     * The returned value is always double, independently of the type of the
     * array.
     */
    double computeStddev() const
    {
        if (NZYXSIZE(*this) <= 1)
            return 0;

        double avg = 0, stddev = 0;

        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        {
            avg += static_cast< double >(*ptr);
            stddev += static_cast< double >(*ptr) *
                      static_cast< double >(*ptr);
        }

        avg /= NZYXSIZE(*this);
        stddev = stddev / NZYXSIZE(*this) - avg * avg;
        stddev *= NZYXSIZE(*this) / (NZYXSIZE(*this) - 1);

        // Foreseeing numerical instabilities
        stddev = sqrt(static_cast<double>((ABS(stddev))));

        return stddev;
    }


    /** Compute statistics.
     * @ingroup Statistics
     *
     * The average, standard deviation, minimum and maximum value are
     * returned.
     */
    void computeStats(double& avg, double& stddev, T& minval, T& maxval) const
    {
        if (NZYXSIZE(*this) <= 0)
            return;

        avg = 0;
        stddev = 0;

        minval = maxval = data[0];

        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        {
            avg += static_cast< double >(*ptr);
            stddev += static_cast< double >(*ptr) *
                      static_cast< double >(*ptr);

            if (*ptr > maxval)
                maxval = *ptr;

            if (*ptr < minval)
                minval = *ptr;
        }

        avg /= NZYXSIZE(*this);

        if (NZYXSIZE(*this) > 1)
        {
            stddev = stddev / NZYXSIZE(*this) - avg * avg;
            stddev *= NZYXSIZE(*this) / (NZYXSIZE(*this) - 1);

            // Foreseeing numerical instabilities
            stddev = sqrt(static_cast< double >(ABS(stddev)));
        }
        else
            stddev = 0;
    }

    /** Compute statistics within 2D region of 2D image.
     * @ingroup Statistics
     *
     * The 2D region is specified by two corners.
     * Note that this function only works for the 0th image in a multi-image array...
     */
    void computeStats(double& avg,
                      double& stddev,
                      T& min_val,
                      T& max_val,
                      Matrix1D< int >& corner1,
                      Matrix1D< int >& corner2,
                      unsigned long n = 0)
    {
        (*this).checkDimension(2);
        min_val = max_val = NZYX_ELEM((*this), n, 0, YY(corner1), XX(corner1));

        Matrix1D< double > r(3);
        double N = 0, sum = 0, sum2 = 0;

        FOR_ALL_ELEMENTS_IN_ARRAY2D_BETWEEN(corner1, corner2)
        {
            sum += (*this)(r);
            sum2 += (*this)(r) * (*this)(r);
            N++;

            if ((*this)(r) < min_val)
                min_val = (*this)(r);
            else if ((*this)(r) > max_val)
                max_val = (*this)(r);
        }

        if (N != 0)
        {
            avg = sum / N;
            stddev = sqrt(sum2 / N - avg * avg);
        }
        else
        {
            avg = stddev = 0;
        }
    }

    /** IsCorner (in 2D or 3D matrix)
     * @ingroup MultidimSize
     *
     * TRUE if the logical index given is a corner of the definition region of this
     * array.
     */

    //ROB Comment YY,XX is for matrix1d not multidim array
    //I comment this routine till is properlly rewritten
    bool isCorner(const Matrix1D< double >& v) const
    {
        if (v.size() < 2)
            REPORT_ERROR(1, "isCorner: index vector has got not enough components");

        else if (XSIZE(*this)==2)
            return ((XX(v) == STARTINGX(*this)  && YY(v) == STARTINGY(*this))  ||
                    (XX(v) == STARTINGX(*this)  && YY(v) == FINISHINGY(*this)) ||
                    (XX(v) == FINISHINGX(*this) && YY(v) == STARTINGY(*this))  ||
                    (XX(v) == FINISHINGX(*this) && YY(v) == FINISHINGY(*this)));
        else if (XSIZE(*this)==3)
            return ((XX(v) == STARTINGX(*this)  && YY(v) == STARTINGY(*this)  && ZZ(v) == STARTINGZ(*this)) ||
                    (XX(v) == STARTINGX(*this)  && YY(v) == FINISHINGY(*this) && ZZ(v) == STARTINGZ(*this)) ||
                    (XX(v) == FINISHINGX(*this) && YY(v) == STARTINGY(*this)  && ZZ(v) == STARTINGZ(*this))  ||
                    (XX(v) == FINISHINGX(*this) && YY(v) == FINISHINGY(*this) && ZZ(v) == STARTINGZ(*this)) ||
                    (XX(v) == STARTINGX(*this)  && YY(v) == STARTINGY(*this)  && ZZ(v) == FINISHINGZ(*this)) ||
                    (XX(v) == STARTINGX(*this)  && YY(v) == FINISHINGY(*this) && ZZ(v) == FINISHINGZ(*this)) ||
                    (XX(v) == FINISHINGX(*this) && YY(v) == STARTINGY(*this)  && ZZ(v) == FINISHINGZ(*this))  ||
                    (XX(v) == FINISHINGX(*this) && YY(v) == FINISHINGY(*this) && ZZ(v) == FINISHINGZ(*this)));
        else
            REPORT_ERROR(1, "isCorner: index vector has too many components");

    }

    /** Median
     * @ingroup Statistics
     *
     * Calculate the median element.
     *
     * @code
     * med = v1.computeMedian();
     * @endcode
     */
    double computeMedian() const
    {
        if (XSIZE(*this) == 0)
            return 0;

        if (XSIZE(*this) == 1)
            return DIRECT_MULTIDIM_ELEM(*this,0);

        // Initialise data
        MultidimArray< double > temp;
        temp.resize(*this);
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(*this)
        DIRECT_MULTIDIM_ELEM(temp,n)=DIRECT_MULTIDIM_ELEM(*this,n);

        // Sort indexes
        double* temp_array = MULTIDIM_ARRAY(temp)-1;
        qcksrt(NZYXSIZE(*this), temp_array);

        // Get median
        if (NZYXSIZE(*this)%2==0)
            return 0.5*(DIRECT_MULTIDIM_ELEM(temp,NZYXSIZE(*this)/2-1)+
                        DIRECT_MULTIDIM_ELEM(temp,NZYXSIZE(*this)/2  ));
        else
            return DIRECT_MULTIDIM_ELEM(temp,NZYXSIZE(*this)/2);
    }

    /** Adjust the range of the array to a given one.
     * @ingroup Statistics
     *
     * A linear operation is performed on the values of the array such that
     * after it, the values of the array are comprissed between the two values
     * set. The actual array is modified itself
     *
     * @code
     * v.rangeAdjust(0, 1);
     * // The array is now ranging from 0 to 1
     * @endcode
     */
    void rangeAdjust(T minF, T maxF)
    {
        if (NZYXSIZE(*this) <= 0)
            return;

        double min0, max0;
        computeDoubleMinMax(min0, max0);

        // If max0==min0, it means that the vector is a constant one, so the
        // only possible transformation is to a fixed minF
        double slope;
        if (max0 != min0)
            slope = static_cast< double >(maxF - minF) /
                    static_cast< double >(max0 - min0);
        else
            slope = 0;

        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        *ptr = minF + static_cast< T >(slope *
                                       static_cast< double >(*ptr - min0));
    }

    /** Adjust the range of the array to a given one within a mask.
     * @ingroup Statistics
     *
     * A linear operation is performed on the values of the array such that
     * after it, the values of the array are comprissed between the two values
     * set. The actual array is modified itself. The linear transformation
    * is computed within the mask, but it is applied everywhere.
     *
     * @code
     * v.rangeAdjust(0, 1, mask);
     * // The array is now ranging from 0 to 1
     * @endcode
     */
    // This function must be explictly implemented outside
    void rangeAdjust(T minF, T maxF, MultidimArray<int> &mask)
    {
        if (MULTIDIM_SIZE(*this) <= 0)
            return;

        double min0, max0;
        bool first=true;
        T* ptr=NULL;
        unsigned long int n;
        int * ptrMask=MULTIDIM_ARRAY(mask);
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        {
            if (*ptrMask)
            {
                T val= *ptr;
                if (first)
                {
                    min0=max0=(double)val;
                    first=false;
                }
                else
                {
                    min0=XMIPP_MIN(min0,val);
                    max0=XMIPP_MAX(max0,val);
                }
            }
            ptrMask++;
        }

        // If max0==min0, it means that the vector is a constant one, so the
        // only possible transformation is to a fixed minF
        double slope;
        if (max0 != min0)
            slope = static_cast< double >(maxF - minF) /
                    static_cast< double >(max0 - min0);
        else
            slope = 0;

        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        *ptr = minF + static_cast< T >(slope *
                                       static_cast< double >(*ptr - min0));
    }

    /** Adjust the range of the array to the range of another array in
        a least squares sense.
     * @ingroup Statistics
     *
     * A linear operation is performed on the values of the array such that
     * after it, the values of the self array are as similar as possible
     * (L2 sense) to the values of the array shown as sample
     */

    //As written this will only work for T=double
    //nevertheless since this is used is better
    //to use T than double or will create problem for int multidim arrays
    void rangeAdjust(const MultidimArray<T> &example,
                     const MultidimArray<int> *mask=NULL)
    {
        if (NZYXSIZE(*this) <= 0)
            return;

        // y=a+bx
        double sumx=0, sumy=0, sumxy=0, sumx2=0;

        T* ptrExample=MULTIDIM_ARRAY(example);
        int* ptrMask=NULL;
        if (mask!=NULL)
            ptrMask=MULTIDIM_ARRAY(*mask);
        T* ptr=NULL;
        unsigned long int n;
        double N=0;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        {
            bool process=true;
            if (mask!=NULL)
                if (*ptrMask==0)
                    process=false;
            if (process)
            {
                T x=*ptr;
                T y=*ptrExample;
                sumy+=y;
                sumxy+=x*y;
                sumx+=x;
                sumx2+=x*x;
                N++;
            }
            ptrExample++;
            if (mask!=NULL)
                ptrMask++;
        }
        double b=(N*sumxy-sumx*sumy)/(N*sumx2-sumx*sumx);
        double a=sumy/N-b*sumx/N;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        *ptr = static_cast< double >(a+b * static_cast< double > (*ptr));
    }

    /** Adjust the average and stddev of the array to given values.
     * @ingroup Statistics
     *
     * A linear operation is performed on the values of the array such
     * that after it, the average and standard deviation of the array
     * are the two values set. The actual array is modified itself
     *
     * @code
     * v.statisticsAdjust(0,1);
     * // The array has got now 0 mean and stddev=1
     * @endcode
     */
    // This function must be explictly implemented outside.
    void statisticsAdjust(double avgF, double stddevF)
    {
        double avg0, stddev0;
        double a, b;

        if (NZYXSIZE(*this) == 0)
            return;

        T minval, maxval;
        computeStats(avg0, stddev0, minval, maxval);

        if (stddev0 != 0)
            a = static_cast< double >(stddevF) / static_cast< double >(stddev0);
        else
            a = 0;

        b = static_cast< double >(avgF) - a * static_cast< double >(avg0);

        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        *ptr = static_cast< T >(a * static_cast< double > (*ptr) + b);
    }

    /// @defgroup Arithmethic Arithmethic operations
    /// @ingroup MultidimensionalArrays

    /** @defgroup ArrayByArray Array "by" array operations.
     * @ingroup Arithmethic
     *
     * These are operations that are performed between 2 arrays of the
     * SAME type (two integer vectors, two double matrices, ...). If they
     * are not of the same type you can convert one of the arrays to the
     * desired type using the function typeCast. The result must have been
     * defined to be of the same type as the operands.
     *
     * In this kind of operations each element of array 1 is operated with its
     * homologous in array 2, it is very important that both have got the
     * same size and starting origins. The result has also got the same
     * shape as the two operated arrays and its former content is lost.
     */

    /** Core array by array operation.
     * @ingroup ArrayByArray
     *
     * It assumes that the result is already resized.
     */
    friend void coreArrayByArray<>(const MultidimArray<T>& op1,
                                   const MultidimArray<T>& op2, MultidimArray<T>& result,
                                   char operation);

    /** Array by array
     * @ingroup ArrayByArray
     *
     * This function must take two vectors of the same size, and operate element
     * by element according to the operation required. This is the function
     * which really implements the operations. Simple calls to it perform much
     * faster than calls to the corresponding operators. Although it is supposed
     * to be a hidden function not useable by normal programmers.
     *
     */
    friend void arrayByArray(const MultidimArray<T>& op1,
                             const MultidimArray<T>& op2, MultidimArray<T>& result,
                             char operation)
    {
        if (!op1.sameShape(op2))
            REPORT_ERROR(1007,
                         (std::string) "Array_by_array: different shapes (" +
                         operation + ")");

        result.resize(op1);
        coreArrayByArray(op1, op2, result, operation);
    }

    /** v3 = v1 + v2.
     * @ingroup ArrayByArray
     */
    MultidimArray<T> operator+(const MultidimArray<T>& op1) const
    {
        MultidimArray<T> tmp;
        arrayByArray(*this, op1, tmp, '+');
        return tmp;
    }

    /** v3 = v1 - v2.
     * @ingroup ArrayByArray
     */
    MultidimArray<T> operator-(const MultidimArray<T>& op1) const
    {
        MultidimArray<T> tmp;
        arrayByArray(*this, op1, tmp, '-');
        return tmp;
    }

    /** v3 = v1 * v2.
     * @ingroup ArrayByArray
     */
    MultidimArray<T> operator*(const MultidimArray<T>& op1) const
    {
        MultidimArray<T> tmp;
        arrayByArray(*this, op1, tmp, '*');
        return tmp;
    }

    /** v3 = v1 / v2.
     * @ingroup ArrayByArray
     */
    MultidimArray<T> operator/(const MultidimArray<T>& op1) const
    {
        MultidimArray<T> tmp;
        arrayByArray(*this, op1, tmp, '/');
        return tmp;
    }

    /** v3 += v2.
     * @ingroup ArrayByArray
     */
    void operator+=(const MultidimArray<T>& op1)
    {
        arrayByArray(*this, op1, *this, '+');
    }

    /** v3 -= v2.
     * @ingroup ArrayByArray
     */
    void operator-=(const MultidimArray<T>& op1)
    {
        arrayByArray(*this, op1, *this, '-');
    }

    /** v3 *= v2.
     * @ingroup ArrayByArray
     */
    void operator*=(const MultidimArray<T>& op1)
    {
        arrayByArray(*this, op1, *this, '*');
    }

    /** v3 /= v2.
     * @ingroup ArrayByArray
     */
    void operator/=(const MultidimArray<T>& op1)
    {
        arrayByArray(*this, op1, *this, '/');
    }

    /** @defgroup ArrayByScalar Array "by" scalar operations
     * @ingroup Arithmethic
     *
     * These operations are between an array and a scalar (of the same type as
     * the array). The result must have been defined to be of the same type as
     * the operands.
     *
     * In this kind of operations each element of array 1 is operated with the
     * given constant. The result has also got the same shape as the input
     * array and its former content is lost
     */

    /** Array by scalar.
     * @ingroup ArrayByScalar
     *
     * This function must take one vector and a constant, and operate element
     * by element according to the operation required. This is the function
     * which really implements the operations. Simple calls to it perform much
     * faster than calls to the corresponding operators. Although it is
     * supposed to be a hidden function not useable by normal programmers.
     *
     * This function is not ported to Python.
     */
    friend void arrayByScalar(const MultidimArray<T>& op1,
                              T op2,
                              MultidimArray<T>& result,
                              char operation)
    {
        result.resize(op1);
        coreArrayByScalar(op1, op2, result, operation);
    }

    /** Core array by scalar operation.
     * @ingroup ArrayByScalar
     *
     * It assumes that the result is already resized.
     *
     * This function is not ported to Python.
     */
    friend void coreArrayByScalar<>(const MultidimArray<T>& op1,
                                    const T& op2,
                                    MultidimArray<T>& result,
                                    char operation);

    /** v3 = v1 + k.
     * @ingroup ArrayByScalar
     */
    MultidimArray<T> operator+(T op1) const
    {
        MultidimArray<T> tmp;
        arrayByScalar(*this, op1, tmp, '+');
        return tmp;
    }

    /** v3 = v1 - k.
     * @ingroup ArrayByScalar
     */
    MultidimArray<T> operator-(T op1) const
    {
        MultidimArray<T> tmp;
        arrayByScalar(*this, op1, tmp, '-');
        return tmp;
    }

    /** v3 = v1 * k.
     * @ingroup ArrayByScalar
     */
    MultidimArray<T> operator*(T op1) const
    {
        MultidimArray<T> tmp;
        arrayByScalar(*this, op1, tmp, '*');
        return tmp;
    }

    /** v3 = v1 / k.
     * @ingroup ArrayByScalar
     */
    MultidimArray<T> operator/(T op1) const
    {
        MultidimArray<T> tmp;
        arrayByScalar(*this, op1, tmp, '/');
        return tmp;
    }

    /** v3 += k.
     * @ingroup ArrayByScalar
     *
     * This function is not ported to Python.
     */
    void operator+=(const T& op1)
    {
        arrayByScalar(*this, op1, *this, '+');
    }

    /** v3 -= k.
     * @ingroup ArrayByScalar
     *
     * This function is not ported to Python.
     */
    void operator-=(const T& op1)
    {
        arrayByScalar(*this, op1, *this, '-');
    }

    /** v3 *= k.
     * @ingroup ArrayByScalar
     *
     * This function is not ported to Python.
     */
    void operator*=(const T& op1)
    {
        arrayByScalar(*this, op1, *this, '*');
    }

    /** v3 /= k.
     * @ingroup ArrayByScalar
     *
     * This function is not ported to Python.
     */
    void operator/=(const T& op1)
    {
        arrayByScalar(*this, op1, *this, '/');
    }

    /** @defgroup ScalarByArray Scalar "by" array operations
     * @ingroup Arithmethic
     *
     * These operations are between a scalar (of the same type as the array)
     * and an array. The result must have been defined to be of the same type
     * as the operand. The former content of the result array is lost after
     * the operation.
     *
     * In this kind of operations the constant is operated with each element
     * of array 2. The result has also got the same shape as the input array
     * and its former content is lost
     */

    /** Scalar by array.
     * @ingroup ScalarByArray
     *
     * This function must take one scalar and a vector, and operate element by
     * element according to the operation required. This is the function which
     * really implements the operations. Simple calls to it perform much faster
     * than calls to the corresponding operators. Although it is supposed to
     * be a hidden function not useable by normal programmers.
     *
     * This function is not ported to Python.
     */
    friend void scalarByArray(T op1,
                              const MultidimArray<T>& op2,
                              MultidimArray<T>& result,
                              char operation)
    {
        result.resize(op2);
        coreScalarByArray(op1, op2, result, operation);
    }

    /** Core array by scalar operation.
     * @ingroup ScalarByArray
     *
     * It assumes that the result is already resized.
     *
     * This function is not ported to Python.
     */
    friend void coreScalarByArray<>(const T& op1,
                                    const MultidimArray<T>& op2,
                                    MultidimArray<T>& result,
                                    char operation);

    /** v3 = k + v2.
     * @ingroup ScalarByArray
     */
    friend MultidimArray<T> operator+(T op1, const MultidimArray<T>& op2)
    {
        MultidimArray<T> tmp;
        scalarByArray(op1, op2, tmp, '+');
        return tmp;
    }

    /** v3 = k - v2.
     * @ingroup ScalarByArray
     */
    friend MultidimArray<T> operator-(T op1, const MultidimArray<T>& op2)
    {
        MultidimArray<T> tmp;
        scalarByArray(op1, op2, tmp, '-');
        return tmp;
    }

    /** v3 = k * v2.
     * @ingroup ScalarByArray
     */
    friend MultidimArray<T> operator*(T op1, const MultidimArray<T>& op2)
    {
        MultidimArray<T> tmp;
        scalarByArray(op1, op2, tmp, '*');
        return tmp;
    }

    /** v3 = k / v2
     * @ingroup ScalarByArray
     */
    friend MultidimArray<T> operator/(T op1, const MultidimArray<T>& op2)
    {
        MultidimArray<T> tmp;
        scalarByArray(op1, op2, tmp, '/');
        return tmp;
    }

    /// @defgroup Initialization Initialization
    /// @ingroup MultidimensionalArrays

    /** Same value in all components.
     * @ingroup Initialization
     *
     * The constant must be of a type compatible with the array type, ie,
     * you cannot  assign a double to an integer array without a casting.
     * It is not an error if the array is empty, then nothing is done.
     *
     * @code
     * v.initConstant(3.14);
     * @endcode
     */
    void initConstant(T val)
    {
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        *ptr = val;
    }

    /** Initialize to zeros following a pattern.
     * @ingroup Initialization
     *
     * All values are set to 0, and the origin and size of the pattern are
     * adopted.
     *
     * @code
     * v2.initZeros(v1);
     * @endcode
     */
    template <typename T1>
    void initZeros(const MultidimArray<T1>& op)
    {
        resize(op);
        initConstant(static_cast< T >(0));
    }

    /** Initialize to zeros with current size.
     * @ingroup Initialization
     *
     * All values are set to 0. The current size and origin are kept. It is not
     * an error if the array is empty, then nothing is done.
     *
     * @code
     * v.initZeros();
     * @endcode
     */
    void initZeros()
    {
        initConstant(static_cast< T >(0));
    }

    /** Initialize to zeros with a given size.
     * @ingroup Initialization
     */
    void initZeros(unsigned long int Ndim, int Zdim, int Ydim, int Xdim)
    {
        resize(Ndim, Zdim,Ydim,Xdim);
        initZeros();
    }

    /** Initialize to zeros with a given size.
     * @ingroup Initialization
     */
    void initZeros(int Xdim)
    {
        initZeros(1, 1, 1, Xdim);
    }

    /** Initialize to zeros with a given size.
     * @ingroup Initialization
     */
    void initZeros(int Ydim, int Xdim)
    {
        initZeros(1, 1, Ydim, Xdim);
    }

    /** Initialize to zeros with a given size.
     * @ingroup Initialization
     */
    void initZeros(int Zdim, int Ydim, int Xdim)
    {
        initZeros(1, Zdim, Ydim, Xdim);
    }


    /** Linear initialization (only for 1D)
     * @ingroup Initialization
     *
     * The 1D vector is filled with values increasing/decreasing linearly within a
     * range or at given steps.
     *
     * Increment functionality: The default increment is 1, the initial point is
     * incremented by this value until the upper limit is reached. This is the
     * default working mode for the function.
     *
     * @code
     * v1.initLinear(1, 3); // v1=[1 2 3]
     * v1.initLinear(1.5, 3.1); // v1=[1.5 2.5]
     * v1.initLinear(0, 10, 3); // v1=[0 3 6 9]
     * v1.initLinear(0, 10, 3, "incr"); // v1=[0 3 6 9]
     * @endcode
     *
     * Step functionality: The given range is divided in as many points as
     * indicated (in the example 6 points).
     *
     * @code
     * v1.initLinear(0, 10, 6, "steps"); // v1=[0 2 4 6 8 10]
     * @endcode
     */
    void initLinear(T minF, T maxF, int n = 1, const std::string& mode = "incr")
    {
        double slope;
        int steps;

        if (mode == "incr")
        {
            steps = 1 + (int) FLOOR((double) ABS((maxF - minF)) / ((double) n));
            slope = n * SGN(maxF - minF);
        }
        else if (mode == "steps")
        {
            steps = n;
            slope = (maxF - minF) / (steps - 1);
        }
        else
            REPORT_ERROR(1005, "Init_linear: Mode not supported (" + mode +
                         ")");

        if (steps == 0)
            clear();
        else
        {
            resize(steps);
            for (int i = 0; i < steps; i++)
                A1D_ELEM(*this, i) = (T)((double) minF + slope * i);
        }
    }

    /** Initialize with random values.
     * @ingroup Initialization
     *
     * This function allows you to initialize the array with a set of random
     * values picked from a uniform random distribution or a gaussian one. You
     * must choose two parameters for each, for the uniform distribution they
     * mean the range where to generate the random numbers, while in the
     * gaussian case they are the mean and the standard deviation. By default
     * the uniform distribution is selected. The size and origin of the array
     * are not modified.
     *
     * @code
     * v.initRandom(0, 1);
     * // uniform distribution between 0 and 1
     *
     * v.initRandom(0, 1, "uniform");
     * // the same
     *
     * v.initRandom(0, 1, "gaussian");
     * // gaussian distribution with 0 mean and stddev=1
     * @endcode
     */
    void initRandom(double op1, double op2, const std::string& mode = "uniform")
    {
        T* ptr=NULL;
        unsigned long int n;
        if (mode == "uniform")
            FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
            *ptr = static_cast< T >(rnd_unif(op1, op2));
        else if (mode == "gaussian")
            FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
            *ptr = static_cast< T >(rnd_gaus(op1, op2));
        else
            REPORT_ERROR(1005,
                         static_cast< std::string >("InitRandom: Mode not supported (" +
                                                    mode + ")"));
    }

    /** Add noise to actual values.
     * @ingroup Initialization
     *
     * This function add some noise to the actual values of the array according
     * to a certain random distribution. You must choose two parameters for
     * each, for the uniform distribution they mean the range where to generate
     * the random numbers, while in the gaussian case they are the mean and the
     * standard deviation. By default the uniform distribution is selected. The
     * size and origin of the array are not modified. The array itself is
     * modified.
     *
     * @code
     * v1.addNoise(0, 1);
     * // uniform distribution between 0 and 1
     *
     * v1.addNoise(0, 1, "uniform");
     * // the same
     *
     * v1.addNoise(0, 1, "gaussian");
     * // gaussian distribution with 0 mean and stddev=1
     *
     * v1.addNoise(0, 1, "student", 3);
     * // t-student distribution with 0 mean and stddev=1, and 3 degrees of freedom
     *

     * @endcode
     */
    void addNoise(double op1,
                  double op2,
                  const std::string& mode = "uniform",
                  double df = 3) const
    {
        T* ptr=NULL;
        unsigned long int n;
        if (mode == "uniform")
            FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
            *ptr += static_cast< T >(rnd_unif(op1, op2));
        else if (mode == "gaussian")
            FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
            *ptr += static_cast< T >(rnd_gaus(op1, op2));
        else if (mode == "student")
            FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
            *ptr += static_cast< T >(rnd_student_t(df, op1, op2));
        else
            REPORT_ERROR(1005,
                         static_cast< std::string >("AddNoise: Mode not supported (" +
                                                    mode + ")"));
    }

    /** @defgroup MultidimUtilities Utilities
     *  @ingroup MultidimensionalArrays
     *
     * Here you have several easy functions to manage the values of
     * the array.
     */

    /** Computes the center of mass of the nth array
     * @ingroup MultidimUtilities
     */
    void centerOfMass(Matrix1D< double >& center, void * mask=NULL, unsigned long n = 0)
    {
        center.initZeros(3);
        double mass = 0;
        MultidimArray< int >* imask = (MultidimArray< int >*) mask;

        FOR_ALL_ELEMENTS_IN_ARRAY3D(*this)
        {
            if ((imask == NULL || NZYX_ELEM(*imask, n, k, i, j)) &&
                A3D_ELEM(*this, k, i, j) > 0)
            {
                XX(center) += j * NZYX_ELEM(*this, n, k, i, j);
                YY(center) += i * NZYX_ELEM(*this, n, k, i, j);
                ZZ(center) += k * NZYX_ELEM(*this, n, k, i, j);

                mass += NZYX_ELEM(*this, n, k, i, j);
            }
        }

        if (mass != 0)
            center /= mass;
    }

    /** Sort 1D vector elements
     * @ingroup VectorsUtilities
     *
     * Sort in ascending order the vector elements. You can use the "reverse"
     * function to sort in descending order.
     *
     * @code
     * v2 = v1.sort();
     * @endcode
     */
    MultidimArray<T> sort() const
    {
        checkDimension(1);

        MultidimArray<T> temp;
        MultidimArray< double > aux;

        if (xdim == 0)
            return temp;

        // Initialise data
        typeCast(*this, aux);

        // Sort
        double * aux_array = aux.adaptForNumericalRecipes1D();
        qcksrt(xdim, aux_array);

        typeCast(aux, temp);
        return temp;
    }

    /** Gives a vector with the indexes for a sorted vector
     * @ingroup MultidimUtilities
     *
     * This function returns the indexes of a sorted vector. The input vector is
     * not modified at all. For instance, if the input vector is [3 2 -1 0] the
     * result of this function would be [3 4 2 1] meaning that the lowest value
     * is at index 3, then comes the element at index 4, ... Note that
     * indexes start at 1.
     *
     * @code
     * v2 = v1.indexSort();
     * @endcode
     */
    MultidimArray< int > indexSort() const
    {
        checkDimension(1);

        MultidimArray< int >   indx;
        MultidimArray< double > temp;

        if (xdim == 0)
            return indx;

        if (xdim == 1)
        {
            indx.resize(1);
            indx(0) = 1;
            return indx;
        }

        // Initialise data
        indx.resize(xdim);
        typeCast(*this, temp);

        // Sort indexes
        double* temp_array = temp.adaptForNumericalRecipes1D();
        int* indx_array = indx.adaptForNumericalRecipes1D();
        indexx(XSIZE(*this), temp_array, indx_array);

        return indx;
    }

    /** Several thresholding.
     * @ingroup MultidimUtilities
     *
     * Apply a threshold to the array, the object is modified itself. There
     * are several kinds of thresholding and you must specify it, the values
     * given in the fuction have different meanings according to the threshold
     * applied.
     *
     * abs_above: if |x|>a => x=b
     * abs_below: if |x|<a => x=b
     * above:     if  x >a => x=b
     * below:     if  x <a => x=b
     * range:     if  x <a => x=a   and    if x>b => x=b
     *
     * @code
     * v.threshold("abs_above", 10, 10);
     * // any value whose absolute value is above 10 will be substituted by
     * // -10 (if it is negative) or 10 (if it is positive)
     *
     * v.threshold("abs_below", 0.1, 0);
     * // any value whose absolute value is below 0.1 will be substituted by
     * // -0 (if it is negative) or 0 (if it is positive)
     *
     * v.threshold("above", 10, 10);
     * // any value above 10 will be substituted by 10
     *
     * v.threshold("below", -10, -10);
     * // any value below -10 will be substituted by -10
     *
     * v.threshold("range", 0, 1);
     * // v is "saturated" by values 0 and 1, any value outside this range
     * // will be substituted by its nearest border
     * @endcode
     */
    void threshold(const std::string& type,
                   T a,
                   T b,
                   MultidimArray<int> * mask = NULL )
    {
        int mode;

        if (type == "abs_above")
            mode = 1;
        else if (type == "abs_below")
            mode = 2;
        else if (type == "above")
            mode = 3;
        else if (type == "below")
            mode = 4;
        else if (type == "range")
            mode = 5;
        else
            REPORT_ERROR(1005,
                         static_cast< std::string >("Threshold: mode not supported (" +
                                                    type + ")"));

        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        {
            if (mask == NULL || DIRECT_MULTIDIM_ELEM(*mask,n) > 0 )
            {
                switch (mode)
                {
                case 1:
                    if (ABS(*ptr) > a)
                        *ptr = SGN(*ptr) * b;
                    break;
                case 2:
                    if (ABS(*ptr) < a)
                        *ptr = SGN(*ptr) * b;
                    break;
                case 3:
                    if (*ptr > a)
                        *ptr = b;
                    break;
                case 4:
                    if (*ptr < a)
                        *ptr = b;
                    break;
                case 5:
                    if (*ptr < a)
                        *ptr = a;
                    else if (*ptr > b)
                        *ptr = b;
                    break;
                }
            }
        }
    }

    /** Count with threshold.
     * @ingroup MultidimUtilities
     *
     * This function returns the number of elements meeting the threshold
     * condition.
     */
    unsigned long countThreshold(const std::string& type,
                                 T a,
                                 T b,
                                 MultidimArray<int> * mask = NULL )
    {
        int mode;

        if (type == "abs_above")
            mode = 1;
        else if (type == "abs_below")
            mode = 2;
        else if (type == "above")
            mode = 3;
        else if (type == "below")
            mode = 4;
        else if (type == "range")
            mode = 5;
        else
            REPORT_ERROR(1005,
                         static_cast< std::string >("CountThreshold: mode not supported (" +
                                                    type + ")"));

        unsigned long ret = 0;

        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        if (mask == NULL || DIRECT_MULTIDIM_ELEM(*mask,n) > 0 )
        {
            switch (mode)
            {
            case 1:
                if (ABS(*ptr) > a)
                    ret++;
                break;
            case 2:
                if (ABS(*ptr) < a)
                    ret++;
                break;
            case 3:
                if (*ptr > a)
                    ret++;
                break;
            case 4:
                if (*ptr < a)
                    ret++;
                break;
            case 5:
                if (*ptr >= a && *ptr <= b)
                    ret++;
                break;
            }
        }
        return ret;
    }

    /** Substitute a value by another.
     * @ingroup MultidimUtilities
     *
     * Substitute an old value by a new one. The accuracy is used to say if
     * the value in the array is equal to the old value. Set it to 0 for
     * perfect accuracy.
     */
    void substitute(T oldv,
                    T newv,
                    double accuracy = XMIPP_EQUAL_ACCURACY,
                    MultidimArray<int> * mask = NULL )
    {
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        if (mask == NULL || DIRECT_MULTIDIM_ELEM(*mask,n) > 0 )
            if (ABS(*ptr - oldv) <= accuracy)
                *ptr = newv;

    }

    /** Substitute a given value by a sample from a Gaussian distribution.
     * @ingroup MultidimUtilities
     *
     * Substitute  a given value by a sample from a Gaussian distribution.
     * The accuracy is used to say if the value in the array is equal
     * to the old value.  Set it to 0 for perfect accuracy.
     */
    void randomSubstitute(T oldv,
                          T avgv,
                          T sigv,
                          double accuracy = XMIPP_EQUAL_ACCURACY,
                          MultidimArray<int> * mask = NULL )
    {
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        if (mask == NULL || DIRECT_MULTIDIM_ELEM(*mask,n) > 0 )
            if (ABS(*ptr - oldv) <= accuracy)
                *ptr = rnd_gaus(avgv, sigv);

    }

    /** Binarize.
     * @ingroup MultidimUtilities
     *
     * This functions substitutes all values in a volume which are greater
     * than val+accuracy by 1 and the rest are set to 0. Use threshold to get a
     * very powerful binarization.
     */
    void binarize(double val = 0,
                  double accuracy = XMIPP_EQUAL_ACCURACY,
                  MultidimArray<int> * mask = NULL )
    {
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        if (mask == NULL || DIRECT_MULTIDIM_ELEM(*mask,n) > 0 )
            if (*ptr <= val + accuracy)
                *ptr = 0;
            else
                *ptr = 1;

    }

    /** ROUND
     * @ingroup MultidimUtilities
     *
     * Applies a ROUND (look for the nearest integer) to each array element.
     */
    void selfROUND()
    {
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        *ptr = ROUND(*ptr);
    }

    /** CEILING
     * @ingroup MultidimUtilities
     *
     * Applies a CEILING (look for the nearest larger integer) to each
     * array element.
     */
    void selfCEIL()
    {
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        *ptr = CEIL(*ptr);
    }

    /** FLOOR
     * @ingroup MultidimUtilities
     *
     * Applies a FLOOR (look for the nearest larger integer) to each
     * array element.
     */
    void selfFLOOR()
    {
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        *ptr = FLOOR(*ptr);
    }

    /** ABS
     * @ingroup MultidimUtilities
     *
     * Applies an ABS (absolute value) to each array element.
     */
    void selfABS()
    {
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        *ptr = ABS(*ptr);
    }

    /** MAX
     * @ingroup MultidimUtilities
     *
     * Each component of the result is the maximum of the correspoing
     * components of the two input arrays. They must have the same shape, if
     * not an exception is thrown
     */
    friend void MAX(const MultidimArray<T>& v1, const MultidimArray<T>& v2,
                    MultidimArray<T>& result)
    {
        if (!v1.sameShape(v2))
            REPORT_ERROR(1007, "MAX: arrays of different shape");

        result.resize(v1);
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(result)
        DIRECT_MULTIDIM_ELEM(result,n) = XMIPP_MAX(
                                             DIRECT_MULTIDIM_ELEM(v1,n),
                                             DIRECT_MULTIDIM_ELEM(v2,n));
    }

    /** MIN
     * @ingroup MultidimUtilities
     *
     * Each component of the result is the minimum of the correspoing
     * components of the two input arrays. They must have the same shape, if
     * not an exception is thrown
     */
    friend void MIN(const MultidimArray<T>& v1, const MultidimArray<T>& v2,
                    MultidimArray<T>& result)
    {
        if (!v1.sameShape(v2))
            REPORT_ERROR(1007, "MIN: arrays of different shape");

        result.resize(v1);
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(result)
        DIRECT_MULTIDIM_ELEM(result,n) = XMIPP_MIN(
                                             DIRECT_MULTIDIM_ELEM(v1,n),
                                             DIRECT_MULTIDIM_ELEM(v2,n));
    }

    /** Sqrt.
     * @ingroup MultidimUtilities
     *
     * Each component of the result is the square root of the original
     * component.
     */
    void selfSQRT()
    {
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        *ptr = static_cast< T >(sqrt(static_cast< double >(*ptr)));
    }

    /** Sum of matrix values.
     * @ingroup MultidimUtilities
     *
     * This function returns the sum of all internal values.
     *
     * @code
     * double sum = m.sum();
     * @endcode
     */
    double sum() const
    {
        double sum = 0;
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        sum += *ptr;
        return sum;
    }

    /** Sum of squared vector values.
     * @ingroup MultidimUtilities
     *
     * This function returns the sum of all internal values to the second
     * power.
     *
     * @code
     * double sum2 = m.sum2();
     * @endcode
     */
    double sum2() const
    {
        double sum = 0;
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        sum += *ptr * *ptr;
        return sum;
    }

    /** Log10.
     * @ingroup MultidimUtilities
     *
     * Each component of the result is the log10 of the original components.
     */
    void selfLog10()
    {
        T* ptr=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(*this,n,ptr)
        *ptr = static_cast< T >(log10(static_cast< double >(*ptr)));
    }

    /** Reverse matrix values over X axis, keep in this object.
     * @ingroup Utilites
     *
     * Maybe better with an example:
     *
     * @code
     * slice 0
     * [01 02 03          [07 08 09
     *  04 05 06           04 05 06
     *  07 08 09]          01 02 03]
     *
     * ----->
     *
     * slice 1
     * [11 12 13          [17 18 19
     *  14 15 16           14 15 16
     *  17 18 19]          11 12 13]
     * @endcode
     *
     */
    void selfReverseX()
    {
        for (int l = 0; l < NSIZE(*this); l++)
            for (int k = 0; k < ZSIZE(*this); k++)
                for (int i = 0; i < YSIZE(*this); i++)
                    for (int j = 0; j <= (int)(XSIZE(*this) - 1) / 2; j++)
                    {
                        T aux;
                        SWAP(DIRECT_NZYX_ELEM(*this, l, k, i, j),
                             DIRECT_NZYX_ELEM(*this, l, k, i, XSIZE(*this) - 1 - j),
                             aux);
                    }

        STARTINGX(*this) = -FINISHINGX(*this);
    }

    /** Reverse matrix values over Y axis, keep in this object.
     * @ingroup Utilites
     *
     * Maybe better with an example:
     *
     * @code
     * slice 0
     * [01 02 03          [03 02 01
     *  04 05 06           06 05 04
     *  07 08 09]          09 08 07]
     *
     * ----->
     *
     * slice 1
     * [11 12 13          [13 12 11
     *  14 15 16           16 15 14
     *  17 18 19]          19 18 17]
     * @endcode
     *
     */
    void selfReverseY()
    {
        for (int l = 0; l < NSIZE(*this); l++)
            for (int k = 0; k < ZSIZE(*this); k++)
                for (int i = 0; i <= (int)(YSIZE(*this) - 1) / 2; i++)
                    for (int j = 0; j < XSIZE(*this); j++)
                    {
                        T aux;
                        SWAP(DIRECT_NZYX_ELEM(*this, l, k, i, j),
                             DIRECT_NZYX_ELEM(*this, l, k, YSIZE(*this) - 1 - i, j),
                             aux);
                    }

        STARTINGY(*this) = -FINISHINGY(*this);
    }

    /** Reverse matrix values over Z axis, keep in this object.
     * @ingroup Utilites
     *
     * Maybe better with an example:
     *
     * @code
     * slice 0
     * [01 02 03          [11 12 13
     *  04 05 06           14 15 16
     *  07 08 09]          17 18 19]
     *
     *  ----->
     *
     * slice 1
     * [11 12 13          [01 02 03
     *  14 15 16           04 05 06
     *  17 18 19]          07 08 09]
     * @endcode
     *
     */
    void selfReverseZ()
    {

        for (int l = 0; l < NSIZE(*this); l++)
            for (int k = 0; k <= (int)(ZSIZE(*this) - 1) / 2; k++)
                for (int i = 0; i < YSIZE(*this); i++)
                    for (int j = 0; j < XSIZE(*this); j++)
                    {
                        T aux;
                        SWAP(DIRECT_NZYX_ELEM(*this, l, k, i, j),
                             DIRECT_NZYX_ELEM(*this, l, ZSIZE(*this) - 1 - k, i, j),
                             aux);
                    }

        STARTINGZ(*this) = -FINISHINGZ(*this);
    }


    /// @defgroup Operators Operators
    /// @ingroup MultidimensionalArrays

    /** Assignment.
     * @ingroup Operators
     *
     * You can build as complex assignment expressions as you like. Multiple
     * assignment is allowed.
     *
     * @code
     * v1 = v2 + v3;
     * v1 = v2 = v3;
     * @endcode
     *
     * This function is ported to Python as assign.
     */
    MultidimArray<T>& operator=(const MultidimArray<T>& op1)
    {
        if (&op1 != this)
        {
            resize(op1);
            T *ptr;
            unsigned long int n;
            FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(op1,n,ptr)
            DIRECT_MULTIDIM_ELEM(*this,n) = *ptr;
        }
        return *this;
    }

    /** Unary minus.
     * @ingroup Operators
     *
     * It is used to build arithmetic expressions. You can make a minus
     * of anything as long as it is correct semantically.
     *
     * @code
     * v1 = -v2;
     * v1 = -v2.transpose();
     * @endcode
     */
    MultidimArray<T> operator-() const
    {
        MultidimArray<T> tmp(*this);
        T* ptr;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(tmp,n,ptr)
        *ptr = -(*ptr);
        return tmp;
    }

    /** Input from input stream.
     * @ingroup Operators
     *
     * Actual size of the array is used to know how many values must be read.
     *
     * @code
     * v.<3);
     * std::cin >> v;
     * @endcode
     *
     * This function is not ported to Python.
     */
    // This function must be explictly implemented outside
    friend std::istream& operator>>(std::istream& in, MultidimArray<T>& v)
    {
        T* ptr;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(v,n,ptr)
        in >> *ptr;
        return in;
    }

    /** Equality.
     * @ingroup Operators
     *
     * Returns true if this object has got the same shape (origin and size)
     * than the argument and the same values (within accuracy).
     */
    bool equal(const MultidimArray<T>& op,
               double accuracy = XMIPP_EQUAL_ACCURACY) const
    {
        if (!sameShape(op) || data==NULL || op.data == NULL)
            return false;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY(*this)
        if (ABS(DIRECT_MULTIDIM_ELEM(*this,n) -
                DIRECT_MULTIDIM_ELEM(op,n)) > accuracy)
            return false;
        return true;
    }


    /** Extracts the 1D profile between two points in a 2D array
     * @ingroup MultidimUtilities
     *
     * Given two logical indexes, this function returns samples of the line that
     * joins them. This is done by bilinear interpolation. The number of samples
     * in the line is N.
     */
    void profile(int x0, int y0, int xF, int yF, int N,
                 MultidimArray< double >& profile) const
    {
        checkDimension(2);
        profile.initZeros(N);
        double tx_step = (double)(xF - x0) / (N - 1);
        double ty_step = (double)(yF - y0) / (N - 1);
        double tx = x0, ty = y0;

        for (int i = 0; i < N; i++)
        {
            profile(i) = interpolatedElement2D(tx, ty);
            tx += tx_step;
            ty += ty_step;
        }
    }

    /** Show using gnuplot
     * @ingroup MultidimArrayUtilities
     *
     * This function uses gnuplot to plot this vector. You must supply the
     * xlabel, ylabel, and title.
     */
    void showWithGnuPlot(const std::string& xlabel, const std::string& title)
    {
        checkDimension(1);

        FileName fn_tmp;
        fn_tmp.init_random(10);
        MultidimArray<T>::write(static_cast<std::string>("PPP") +
                                fn_tmp + ".txt");

        std::ofstream fh_gplot;
        fh_gplot.open((static_cast<std::string>("PPP") + fn_tmp +
                       ".gpl").c_str());
        if (!fh_gplot)
            REPORT_ERROR(1,
                         static_cast<std::string>("vector::showWithGnuPlot: Cannot open PPP")
                         + fn_tmp + ".gpl for output");
        fh_gplot << "set xlabel \"" + xlabel + "\"\n";
        fh_gplot << "plot \"PPP" + fn_tmp + ".txt\" title \"" + title +
        "\" w l\n";
        fh_gplot << "pause 300 \"\"\n";
        fh_gplot.close();
        system((static_cast<std::string>("(gnuplot PPP") + fn_tmp +
                ".gpl; rm PPP" + fn_tmp + ".txt PPP" + fn_tmp + ".gpl) &").c_str());
    }

    /** Edit with xmipp_editor.
     * @ingroup MultidimUtilities
     *
     *
     * This function generates a random filename starting with PPP and
     * edits it with xmipp_editor. After closing the editor the file is
     * removed.
     */
    // FIXME This is not good practice.. (system)
    void edit()
    {
        FileName nam;
        nam.init_random(15);

        nam = static_cast< std::string >("PPP" + nam + ".txt");
        write(nam);

        system((static_cast< std::string >("xmipp_edit -i " + nam +
                                           " -remove &").c_str()));
    }

    /** Write to an ASCII file.
     * @ingroup Operators
     */
    void write(const FileName& fn) const
    {
        std::ofstream out;
        out.open(fn.c_str(), std::ios::out);
        if (!out)
            REPORT_ERROR(1,
                         static_cast< std::string >("MultidimArray::write: File " + fn
                                                    + " cannot be opened for output"));

        out << *this;
        out.close();
    }

    ////////////// VECTORS
    /// @defgroup VectorsUtilities
    /// @ingroup Vectors


};

/// @defgroup MultidimFunctions Functions for all multidimensional arrays
/// @ingroup MultidimensionalArrays

/** Conversion from one type to another.
 * @ingroup MultidimFunctions
 *
 * If we have an integer array and we need a double one, we can use this
 * function. The conversion is done through a type casting of each element
 * If n >= 0, only the nth volumes will be converted, otherwise all NSIZE volumes
 */
template<typename T1, typename T2>
void typeCast(const MultidimArray<T1>& v1,  MultidimArray<T2>& v2, long n = -1)
{
    if (NZYXSIZE(v1) == 0)
    {
        v2.clear();
        return;
    }

    if (n < 0)
    {
        v2.resize(v1);
        T1* ptr1=NULL;
        unsigned long int n;
        FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(v1,n,ptr1)
        DIRECT_MULTIDIM_ELEM(v2,n) = static_cast< T2 >(*ptr1);
    }
    else
    {
        v2.resize(ZSIZE(v1),YSIZE(v1),XSIZE(v1));
        FOR_ALL_DIRECT_ELEMENTS_IN_ARRAY3D(v2)
        DIRECT_A3D_ELEM(v2,k,i,j) = static_cast< T2 >DIRECT_NZYX_ELEM(v1,n,k,i,j);
    }

}

template <typename T>
void coreArrayByArray(const MultidimArray<T>& op1,
                      const MultidimArray<T>& op2, MultidimArray<T>& result,
                      char operation)
{
    T* ptrResult=NULL;
    T* ptrOp1=NULL;
    T* ptrOp2=NULL;
    unsigned long int n;
    for (n=0, ptrResult=result.data, ptrOp1=op1.data,ptrOp2=op2.data;
         n<op1.zyxdim; ++n, ++ptrResult, ++ptrOp1, ++ptrOp2)
        switch (operation)
        {
        case '+':
            *ptrResult = *ptrOp1 + *ptrOp2;
            break;
        case '-':
            *ptrResult = *ptrOp1 - *ptrOp2;
            break;
        case '*':
            *ptrResult = *ptrOp1 * *ptrOp2;
            break;
        case '/':
            *ptrResult = *ptrOp1 / *ptrOp2;
            break;
        }
}

template <typename T>
void coreArrayByScalar(const MultidimArray<T>& op1,
                       const T& op2,
                       MultidimArray<T>& result,
                       char operation)
{
    T* ptrResult=NULL;
    T* ptrOp1=NULL;
    unsigned long int n;
    for (n=0, ptrResult=result.data, ptrOp1=op1.data;
         n<op1.zyxdim; ++n, ++ptrResult, ++ptrOp1)
        switch (operation)
        {
        case '+':
            *ptrResult = *ptrOp1 + op2;
            break;
        case '-':
            *ptrResult = *ptrOp1 - op2;
            break;
        case '*':
            *ptrResult = *ptrOp1 * op2;
            break;
        case '/':
            *ptrResult = *ptrOp1 / op2;
            break;
        }
}

template <typename T>
void coreScalarByArray(const T& op1,
                       const MultidimArray<T>& op2,
                       MultidimArray<T>& result,
                       char operation)
{
    T* ptrResult=NULL;
    T* ptrOp2=NULL;
    unsigned long int n;
    for (n=0, ptrResult=result.data, ptrOp2=op2.data;
         n<op2.zyxdim; ++n, ++ptrResult, ++ptrOp2)
        switch (operation)
        {
        case '+':
            *ptrResult = op1 + *ptrOp2;
            break;
        case '-':
            *ptrResult = op1 - *ptrOp2;
            break;
        case '*':
            *ptrResult = op1 * *ptrOp2;
            break;
        case '/':
            *ptrResult = op1 / *ptrOp2;
            break;
        }
}

/** MultidimArray equality.
 * @ingroup MultidimFunctions */
template<typename T>
bool operator==(const MultidimArray<T>& op1, const MultidimArray<T>& op2)
{
    return op1.equal(op2);
}

/** MultidimArray inequality.
 * @ingroup MultidimFunctions */
template<typename T>
bool operator!=(const MultidimArray<T>& op1, const MultidimArray<T>& op2)
{
    return !(op1==op2);
}

/** Reduce both volumes to a common size.
 * @ingroup MultidimFunctions
 *
 * Search the range of logical indexes for which both volumes have got valid
 * values, and cut both to that size, the corresponding origin is automatically
 * computed.
 *
 * @code
 * MultidimArray< double > V1(4, 5, 3);
 * V1.startingX() = -2;
 * V1.startingY() = -2;
 * V1.startingZ() = -2;
 *
 * MultidimArray< double > V2(4, 2, 3);
 * V2.startingX() = 0;
 * V2.startingY() = 0;
 * V2.startingZ() = 0;
 *
 * // V1 and V2 range from (0,0,0)=(z,y,x) to (1,1,0)
 * cutToCommonSize(V1, V2);
 * @endcode
 */
template<typename T>
void cutToCommonSize(MultidimArray<T>& V1, MultidimArray<T>& V2)
{
    int z0 = XMIPP_MAX(STARTINGZ(V1), STARTINGZ(V2));
    int zF = XMIPP_MIN(FINISHINGZ(V1), FINISHINGZ(V2));
    int y0 = XMIPP_MAX(STARTINGY(V1), STARTINGY(V2));
    int yF = XMIPP_MIN(FINISHINGY(V1), FINISHINGY(V2));
    int x0 = XMIPP_MAX(STARTINGX(V1), STARTINGX(V2));
    int xF = XMIPP_MIN(FINISHINGX(V1), FINISHINGX(V2));

    V1.window(z0, y0, x0, zF, yF, xF);
    V2.window(z0, y0, x0, zF, yF, xF);
}



template<typename T>
std::ostream& operator<<(std::ostream& ostrm, const MultidimArray<T>& v)
{
    if (v.xdim == 0)
        ostrm << "NULL Array\n";
    else
        ostrm << std::endl;

    double max_val = ABS(DIRECT_A3D_ELEM(v , 0, 0, 0));

    T* ptr;
    unsigned long int n;
    FOR_ALL_DIRECT_ELEMENTS_IN_MULTIDIMARRAY_ptr(v,n,ptr)
    max_val = XMIPP_MAX(max_val, ABS(*ptr));

    int prec = bestPrecision(max_val, 10);

    if (YSIZE(v)==1 && ZSIZE(v)==1)
    {
        for (int j = STARTINGX(v); j <= FINISHINGX(v); j++)
            ostrm << floatToString((double) A3D_ELEM(v, 0, 0, j), 10, prec)
            << std::endl;
    }
    else
    {
        for (int l = 0; l < NSIZE(v); l++)
        {
            if (NSIZE(v)>1)
                ostrm << "Image No. " << l << std::endl;
            for (int k = STARTINGZ(v); k <= FINISHINGZ(v); k++)
            {
                if (ZSIZE(v)>1)
                    ostrm << "Slice No. " << k << std::endl;
                for (int i = STARTINGY(v); i <= FINISHINGY(v); i++)
                {
                    for (int j = STARTINGX(v); j <= FINISHINGX(v); j++)
                    {
                        ostrm << floatToString((double) A3D_ELEM(v, k, i, j), 10, prec) << ' ';
                    }
                    ostrm << std::endl;
                }
            }
        }
    }

    return ostrm;
}

// Specializations cases for complex numbers
template<>
std::ostream& operator<<(std::ostream& ostrm, const MultidimArray< std::complex<double> >& v);


#endif
