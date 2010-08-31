/***************************************************************************
 *
 * Authors: Carlos Oscar (coss@cnb.csic.es)
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

#include "micrograph.h"
#include "args.h"
#include "metadata.h"
#include "mask.h"
#include "geometry.h"

#include <fstream>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#ifdef LINUX
#include <unistd.h>
#endif

/* Clear ------------------------------------------------------------------- */
void Micrograph::clear()
{
    single_particle.clear();
    coords.clear();
    fn_coords = fn_micrograph = "";
    X_window_size = Y_window_size = -1;
    fh_micrograph = -1;
    Xdim = Ydim = -1;
    __depth = -1;
    m8 = NULL;
    m16 = NULL;
    um16 = NULL;
    m32 = NULL;
    compute_transmitance = false;
    compute_inverse = false;
    __scaling_valid = false;
    /* __in_core=FALSE;*/
}

/* Open micrograph --------------------------------------------------------- */
void Micrograph::open_micrograph(const FileName &_fn_micrograph,
                                 /*bool in_core,*/ bool reversed)
{
    struct stat info;

    // Micrograph name
    fn_micrograph = _fn_micrograph;

    // Look for micrograph dimensions
    // check if the file format is spider
    ///// FIXME READING SPIDER FORMAT!!!!
    /*
    if (Is_ImageXmipp(fn_micrograph))
{
        headerXmipp     header;
        header.read(fn_micrograph);
        float fXdim, fYdim;
        header.getDimensionension(fYdim, fXdim);
        Xdim = (int) fXdim;
        Ydim = (int)fYdim;
        __offset = header.get_header_size();
        __depth = 32;
        reversed = header.reversed();
}
    else
{
    */
    fn_inf = fn_micrograph.add_extension("inf");
    FILE *fh_inf = fopen(fn_inf.c_str(), "r");
    if (!fh_inf)
        REPORT_ERROR(ERR_IO_NOTOPEN, (std::string)"Micrograph::open_micrograph: Cannot find " +
                     fn_inf);
    Xdim = textToInteger(getParameter(fh_inf, "Xdim"));
    Ydim = textToInteger(getParameter(fh_inf, "Ydim"));
    __depth = textToInteger(getParameter(fh_inf, "bitspersample"));
    if (checkParameter(fh_inf, "offset"))
        __offset = textToInteger(getParameter(fh_inf, "offset"));
    else
        __offset = 0;
    if (checkParameter(fh_inf, "is_signed"))
        __is_signed = (getParameter(fh_inf, "is_signed") == "true" ||
                       getParameter(fh_inf, "is_signed") == "TRUE");
    else
        __is_signed = false;
    fclose(fh_inf);
    /*
}
    */

    // Open micrograph and map
    fh_micrograph = open(fn_micrograph.c_str(), O_RDWR, S_IREAD | S_IWRITE);
    if (fh_micrograph == -1)
        REPORT_ERROR(ERR_IO_NOTOPEN, (std::string)"Micrograph::open_micrograph: There is a "
                     "problem opening " + fn_micrograph +
                     "\nCheck that the file has write permission");
    char *aux_ptr;
    switch (__depth)
    {
    case 8:
        /* if (!in_core) { */
        m8 = (unsigned char *) mmap(0, (__depth / 8) * Ydim * Xdim + __offset,
                                    PROT_READ | PROT_WRITE, MAP_SHARED, fh_micrograph, 0);
        if (m8 == MAP_FAILED)
            REPORT_ERROR(ERR_MMAP_NOTADDR, (std::string)"Micrograph::open_micrograph: cannot map " +
                         _fn_micrograph + " in memory");
        m8 += __offset;
        /* } else {
           m8=new unsigned char (Ydim*Xdim*__depth/8);
           int length=Ydim*Xdim*__depth/8;
           int read_length=read(fh_micrograph,m8,length);
           std::cout << SSIZE_MAX << std::endl;
           std::cout << read_length << std::endl;
           if (read_length!=length)
              REPORT_ERROR(1,(std::string)"Micrograph::open_micrograph: cannot read "+
                 _fn_micrograph+" in memory");
    } */
        break;
    case 16:
        /* if (!in_core) { */
        if (__is_signed)
        {
            m16 = (short int *) mmap(0, (__depth / 8) * Ydim * Xdim + __offset,
                                     PROT_READ | PROT_WRITE, MAP_SHARED, fh_micrograph, 0);
            if (m16 == MAP_FAILED)
            {
                /*
                switch (errno) {
                case EACCES:    std::cout << "EACCES:   \n"; break;
                  case EAGAIN:    std::cout << "EAGAIN:   \n"; break;
                case EBADF:     std::cout << "EBADF:    \n"; break;
                case EINVAL:    std::cout << "EINVAL:   \n"; break;
                case EMFILE:    std::cout << "EMFILE:   \n"; break;
                case ENODEV:    std::cout << "ENODEV:   \n"; break;
                case ENOMEM:    std::cout << "ENOMEM:   \n"; break;
                case ENOTSUP:   std::cout << "ENOTSUP:  \n"; break;
                case ENXIO:     std::cout << "ENXIO:    \n"; break;
                case EOVERFLOW: std::cout << "EOVERFLOW:\n"; break;
            }
                */
                REPORT_ERROR(ERR_MMAP_NOTADDR, (std::string)"Micrograph::open_micrograph: cannot map " +
                             _fn_micrograph + " in memory");
            }
            aux_ptr = (char *)m16;

            aux_ptr += __offset;
            m16 = (short int *) aux_ptr;
        }
        else
        {
            um16 = (unsigned short int *) mmap(0, (__depth / 8) * Ydim * Xdim + __offset,
                                               PROT_READ | PROT_WRITE, MAP_SHARED, fh_micrograph, 0);
            if (um16 == MAP_FAILED)
            {
                /*
                switch (errno) {
                case EACCES:    std::cout << "EACCES:   \n"; break;
                  case EAGAIN:    std::cout << "EAGAIN:   \n"; break;
                case EBADF:     std::cout << "EBADF:    \n"; break;
                case EINVAL:    std::cout << "EINVAL:   \n"; break;
                case EMFILE:    std::cout << "EMFILE:   \n"; break;
                case ENODEV:    std::cout << "ENODEV:   \n"; break;
                case ENOMEM:    std::cout << "ENOMEM:   \n"; break;
                case ENOTSUP:   std::cout << "ENOTSUP:  \n"; break;
                case ENXIO:     std::cout << "ENXIO:    \n"; break;
                case EOVERFLOW: std::cout << "EOVERFLOW:\n"; break;
            }
                */
                REPORT_ERROR(ERR_MMAP_NOTADDR, (std::string)"Micrograph::open_micrograph: cannot map " +
                             _fn_micrograph + " in memory");
            }
            aux_ptr = (char *)um16;
            aux_ptr += __offset;
            um16 = (unsigned short int *) aux_ptr;
        }
        /* } else {
           m16=new short int (Ydim*Xdim*__depth/8);
           int length=Ydim*Xdim*__depth/8;
           if (read(fh_micrograph,m16,length)!=length)
              REPORT_ERROR(1,(std::string)"Micrograph::open_micrograph: cannot read "+
                 _fn_micrograph+" in memory");
    } */
        break;
    case 32:
        // Map file in memory
        m32 = (float*) mmap(0, (__depth / 8) * Ydim * Xdim + __offset,
                            PROT_READ | PROT_WRITE, MAP_SHARED, fh_micrograph, 0);
        if (m32 == MAP_FAILED)
            REPORT_ERROR(ERR_MMAP_NOTADDR, (std::string)"Micrograph::open_micrograph: cannot map " +
                         _fn_micrograph + " in memory");
        aux_ptr = (char *)m32;
        aux_ptr += __offset;
        m32 = (float *) aux_ptr;
        break;
    default:
        REPORT_ERROR(ERR_TYPE_INCORRECT, "Micrograph::open_micrograph: depth is not 8, 16 nor 32");
    }
    /*__in_core=in_core; */
    __reversed = reversed;
}

