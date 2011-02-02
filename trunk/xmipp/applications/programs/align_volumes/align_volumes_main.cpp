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

#include <data/args.h>
#include <data/image.h>
#include <data/filters.h>
#include <data/geometry.h>
#include <data/mask.h>

#include <data/program.h>

// Alignment parameters needed by fitness ----------------------------------
class AlignParams
{
public:
#define COVARIANCE       1
#define LEAST_SQUARES    2

    int alignment_method;

    Image<double> V1;
    Image<double> V2;
    Image<double> Vaux;
    const MultidimArray<int> *mask_ptr;
};

// Global parameters needed by fitness ------------------------------------
AlignParams params;

// Apply transformation ---------------------------------------------------
void applyTransformation(const MultidimArray<double> &V2,
                         MultidimArray<double> &Vaux,
                         double *p)
{
    Matrix2D<double> A, Aaux;

    double greyScale = p[0];
    double greyShift = p[1];
    double rot       = p[2];
    double tilt      = p[3];
    double psi       = p[4];
    double scale     = p[5];
    double shiftz    = p[6];
    double shifty    = p[7];
    double shiftx    = p[8];

    Euler_angles2matrix(rot, tilt, psi, A, true);
    translationMatrix(Aaux,shiftx,shifty,shiftz);
    A = A * Aaux;
    scaleMatrix(Aaux, scale, scale, scale);
    A = A * Aaux;

    applyGeometry(LINEAR, Vaux, V2, A, IS_NOT_INV, WRAP);
    Vaux*=greyScale;
    Vaux+=greyShift;
}


// Fitness between two volumes --------------------------------------------
double fitness(double *p)
{
    applyTransformation(params.V2(),params.Vaux(),p);

    // Correlate
    double fit;
    switch (params.alignment_method)
    {
    case (COVARIANCE):
                    fit = -correlation_index(params.V1(), params.Vaux(), params.mask_ptr);
        break;
    case (LEAST_SQUARES):
                    fit = rms(params.V1(), params.Vaux(), params.mask_ptr);
        break;
    }
    return fit;
}


double wrapperFitness(double *p, void *params)
{
    return fitness(p+1);
}


class ProgAlignVolumes : public XmippProgram
{

public:

    Mask mask;

    FileName fn1, fn2;
    double   rot0, rotF, tilt0, tiltF, psi0, psiF;
    double   step_rot, step_tilt, step_psi;
    double   scale0, scaleF, step_scale;
    double   z0, zF, y0, yF, x0, xF, step_z, step_y, step_x;
    double   grey_scale0, grey_scaleF, step_grey;
    double   grey_shift0, grey_shiftF, step_grey_shift;
    int      tell;
    bool     apply;
    bool     mask_enabled, mean_in_mask;
    bool     usePowell, onlyShift;

public:

    void defineParams()
    {

        addUsageLine("Align two volumes varying orientation, position and scale");
        addUsageLine("Example of use: Align volume1 and volume2 with default options");
        addUsageLine("   xmipp_align_volumes --i1 volume1.vol --i2 volume2.vol");

        addParamsLine("   --i1 <volume1>        : the first volume to align");
        addParamsLine("   --i2 <volume2>        : the second one");
        addParamsLine("  [--rot   <rot0=0>  <rotF=0>  <step_rot=1>]  : in degrees");
        addParamsLine("  [--tilt  <tilt0=0> <tiltF=0> <step_tilt=1>] : in degrees");
        addParamsLine("  [--psi   <psi0=0>  <psiF=0>  <step_psi=1>]  : in degrees");
        addParamsLine("  [--scale <sc0=1>   <scF=1>   <step_sc=1>]   : size scale margin");
        addParamsLine("  [--grey_scale <sc0=1> <scF=1> <step_sc=1>]  : grey scale margin");
        addParamsLine("  [--grey_shift <sh0=0> <shF=0> <step_sh=1>]  : grey shift margin");
        addParamsLine("  [-z <z0=0> <zF=0> <step_z=1>] : Z position in pixels");
        addParamsLine("  [-y <y0=0> <yF=0> <step_y=1>] : Y position in pixels");
        addParamsLine("  [-x <x0=0> <xF=0> <step_x=1>] : X position in pixels");
        addParamsLine("  [--show_fit]      : Show fitness values");
        addParamsLine("  [--apply]         : Apply best movement to --i2");
        addParamsLine("  [--mean_in_mask]  : Use the means within the mask");
        addParamsLine("  [--covariance]    : Covariance fitness criterion");
        addParamsLine("  [--least_squares] : LS fitness criterion");
        addParamsLine("  [--local]         : Use local optimizer instead of exhaustive search");
        addParamsLine("  [--onlyShift]     : Only shift");
        addParamsLine(" == Mask Options == ");
        mask.defineParams(this);
    }

