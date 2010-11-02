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

#include "ctf.h"
#include "args.h"
#include "fft.h"
#include "metadata.h"

/* Read -------------------------------------------------------------------- */
void CTFDescription::read(const FileName &fn, bool disable_if_not_K)
{
    if (fn.isMetaData())
    {
        MetaData MD;
        MD.read(fn);

        if (!MD.getValue(MDL_CTF_SAMPLING_RATE,Tm))
            Tm=1;
        if (enable_CTF)
        {
            if (!MD.getValue(MDL_CTF_VOLTAGE,kV))
                kV=100;
            if (!MD.getValue(MDL_CTF_DEFOCUSU,DeltafU))
                DeltafU=0;
            if (!MD.getValue(MDL_CTF_DEFOCUSV,DeltafV))
                DeltafV=DeltafU;
            if (!MD.getValue(MDL_CTF_DEFOCUS_ANGLE,azimuthal_angle))
                azimuthal_angle=0;
            if (!MD.getValue(MDL_CTF_CS,Cs))
                Cs=0;
            if (!MD.getValue(MDL_CTF_CA,Ca))
                Ca=0;
            if (!MD.getValue(MDL_CTF_ENERGY_LOSS,espr))
                espr=0;
            if (!MD.getValue(MDL_CTF_LENS_STABILITY,ispr))
                ispr=0;
            if (!MD.getValue(MDL_CTF_CONVERGENCE_CONE,alpha))
                alpha=0;
            if (!MD.getValue(MDL_CTF_LONGITUDINAL_DISPLACEMENT,DeltaF))
                DeltaF=0;
            if (!MD.getValue(MDL_CTF_TRANSVERSAL_DISPLACEMENT,DeltaR))
                DeltaR=0;
            if (!MD.getValue(MDL_CTF_Q0,Q0))
                Q0=0;
            if (!MD.getValue(MDL_CTF_K,K))
                K=1;
            if (K == 0 && disable_if_not_K)
                enable_CTF = false;
        }
        if (enable_CTFnoise)
        {
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN_K,gaussian_K))
                gaussian_K=0;
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN_SIGMAU,sigmaU))
                sigmaU=0;
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN_SIGMAV,sigmaV))
                sigmaV=sigmaU;
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN_CU,cU))
                cU=0;
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN_CV,cV))
                cV=cU;
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN_ANGLE,gaussian_angle))
                gaussian_angle=0;
            if (!MD.getValue(MDL_CTFBG_SQRT_K,sqrt_K))
                sqrt_K=0;
            if (!MD.getValue(MDL_CTFBG_SQRT_U,sqU))
                sqU=0;
            if (!MD.getValue(MDL_CTFBG_SQRT_V,sqV))
                sqV=sqU;
            if (!MD.getValue(MDL_CTFBG_SQRT_ANGLE,sqrt_angle))
                sqrt_angle=0;
            if (!MD.getValue(MDL_CTFBG_BASELINE,base_line))
                base_line=0;
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN2_K,gaussian_K2))
                gaussian_K2=0;
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN2_SIGMAU,sigmaU2))
                sigmaU2=0;
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN2_SIGMAV,sigmaV2))
                sigmaV2=sigmaU2;
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN2_CU,cU2))
                cU2=0;
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN2_CV,cV2))
                cV2=cU2;
            if (!MD.getValue(MDL_CTFBG_GAUSSIAN2_ANGLE,gaussian_angle2))
                gaussian_angle2=0;
            if (gaussian_K == 0 && sqrt_K == 0 && base_line == 0 && gaussian_K2 == 0 &&
                disable_if_not_K)
                enable_CTFnoise = false;
        }
    }
    else
    {
        FILE *fh_param;
        if ((fh_param = fopen(fn.c_str(), "r")) == NULL)
            REPORT_ERROR(ERR_IO_NOTOPEN,
                         (std::string)"XmippCTF::read: There is a problem "
                         "opening the file " + fn);

        try
        {
            Tm = textToFloat(getParameter(fh_param, "sampling_rate", 0, "1"));
            if (enable_CTF)
            {
                DeltafU = textToFloat(getParameter(fh_param, "defocusU", 0, "0"));
                if (checkParameter(fh_param, "defocusV"))
                    DeltafV = textToFloat(getParameter(fh_param, "defocusV", 0));
                else DeltafV = DeltafU;
                azimuthal_angle = textToFloat(getParameter(fh_param, "azimuthal_angle", 0, "0"));
                kV = textToFloat(getParameter(fh_param, "voltage", 0, "100"));
                Cs = textToFloat(getParameter(fh_param, "spherical_aberration", 0, "0"));
                Ca = textToFloat(getParameter(fh_param, "chromatic_aberration", 0, "0"));
                espr = textToFloat(getParameter(fh_param, "energy_loss", 0, "0"));
                ispr = textToFloat(getParameter(fh_param, "lens_stability", 0, "0"));
                alpha = textToFloat(getParameter(fh_param, "convergence_cone", 0, "0"));
                DeltaF = textToFloat(getParameter(fh_param, "longitudinal_displace", 0, "0"));
                DeltaR = textToFloat(getParameter(fh_param, "transversal_displace", 0, "0"));
                Q0 = textToFloat(getParameter(fh_param, "Q0", 0, "0"));
                K = textToFloat(getParameter(fh_param, "K", 0, "1"));
                if (K == 0 && disable_if_not_K) enable_CTF = false;
            }

            if (enable_CTFnoise)
            {
                base_line     = textToFloat(getParameter(fh_param, "base_line", 0, "0"));

                gaussian_K    = textToFloat(getParameter(fh_param, "gaussian_K", 0, "0"));
                sigmaU        = textToFloat(getParameter(fh_param, "sigmaU", 0, "0"));
                if (checkParameter(fh_param, "sigmaV"))
                    sigmaV     = textToFloat(getParameter(fh_param, "sigmaV", 0));
                else sigmaV   = sigmaU;
                cU            = textToFloat(getParameter(fh_param, "cU", 0, "0"));
                if (checkParameter(fh_param, "cV"))
                    cV         = textToFloat(getParameter(fh_param, "cV", 0));
                else cV       = cU;
                gaussian_angle = textToFloat(getParameter(fh_param, "gaussian_angle", 0, "0"));

                sqU           = textToFloat(getParameter(fh_param, "sqU", 0, "0"));
                if (checkParameter(fh_param, "sqV"))
                    sqV        = textToFloat(getParameter(fh_param, "sqV", 0));
                else sqV      = sqU;
                sqrt_angle = textToFloat(getParameter(fh_param, "sqrt_angle", 0, "0"));
                sqrt_K        = textToFloat(getParameter(fh_param, "sqrt_K", 0, "0"));

                gaussian_K2    = textToFloat(getParameter(fh_param, "gaussian_K2", 0, "0"));
                sigmaU2        = textToFloat(getParameter(fh_param, "sigmaU2", 0, "0"));
                if (checkParameter(fh_param, "sigmaV2"))
                    sigmaV2     = textToFloat(getParameter(fh_param, "sigmaV2", 0));
                else sigmaV2   = sigmaU2;
                cU2            = textToFloat(getParameter(fh_param, "cU2", 0, "0"));
                if (checkParameter(fh_param, "cV2"))
                    cV2         = textToFloat(getParameter(fh_param, "cV2", 0));
                else cV2       = cU2;
                gaussian_angle2 = textToFloat(getParameter(fh_param, "gaussian_angle2", 0, "0"));

                if (gaussian_K == 0 && sqrt_K == 0 && base_line == 0 && gaussian_K2 == 0 &&
                    disable_if_not_K)
                    enable_CTFnoise = false;
            }

        }
        catch (XmippError XE)
        {
            std::cout << XE << std::endl;
            REPORT_ERROR(ERR_IO_NOREAD, (std::string)"There is an error reading " + fn);
        }
        fclose(fh_param);
    }
}