/* Close micrograph -------------------------------------------------------- */
void Micrograph::close_micrograph()
{
    if (fh_micrograph != -1)
    {
        close(fh_micrograph);
        /* if (!__in_core) { */
        if (__depth == 8)
        {
            m8 -= __offset;
            munmap((char *)m8, Ydim*Xdim*__depth / 8 + __offset);
        }
        else if (__depth == 16)
        {
            if (__is_signed)
            {
                char *aux_ptr = (char *)m16;
                aux_ptr -= __offset;
                m16 = (short int *)aux_ptr;
                munmap((char *)m16, Ydim*Xdim*__depth / 8 + __offset);
            }
            else
            {
                char *aux_ptr = (char *)um16;
                aux_ptr -= __offset;
                um16 = (unsigned short int *)aux_ptr;
                munmap((char *)um16, Ydim*Xdim*__depth / 8 + __offset);
            }
        }
        else if (__depth == 32)
        {
            char *aux_ptr = (char *)m32;
            aux_ptr -= __offset;
            m32 = (float *)aux_ptr;
            munmap((char *)m32, Ydim*Xdim*__depth / 8 + __offset);
        }
        /* } else {
           if      (__depth== 8) delete m8;
           else if (__depth==16) delete m16;
    } */
    }
}

/* Compute 8 bit scaling --------------------------------------------------- */
//#define DEBUG
void Micrograph::compute_8_bit_scaling()
{
    std::cerr << "Computing 8 bit scaling ...\n";

    // Compute minimum and maximum value
    float minval, maxval;
    minval = maxval = (*this)(0, 0);
    for (int i = 0; i < Ydim; i++)
    {
        for (int j = 0; j < Xdim; j++)
        {
            float tmp = (*this)(j, i);
            if (tmp < minval)
                minval = tmp;
            else if (tmp > maxval)
                maxval = tmp;
            /*
            if(maxval > 32000)
              std::cout << "(i,j) max min valuefloat value" << i << " " << j
                 << " " << maxval << " " << minval << " "<< tmp
                 << " " << (*this)(j,i) << std::endl;
            */
        }
    }
#ifdef DEBUG
    std::cout << "minval=" << minval << " maxval=" << maxval << std::endl;
#endif

    // Compute output range
    float minF, maxF;
    if (maxval - minval < 32)
    {
        minF = 0;
        maxF = 255;
    }
    else if (minval < 0)
    {
        minF = 0;
        maxF = XMIPP_MIN(255, maxval - minval);
    }
    else if (maxval > 255)
    {
        minF = XMIPP_MAX(0, minval - (maxval - 255));
        maxF = 255;
    }
    else
    {
        minF = minval;
        maxF = maxval;
    }
#ifdef DEBUG
    std::cout << "minF=" << minF << " maxF=" << maxF << std::endl;
#endif

    // Compute scaling
    __a = (maxF - minF) / (maxval - minval);
    __b = minF - __a * minval;
    __scaling_valid = true;
#ifdef DEBUG

    std::cerr <<  "__a  " << __a  << "__b" << __b << std::endl;
#endif
}
#undef DEBUG

