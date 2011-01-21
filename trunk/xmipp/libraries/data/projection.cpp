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

#include "projection.h"
#include "geometry.h"

#define x0   STARTINGX(IMGMATRIX(proj))
#define xF   FINISHINGX(IMGMATRIX(proj))
#define y0   STARTINGY(IMGMATRIX(proj))
#define yF   FINISHINGY(IMGMATRIX(proj))
#define xDim XSIZE(IMGMATRIX(proj))
#define yDim YSIZE(IMGMATRIX(proj))

// These two structures are needed when projecting and backprojecting using
// threads. They make mutual exclusion and synchronization possible.
barrier_t project_barrier;
pthread_mutex_t project_mutex = PTHREAD_MUTEX_INITIALIZER;
project_thread_params * project_threads;

/* Empty constructor ======================================================= */
Projection::Projection(): Image<double>()
{}

/* Reset =================================================================== */
void Projection::reset(int Ydim, int Xdim)
{
    data.initZeros(Ydim, Xdim);
    data.setXmippOrigin();
}

/* Set angles ============================================================== */
void Projection::set_angles(double _rot, double _tilt, double _psi)
{
    setEulerAngles(_rot, _tilt, _psi);
    Euler_angles2matrix(_rot, _tilt, _psi, euler);
    eulert = euler.transpose();
    euler.getRow(2, direction);
    direction.selfTranspose();
}

/* Read ==================================================================== */
void Projection::read(const FileName &fn, const bool &apply_shifts,
                      bool readdata , MDRow * row)
{
    Image<double>::read(fn, readdata, 0, false, apply_shifts, row);
    Euler_angles2matrix(rot(), tilt(), psi(), euler);
    eulert = euler.transpose();
    euler.getRow(2, direction);
    direction.selfTranspose();
}

/* Assignment ============================================================== */
Projection & Projection::operator = (const Projection &P)
{
    // Esto hay que ponerlo mas elegantemente accediendo al = del padre
    *(Image<double> *)this = * ((Image<double> *) & P);
    direction = P.direction;
    euler     = P.euler;
    eulert    = P.eulert;
    return *this;
}

/* Another function for assignment ========================================= */
void Projection::assign(const Projection &P)
{
    *this = P;
}

/* Read Projection Parameters ============================================== */
void ParametersProjectionTomography::read(const FileName &fn_proj_param)
{
    if (fn_proj_param.isMetaData())
    {
        //TODO Reading from Metadata file to be completed!!
        REPORT_ERROR(ERR_NOT_IMPLEMENTED,"Need to include new LABEL_VECTOR_STRING for MDL.. CUBANITO HELP!!.");

        //        MetaData MD;
        //        MD.read(fn_proj_param);
        //        MD.getValue(MDL_PRJ_VOL,fnPhantom);

    }
    else
    {
        FILE    *fh_param;
        char    line[201];
        int     lineNo = 0;
        char    *auxstr;

        if ((fh_param = fopen(fn_proj_param.c_str(), "r")) == NULL)
            REPORT_ERROR(ERR_IO_NOTOPEN,
                         (std::string)"ParametersProjectionTomography::read: There is a problem "
                         "opening the file " + fn_proj_param);
        while (fgets(line, 200, fh_param) != NULL)
        {
            if (line[0] == 0)
                continue;
            if (line[0] == '#')
                continue;
            if (line[0] == '\n')
                continue;
            switch (lineNo)
            {
            case 0:
                fnPhantom = firstWord(line);
                lineNo = 1;
                break;
            case 1:
                fnProjectionSeed =
                    firstWord(line);
                // Next two parameters are optional
                auxstr = nextToken();
                if (auxstr != NULL)
                    starting =
                        textToInteger(auxstr);
                fn_projection_extension = nextToken();
                lineNo = 2;
                break;
            case 2:
                proj_Xdim = textToInteger(firstToken(line));
                proj_Ydim = textToInteger(nextToken());
                lineNo = 3;
                break;
            case 3:
                axisRot = textToFloat(firstToken(line));
                axisTilt = textToFloat(nextToken());
                lineNo = 4;
                break;
            case 4:
                raxis.resize(3);
                XX(raxis) = textToFloat(firstToken(line));
                YY(raxis) = textToFloat(nextToken());
                ZZ(raxis) = textToFloat(nextToken());
                lineNo = 5;
                break;
            case 5:
                tilt0 = textToFloat(firstToken(line));
                tiltF = textToFloat(nextToken());
                tiltStep = textToFloat(nextToken());
                lineNo = 6;
                break;
            case 6:
                Nangle_dev = textToFloat(firstWord(line));
                auxstr = nextToken();
                if (auxstr != NULL)
                    Nangle_avg = textToFloat(auxstr);
                else
                    Nangle_avg = 0;
                lineNo = 7;
                break;
            case 7:
                Npixel_dev = textToFloat(firstWord(line));
                auxstr = nextToken();
                if (auxstr != NULL)
                    Npixel_avg = textToFloat(auxstr);
                else
                    Npixel_avg = 0;
                lineNo = 8;
                break;
            case 8:
                Ncenter_dev = textToFloat(firstWord(line));
                auxstr = nextToken();
                if (auxstr != NULL)
                    Ncenter_avg = textToFloat(auxstr);
                else
                    Ncenter_avg = 0;
                lineNo = 9;
                break;
            } /* switch end */
        } /* while end */
        if (lineNo != 9)
            REPORT_ERROR(ERR_ARG_MISSING, (std::string)"ParametersProjectionTomography::read: I "
                         "couldn't read all parameters from file " + fn_proj_param);
        fclose(fh_param);
    }
}