/* Write ------------------------------------------------------------------- */
void CTFDescription::write(const FileName &fn)
{
    MetaData MD;
    MD.setColumnFormat(false);
    MD.addObject();
    MD.setValue(MDL_CTF_SAMPLING_RATE,Tm);
    if (enable_CTF)
    {
        MD.setValue(MDL_CTF_VOLTAGE,kV);
        MD.setValue(MDL_CTF_DEFOCUSU,DeltafU);
        MD.setValue(MDL_CTF_DEFOCUSV,DeltafV);
        MD.setValue(MDL_CTF_DEFOCUS_ANGLE,azimuthal_angle);
        MD.setValue(MDL_CTF_CS,Cs);
        MD.setValue(MDL_CTF_CA,Ca);
        MD.setValue(MDL_CTF_ENERGY_LOSS,espr);
        MD.setValue(MDL_CTF_LENS_STABILITY,ispr);
        MD.setValue(MDL_CTF_CONVERGENCE_CONE,alpha);
        MD.setValue(MDL_CTF_LONGITUDINAL_DISPLACEMENT,DeltaF);
        MD.setValue(MDL_CTF_TRANSVERSAL_DISPLACEMENT,DeltaR);
        MD.setValue(MDL_CTF_Q0,Q0);
        MD.setValue(MDL_CTF_K,K);
    }
    if (enable_CTFnoise)
    {
        MD.setValue(MDL_CTFBG_GAUSSIAN_K,gaussian_K);
        MD.setValue(MDL_CTFBG_GAUSSIAN_SIGMAU,sigmaU);
        MD.setValue(MDL_CTFBG_GAUSSIAN_SIGMAV,sigmaV);
        MD.setValue(MDL_CTFBG_GAUSSIAN_CU,cU);
        MD.setValue(MDL_CTFBG_GAUSSIAN_CV,cV);
        MD.setValue(MDL_CTFBG_GAUSSIAN_ANGLE,gaussian_angle);
        MD.setValue(MDL_CTFBG_SQRT_K,sqrt_K);
        MD.setValue(MDL_CTFBG_SQRT_U,sqU);
        MD.setValue(MDL_CTFBG_SQRT_V,sqV);
        MD.setValue(MDL_CTFBG_SQRT_ANGLE,sqrt_angle);
        MD.setValue(MDL_CTFBG_BASELINE,base_line);
        MD.setValue(MDL_CTFBG_GAUSSIAN2_K,gaussian_K2);
        MD.setValue(MDL_CTFBG_GAUSSIAN2_SIGMAU,sigmaU2);
        MD.setValue(MDL_CTFBG_GAUSSIAN2_SIGMAV,sigmaV2);
        MD.setValue(MDL_CTFBG_GAUSSIAN2_CU,cU2);
        MD.setValue(MDL_CTFBG_GAUSSIAN2_CV,cV2);
        MD.setValue(MDL_CTFBG_GAUSSIAN2_ANGLE,gaussian_angle2);
    }

    MD.write(fn);
}