/* Write as 8 bits --------------------------------------------------------- */
void Micrograph::write_as_8_bits(const FileName &fn8bits)
{
    if (!__scaling_valid)
        compute_8_bit_scaling();

    // Create empty output file
    create_empty_file(fn8bits, ((unsigned long long)Ydim)*Xdim);

    std::ofstream fh8bits_inf;
    fh8bits_inf.open((fn8bits + ".inf").c_str());
    if (!fh8bits_inf)
        REPORT_ERROR(ERR_IO_NOTOPEN, (std::string)"write_as_8_bits: Cannot open " + fn8bits +
                     ".inf");
    fh8bits_inf << "# Generated by write_as_8_bits\n";
    fh8bits_inf << "# Original file: " << fn_micrograph << std::endl;
    fh8bits_inf << "# Image width\n";
    fh8bits_inf << "Xdim= " << Xdim << std::endl;
    fh8bits_inf << "# Image length\n";
    fh8bits_inf << "Ydim= " << Ydim << std::endl;
    fh8bits_inf << "# Pixel depth\n";
    fh8bits_inf << "bitspersample=8\n";
    fh8bits_inf.close();

    // Open micrograph
    Micrograph Mp;
    Mp.open_micrograph(fn8bits, false);
    for (int y=0; y<Ydim; y++)
        for (int x=0; x<Xdim; x++)
            Mp.set_val(x,y,val8(x,y));
    Mp.close_micrograph();
}