// Projection from a voxel volume ==========================================
/* Project a voxel volume -------------------------------------------------- */
//#define DEBUG
void project_Volume(MultidimArray<double> &V, Projection &P, int Ydim, int Xdim,
                    double rot, double tilt, double psi,
                    const Matrix1D<double> *roffset)
{
    SPEED_UP_temps;

    // Initialise projection
    P.reset(Ydim, Xdim);
    P.set_angles(rot, tilt, psi);

    // Compute the distance for this line crossing one voxel
    int x_0 = STARTINGX(V), x_F = FINISHINGX(V);
    int y_0 = STARTINGY(V), y_F = FINISHINGY(V);
    int z_0 = STARTINGZ(V), z_F = FINISHINGZ(V);

    // Distances in X and Y between the center of the projection pixel begin
    // computed and each computed ray
    double step = 1.0 / 3.0;

    // Avoids divisions by zero and allows orthogonal rays computation
    if (XX(P.direction) == 0)
        XX(P.direction) = XMIPP_EQUAL_ACCURACY;
    if (YY(P.direction) == 0)
        YY(P.direction) = XMIPP_EQUAL_ACCURACY;
    if (ZZ(P.direction) == 0)
        ZZ(P.direction) = XMIPP_EQUAL_ACCURACY;

    // Some precalculated variables
    int x_sign = SGN(XX(P.direction));
    int y_sign = SGN(YY(P.direction));
    int z_sign = SGN(ZZ(P.direction));
    double half_x_sign = 0.5 * x_sign;
    double half_y_sign = 0.5 * y_sign;
    double half_z_sign = 0.5 * z_sign;
    double iXXP_direction=1.0/XX(P.direction);
    double iYYP_direction=1.0/YY(P.direction);
    double iZZP_direction=1.0/ZZ(P.direction);

    MultidimArray<double> &mP = P();
    FOR_ALL_ELEMENTS_IN_ARRAY2D(mP)
    {
        Matrix1D<double> r_p(3); // r_p are the coordinates of the
        // pixel being projected in the
        // coordinate system attached to the
        // projection
        Matrix1D<double> p1(3);  // coordinates of the pixel in the
        // universal space
        double ray_sum = 0.0;    // Line integral value

        // Computes 4 different rays for each pixel.
        for (int rays_per_pixel = 0; rays_per_pixel < 4; rays_per_pixel++)
        {
            // universal coordinate system
            switch (rays_per_pixel)
            {
            case 0:
                VECTOR_R3(r_p, j - step, i - step, 0);
                break;
            case 1:
                VECTOR_R3(r_p, j - step, i + step, 0);
                break;
            case 2:
                VECTOR_R3(r_p, j + step, i - step, 0);
                break;
            case 3:
                VECTOR_R3(r_p, j + step, i + step, 0);
                break;
            }

            // Express r_p in the universal coordinate system
            if (roffset!=NULL)
                r_p-=*roffset;
            M3x3_BY_V3x1(p1, P.eulert, r_p);

            // Compute the minimum and maximum alpha for the ray
            // intersecting the given volume
            double alpha_xmin = (x_0 - 0.5 - XX(p1))* iXXP_direction;
            double alpha_xmax = (x_F + 0.5 - XX(p1))* iXXP_direction;
            double alpha_ymin = (y_0 - 0.5 - YY(p1))* iYYP_direction;
            double alpha_ymax = (y_F + 0.5 - YY(p1))* iYYP_direction;
            double alpha_zmin = (z_0 - 0.5 - ZZ(p1))* iZZP_direction;
            double alpha_zmax = (z_F + 0.5 - ZZ(p1))* iZZP_direction;

            double alpha_min = XMIPP_MAX(XMIPP_MIN(alpha_xmin, alpha_xmax),
                                         XMIPP_MIN(alpha_ymin, alpha_ymax));
            alpha_min = XMIPP_MAX(alpha_min, XMIPP_MIN(alpha_zmin, alpha_zmax));
            double alpha_max = XMIPP_MIN(XMIPP_MAX(alpha_xmin, alpha_xmax),
                                         XMIPP_MAX(alpha_ymin, alpha_ymax));
            alpha_max = XMIPP_MIN(alpha_max, XMIPP_MAX(alpha_zmin, alpha_zmax));
            if (alpha_max - alpha_min < XMIPP_EQUAL_ACCURACY)
                continue;

#ifdef DEBUG

            std::cout << "Pixel:  " << r_p.transpose() << std::endl
            << "Univ:   " << p1.transpose() << std::endl
            << "Dir:    " << P.direction.transpose() << std::endl
            << "Alpha x:" << alpha_xmin << " " << alpha_xmax << std::endl
            << "   " << (p1 + alpha_xmin*P.direction).transpose() << std::endl
            << "   " << (p1 + alpha_xmax*P.direction).transpose() << std::endl
            << "Alpha y:" << alpha_ymin << " " << alpha_ymax << std::endl
            << "   " << (p1 + alpha_ymin*P.direction).transpose() << std::endl
            << "   " << (p1 + alpha_ymax*P.direction).transpose() << std::endl
            << "Alpha z:" << alpha_zmin << " " << alpha_zmax << std::endl
            << "   " << (p1 + alpha_zmin*P.direction).transpose() << std::endl
            << "   " << (p1 + alpha_zmax*P.direction).transpose() << std::endl
            << "alpha  :" << alpha_min  << " " << alpha_max  << std::endl
            << std::endl;
#endif

            // Compute the first point in the volume intersecting the ray
            Matrix1D<double>  v(3);
            double zz_idxd, yy_idxd, xx_idxd;
            int    zz_idx , yy_idx , xx_idx;
            V3_BY_CT(v, P.direction, alpha_min);
            V3_PLUS_V3(v, p1, v);

            // Compute the index of the first voxel
            xx_idxd = xx_idx = CLIP(ROUND(XX(v)), x_0, x_F);
            yy_idxd = yy_idx = CLIP(ROUND(YY(v)), y_0, y_F);
            zz_idxd = zz_idx = CLIP(ROUND(ZZ(v)), z_0, z_F);

#ifdef DEBUG

            std::cout << "First voxel: " << v.transpose() << std::endl;
            std::cout << "   First index: " << idx.transpose() << std::endl;
            std::cout << "   Alpha_min: " << alpha_min << std::endl;
#endif

            // Follow the ray
            double alpha = alpha_min;
            do
            {
#ifdef DEBUG
                std::cout << " \n\nCurrent Value: " << V(zz_idx, yy_idx, xx_idx) << std::endl;
#endif

                double alpha_x = (xx_idxd + half_x_sign - XX(p1))* iXXP_direction;
                double alpha_y = (yy_idxd + half_y_sign - YY(p1))* iYYP_direction;
                double alpha_z = (zz_idxd + half_z_sign - ZZ(p1))* iZZP_direction;

                // Which dimension will ray move next step into?, it isn't neccesary to be only
                // one.
                double aux, diffx, diffy, diffz, diff_alpha;
                aux=alpha-alpha_x;
                //diffx = ABS(aux);
                diffx = fabs(aux);
                aux=alpha-alpha_y;
                //diffy = ABS(aux);
                diffy = fabs(aux);
                diff_alpha = XMIPP_MIN(diffx, diffy);
                aux=alpha-alpha_z;
                //diffz = ABS(aux);
                diffz = fabs(aux);
                diff_alpha=XMIPP_MIN(diff_alpha, diffz);

                ray_sum += diff_alpha * A3D_ELEM(V, zz_idx, yy_idx, xx_idx);

                aux=diff_alpha - diffx;
                if (fabs(aux) <= XMIPP_EQUAL_ACCURACY)
                {
                    alpha = alpha_x;
                    xx_idx += x_sign;
                    xx_idxd = xx_idx;
                }
                aux=diff_alpha - diffy;
                if (fabs(aux) <= XMIPP_EQUAL_ACCURACY)
                {
                    alpha = alpha_y;
                    yy_idx += y_sign;
                    yy_idxd = yy_idx;
                }
                aux=diff_alpha - diffz;
                if (fabs(aux) <= XMIPP_EQUAL_ACCURACY)
                {
                    alpha = alpha_z;
                    zz_idx += z_sign;
                    zz_idxd = zz_idx;
                }

#ifdef DEBUG
                std::cout << "Alpha x,y,z: " << alpha_x << " " << alpha_y
                << " " << alpha_z << " ---> " << alpha << std::endl;

                XX(v) += diff_alpha * XX(P.direction);
                YY(v) += diff_alpha * YY(P.direction);
                ZZ(v) += diff_alpha * ZZ(P.direction);

                std::cout << "    Next entry point: " << v.transpose() << std::endl
                << "    Index: " << idx.transpose() << std::endl
                << "    diff_alpha: " << diff_alpha << std::endl
                << "    ray_sum: " << ray_sum << std::endl
                << "    Alfa tot: " << alpha << "alpha_max: " << alpha_max <<
                std::endl;
#endif

            }
            while ((alpha_max - alpha) > XMIPP_EQUAL_ACCURACY);
        } // for

        A2D_ELEM(IMGMATRIX(P), i, j) = ray_sum * 0.25;
#ifdef DEBUG

        std::cout << "Assigning P(" << i << "," << j << ")=" << ray_sum << std::endl;
#endif

    }
}
#undef DEBUG