    void readParams()
    {
        // Get parameters =======================================================
        fn1 = getParam("--i1");
        fn2 = getParam("--i2");

        rot0 = getDoubleParam("--rot",0);
        rotF = getDoubleParam("--rot",1);
        step_rot = getDoubleParam("--rot",2);

        tilt0 = getDoubleParam("--tilt",0);
        tiltF = getDoubleParam("--tilt",1);
        step_tilt = getDoubleParam("--tilt",2);

        psi0 = getDoubleParam("--psi",0);
        psiF = getDoubleParam("--psi",1);
        step_psi = getDoubleParam("--psi",2);

        scale0 = getDoubleParam("--scale",0);
        scaleF = getDoubleParam("--scale",1);
        step_scale = getDoubleParam("--scale",2);

        grey_scale0 = getDoubleParam("--grey_scale",0);
        grey_scaleF = getDoubleParam("--grey_scale",1);
        step_grey = getDoubleParam("--grey_scale",2);

        grey_shift0 = getDoubleParam("--grey_shift",0);
        grey_shiftF = getDoubleParam("--grey_shift",1);
        step_grey_shift = getDoubleParam("--grey_shift",2);

        z0 = getDoubleParam("-z",0);
        zF = getDoubleParam("-z",1);
        step_z = getDoubleParam("-z",2);

        y0 = getDoubleParam("-y",0);
        yF = getDoubleParam("-y",1);
        step_y = getDoubleParam("-y",2);

        x0 = getDoubleParam("-x",0);
        xF = getDoubleParam("-x",1);
        step_x = getDoubleParam("-x",2);

        mask_enabled = checkParam("--mask");
        mean_in_mask = checkParam("--mean_in_mask");
        if (mask_enabled)
            mask.read(argc, argv);

        usePowell = checkParam("--local");
        onlyShift = checkParam("--onlyShift");

        if (step_rot   == 0)
            step_rot = 1;
        if (step_tilt  == 0)
            step_tilt  = 1;
        if (step_psi   == 0)
            step_psi = 1;
        if (step_scale == 0)
            step_scale = 1;
        if (step_grey  == 0)
            step_grey  = 1;
        if (step_grey_shift  == 0)
            step_grey_shift  = 1;
        if (step_z     == 0)
            step_z = 1;
        if (step_y     == 0)
            step_y = 1;
        if (step_x     == 0)
            step_x = 1;

        tell = checkParam("--show_fit");
        apply = checkParam("--apply");

        if (checkParam("--covariance"))
        {
            params.alignment_method = COVARIANCE;
        }
        else if (checkParam("--least_squares"))
        {
            params.alignment_method = LEAST_SQUARES;
        }
        else
        {
            params.alignment_method = COVARIANCE;
        }
    }