/* Usage ------------------------------------------------------------------- */
void CTFDescription::Usage()
{
    std::cerr << "  [defocusU=<DeltafU>]              : Defocus in Angstroms (Ex: -800)\n"
    << "  [defocusV=<DeltafV=DeltafU>]      : If astigmatism\n"
    << "  [azimuthal_angle=<ang=0>]         : Angle between X and U (degrees)\n"
    << "  [sampling_rate=<Tm=1>]            : Angstroms/pixel\n"
    << "  [voltage=<kV=100>]                : Accelerating voltage (kV)\n"
    << "  [spherical_aberration=<Cs=0>]     : Milimiters. Ex: 5.6\n"
    << "  [chromatic_aberration=<Ca=0>]     : Milimiters. Ex: 2\n"
    << "  [energy_loss=<espr=0>]            : eV. Ex: 1\n"
    << "  [lens_stability=<ispr=0>]         : ppm. Ex: 1\n"
    << "  [convergence_cone=<alpha=0>]      : mrad. Ex: 0.5\n"
    << "  [longitudinal_displace=<DeltaF=0>]: Angstrom. Ex: 100\n"
    << "  [transversal_displace=<DeltaR=0>] : Angstrom. Ex: 3\n"
    << "  [Q0=<Q0=0>]                       : Percentage of cosine\n"
    << "  [K=<K=1>]                         : Global gain\n"
    << std::endl
    << "  [base_line=<b=0>]                 : Global base line\n"
    << "  [gaussian_K=<K=0>]                : Gaussian gain\n"
    << "  [sigmaU=<s=0>]                    : Gaussian width\n"
    << "  [sigmaV=<s=0>]                    : if astigmatism\n"
    << "  [cU=<s=0>]                        : Gaussian center (in cont. freq)\n"
    << "  [cV=<s=0>]                        : if astigmatism\n"
    << "  [gaussian_angle=<ang=0>]          : Angle between X and U (degrees)\n"
    << "  [sqrt_K=<K=0>]                    : Square root gain\n"
    << "  [sqU=<sqU=0>]                     : Square root width\n"
    << "  [sqV=<sqV=0>]                     : if astigmatism\n"
    << "  [sqrt_angle=<ang=0>]              : Angle between X and U (degrees)\n"
    << "  [gaussian_K2=<K=0>]               : Second Gaussian gain\n"
    << "  [sigmaU2=<s=0>]                   : Second Gaussian width\n"
    << "  [sigmaV2=<s=0>]                   : if astigmatism\n"
    << "  [cU2=<s=0>]                       : Second Gaussian center (in cont. freq)\n"
    << "  [cV2=<s=0>]                       : if astigmatism\n"
    << "  [gaussian_angle2=<ang=0>]         : Angle between X and U (degrees)\n"
    ;
}