/* Project a voxel volume with respect to an offcentered axis -------------- */
//#define DEBUG
void project_Volume_offCentered(MultidimArray<double> &V, Projection &P,
                                int Ydim, int Xdim, double axisRot, double axisTilt,
                                const Matrix1D<double> &raxis, double angle, double inplaneRot,
                                const Matrix1D<double> &rinplane)
{
    // Find Euler rotation matrix
    Matrix1D<double> axis;
    Euler_direction(axisRot,axisTilt,0,axis);
    Matrix2D<double> Raxis;
    rotation3DMatrix(angle,axis,Raxis,false);
    Matrix2D<double> Rinplane;
    rotation3DMatrix(inplaneRot,'Z',Rinplane,false);
    double rot, tilt, psi;
    Euler_matrix2angles(Rinplane*Raxis, rot, tilt, psi);

    // Find displacement because of axis offset and inplane shift
    Matrix1D<double> roffset=Rinplane*(raxis-Raxis*raxis)+rinplane;

#ifdef DEBUG

    std::cout << "axisRot=" << axisRot << " axisTilt=" << axisTilt
    << " axis=" << axis.transpose() << std::endl
    << "angle=" << angle << std::endl
    << "Raxis\n" << Raxis
    << "Rinplane\n" << Rinplane
    << "Raxis*Rinplane\n" << Raxis*Rinplane
    << "rot=" << rot << " tilt=" << tilt << " psi=" << psi
    << std::endl;
    Matrix2D<double> E;
    Euler_angles2matrix(rot,tilt,psi,E);
    std::cout << "E\n" << E << std::endl;
#endif

    project_Volume(V, P, Ydim, Xdim, rot, tilt, psi, &roffset);
}
#undef DEBUG