/* Save coordinates to disk ------------------------------------------------ */
void Micrograph::write_coordinates(int label, const FileName &_fn_coords)
{
    std::ofstream fh;
    if (_fn_coords != "")
        fn_coords = _fn_coords;

    MetaData MD;
    MD.setComment((std::string)"Selected Coordinates for file " + fn_coords);
    int imax = coords.size();
    int x,y;
    for (int i = 0; i < imax; i++)
    {
        MD.addObject();
        MD.setValue(MDL_XINT,coords[i].X);
        MD.setValue(MDL_YINT,coords[i].Y);
    }
    MD.write(fn_coords);
}

/* Read coordinates from disk ---------------------------------------------- */
void Micrograph::read_coordinates(int label, const FileName &_fn_coords)
{
    std::ifstream  fh;
    int            line_no = 0;
    std::string    line;

    fn_coords = _fn_coords;

    MetaData MD;
    MD.read(fn_coords);
    line_no = MD.size();

    // Resize coordinate list and read
    coords.reserve(line_no);
    struct Particle_coords aux;
    aux.valid = true;
    aux.label = label;
    aux.cost = 1;
    FOR_ALL_OBJECTS_IN_METADATA(MD)
    {
    	int x,y;
    	MD.getValue(MDL_XINT,aux.X); //aux.X=x;
    	MD.getValue(MDL_YINT,aux.Y); //aux.Y=y;
        coords.push_back(aux);
    }
}

/* Transform all coordinates ---------------------------------------------- */
void Micrograph::transform_coordinates(const Matrix2D<double> &M)
{
    Matrix1D<double> m(3);
    SPEED_UP_temps;

    int imax = coords.size();
    for (int i = 0; i < imax; i++)
    {
        if (coords[i].valid)
        {
            VECTOR_R3(m,coords[i].X, coords[i].Y,1);
            M3x3_BY_V3x1(m, M, m);
            coords[i].X=(int)XX(m);
            coords[i].Y=(int)YY(m);
        }
    }
}

/* Multiply coordinates by a constant -------------------------------------- */
void Micrograph::scale_coordinates(const double &c)
{
    int imax = coords.size();
    for (int i = 0; i < imax; i++)
    {
        if (coords[i].valid)
        {
            coords[i].X = (int)coords[i].X*c;
            coords[i].Y = (int)coords[i].Y*c;
        }
    }

}