/* Show -------------------------------------------------------------------- */
std::ostream & operator << (std::ostream &out, const CTFDescription &ctf)
{
    if (ctf.enable_CTF)
    {
        out << "sampling_rate=        " << ctf.Tm              << std::endl
        << "voltage=              " << ctf.kV              << std::endl
        << "defocusU=             " << ctf.DeltafU         << std::endl
        << "defocusV=             " << ctf.DeltafV         << std::endl
        << "azimuthal_angle=      " << ctf.azimuthal_angle << std::endl
        << "spherical_aberration= " << ctf.Cs              << std::endl
        << "chromatic_aberration= " << ctf.Ca              << std::endl
        << "energy_loss=          " << ctf.espr            << std::endl
        << "lens_stability=       " << ctf.ispr            << std::endl
        << "convergence_cone=     " << ctf.alpha           << std::endl
        << "longitudinal_displace=" << ctf.DeltaF          << std::endl
        << "transversal_displace= " << ctf.DeltaR          << std::endl
        << "Q0=                   " << ctf.Q0              << std::endl
        << "K=                    " << ctf.K               << std::endl
        ;
    }
    if (ctf.enable_CTFnoise)
    {
        out << "gaussian_K=           " << ctf.gaussian_K      << std::endl
        << "sigmaU=               " << ctf.sigmaU          << std::endl
        << "sigmaV=               " << ctf.sigmaV          << std::endl
        << "cU=                   " << ctf.cU              << std::endl
        << "cV=                   " << ctf.cV              << std::endl
        << "gaussian_angle=       " << ctf.gaussian_angle  << std::endl
        << "sqrt_K=               " << ctf.sqrt_K          << std::endl
        << "sqU=                  " << ctf.sqU             << std::endl
        << "sqV=                  " << ctf.sqV             << std::endl
        << "sqrt_angle=           " << ctf.sqrt_angle      << std::endl
        << "base_line=            " << ctf.base_line       << std::endl
        << "gaussian_K2=          " << ctf.gaussian_K2     << std::endl
        << "sigmaU2=              " << ctf.sigmaU2         << std::endl
        << "sigmaV2=              " << ctf.sigmaV2         << std::endl
        << "cU2=                  " << ctf.cU2             << std::endl
        << "cV2=                  " << ctf.cV2             << std::endl
        << "gaussian_angle2=      " << ctf.gaussian_angle2 << std::endl
        ;
    }
    return out;
}

/* Default values ---------------------------------------------------------- */
void CTFDescription::clear()
{
    enable_CTF = true;
    enable_CTFnoise = false;
    clear_noise();
    clear_pure_ctf();
}

void CTFDescription::clear_noise()
{
    base_line = 0;
    cU = cV = sigmaU = sigmaV = gaussian_angle = gaussian_K = 0;
    sqU = sqV = sqrt_K = sqrt_angle = 0;
    cU2 = cV2 = sigmaU2 = sigmaV2 = gaussian_angle2 = gaussian_K2 = 0;
}

void CTFDescription::clear_pure_ctf()
{
    enable_CTF = true;
    enable_CTFnoise = false;
    Tm = 2;
    kV = 100;
    DeltafU = DeltafV = azimuthal_angle = 0;
    Cs = Ca = espr = ispr = alpha = DeltaF = DeltaR = 0;
    K = 1;
    Q0 = 0;
}