// Perform a backprojection ================================================
/* Backproject a single projection ----------------------------------------- */
//#define DEBUG

// Sjors, 16 May 2005
// This routine may give volumes with spurious high frequencies.....
void singleWBP(MultidimArray<double> &V, Projection &P)
{
    SPEED_UP_temps;

    // Compute the distance for this line crossing one voxel
    int x_0 = STARTINGX(V), x_F = FINISHINGX(V);
    int y_0 = STARTINGY(V), y_F = FINISHINGY(V);
    int z_0 = STARTINGZ(V), z_F = FINISHINGZ(V);

    // Distances in X and Y between the center of the projection pixel begin
    // computed and each computed ray
    double step = 1.0 / 3.0;

    // Avoids divisions by zero and allows orthogonal rays computation
    if (XX(P.direction) == 0)
        XX(P.direction) = XMIPP_EQUAL_ACCURACY;
    if (YY(P.direction) == 0)
        YY(P.direction) = XMIPP_EQUAL_ACCURACY;
    if (ZZ(P.direction) == 0)
        ZZ(P.direction) = XMIPP_EQUAL_ACCURACY;

    // Some precalculated variables
    int x_sign = SGN(XX(P.direction));
    int y_sign = SGN(YY(P.direction));
    int z_sign = SGN(ZZ(P.direction));
    double half_x_sign = 0.5 * x_sign;
    double half_y_sign = 0.5 * y_sign;
    double half_z_sign = 0.5 * z_sign;

    MultidimArray<double> &mP = P();
    FOR_ALL_ELEMENTS_IN_ARRAY2D(mP)
    {
        Matrix1D<double> r_p(3); // r_p are the coordinates of the
        // pixel being projected in the
        // coordinate system attached to the
        // projection
        Matrix1D<double> p1(3);  // coordinates of the pixel in the
        // universal space
        double ray_sum = 0.0;    // Line integral value

        // Computes 4 different rays for each pixel.
        VECTOR_R3(r_p, j, i, 0);

        // Express r_p in the universal coordinate system
        M3x3_BY_V3x1(p1, P.eulert, r_p);

        // Compute the minimum and maximum alpha for the ray
        // intersecting the given volume
        double alpha_xmin = (x_0 - 0.5 - XX(p1)) / XX(P.direction);
        double alpha_xmax = (x_F + 0.5 - XX(p1)) / XX(P.direction);
        double alpha_ymin = (y_0 - 0.5 - YY(p1)) / YY(P.direction);
        double alpha_ymax = (y_F + 0.5 - YY(p1)) / YY(P.direction);
        double alpha_zmin = (z_0 - 0.5 - ZZ(p1)) / ZZ(P.direction);
        double alpha_zmax = (z_F + 0.5 - ZZ(p1)) / ZZ(P.direction);

        double alpha_min = XMIPP_MAX(XMIPP_MIN(alpha_xmin, alpha_xmax),
                                     XMIPP_MIN(alpha_ymin, alpha_ymax));
        alpha_min = XMIPP_MAX(alpha_min, XMIPP_MIN(alpha_zmin, alpha_zmax));
        double alpha_max = XMIPP_MIN(XMIPP_MAX(alpha_xmin, alpha_xmax),
                                     XMIPP_MAX(alpha_ymin, alpha_ymax));
        alpha_max = XMIPP_MIN(alpha_max, XMIPP_MAX(alpha_zmin, alpha_zmax));
        if (alpha_max - alpha_min < XMIPP_EQUAL_ACCURACY)
            continue;

        // Compute the first point in the volume intersecting the ray
        Matrix1D<double>  v(3);
        Matrix1D<int>    idx(3);
        V3_BY_CT(v, P.direction, alpha_min);
        V3_PLUS_V3(v, p1, v);

        // Compute the index of the first voxel
        XX(idx) = CLIP(ROUND(XX(v)), x_0, x_F);
        YY(idx) = CLIP(ROUND(YY(v)), y_0, y_F);
        ZZ(idx) = CLIP(ROUND(ZZ(v)), z_0, z_F);


#ifdef DEBUG

        std::cout << "First voxel: " << v.transpose() << std::endl;
        std::cout << "   First index: " << idx.transpose() << std::endl;
        std::cout << "   Alpha_min: " << alpha_min << std::endl;
#endif

        // Follow the ray
        double alpha = alpha_min;
        do
        {
#ifdef DEBUG
            std::cout << " \n\nCurrent Value: " << V(ZZ(idx), YY(idx), XX(idx)) << std::endl;
#endif

            double alpha_x = (XX(idx) + half_x_sign - XX(p1)) / XX(P.direction);
            double alpha_y = (YY(idx) + half_y_sign - YY(p1)) / YY(P.direction);
            double alpha_z = (ZZ(idx) + half_z_sign - ZZ(p1)) / ZZ(P.direction);

            // Which dimension will ray move next step into?, it isn't neccesary to be only
            // one.
            double diffx = ABS(alpha - alpha_x);
            double diffy = ABS(alpha - alpha_y);
            double diffz = ABS(alpha - alpha_z);

            double diff_alpha = XMIPP_MIN(XMIPP_MIN(diffx, diffy), diffz);

            A3D_ELEM(V, ZZ(idx), YY(idx), XX(idx)) += diff_alpha * A2D_ELEM(P(), i, j);

            if (ABS(diff_alpha - diffx) <= XMIPP_EQUAL_ACCURACY)
            {
                alpha = alpha_x;
                XX(idx) += x_sign;
            }
            if (ABS(diff_alpha - diffy) <= XMIPP_EQUAL_ACCURACY)
            {
                alpha = alpha_y;
                YY(idx) += y_sign;
            }
            if (ABS(diff_alpha - diffz) <= XMIPP_EQUAL_ACCURACY)
            {
                alpha = alpha_z;
                ZZ(idx) += z_sign;
            }

        }
        while ((alpha_max - alpha) > XMIPP_EQUAL_ACCURACY);
#ifdef DEBUG

        std::cout << "Assigning P(" << i << "," << j << ")=" << ray_sum << std::endl;
#endif

    }
}
#undef DEBUG