/* Scissor ----------------------------------------------------------------- */
int Micrograph::scissor(const Particle_coords &P, Image<double> &result,
                        double Dmin, double Dmax, double scaleX, double scaleY,
                        bool only_check)
{
    if (X_window_size == -1 || Y_window_size == -1)
        REPORT_ERROR(ERR_VALUE_NOTSET, "Micrograph::scissor: window size not set");

    result().resize(Y_window_size, X_window_size);
    int i0 = ROUND(scaleY * P.Y) + FIRST_XMIPP_INDEX(Y_window_size);
    int iF = ROUND(scaleY * P.Y) + LAST_XMIPP_INDEX(Y_window_size);
    int j0 = ROUND(scaleX * P.X) + FIRST_XMIPP_INDEX(X_window_size);
    int jF = ROUND(scaleX * P.X) + LAST_XMIPP_INDEX(X_window_size);
    int retval = 1;
    double range, temp;
    range = Dmax - Dmin;
    if (i0 < 0 || iF >= Ydim || j0 < 0 || jF >= Xdim)
    {
        result().initZeros();
        retval = 0;
    }
    else
        if (!only_check)
        {
            for (int i = i0; i <= iF; i++)
                for (int j = j0; j <= jF; j++)
                {
                    if (compute_transmitance)
                    {
                        if ((*this)(j, i) < 1)
                            temp = (*this)(j, i);
                        else
                            temp = log10((double)(*this)(j, i));
                        if (compute_inverse)
                            A2D_ELEM(result(),i - i0, j - j0) = (Dmax - temp) / range;
                        else
                            A2D_ELEM(result(),i - i0, j - j0) = (temp - Dmin) / range;
                    }
                    else
                    {
                        if (compute_inverse)
                            A2D_ELEM(result(), i - i0, j - j0) = (Dmax - (*this)(j, i)) / range;
                        else
                            A2D_ELEM(result(), i - i0, j - j0) = (*this)(j, i);
                    }
                }
        }
    return retval;
}

/* Get linear transformation ----------------------------------------------- */
void Micrograph::getLinearTransformatioVal8(double &a, double &b) const
{
    a=__a;
    b=__b;
}

/* Produce all images ------------------------------------------------------ */
void Micrograph::produce_all_images(int label, const FileName &fn_root,
                                    int starting_index, const FileName &fn_image, double ang, double tilt,
                                    double psi)
{
    MetaData SF;
    FileName fn_out;
    Image<double> I;
    Micrograph *M;

    // Set Source image
    if (fn_image == "")
        M = this;
    else
    {
        M = new Micrograph;
        M->open_micrograph(fn_image, __reversed);
        M->set_window_size(X_window_size, Y_window_size);
        M->set_transmitance_flag(compute_transmitance);
        M->set_inverse_flag(compute_inverse);
    }

    // Set scale for particles
    int MXdim, MYdim, thisXdim, thisYdim;
    M->size(MXdim, MYdim);
    this->size(thisXdim, thisYdim);
    double scaleX = (double)MXdim / thisXdim;
    double scaleY = (double)MYdim / thisYdim;

    // Compute max and minimum if compute_transmitance
    // or compute_inverse flags are ON
    double Dmax, Dmin;
    if (compute_transmitance || compute_inverse)
    {
        (*this).computeDoubleMinMax(Dmin, Dmax);

        if (compute_transmitance)
        {
            if (Dmin > 1)
                Dmin = log10(Dmin);
            if (Dmax > 1)
                Dmax = log10(Dmax);
        }
    }
    // Scissor all particles
    if (ang != 0)
        std::cout << "Angle from Y axis to tilt axis " << ang << std::endl
        << "   applying apropriate rotation\n";
    int i = starting_index;
    int nmax = ParticleNo();
    for (int n = 0; n < nmax; n++)
        if (coords[n].valid && coords[n].label == label)
        {
            SF.addObject();
            fn_out.compose(fn_root, i++, "xmp");
            if (!M->scissor(coords[n], (Image<double> &) I, Dmin, Dmax, scaleX, scaleY))
            {
                std::cout << "Particle " << fn_out << " is very near the border, "
                << "corresponding image is set to blank\n";
                SF.setValue( MDL_IMAGE, fn_out);
                SF.setValue( MDL_ENABLED, 1);
            }
            else
            {
                SF.setValue( MDL_IMAGE, fn_out);
                SF.setValue( MDL_ENABLED, 1);
            }
            //  if (ang!=0) I().rotate(-ang);
            /// FIXME: HEADER MODIFICATION
            /*
            I.set_rot((float)ang);
            I.set_tilt((float)tilt);
            I.set_psi((float)psi);
            */
            I.write(fn_out);
        }
    if (labels[label] != "")
    {
        SF.write(fn_micrograph.remove_directories() + "." + labels[label] + ".sel");
        write_coordinates(label, fn_micrograph + "." + labels[label] + ".pos");
    }
    else
    {
        SF.write(fn_micrograph.remove_directories() + ".sel");
        write_coordinates(label, fn_micrograph + ".pos");
    }

    // Free source image??
    if (fn_image != "")
    {
        M->close_micrograph();
        delete M;
    }
}