/* Produce Side Information ------------------------------------------------ */
void CTFDescription::Produce_Side_Info()
{
    // Change units
    double local_alpha = alpha / 1000;
    double local_Cs = Cs * 1e7;
    double local_Ca = Ca * 1e7;
    double local_kV = kV * 1e3;
    double local_ispr = ispr * 1e6;
    rad_azimuth = DEG2RAD(azimuthal_angle);
    rad_gaussian = DEG2RAD(gaussian_angle);
    rad_gaussian2 = DEG2RAD(gaussian_angle2);
    rad_sqrt = DEG2RAD(sqrt_angle);

    // lambda=h/sqrt(2*m*e*kV)
    //    h: Planck constant
    //    m: electron mass
    //    e: electron charge
    // lambda=0.387832/sqrt(kV*(1.+0.000978466*kV)); // Hewz: Angstroms
    lambda = 12.3 / sqrt(local_kV * (1. + local_kV * 1e-6)); // ICE

    // Phase shift for spherical aberration
    // X(u)=-PI*deltaf(u)*lambda*u^2+PI/2*Cs*lambda^3*u^4
    // ICE: X(u)=-PI/2*deltaf(u)*lambda*u^2+PI/2*Cs*lambda^3*u^4
    //          = K1*deltaf(u)*u^2         +K2*u^4
    K1 = PI / 2 * 2 * lambda;
    K2 = PI / 2 * local_Cs * lambda * lambda * lambda;

    // Envelope
    // D(u)=Ed(u)*Ealpha(u)
    // Ed(u)=exp(-1/2*PI^2*lambda^2*D^2*u^4)
    // Ealpha(u)=exp(-PI^2*alpha^2*u^2*(Cs*lambda^2*u^2+Deltaf(u))^2)
    // ICE: Eespr(u)=exp(-(1/4*PI*Ca*lambda*espr/kV)^2*u^4/log2)
    // ICE: Eispr(u)=exp(-(1/2*PI*Ca*lambda*ispr)^2*u^4/log2)
    // ICE: EdeltaF(u)=bessj0(PI*DeltaF*lambda*u^2)
    // ICE: EdeltaR(u)=sinc(u*DeltaR)
    // ICE: Ealpha(u)=exp(-PI^2*alpha^2*(Cs*lambda^2*u^3+Deltaf(u)*u)^2)
    // CO: K3=pow(0.25*PI*Ca*lambda*(espr/kV,2)/log(2); Both combines in new K3
    // CO: K4=pow(0.5*PI*Ca*lambda*ispr,2)/log(2);
    K3 = pow(0.25 * PI * local_Ca * lambda * (espr / kV + 2 * local_ispr), 2) / log(2.0);
    K5 = PI * DeltaF * lambda;
    K6 = PI * PI * alpha * alpha;
    K7 = local_Cs * lambda * lambda;
}

/* Zero -------------------------------------------------------------------- */
//#define DEBUG
void CTFDescription::zero(int n, const Matrix1D<double> &u, Matrix1D<double> &freq)
{
    double wmax = 1 / (2 * Tm);
    double wstep = wmax / 300;
    int sign_changes = 0;
    precomputeValues(0,0);
    double last_ctf = CTF_at(), ctf;
    double w;
    for (w = 0; w <= wmax; w += wstep)
    {
        V2_BY_CT(freq, u, w);
        precomputeValues(XX(freq), YY(freq));
        ctf = CTFpure_at();
        if (SGN(ctf) != SGN(last_ctf))
        {
            sign_changes++;
            if (sign_changes == n)
                break;
        }
        last_ctf = ctf;
    }
    if (sign_changes != n)
    {
        VECTOR_R2(freq, -1, -1);
    }
    else
    {
        // Compute more accurate zero
#ifdef DEBUG
        std::cout << n << " zero: w=" << w << " (" << wmax << ") freq="
        << (u*w).transpose()
        << " last_ctf=" << last_ctf << " ctf=" << ctf << " ";
#endif

        w += ctf * wstep / (last_ctf - ctf);
        V2_BY_CT(freq, u, w);
#ifdef DEBUG

        std::cout << " final w= " << w << " final freq=" << freq.transpose() << std::endl;
#endif

    }
}
#undef DEBUG

/* Apply the CTF to an image ----------------------------------------------- */
void CTFDescription::Apply_CTF(MultidimArray < std::complex<double> > &FFTI)
{
    Matrix1D<int>    idx(2);
    Matrix1D<double> freq(2);
    if ( ZSIZE(FFTI) > 1 )
        REPORT_ERROR(ERR_MULTIDIM_DIM,"ERROR: Apply_CTF only works on 2D images, not 3D.");

    FOR_ALL_ELEMENTS_IN_ARRAY2D(FFTI)
    {
        XX(idx) = j;
        YY(idx) = i;
        FFT_idx2digfreq(FFTI, idx, freq);
        precomputeValues(XX(freq), YY(freq));
        double ctf = CTF_at();
        FFTI(i, j) *= ctf;
    }
}