// Projections from crystals particles #####################################
// The projection is not precleaned (set to 0) before projecting and its
// angles are supposed to be already written (and all Euler matrices
// precalculated
// The projection plane is supposed to pass through the Universal coordinate
// origin

/* Algorithm
Compute Eg, proj(ai), proj(bi) and A
Compute prjX, prjY, prjZ and prjO which are the projections of the origin
   and grid axes
Compute the blob footprint size in the deformed image space
   (=deffootprintsize)
For each blob in the grid
   if it is within the unit cell mask
      // compute actual deformed projection position
      defactprj=A*(k*projZ+i*projY+j*projX+projO)
      // compute corners in the deformed image
      corner1=defactprj-deffootprintsize;
      corner2=defactprj+ deffootprintsize;

      for each point (r) in the deformed projection space between (corner1, corner2)
         compute position (rc) in the undeformed projection space
         compute position within footprint corresponding to rc (=foot)
         if it is inside footprint
            rw=wrap(r);
            update projection at rw with the data at foot  
*/

//#define DEBUG_LITTLE
//#define DEBUG
//#define DEBUG_INTERMIDIATE
#define wrap_as_Crystal(x,y,xw,yw)  \
    xw=(int) intWRAP(x,x0,xF); \
    yw=(int) intWRAP(y,y0,yF);