/* Search coordinate near a position --------------------------------------- */
int Micrograph::search_coord_near(int x, int y, int prec) const
{
    int imax = coords.size();
    int prec2 = prec * prec;
    for (int i = 0; i < imax; i++)
        if ((coords[i].X - x)*(coords[i].X - x) + (coords[i].Y - y)*(coords[i].Y - y) < prec2
                && coords[i].valid)
            return i;
    return -1;
}

/* Invalidate a coordinate ------------------------------------------------- */
void Micrograph::invalidate_coord(int n)
{
    if (n < 0 || n >= ParticleNo())
        REPORT_ERROR(ERR_INDEX_OUTOFBOUNDS, "Micrograph::invalidate_coord: Index out of range");
    coords[n].valid = false;
}

/* Add coordinate ---------------------------------------------------------- */
int Micrograph::add_coord(int x, int y, int label, double cost)
{
    struct Particle_coords aux;
    aux.valid = true;
    aux.X = x;
    aux.Y = y;
    aux.label = label;
    aux.cost = cost;
    coords.push_back(aux);
    return coords.size() - 1;
}

/* Move last coordinate ---------------------------------------------------- */
void Micrograph::move_last_coord_to(int x, int y)
{
    if (coords.size() > 0)
    {
        coords.back().X = x;
        coords.back().Y = y;
    }
}