/* Generate CTF Image ------------------------------------------------------ */
//#define DEBUG
void CTFDescription::Generate_CTF(int Ydim, int Xdim,
                            MultidimArray < std::complex<double> > &CTF)
{
    Matrix1D<int>    idx(2);
    Matrix1D<double> freq(2);
    CTF.resize(Ydim, Xdim);
#ifdef DEBUG

    std::cout << "CTF:\n" << *this << std::endl;
#endif

    FOR_ALL_ELEMENTS_IN_ARRAY2D(CTF)
    {
        XX(idx) = j;
        YY(idx) = i;
        FFT_idx2digfreq(CTF, idx, freq);
        digfreq2contfreq(freq, freq, Tm);
        precomputeValues(XX(freq), YY(freq));
        CTF(i, j) = CTF_at();
#ifdef DEBUG

        if (i == 0)
            std::cout << i << " " << j << " " << YY(freq) << " " << XX(freq)
            << " " << CTF(i, j) << std::endl;
#endif

    }
}
#undef DEBUG

/* Physical meaning -------------------------------------------------------- */
//#define DEBUG
bool CTFDescription::physical_meaning()
{
    bool retval;
    if (enable_CTF)
    {
    	precomputeValues(0,0);
        retval =
            K >= 0       && base_line >= 0  &&
            kV >= 50     && kV <= 1000      &&
            espr >= 0    && espr <= 20      &&
            ispr >= 0    && ispr <= 20      &&
            Cs >= 0      && Cs <= 20        &&
            Ca >= 0      && Ca <= 3         &&
            alpha >= 0   && alpha <= 5      &&
            DeltaF >= 0  && DeltaF <= 1000  &&
            DeltaR >= 0  && DeltaR <= 100   &&
            Q0 >= -0.40  && Q0 <= 0         &&
            DeltafU <= 0 && DeltafV <= 0    &&
            CTF_at() >= 0;
#ifdef DEBUG

        if (retval == false)
        {
            std::cout << *this << std::endl;
            std::cout << "K>=0       && base_line>=0  " << (K >= 0       && base_line >= 0) << std::endl
            << "kV>=50     && kV<=1000      " << (kV >= 50     && kV <= 1000)     << std::endl
            << "espr>=0    && espr<=20      " << (espr >= 0    && espr <= 20)     << std::endl
            << "ispr>=0    && ispr<=20      " << (ispr >= 0    && ispr <= 20)     << std::endl
            << "Cs>=0      && Cs<=20        " << (Cs >= 0      && Cs <= 20)       << std::endl
            << "Ca>=0      && Ca<=3         " << (Ca >= 0      && Ca <= 3)        << std::endl
            << "alpha>=0   && alpha<=5      " << (alpha >= 0   && alpha <= 5)     << std::endl
            << "DeltaF>=0  && DeltaF<=1000  " << (DeltaF >= 0  && DeltaF <= 1000) << std::endl
            << "DeltaR>=0  && DeltaR<=100   " << (DeltaR >= 0  && DeltaR <= 100)  << std::endl
            << "Q0>=-0.40  && Q0<=0       " << (Q0 >= -0.40  && Q0 <= 0)          << std::endl
            << "DeltafU<=0 && DeltafV<=0    " << (DeltafU <= 0 && DeltafV <= 0)   << std::endl
            << "CTF_at(0,0)>=0       " << (CTF_at(0, 0) >= 0)         << std::endl
            ;
            std::cout << "CTF_at(0,0)=" << CTF_at(0, 0, true) << std::endl;
        }
#endif

    }
    else
        retval = true;
    bool retval2;
    if (enable_CTFnoise)
    {
        double min_sigma = XMIPP_MIN(sigmaU, sigmaV);
        double min_c = XMIPP_MIN(cU, cV);
        double min_sigma2 = XMIPP_MIN(sigmaU2, sigmaV2);
        double min_c2 = XMIPP_MIN(cU2, cV2);
        retval2 =
            base_line >= 0       &&
            gaussian_K >= 0      &&
            sigmaU >= 0          && sigmaV >= 0          &&
            sigmaU <= 100e3      && sigmaV <= 100e3       &&
            cU >= 0              && cV >= 0              &&
            sqU >= 0             && sqV >= 0             &&
            sqrt_K >= 0          &&
            gaussian_K2 >= 0     &&
            sigmaU2 >= 0         && sigmaV2 >= 0          &&
            sigmaU2 <= 100e3     && sigmaV2 <= 100e3      &&
            cU2 >= 0             && cV2 >= 0              &&
            gaussian_angle >= 0  && gaussian_angle <= 90  &&
            sqrt_angle >= 0      && sqrt_angle <= 90      &&
            gaussian_angle2 >= 0 && gaussian_angle2 <= 90
            ;
        if (min_sigma > 0)
            retval2 = retval2 && ABS(sigmaU - sigmaV) / min_sigma <= 3;
        if (min_c > 0)
            retval2 = retval2 && ABS(cU - cV) / min_c <= 3;
        if (gaussian_K != 0)
            retval2 = retval2 && (cU * Tm >= 0.01) && (cV * Tm >= 0.01);
        if (min_sigma2 > 0)
            retval2 = retval2 && ABS(sigmaU2 - sigmaV2) / min_sigma2 <= 3;
        if (min_c2 > 0)
            retval2 = retval2 && ABS(cU2 - cV2) / min_c2 <= 3;
        if (gaussian_K2 != 0)
            retval2 = retval2 && (cU2 * Tm >= 0.01) && (cV2 * Tm >= 0.01);
#ifdef DEBUG

        if (retval2 == false)
        {
            std::cout << *this << std::endl;
            std::cout << "base_line>=0       &&        " << (base_line >= 0)           << std::endl
            << "gaussian_K>=0      &&        " << (gaussian_K >= 0)    << std::endl
            << "sigmaU>=0      && sigmaV>=0     " << (sigmaU >= 0      && sigmaV >= 0)   << std::endl
            << "sigmaU<=100e3      && sigmaV<=100e3       " << (sigmaU <= 100e3      && sigmaV <= 100e3)   << std::endl
            << "cU>=0       && cV>=0      " << (cU >= 0       && cV >= 0)   << std::endl
            << "sqU>=0      && sqV>=0      " << (sqU >= 0        && sqV >= 0)   << std::endl
            << "sqrt_K>=0      &&        " << (sqrt_K >= 0)     << std::endl
            << "gaussian_K2>=0     &&        " << (gaussian_K2 >= 0)    << std::endl
            << "sigmaU2>=0      && sigmaV2>=0     " << (sigmaU2 >= 0      && sigmaV2 >= 0)   << std::endl
            << "sigmaU2<=100e3     && sigmaV2<=100e3      " << (sigmaU2 <= 100e3     && sigmaV2 <= 100e3)  << std::endl
            << "cU2>=0       && cV2>=0      " << (cU2 >= 0      && cV2 >= 0)  << std::endl
            << "gaussian_angle>=0  && gaussian_angle<=90  " << (gaussian_angle >= 0  && gaussian_angle <= 90)  << std::endl
            << "sqrt_angle>=0      && sqrt_angle<=90      " << (sqrt_angle >= 0      && sqrt_angle <= 90)      << std::endl
            << "gaussian_angle2>=0 && gaussian_angle2<=90 " << (gaussian_angle2 >= 0 && gaussian_angle2 <= 90) << std::endl
            ;
            if (min_sigma > 0)
                std::cout << "ABS(sigmaU-sigmaV)/min_sigma<=3         " << (ABS(sigmaU - sigmaV) / min_sigma <= 3)     << std::endl;
            if (min_c > 0)
                std::cout << "ABS(cU-cV)/min_c<=3                     " << (ABS(cU - cV) / min_c <= 3)                 << std::endl;
            if (gaussian_K > 0)
                std::cout << "(cU*Tm>=0.01) && (cV*Tm>=0.01)          " << ((cU*Tm >= 0.01) && (cV*Tm >= 0.01))      << std::endl;
            if (min_sigma2 > 0)
                std::cout << "ABS(sigmaU2-sigmaV2)/min_sigma2<=3      " << (ABS(sigmaU2 - sigmaV2) / min_sigma2 <= 3)  << std::endl;
            if (min_c2 > 0)
                std::cout << "ABS(cU2-cV2)/min_c2<=3                  " << (ABS(cU2 - cV2) / min_c2 <= 3)              << std::endl;
            if (gaussian_K2 > 0)
                std::cout << "(cU2*Tm>=0.01) && (cV2*Tm>=0.01)        " << ((cU2*Tm >= 0.01) && (cV2*Tm >= 0.01))    << std::endl;
            std::cout << cV2*Tm << std::endl;
        }
#endif

    }
    else
        retval2 = true;
#ifdef DEBUG
    // std::cout << "Retval= " << retval << " retval2= " << retval2 << std::endl;
#endif

    return retval && retval2;
}
#undef DEBUG