    void run ()
    {
        mask.allowed_data_types = INT_MASK;

        // Main program =========================================================
        params.V1.read(fn1);
        params.V1().setXmippOrigin();
        params.V2.read(fn2);
        params.V2().setXmippOrigin();

        // Initialize best_fit
        double best_fit;
        Matrix1D<double> best_align(8);
        bool first = true;

        // Generate mask
        if (mask_enabled)
        {
            mask.generate_mask(params.V1());
            params.mask_ptr = &(mask.get_binary_mask());
        }
        else
            params.mask_ptr = NULL;

        // Exhaustive search
        if (!usePowell)
        {
            // Count number of iterations
            int times = 1;
            if (!tell)
            {
                if (grey_scale0 != grey_scaleF)
                    times *= FLOOR(1 + (grey_scaleF - grey_scale0) / step_grey);
                if (grey_shift0 != grey_shiftF)
                    times *= FLOOR(1 + (grey_shiftF - grey_shift0) / step_grey_shift);
                if (rot0 != rotF)
                    times *= FLOOR(1 + (rotF - rot0) / step_rot);
                if (tilt0 != tiltF)
                    times *= FLOOR(1 + (tiltF - tilt0) / step_tilt);
                if (psi0 != psiF)
                    times *= FLOOR(1 + (psiF - psi0) / step_psi);
                if (scale0 != scaleF)
                    times *= FLOOR(1 + (scaleF - scale0) / step_scale);
                if (z0 != zF)
                    times *= FLOOR(1 + (zF - z0) / step_z);
                if (y0 != yF)
                    times *= FLOOR(1 + (yF - y0) / step_y);
                if (x0 != xF)
                    times *= FLOOR(1 + (xF - x0) / step_x);
                init_progress_bar(times);
            }
            else
                std::cout << "#grey_factor rot tilt psi scale z y x fitness\n";

            // Iterate
            int itime = 0;
            int step_time = CEIL((double)times / 60.0);
            Matrix1D<double> r(3);
            for (double grey_scale = grey_scale0; grey_scale <= grey_scaleF ; grey_scale += step_grey)
                for (double grey_shift = grey_shift0; grey_shift <= grey_shiftF ; grey_shift += step_grey_shift)
                    for (double rot = rot0; rot <= rotF ; rot += step_rot)
                        for (double tilt = tilt0; tilt <= tiltF ; tilt += step_tilt)
                            for (double psi = psi0; psi <= psiF ; psi += step_psi)
                                for (double scale = scale0; scale <= scaleF ; scale += step_scale)
                                    for (ZZ(r) = z0; ZZ(r) <= zF ; ZZ(r) += step_z)
                                        for (YY(r) = y0; YY(r) <= yF ; YY(r) += step_y)
                                            for (XX(r) = x0; XX(r) <= xF ; XX(r) += step_x)
                                            {
                                                // Form trial vector
                                                Matrix1D<double> trial(9);
                                                trial(0) = grey_scale;
                                                trial(1) = grey_shift;
                                                trial(2) = rot;
                                                trial(3) = tilt;
                                                trial(4) = psi;
                                                trial(5) = scale;
                                                trial(6) = ZZ(r);
                                                trial(7) = YY(r);
                                                trial(8) = XX(r);

                                                // Evaluate
                                                double fit =
                                                    fitness(MATRIX1D_ARRAY(trial));

                                                // The best?
                                                if (fit < best_fit || first)
                                                {
                                                    best_fit = fit;
                                                    best_align = trial;
                                                    first = false;
                                                }

                                                // Show fit
                                                if (tell)
                                                    std::cout << trial.transpose()
                                                    << fit << std::endl;
                                                else
                                                    if (++itime % step_time == 0)
                                                        progress_bar(itime);
                                            }
            if (!tell)
                progress_bar(times);
        }
        else
        {
            // Use Powell optimization
            Matrix1D<double> x(9), steps(9);
            double fitness;
            int iter;
            steps.initConstant(1);
            if (onlyShift)
                steps(0)=steps(1)=steps(2)=steps(3)=steps(4)=steps(5)=0;
            x(0)=(grey_scale0+grey_scaleF)/2;
            x(1)=(grey_shift0+grey_shiftF)/2;
            x(2)=(rot0+rotF)/2;
            x(3)=(tilt0+tiltF)/2;
            x(4)=(psi0+psiF)/2;
            x(5)=(scale0+scaleF)/2;
            x(6)=(z0+zF)/2;
            x(7)=(y0+yF)/2;
            x(8)=(x0+xF)/2;

            powellOptimizer(x,1,9,&wrapperFitness,NULL,0.01,fitness,iter,steps,true);
            best_align=x;
            best_fit=fitness;
            first=false;
        }

        if (!first)
            std::cout << "The best correlation is for\n"
            << "Scale                  : " << best_align(5) << std::endl
            << "Translation (X,Y,Z)    : " << best_align(8)
            << " " << best_align(7) << " " << best_align(6)
            << std::endl
            << "Rotation (rot,tilt,psi): "
            << best_align(2) << " " << best_align(3) << " "
            << best_align(4) << std::endl
            << "Best grey scale       : " << best_align(0) << std::endl
            << "Best grey shift       : " << best_align(1) << std::endl
            << "Fitness value         : " << best_fit << std::endl;
        if (apply)
        {
            applyTransformation(params.V2(),params.Vaux(),MATRIX1D_ARRAY(best_align));
            params.V2()=params.Vaux();
            params.V2.write();
        }

    }

};


int main(int argc, char **argv)
{
    ProgAlignVolumes program;
    try
    {
        program.read(argc, argv);
        program.run();
    }
    catch (XmippError xe)
    {
        std::cerr << xe;
        return 1;
    }
    return 0;
}