void project_Crystal_SimpleGrid(Image<double> &vol, const SimpleGrid &grid,
                                const Basis &basis,
                                Projection &proj, Projection &norm_proj,
                                const Matrix1D<double> &shift,
                                const Matrix1D<double> &aint, const Matrix1D<double> &bint,
                                const Matrix2D<double> &D,  const Matrix2D<double> &Dinv,
                                const MultidimArray<int> &mask, int FORW, int eq_mode)
{
    Matrix1D<double> prjX(3);                // Coordinate: Projection of the
    Matrix1D<double> prjY(3);                // 3 grid vectors
    Matrix1D<double> prjZ(3);
    Matrix1D<double> prjOrigin(3);           // Coordinate: Where in the 2D
    // projection plane the origin of
    // the grid projects
    Matrix1D<double> prjaint(3);             // Coordinate: Projection of the
    Matrix1D<double> prjbint(3);             // 2 deformed lattice vectors
    Matrix1D<double> prjDir;                 // Direction of projection
    // in the deformed space

    Matrix1D<double> actprj(3);              // Coord: Actual position inside
    // the projection plane
    Matrix1D<double> defactprj(3);           // Coord: Actual position inside
    // the deformed projection plane
    Matrix1D<double> beginZ(3);              // Coord: Plane coordinates of the
    // projection of the 3D point
    // (z0,YY(lowest),XX(lowest))
    Matrix1D<double> beginY(3);              // Coord: Plane coordinates of the
    // projection of the 3D point
    // (z0,y0,XX(lowest))
    Matrix1D<double> footprint_size(2);      // The footprint is supposed
    // to be defined between
    // (-vmax,+vmax) in the Y axis,
    // and (-umax,+umax) in the X axis
    // This footprint size is the
    // R2 vector (umax,vmax).
    Matrix1D<double> deffootprint_size(2);   // The footprint size
    // in the deformed space
    int XX_corner2, XX_corner1;              // Coord: Corners of the
    int YY_corner2, YY_corner1;              // footprint when it is projected
    // onto the projection plane
    Matrix1D<double> rc(2), r(2);            // Position vector which will
    // move from corner1 to corner2.
    // In rc the wrapping is not
    // considered, while it is in r
    int           foot_V, foot_U;            // Img Coord: coordinate
    // corresponding to the blobprint
    // point which matches with this
    // pixel position
    double        vol_corr;                  // Correction for a volum element
    int           N_eq;                      // Number of equations in which
    // a blob is involved
    int           i, j, k;                   // volume element indexes
    SPEED_UP_temps;                       // Auxiliar variables for
    // fast multiplications

    // Check that it is a blob volume .......................................
    if (basis.type != Basis::blobs)
        REPORT_ERROR(ERR_VALUE_INCORRECT, "project_Crystal_SimpleGrid: Cannot project other than "
                     "blob volumes");

    // Compute the deformed direction of projection .........................
    Matrix2D<double> Eulerg;
    Eulerg = proj.euler * D;

    // Compute deformation in the projection space ..........................
    // The following two vectors are defined in the deformed volume space
    VECTOR_R3(actprj, XX(aint), YY(aint), 0);
    grid.Gdir_project_to_plane(actprj, Eulerg, prjaint);
    VECTOR_R3(actprj, XX(bint), YY(bint), 0);
    grid.Gdir_project_to_plane(actprj, Eulerg, prjbint);
#ifdef DEBUG_LITTLE

    double rot, tilt, psi;
    Euler_matrix2angles(proj.euler, rot, tilt, psi);
    std::cout << "rot= "  << rot << " tilt= " << tilt
    << " psi= " << psi << std::endl;
    std::cout << "D\n" << D << std::endl;
    std::cout << "Eulerf\n" << proj.euler << std::endl;
    std::cout << "Eulerg\n" << Eulerg << std::endl;
    std::cout << "aint    " << aint.transpose() << std::endl;
    std::cout << "bint    " << bint.transpose() << std::endl;
    std::cout << "prjaint " << prjaint.transpose() << std::endl;
    std::cout << "prjbint " << prjbint.transpose() << std::endl;
    std::cout.flush();
#endif
    // Project grid axis ....................................................
    // These vectors ((1,0,0),(0,1,0),...) are referred to the grid
    // coord. system and the result is a 2D vector in the image plane
    // The unit size within the image plane is 1, ie, the same as for
    // the universal grid.
    // Be careful that these grid vectors are defined in the deformed
    // volume space, and the projection are defined in the deformed
    // projections,
    VECTOR_R3(actprj, 1, 0, 0);
    grid.Gdir_project_to_plane(actprj, Eulerg, prjX);
    VECTOR_R3(actprj, 0, 1, 0);
    grid.Gdir_project_to_plane(actprj, Eulerg, prjY);
    VECTOR_R3(actprj, 0, 0, 1);
    grid.Gdir_project_to_plane(actprj, Eulerg, prjZ);

    // This time the origin of the grid is in the universal coord system
    // but in the deformed space
    Uproject_to_plane(grid.origin, Eulerg, prjOrigin);

    // This is a correction used by the crystallographic symmetries
    prjOrigin += XX(shift) * prjaint + YY(shift) * prjbint;

#ifdef DEBUG_LITTLE

    std::cout << "prjX      " << prjX.transpose() << std::endl;
    std::cout << "prjY      " << prjY.transpose() << std::endl;
    std::cout << "prjZ      " << prjZ.transpose() << std::endl;
    std::cout << "prjOrigin " << prjOrigin.transpose() << std::endl;
    std::cout.flush();
#endif

    // Now I will impose that prja becomes (Xdim,0) and prjb, (0,Ydim)
    // A is a matrix such that
    // A*prjaint=(Xdim,0)'
    // A*prjbint=(0,Ydim)'
    Matrix2D<double> A(2, 2), Ainv(2, 2);
    A(0, 0) = YY(prjbint) * xDim;
    A(0, 1) = -XX(prjbint) * xDim;
    A(1, 0) = -YY(prjaint) * yDim;
    A(1, 1) = XX(prjaint) * yDim;
    double nor = 1 / (XX(prjaint) * YY(prjbint) - XX(prjbint) * YY(prjaint));
    M2x2_BY_CT(A, A, nor);
    M2x2_INV(Ainv, A);

#ifdef DEBUG_LITTLE

    std::cout << "A\n" << A << std::endl;
    std::cout << "Ainv\n" << Ainv << std::endl;
    std::cout << "Check that A*prjaint=(Xdim,0)    "
    << (A*vectorR2(XX(prjaint), YY(prjaint))).transpose() << std::endl;
    std::cout << "Check that A*prjbint=(0,Ydim)    "
    << (A*vectorR2(XX(prjbint), YY(prjbint))).transpose() << std::endl;
    std::cout << "Check that Ainv*(Xdim,0)=prjaint "
    << (Ainv*vectorR2(xDim, 0)).transpose() << std::endl;
    std::cout << "Check that Ainv*(0,Ydim)=prjbint "
    << (Ainv*vectorR2(0, yDim)).transpose() << std::endl;
    std::cout.flush();
#endif

    // Footprint size .......................................................
    // The following vectors are integer valued vectors, but they are
    // stored as real ones to make easier operations with other vectors.
    // Look out that this footprint size is in the deformed projection,
    // that is why it is not a square footprint but a parallelogram
    // and we have to look for the smaller rectangle which surrounds it
    XX(footprint_size) = basis.blobprint.umax();
    YY(footprint_size) = basis.blobprint.vmax();
    N_eq = (2 * basis.blobprint.umax() + 1) * (2 * basis.blobprint.vmax() + 1);
    Matrix1D<double> c1(3), c2(3);
    XX(c1)           = XX(footprint_size);
    YY(c1)           = YY(footprint_size);
    XX(c2)           = -XX(c1);
    YY(c2)           = YY(c1);
    M2x2_BY_V2x1(c1, A, c1);
    M2x2_BY_V2x1(c2, A, c2);
    XX(deffootprint_size) = XMIPP_MAX(ABS(XX(c1)), ABS(XX(c2)));
    YY(deffootprint_size) = XMIPP_MAX(ABS(YY(c1)), ABS(YY(c2)));

#ifdef DEBUG_LITTLE

    std::cout << "Footprint_size " << footprint_size.transpose() << std::endl;
    std::cout << "c1: " << c1.transpose() << std::endl;
    std::cout << "c2: " << c2.transpose() << std::endl;
    std::cout << "Deformed Footprint_size " << deffootprint_size.transpose()
    << std::endl;
    std::cout.flush();
#endif

    // This type conversion gives more speed
    int ZZ_lowest = (int) ZZ(grid.lowest);
    int YY_lowest = XMIPP_MAX((int) YY(grid.lowest), STARTINGY(mask));
    int XX_lowest = XMIPP_MAX((int) XX(grid.lowest), STARTINGX(mask));
    int ZZ_highest = (int) ZZ(grid.highest);
    int YY_highest = XMIPP_MIN((int) YY(grid.highest), FINISHINGY(mask));
    int XX_highest = XMIPP_MIN((int) XX(grid.highest), FINISHINGX(mask));

    // Project the whole grid ...............................................
    // Corner of the plane defined by Z. These coordinates try to be within
    // the valid indexes of the projection (defined between STARTING and
    // FINISHING values, but it could be that they may lie outside.
    // These coordinates need not to be integer, in general, they are
    // real vectors.
    // The vectors returned by the projection routines are R3 but we
    // are only interested in their first 2 components, ie, in the
    // in-plane components
    beginZ = XX_lowest * prjX + YY_lowest * prjY + ZZ_lowest * prjZ + prjOrigin;

#ifdef DEBUG_LITTLE

    std::cout << "BeginZ     " << beginZ.transpose()             << std::endl;
    std::cout << "Mask       ";
    mask.printShape();
    std::cout << std::endl;
    std::cout << "Vol        ";
    vol().printShape();
    std::cout << std::endl;
    std::cout << "Proj       ";
    proj().printShape();
    std::cout << std::endl;
    std::cout << "Norm Proj  ";
    norm_proj().printShape();
    std::cout << std::endl;
    std::cout << "Footprint  ";
    footprint().printShape();
    std::cout << std::endl;
    std::cout << "Footprint2 ";
    footprint2().printShape();
    std::cout << std::endl;
#endif

    Matrix1D<double> grid_index(3);
    for (k = ZZ_lowest; k <= ZZ_highest; k++)
    {
        // Corner of the row defined by Y
        beginY = beginZ;
        for (i = YY_lowest; i <= YY_highest; i++)
        {
            // First point in the row
            actprj = beginY;
            for (j = XX_lowest; j <= XX_highest; j++)
            {
                VECTOR_R3(grid_index, j, i, k);
#ifdef DEBUG

                std::cout << "Visiting " << i << " " << j << std::endl;
#endif

                // Be careful that you cannot skip any blob, although its
                // value be 0, because it is useful for norm_proj
                // unless it doesn't belong to the reconstruction mask
                if (A2D_ELEM(mask, i, j) && grid.is_interesting(grid_index))
                {
                    // Look for the position in the deformed projection
                    M2x2_BY_V2x1(defactprj, A, actprj);

                    // Search for integer corners for this blob
                    XX_corner1 = CEIL(XX(defactprj) - XX(deffootprint_size));
                    YY_corner1 = CEIL(YY(defactprj) - YY(deffootprint_size));
                    XX_corner2 = FLOOR(XX(defactprj) + XX(deffootprint_size));
                    YY_corner2 = FLOOR(YY(defactprj) + YY(deffootprint_size));

#ifdef DEBUG

                    std::cout << "  k= " << k << " i= " << i << " j= " << j << std::endl;
                    std::cout << "  Actual position: " << actprj.transpose() << std::endl;
                    std::cout << "  Deformed position: " << defactprj.transpose() << std::endl;
                    std::cout << "  Blob corners: (" << XX_corner1 << "," << YY_corner1
                    << ") (" << XX_corner2 << "," << YY_corner2 << ")\n";
                    std::cout.flush();
#endif

                    // Now there is no way that both corners collapse into a line,
                    // since the projection points are wrapped

                    if (!FORW)
                        vol_corr = 0;

                    // Effectively project this blob
                    // (xc,yc) is the position of the considered pixel
                    // in the crystal undeformed projection
                    // When wrapping and deformation are considered then x and
                    // y are used
#define xc XX(rc)
#define yc YY(rc)
#define x  XX(r)
#define y  YY(r)

                    for (y = YY_corner1; y <= YY_corner2; y++)
                        for (x = XX_corner1; x <= XX_corner2; x++)
                        {
                            // Compute position in undeformed space and
                            // corresponding sample in the blob footprint
                            M2x2_BY_V2x1(rc, Ainv, r);
                            OVER2IMG(basis.blobprint, yc - YY(actprj), xc - XX(actprj),
                                     foot_V, foot_U);

#ifdef DEBUG

                            std::cout << "    Studying: " << r.transpose()
                            << "--> " << rc.transpose()
                            << "dist (" << xc - XX(actprj)
                            << "," << yc - YY(actprj) << ") --> "
                            << foot_U << " " << foot_V << std::endl;
                            std::cout.flush();
#endif

                            if (IMGMATRIX(basis.blobprint).outside(foot_V, foot_U))
                                continue;

                            // Wrap positions
                            int yw, xw;
                            wrap_as_Crystal(x, y, xw, yw);
#ifdef DEBUG

                            std::cout << "      After wrapping " << xw << " " << yw << std::endl;
                            std::cout << "      Value added = " << VOLVOXEL(vol, k, i, j) *
                            IMGPIXEL(basis.blobprint, foot_V, foot_U) << " Blob value = "
                            << IMGPIXEL(basis.blobprint, foot_V, foot_U)
                            << " Blob^2 " << IMGPIXEL(basis.blobprint2, foot_V, foot_U)
                            << std::endl;
                            std::cout.flush();
#endif

                            if (FORW)
                            {
                                IMGPIXEL(proj, yw, xw) += VOLVOXEL(vol, k, i, j) *
                                                          IMGPIXEL(basis.blobprint, foot_V, foot_U);
                                switch (eq_mode)
                                {
                                case CAVARTK:
                                case ARTK:
                                    IMGPIXEL(norm_proj, yw, xw) +=
                                        IMGPIXEL(basis.blobprint2, foot_V, foot_U);
                                    break;
                                }
                            }
                            else
                            {
                                vol_corr += IMGPIXEL(norm_proj, yw, xw) *
                                            IMGPIXEL(basis.blobprint, foot_V, foot_U);
                            }

                        }

#ifdef DEBUG_INTERMIDIATE
                    proj.write("inter.xmp");
                    norm_proj.write("inter_norm.xmp");
                    std::cout << "Press any key\n";
                    char c;
                    std::cin >> c;
#endif

                    if (!FORW)
                        switch (eq_mode)
                        {
                        case ARTK:
                            VOLVOXEL(vol, k, i, j) += vol_corr;
                            break;
                        case CAVARTK:
                            VOLVOXEL(vol, k, i, j) += vol_corr / N_eq;
                            break;
                        }
                }

                // Prepare for next iteration
                V2_PLUS_V2(actprj, actprj, prjX);
            }
            V2_PLUS_V2(beginY, beginY, prjY);
        }
        V2_PLUS_V2(beginZ, beginZ, prjZ);
    }
    //#define DEBUG_AT_THE_END
#ifdef DEBUG_AT_THE_END
    proj.write("inter.xmp");
    norm_proj.write("inter_norm.xmp");
    std::cout << "Press any key\n";
    char c;
    std::cin >> c;
#endif
}
#undef DEBUG
#undef DEBUG_LITTLE
#undef wrap_as_Crystal