/* Downsample -------------------------------------------------------------- */
void downsample(const Micrograph &M, int Xstep, int Ystep,
                const MultidimArray<double> &kernel, Micrograph &Mp,
                bool do_fourier, int nThreads)
{
    // Find first and last indexes in each direction
    // it is assumed that (0,0) is within the kernel
    int Ydim, Xdim, Ypdim, Xpdim;
    M.size(Xdim, Ydim);
    Mp.size(Xpdim, Ypdim);
    int x0 = 0;
    int y0 = 0;
    int xF = Xdim;
    int yF = Ydim;

    double pixval;
    int ii, y, jj, x;
    time_config();

    // Look for input/output ranges
    double a = 1;
    double b = 0;
    double scale = 1;
    if (do_fourier)
    {
        std::cout << "Performing the Fourier downsampling\n";
        // Read the micrograph in memory as doubles
        MultidimArray<double> Mmem(Ydim,Xdim);
        MultidimArray<std::complex<double> > MmemFourier;
        FOR_ALL_ELEMENTS_IN_ARRAY2D(Mmem)
        Mmem(i,j)=(double)M(j,i);

        // Perform the Fourier transform
        FourierTransformer transformerM;
        transformerM.setThreadsNumber(nThreads);
        transformerM.FourierTransform(Mmem, MmemFourier, false);

        // Create space for the downsampled image and its Fourier transform
        MultidimArray<double> Mpmem(Ypdim,Xpdim);
        MultidimArray<std::complex<double> > MpmemFourier;
        FourierTransformer transformerMp;
        transformerMp.setThreadsNumber(nThreads);
        transformerMp.setReal(Mpmem);
        transformerMp.getFourierAlias(MpmemFourier);

        int ihalf=YSIZE(MpmemFourier)/2+1;
        for (int i=0; i<ihalf; i++)
            for (int j=0; j<XSIZE(MpmemFourier); j++)
                MpmemFourier(i,j)=MmemFourier(i,j);
        for (int i=ihalf; i<YSIZE(MpmemFourier); i++)
        {
            int ip=YSIZE(MmemFourier)-YSIZE(MpmemFourier)+i;
            for (int j=0; j<XSIZE(MpmemFourier); j++)
                MpmemFourier(i,j)=MmemFourier(ip,j);
        }

        // Transform data
        transformerMp.inverseFourierTransform();

        // Find minimun and range in output data
        double omin,omax;
        Mpmem.computeDoubleMinMax(omin,omax);
        double orange = omax - omin;
        a = (pow(2.0, Mp.depth()) - 1.0) / orange;
        b = -omin;

        // Copy back data
        FOR_ALL_ELEMENTS_IN_ARRAY2D(Mpmem)
        {
            pixval=Mpmem(i,j);
            if (Mp.depth() != 32)
                Mp.set_val(j, i, FLOOR(a*(pixval*scale + b)));
            else
                Mp.set_val(j, i, pixval);
        }
    }
    else
    {
        if (Mp.depth() != 32)
        {
            double imin, imax;
            double omin, omax;
            bool ifirst = true, ofirst = true;

            if (M.depth() != 32)
                scale = (pow(2.0, Mp.depth()) - 1.0) / (pow(2.0, M.depth()) - 1.0);
            else if (M.depth() == 32)
                scale = 1;
            if(do_fourier)
                init_progress_bar(yF / Ystep);
            for (ii = 0, y = y0; y < yF; y += Ystep, ii++)
            {
                for (jj = 0, x = x0; x < xF; x += Xstep, jj++)
                {
                    pixval = 0;
                    FOR_ALL_ELEMENTS_IN_ARRAY2D(kernel)
                    {
                        int j2 = intWRAP(j + x, 0, xF - 1);
                        int i2 = intWRAP(i + y, 0, yF - 1);
                        if (ifirst)
                        {
                            imin = imax = M(j2, i2);
                            ifirst = false;
                        }
                        else
                        {
                            imin = XMIPP_MIN(imin, M(j2, i2));
                            imax = XMIPP_MAX(imax, M(j2, i2));
                        }
                        pixval += kernel(i, j) * M(j2, i2);
                    }
                    pixval *= scale;
                    if (ii < Ypdim && jj < Xpdim)
                    {
                        if (ofirst)
                        {
                            omin = omax = pixval;
                            ofirst = false;
                        }
                        else
                        {
                            omin = XMIPP_MIN(omin, pixval);
                            omax = XMIPP_MAX(omax, pixval);
                        }
                    }
                }
                if (ii % 50 == 0)
                    progress_bar(ii);
            }
            progress_bar(yF / Ystep);

            // Compute range transformation
            double irange = imax - imin;
            double orange = omax - omin;

            if (M.depth() != 32)
            {
                a = scale * irange / orange;
                b = -omin;
            }
            else
            {
                a = (pow(2.0, Mp.depth()) - 1.0) / orange;
                scale = 1;
                b = -omin;
            }
        }

        // Really downsample
        init_progress_bar(yF / Ystep);
        for (ii = 0, y = y0; y < yF; y += Ystep, ii++)
        {
            for (jj = 0, x = x0; x < xF; x += Xstep, jj++)
            {
                pixval = 0;
                for (int i=STARTINGY(kernel); i<=FINISHINGY(kernel); i++)
                {
                    int i2 = intWRAP(i + y, 0, yF - 1);
                    for (int j=STARTINGX(kernel); j<=FINISHINGX(kernel); j++)
                    {
                        int j2 = intWRAP(j + x, 0, xF - 1);
                        pixval += kernel(i, j) * M(j2, i2);
                    }
                }

                if (ii < Ypdim && jj < Xpdim)
                    if (Mp.depth() != 32)
                        Mp.set_val(jj, ii, FLOOR(a*(pixval*scale + b)));
                    else
                        Mp.set_val(jj, ii, pixval);
            }
            if (ii % 50 == 0)
                progress_bar(ii);
        }
        progress_bar(yF / Ystep);
    }
}