/* Force Physical meaning -------------------------------------------------- */
void CTFDescription::force_physical_meaning()
{
    if (enable_CTF)
    {
        if (K < 0)
            K = 0;
        if (base_line < 0)
            base_line = 0;
        if (kV < 50)
            kV = 50;
        if (kV > 1000)
            kV = 1000;
        if (espr < 0)
            espr = 0;
        if (espr > 20)
            espr = 20;
        if (ispr < 0)
            ispr = 0;
        if (ispr > 20)
            ispr = 20;
        if (Cs < 0)
            Cs = 0;
        if (Cs > 20)
            Cs = 20;
        if (Ca < 0)
            Ca = 0;
        if (Ca > 3)
            Ca = 3;
        if (alpha < 0)
            alpha = 0;
        if (alpha > 5)
            alpha = 5;
        if (DeltaF < 0)
            DeltaF = 0;
        if (DeltaF > 1000)
            DeltaF = 1000;
        if (DeltaR < 0)
            DeltaR = 0;
        if (DeltaR > 1000)
            DeltaR = 1000;
        if (Q0 < -0.40)
            Q0 = -0.40;
        if (Q0 > 0)
            Q0 = 0;
        if (DeltafU > 0)
            DeltafU = 0;
        if (DeltafV > 0)
            DeltafV = 0;
    }
    if (enable_CTFnoise)
    {
        double min_sigma = XMIPP_MIN(sigmaU, sigmaV);
        double min_c = XMIPP_MIN(cU, cV);
        double min_sigma2 = XMIPP_MIN(sigmaU2, sigmaV2);
        double min_c2 = XMIPP_MIN(cU2, cV2);
        if (base_line < 0)
            base_line = 0;
        if (gaussian_K < 0)
            gaussian_K = 0;
        if (sigmaU < 0)
            sigmaU = 0;
        if (sigmaV < 0)
            sigmaV = 0;
        if (sigmaU > 100e3)
            sigmaU = 100e3;
        if (sigmaV > 100e3)
            sigmaV = 100e3;
        if (cU < 0)
            cU = 0;
        if (cV < 0)
            cV = 0;
        if (sqU < 0)
            sqU = 0;
        if (sqV < 0)
            sqV = 0;
        if (sqrt_K < 0)
            sqrt_K = 0;
        if (gaussian_K2 < 0)
            gaussian_K2 = 0;
        if (sigmaU2 < 0)
            sigmaU2 = 0;
        if (sigmaV2 < 0)
            sigmaV2 = 0;
        if (sigmaU2 > 100e3)
            sigmaU2 = 100e3;
        if (sigmaV2 > 100e3)
            sigmaV2 = 100e3;
        if (cU2 < 0)
            cU2 = 0;
        if (cV2 < 0)
            cV2 = 0;
        if (gaussian_angle < 0)
            gaussian_angle = 0;
        if (gaussian_angle > 90)
            gaussian_angle = 90;
        if (sqrt_angle < 0)
            sqrt_angle = 0;
        if (sqrt_angle > 90)
            sqrt_angle = 90;
        if (gaussian_angle2 < 0)
            gaussian_angle2 = 0;
        if (gaussian_angle2 > 90)
            gaussian_angle2 = 90;
        if (min_sigma > 0)
            if (ABS(sigmaU - sigmaV) / min_sigma > 3)
            {
                if (sigmaU < sigmaV)
                    sigmaV = 3.9 * sigmaU;
                else
                    sigmaU = 3.9 * sigmaV;
            }
        if (min_c > 0)
            if (ABS(cU - cV) / min_c > 3)
            {
                if (cU < cV)
                    cV = 3.9 * cU;
                else
                    cU = 3.9 * cV;
            }
        if (gaussian_K != 0)
        {
            if (cU*Tm < 0.01)
                cU = 0.011 / Tm;
            if (cV*Tm < 0.01)
                cV = 0.011 / Tm;
        }
        if (min_sigma2 > 0)
            if (ABS(sigmaU2 - sigmaV2) / min_sigma2 > 3)
            {
                if (sigmaU2 < sigmaV2)
                    sigmaV2 = 3.9 * sigmaU2;
                else
                    sigmaU2 = 3.9 * sigmaV2;
            }
        if (min_c2 > 0)
            if (ABS(cU2 - cV2) / min_c2 > 3)
            {
                if (cU2 < cV2)
                    cV2 = 3.9 * cU2;
                else
                    cU2 = 3.9 * cV2;
            }
        if (gaussian_K2 != 0)
        {
            if (cU2*Tm < 0.01)
                cU2 = 0.011 / Tm;
            if (cV2*Tm < 0.01)
                cV2 = 0.011 / Tm;
        }
    }
}
#undef DEBUG