/* Project a Grid Volume --------------------------------------------------- */
//#define DEBUG
void project_Crystal_Volume(
    GridVolume &vol,                      // Volume
    const Basis &basis,                   // Basis
    Projection       &proj,               // Projection
    Projection       &norm_proj,          // Projection of a unitary volume
    int              Ydim,                // Dimensions of the projection
    int              Xdim,
    double rot, double tilt, double psi,  // Euler angles
    const Matrix1D<double> &shift,        // Shift to apply to projections
    const Matrix1D<double> &aint,         // First lattice vector (2x1) in voxels
    const Matrix1D<double> &bint,         // Second lattice vector (2x1) in voxels
    const Matrix2D<double> &D,            // volume deformation matrix
    const Matrix2D<double> &Dinv,         // volume deformation matrix
    const MultidimArray<int>    &mask,         // volume mask
    int              FORW,                // 1 if we are projecting a volume
    //   norm_proj is calculated
    // 0 if we are backprojecting
    //   norm_proj must be valid
    int eq_mode)                          // ARTK, CAVARTK, CAVK or CAV
{
    // If projecting forward initialise projections
    if (FORW)
    {
        proj.reset(Ydim, Xdim);
        proj.set_angles(rot, tilt, psi);
        norm_proj().resize(proj());
    }

    // Project each subvolume
    for (int i = 0; i < vol.VolumesNo(); i++)
    {
        project_Crystal_SimpleGrid(vol(i), vol.grid(i), basis,
                                   proj, norm_proj, shift, aint, bint, D, Dinv, mask, FORW, eq_mode);

#ifdef DEBUG

        Image<double> save;
        save = norm_proj;
        if (FORW)
            save.write((std::string)"PPPnorm_FORW" + (char)(48 + i));
        else
            save.write((std::string)"PPPnorm_BACK" + (char)(48 + i));
#endif

    }
}
#undef DEBUG

/* Count equations in a grid volume --------------------------------------- */
void count_eqs_in_projection(GridVolumeT<int> &GVNeq,
                             const Basis &basis, Projection &read_proj)
{
    for (int i = 0; i < GVNeq.VolumesNo(); i++)
        project_SimpleGrid(&(GVNeq(i)), &(GVNeq.grid(i)), &basis,
                           &read_proj, &read_proj, FORWARD, COUNT_EQ, NULL, NULL);
}
