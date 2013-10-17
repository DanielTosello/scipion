#!/bin/sh
set -x #uncomment for debugging

#############################
# XMIPP installation script #
# --------------------------################################################################################
# X-Window-based Microscopy Image Processing Package                                                       #
# Xmipp is a suite of image processing programs, primarily aimed at single-particle 3D electron microscopy #
# More info: http://xmipp.cnb.csic.es - xmipp@cnb.csic.es                                                  #
# Instruct Image Processing Center - National Center of Biotechnology - CSIC. Spain                        #
############################################################################################################


##################################################################################
#################### DEFINITIONS #################################################
##################################################################################

#Some interface definitions
TITLE="Xmipp installation script"
BLACK='\033[30m'
WHITE='\033[37m'
YELLOW='\033[33m'
RED='\033[31m'
BLUE='\033[36m'
GREEN="\033[32m"
RED="\033[31m"
ENDC="\033[0m"

export NUMBER_OF_CPU=1
GLOB_STATE=0
GLOB_COMMAND=""
TIMESTAMP=""
EXT_PATH=
BUILD_PATH=
OS_TYPE=$(uname)
IS_MAC=false
IS_CYGWIN=false
IS_MINGW=false
IS_LINUX=false

#Some flags variables
DO_UNTAR=true
DO_CLTOMO=false #for scipy
CONFIGURE_ARGS=""
COMPILE_ARGS=""
GUI_ARGS="gui"

DO_CLEAN=true
DO_STATIC=false
DO_UPDATE=false
DO_SETUP=true
DO_GUI=true
DO_UNATTENDED=false


#External libraries definitions
VALGLIB=3.8.0
ALGLIB_FOLDER="alglib-${VALGLIB}.cpp"
ALGLIB_TAR="${ALGLIB_FOLDER}.tgz"
DO_ALGLIB=false

VBILIB=0.0
BILIB_FOLDER=bilib
BILIB_TAR="${BILIB_FOLDER}.tgz"
DO_BILIB=false

VCONDOR=0.0
CONDOR_FOLDER=condor
CONDOR_TAR="${CONDOR_FOLDER}"
DO_CONDOR=false

VFFTW=3.3.3
FFTW_FOLDER="fftw-${VFFTW}"
FFTW_TAR="${FFTW_FOLDER}.tgz"
DO_FFTW=false

VGTEST=1.6.0
GTEST_FOLDER="gtest-${VGTEST}"
GTEST_TAR="${GTEST_FOLDER}.tgz"
DO_GTEST=false

VHDF5=1.8.10
HDF5_FOLDER="hdf5-${VHDF5}"
HDF5_TAR="${HDF5_FOLDER}.tgz"
DO_HDF5=false

VIMAGEJ=1.45g
IMAGEJ_FOLDER="imagej"
IMAGEJ_TAR="${IMAGEJ_FOLDER}.tgz"
DO_IMAGEJ=false

VJPEG=8c
JPEG_FOLDER="jpeg-${VJPEG}"
JPEG_TAR="jpegsrc.v${VJPEG}.tgz"
DO_JPEG=false

VSCONS=1.2.0
SCONS_FOLDER="scons"
SCONS_TAR="${SCONS_FOLDER}.tgz"
DO_JPEG=false

VSQLITE=3.6.23
SQLITE_FOLDER="sqlite-${VSQLITE}"
SQLITE_TAR="${SQLITE_FOLDER}.tgz"
SQLITE_EXT_FOLDER=sqliteExt
DO_SQLITE=false

VTIFF=3.9.4
TIFF_FOLDER="tiff-${VTIFF}"
TIFF_TAR="${TIFF_FOLDER}.tgz"
DO_TIFF=false

VNMA=0.0
NMA_FOLDER="NMA"
NMA_TAR="${NMA_FOLDER}.tgz"
DO_NMA=false

VSHALIGNMENT=0.0
SHALIGNMENT_FOLDER="sh_alignment"
SHALIGNMENT_TAR="${SH_ALIGNMENT_FOLDER}.tgz"
DO_SHALIGNMENT=false

#External libraries arrays. For adding a new external library, define his decompress folder and tar names, put them in EXTERNAL_LIBRARIES and EXTERNAL_LIBRARIES_FILES arrays, and put 1 whther it has to be installed by default, 0 otherwise in EXTERNAL_LIBRARIES_DEFAULT in the appropiate positions.
EXTERNAL_LIBRARIES=(         $ALGLIB_FOLDER $BILIB_FOLDER $CONDOR_FOLDER $FFTW_FOLDER $GTEST_FOLDER $HDF5_FOLDER $IMAGEJ_FOLDER $JPEG_FOLDER $SCONS_FOLDER $SQLITE_FOLDER $TIFF_FOLDER $NMA_FOLDER $SHALIGNMENT_FOLDER )
EXTERNAL_LIBRARIES_FILES=(   $ALGLIB_TAR    $BILIB_TAR    $CONDOR_TAR    $FFTW_TAR    $GTEST_TAR    $HDF5_TAR    $IMAGEJ_TAR    $JPEG_TAR    $SCONS_TAR    $SQLITE_TAR    $TIFF_TAR    $NMA_TAR    $SHALIGNMENT_TAR    )
EXTERNAL_LIBRARIES_DO=     ( $DO_ALGLIB     $DO_BILIB     $DO_CONDOR     $DO_FFTW     $DO_GTEST     $DO_HDF5     $DO_IMAGEJ     $DO_JPEG     $DO_SCONS     $DO_SQLITE     $DO_TIFF     $DO_NMA     $DO_SHALIGNMENT     )
EXTERNAL_LIBRARIES_DEFAULT=(        1             1              1            1             1            1              1            1             1              1            1           0                   0       )


#Python definitions
#Python
VPYTHON=2.7.2
PYTHON_FOLDER="Python-${VPYTHON}"
PYTHON_TAR="${PYTHON_FOLDER}.tgz"
DO_PYTHON=false

#Python modules
DO_PYMOD=true

VMATLIBPLOT=1.1.0
MATLIBPLOT_FOLDER="matplotlib-${VMATLIBPLOT}"
MATLIBPLOT_TAR="${MATLIBPLOT_FOLDER}.tgz"
DO_MATLIBPLOT=false

VPYMPI=1.2.2
PYMPI_FOLDER="mpi4py-${VPYMPI}"
PYMPI_TAR="${PYMPI_FOLDER}.tgz"
DO_PYMPI=false

VNUMPY=1.6.1
NUMPY_FOLDER="numpy-${VNUMPY}"
NUMPY_TAR="${NUMPY_FOLDER}.tgz"
DO_NUMPY=false

VSCIPY=0.12.0
SCIPY_FOLDER="scipy-${VSCIPY}"
SCIPY_TAR="${SCIPY_FOLDER}.tgz"
DO_SCIPY=false

VTCLTK=8.5.10
DO_TCLTK=true
TCL_FOLDER="tcl${$VTCLTK}"
TCL_TAR="${TCL_FOLDER}.tgz"
DO_TCL=false
TK_FOLDER="tk${$VTCLTK}"
TK_TAR="${TK_FOLDER}.tgz"
DO_TK=false

#Python modules arrays. For adding a new python module, define his decompress folder and tar names, put them in PYTHON_MODULES and PYTHON_MODULES_FILES arrays, and put 1 whther it has to be installed by default, 0 otherwise in PYTHON_MODULES_DEFAULT in the appropiate position.
PYTHON_MODULES=(        $MATLIBPLOT_FOLDER $PYMPI_FOLDER $NUMPY_FOLDER $SCIPY_FOLDER $TCL_FOLDER $TK_FOLDER )
PYTHON_MODULES_FILES=(  $MATLIBPLOT_TAR    $PYMPI_TAR    $NUMPY_TAR    $SCIPY_TAR    $TCL_TAR    $TK_TAR    )
PYTHON_MODULES_DO=(     $DO_MATLIBPLOT     $DO_PYMPI     $DO_NUMPY     $DO_SCIPY     $DO_TCL     $DO_TK     )
PYTHON_MODULES_DEFAULT=(           1             1             1             0           1          1       )


##################################################################################
#################### FUNCTIONS ###################################################
##################################################################################

decideOS()
{
  echo "The OS is $OS_TYPE"
  case "$OS_TYPE" in
    Darwin)
      IS_MAC=true
      CONFIGURE_ARGS="mpi=True MPI_CXX=mpic++ MPI_LINKERFORPROGRAMS=mpic++"
      ;;
    CYGWIN*)
      IS_CYGWIN=true
      ;;
    MINGW*)
      IS_MINGW=true
      ;;
    *)
      IS_LINUX=true
      ;;
  esac
}


# Helping function to get the timestamp for measuring the time spent at any part of the code
tic()
{
   TIMESTAMP="$(date +%s)"
}


# Helping function to get the second timestamp for measuring the difference with the first and then know the time spent at any part of the code
toc()
{
   NOW="$(date +%s)"
   ELAPSED="$(expr $NOW - $TIMESTAMP)"
   echo "*** Elapsed time: $ELAPSED seconds"
}


# Print a green msg using terminal escaped color sequence
echoGreen()
{
    printf "$GREEN %b $ENDC\n" "$1"
}

# Pinrt a red msg using terminal escaped color sequence
echoRed()
{
    printf "$RED %b $ENDC\n" "$1"
}

# Check last return status by checking GLOB_STATE and GLOB_COMMAND vars. It can receive a parameter, If 1 is given means the program has to exit if non-zero return state is detected
check_state()
{
  if [ $GLOB_STATE -ne 0 ]; then
    echoRed "WARNING: command returned a non-zero status"
    echoRed "COMMAND: $GLOB_COMMAND"
    case $1 in
      1)
        exitGracefully $GLOB_STATE "$GLOB_COMMAND"
      ;;
    esac
  fi
  return $GLOB_STATE
}

# Execute and print the sequence
echoExec()
{
  COMMAND="$@"
  GLOB_COMMAND=${COMMAND}
  echo '-->' ${COMMAND}
  $COMMAND
  GLOB_STATE=$?
  check_state
  return $GLOB_STATE
}

echoExecRedirectEverything()
{
  COMMAND="$1"
  REDIRECTION="$2"
  GLOB_COMMAND=${COMMAND}
  echo '-->' $COMMAND '>' $REDIRECTION '2>&1'
  ${COMMAND} > ${REDIRECTION} 2>&1
  GLOB_STATE=$?
  check_state
  return $GLOB_STATE
}

welcomeMessage()
{
  echo -e "${BLACK}0000000000000000000000000000000000000000000000000001"              
  echo -e "${BLACK}0000000000000000P!!00000!!!!!00000!!0000000000000001"
  echo -e "${BLACK}000000000000P'  ${RED}.:==.           ,=;:.  ${BLACK}\"400000000001"
  echo -e "${BLACK}0000000000!  ${RED}.;=::.::,         .=:.-:=,.  ${BLACK}!000000001"
  echo -e "${BLACK}0000000P' ${RED}.=:-......::=      ::;.......-:=. ${BLACK}\"0000001"
  echo -e "${BLACK}0000000,${RED}.==-.........::;.   .;::.-.......-=:${BLACK}.a#00001"
  echo -e "${BLACK}0000000'${RED}.=;......--:.:.:=:.==::.:--:.:...,=- ${BLACK}!000001"
  echo -e "${BLACK}000000    ${RED}-=...:.:.:-:::::=;::.::.:.:..-:=-    ${BLACK}00001"
  echo -e "${BLACK}0000P       ${RED}==:.:-::. ${YELLOW}.aa.${RED} :::::::::.::=:      ${BLACK}\"4001"
  echo -e "${BLACK}00001        ${RED}:;:::::..${YELLOW}:#0:${RED} =::::::::::::        ${BLACK}j#01"
  echo -e "${BLACK}0001   ${YELLOW}aa _aas  _aa_  .aa, _a__aa_.  .a__aas,    ${BLACK}j#1"
  echo -e "${BLACK}"'0001   '"${YELLOW}"'4WU*!4#gW9!##i .##; 3##P!9#Ai .##U!!Q#_   '"${BLACK}"'j#1'
  echo -e "${BLACK}"'001    '"${YELLOW}"'3O.  :xO.  ]Oi .XX: ]O( .  X2 .XC;  :xX.   '"${BLACK}"'01'
  echo -e "${BLACK}"'001    '"${YELLOW}"'dU.  :jU.  %Ui .WW: ]UL,..aXf .Ums  jd*    '"${BLACK}"'01'
  echo -e "${BLACK}"'0001   '"${YELLOW}"'4W.  :dW. .%W1 :WW: %WVXNWO~  .#U*#WV!    _'"${BLACK}"'01'
  echo -e "${BLACK}"'0001        '"${RED}"'.............. '"${YELLOW}"'%#1  '"${RED}"'.... '"${YELLOW}"'.#A)        '"${BLACK}"'j01'
  echo -e "${BLACK}"'00001      '"${RED}"':=::-:::::;;;;: '"${YELLOW}"'301 '"${RED}"'::::: '"${YELLOW}"'.0A)       '"${BLACK}"'j#01'
  echo -e "${BLACK}0000L    ${RED}.;::.--::::::::;: ,,..:::::... .::.   ${BLACK}_d001"
  echo -e "${BLACK}000000  ${RED}:;:...-.-.:.:::=;   =;:::---.:....::,  ${BLACK}00001"
  echo -e "${BLACK}000000!${RED}:::.....-.:.::::-     :=:-.:.-......:= ${BLACK}!00001"
  echo -e "${BLACK}000000a  ${RED}=;.........:;        .:;........,;;  ${BLACK}a00001"
  echo -e "${BLACK}"'0000000La '"${RED}"'--:_....:=-           :=:...:_:--'"${BLACK}"'_a#000001'
  echo -e "${BLACK}"'0000000000a  '"${RED}"'-=;,:=              -;::=:-  '"${BLACK}"'a000000001'
  echo -e "${BLACK}"'00000000000Laaa '"${RED}"'-\aa              ar- '"${BLACK}"'aaa00000000001'
  echo -e "${BLACK}00000000000000#00000000aaaaaad000000#00#0#0000000001"
  echo ""
  echo -e "${WHITE}Welcome to $TITLE ${ENDC}"
  echo ""
  return 0;
}

#function that prints the script usage help
helpMessage()
{
  echo -e ""
  welcomeMessage
  echo -e "###################################"
  echo -e '# XMIPP INSTALLATION SCRIPT USAGE #'
  echo -e "###################################"
  echo -e "${RED}NAME"
  echo -e "${WHITE}  install.sh - XMIPP installation script. "
  echo -e ""
  echo -e "${RED}SYNOPSIS"
  echo -e "${WHITE}  ./install.sh ${BLUE}[OPERATIONS] [OPTIONS]${WHITE}"
  echo -e ""
  echo -e "${RED}DESCRIPTION"
  echo -e "${WHITE}  Script that automates the XMIPP compilation process. When this script is executed, the compilation sequence starts, depending on the selected options. If no option is given, the script follows the sequence:"
  echo -e "1- untar the external libraries, xmipp_python and xmipp_python modules."
  echo -e "2- Configure and compile one by one every external library"
  echo -e "3- Configure and compile xmipp_python"
  echo -e "4- Compile and install python modules in xmipp_python"
  echo -e "5- Launch SConscript to compile Xmipp"
  echo -e "6- Run the unitary tests"
  echo -e ""
  echo -e "No option is mandatory. The default behaviour (without any given option) Following options are accepted:"
  echo -e ""
  echo -e ""
  echo -e "GENERAL OPTIONS:"
  echo -e ""
  echo -e "${BLUE}--disable-all${WHITE},${BLUE} -d${WHITE}"
  echo -e "    Disable every option in order to let the user to enable just some of them."
  echo -e "${BLUE}--num-cpus=${YELLOW}<NUMCPU>${WHITE},${BLUE} -j ${YELLOW}<NUMCPU>${WHITE}"
  echo -e "    Provides a number of CPUs for the compilation process. Default value is 2."
  echo -e "${BLUE}--configure-args=${YELLOW}<ARGS>${WHITE}"
  echo -e "    Store the arguments the user wants to provide to configure process before compilation start."
  echo -e "${BLUE}--compile-args=${YELLOW}<ARGS>${WHITE}"
  echo -e "    Store the arguments the user wants to provide to compilation process."
  echo -e "${BLUE}--unattended=${YELLOW}[true|false]${WHITE}"
  echo -e "    Tells the script to asume default where no option is given on every question and don't show any GUI. This is intended to be used when no human will be watching the screen (i.e. other scripts)."
  echo -e "${BLUE}--help,${BLUE} -h${WHITE}"
  echo -e "    Shows this help message"
  echo -e ""
  echo -e ""
  echo -e "OPERATIONS:"
  echo -e ""
  echo -e "${BLUE}--untar${WHITE},${BLUE} -u${WHITE}"
  echo -e "    Untar the list of libraries provided to the script (or default libraries array if not provided)."
  echo -e "${BLUE}--configure${WHITE},${BLUE} -f${WHITE}"
  echo -e "    Configure the list of libraries provided to the script (or default libraries array if not provided)."
  echo -e "${BLUE}--compile${WHITE},${BLUE} -c${WHITE}"
  echo -e "    Compile the list of libraries provided to the script (or default libraries array if not provided)."
  echo -e ""
  echo -e ""
  echo -e "EXTERNAL LIBRARIES OPTIONS:"
  echo -e ""
  echo -e "${BLUE}--alglib=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over alglib library. When just --alglib is given, true is asumed."
  echo -e "${BLUE}--bilib=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over bilib library. When just --bilib is given, true is asumed."
  echo -e "${BLUE}--condor=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over condor library. When just --condor is given, true is asumed."
  echo -e "${BLUE}--fftw=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over fftw library. When just --fftw is given, true is asumed."
  echo -e "${BLUE}--gtest=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over gtest library. When just --gtest is given, true is asumed."
  echo -e "${BLUE}--hdf5=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over hdf5 library. When just --hdf5 is given, true is asumed."
  echo -e "${BLUE}--imagej=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over imagej library. When just --hdf5 is given, true is asumed."
  echo -e "${BLUE}--jpeg=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over jpeg library. When just --jpeg is given, true is asumed."
  echo -e "${BLUE}--scons=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over scons library. When just --scons is given, true is asumed."
  echo -e "${BLUE}--sqlite=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over sqlite library. When just --sqlite is given, true is asumed."
  echo -e "${BLUE}--tiff=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over tiff library. When just --tiff is given, true is asumed."
  echo -e "${BLUE}--nma=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over nma library. When just --nma is given, true is asumed."
  echo -e "${BLUE}--sh-alignment=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over nma library. When just --sh-alignment is given, true is asumed."
  echo -e "${BLUE}--cltomo=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over cltomo library. When just --cltomo is given, true is asumed."
  echo -e ""
  echo -e ""
  echo -e "PYTHON-RELATED OPTIONS:"
  echo -e ""
  echo -e "${BLUE}--python=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute selected operation over xmipp_python. Just --python is equal to --python=true."
  echo -e "${BLUE}--matplotlib=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute selected operation over matplotlib module. --matplotlib is equivalent to --matplotlib=true."
  echo -e "${BLUE}--mpi4py=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute selected operation over mpi4py module. --mpi4py is equivalent to --mpi4py=true."
  echo -e "${BLUE}--numpy=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute selected operation over numpy module. --numpy is equivalent to --numpy=true."
  echo -e "${BLUE}--scipy=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute selected operation over scipy module. --scipy is equivalent to --scipy=true."
  echo -e "${BLUE}--tcl-tk=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute selected operation over tcl and tk libraries. --tcl-tk is equivalent to --tcl-tk=true."
  echo -e "${BLUE}--tcl=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute selected operation over tcl library. If --tcl is given, --tcl=true will be understood."
  echo -e "${BLUE}--tk=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute selected operation over tk library. Where --tk means --tk=true."
  echo -e "${BLUE}--pymodules=${YELLOW}[true|false]${WHITE}"
  echo -e "    Execute or not selected operation over every python modules. When just --pymodules is given, true is asumed."
  echo -e ""
  echo -e ""
  echo -e "XMIPP-RELATED OPTIONS:"
  echo -e ""
  echo -e "${BLUE}--gui=${YELLOW}[true|false]${WHITE}"
  echo -e "    When launching scons compilation, select true whether you want the Xmipp compilation GUI or false otherwise. Where --gui means --gui=true."
  echo -e ""
  echo -e ""
  return 0;
}

takeArguments()
{
  while [ "$1" ]; do
    case $1 in
      --disable-all|-d)
        DO_UNTAR=false
        DO_COMPILE=false
        DO_CONFIGURE=false
        DO_ALGLIB=false
        DO_BILIB=false
        DO_CONDOR=false
        DO_FFTW=false
        DO_GTEST=false
        DO_HDF5=false
        DO_IMAGEJ=false
        DO_JPEG=false
        DO_SCONS=false
        DO_SQLITE=false
        DO_TIFF=false
        DO_NMA=false
        DO_SHALIGNMENT=false
        DO_MATLIBPLOT=false
        DO_PYMPI=false
        DO_NUMPY=false
        DO_SCIPY=false
        DO_TCL=false
        DO_TK=false
        DO_PYTHON=false
        DO_TCLTK=false
        DO_PYMOD=false
        DO_CLTOMO=false
        DO_SETUP=false
        ;;
      --num-cpus=*)
        ;;
      -j)
        NUMBER_OF_CPU=$2
        shift
        ;;
      -j*)
        NUMBER_OF_CPU=$(echo "$1"|sed -e 's/-j//g')
        ;;
      --configure-args=*)
        CONFIGURE_ARGS=$(echo "$1"|cut -d '=' -f2)
        ;;
      --compile-args=*)
        COMPILE_ARGS=$(echo "$1"|cut -d '=' -f2)
        ;;
      --unattended=*) #[true|false]
        WITH_UNATTENDED=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_UNATTENDED}" == "true" ]; then
          DO_UNATTENDED=true
        elif [ "${WITH_UNATTENDED}" == "false" ]
          DO_UNATTENDED=false
        else
          echoRed "Parameter --unattended only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --help|-h)
        helpMessage
        exitGracefully
        ;;
      --untar|-u)
        DO_UNTAR=true
        ;;
      --configure|-f)
        DO_CONFIGURE=true
        ;;
      --compile|-c)
        DO_COMPILE=true
        ;;
      --alglib)
        DO_ALGLIB=true
        ;;
      --alglib=*)
        WITH_ALGLIB=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_ALGLIB}" == "true" ]; then
          DO_ALGLIB=true
        elif [ "${WITH_ALGLIB}" == "false" ]
          DO_ALGLIB=false
        else
          echoRed "Parameter --alglib only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --bilib)
        DO_BILIB=true
        ;;
      --bilib=*)
        WITH_BILIB=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_BILIB}" == "true" ]; then
          DO_BILIB=true
        elif [ "${WITH_BILIB}" == "false" ]
          DO_BILIB=false
        else
          echoRed "Parameter --bilib only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --condor)
        DO_CONDOR=true
        ;;
      --condor=*)
        WITH_CONDOR=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_CONDOR}" == "true" ]; then
          DO_CONDOR=true
        elif [ "${WITH_CONDOR}" == "false" ]
          DO_CONDOR=false
        else
          echoRed "Parameter --condor only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --fftw)
        DO_FFTW=true
        ;;
      --fftw=*)
        WITH_FFTW=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_FFTW}" == "true" ]; then
          DO_FFTW=true
        elif [ "${WITH_FFTW}" == "false" ]
          DO_FFTW=false
        else
          echoRed "Parameter --fftw only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --gtest)
        DO_GTEST=true
        ;;
      --gtest=*)
        WITH_GTEST=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_GTEST}" == "true" ]; then
          DO_GTEST=true
        elif [ "${WITH_GTEST}" == "false" ]
          DO_GTEST=false
        else
          echoRed "Parameter --gtest only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --hdf5)
        DO_HDF5=true
        ;;
      --hdf5=*)
        DO_HDF5=$(echo "$1"|cut -d '=' -f2)
        if [ "${DO_HDF5}" == "true" ]; then
          DO_HDF5=true
        elif [ "${DO_HDF5}" == "false" ]
          DO_HDF5=false
        else
          echoRed "Parameter --hdf5 only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --imagej)
        DO_IMAGEJ=true
        ;;
      --imagej=*)
        DO_IMAGEJ=$(echo "$1"|cut -d '=' -f2)
        if [ "${DO_IMAGEJ}" == "true" ]; then
          DO_IMAGEJ=true
        elif [ "${DO_IMAGEJ}" == "false" ]
          DO_IMAGEJ=false
        else
          echoRed "Parameter --imagej only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --jpeg)
        DO_JPEG=true
        ;;
      --jpeg=*)
        WITH_JPEG=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_JPEG}" == "true" ]; then
          DO_JPEG=true
        elif [ "${WITH_JPEG}" == "false" ]
          DO_JPEG=false
        else
          echoRed "Parameter --jpeg only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --scons)
        DO_SCONS=true
        ;;
      --scons=*)
        WITH_SCONS=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_SCONS}" == "true" ]; then
          DO_SCONS=true
        elif [ "${WITH_SCONS}" == "false" ]
          DO_SCONS=false
        else
          echoRed "Parameter --scons only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --sqlite)
        DO_SQLITE=true
        ;;
      --sqlite=*)
        WITH_SQLITE=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_SQLITE}" == "true" ]; then
          DO_SQLITE=true
        elif [ "${WITH_SQLITE}" == "false" ]
          DO_SQLITE=false
        else
          echoRed "Parameter --sqlite only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --tiff)
        DO_TIFF=true
        ;;
      --tiff=*)
        WITH_TIFF=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_TIFF}" == "true" ]; then
          DO_TIFF=true
        elif [ "${WITH_TIFF}" == "false" ]
          DO_TIFF=false
        else
          echoRed "Parameter --tiff only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --nma)
        DO_NMA=true
        ;;
      --nma=*)
        WITH_NMA=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_NMA}" == "true" ]; then
          DO_NMA=true
        elif [ "${WITH_NMA}" == "false" ]
          DO_NMA=false
        else
          echoRed "Parameter --nma only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --sh-alignment)
        DO_SHALIGNMENT=true
        ;;
      --sh-alignment=*)
        WITH_SHALIGNMENT=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_SHALIGNMENT}" == "true" ]; then
          DO_ALIGNMENT=true
        elif [ "${WITH_ALIGNMENT}" == "false" ]
          DO_ALIGNMENT=false
        else
          echoRed "Parameter --sh-alignment only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --cltomo)
        DO_CLTOMO=true
        ;;
      --cltomo=*)
        WITH_CLTOMO=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_CLTOMO}" == "true" ]; then
          DO_CLTOMO=true
        elif [ "${WITH_CLTOMO}" == "false" ]
          DO_CLTOMO=false
        else
          echoRed "Parameter --cltomo only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --python)
        DO_PYTHON=true
        DO_PYMOD=true
        ;;
      --python=*)
        WITH_PYTHON=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_PYTHON}" == "true" ]; then
          DO_PYTHON=true
          DO_PYMOD=true
        elif [ "${WITH_PYTHON}" == "false" ]
          DO_PYTHON=false
          DO_PYMOD=false
        else
          echoRed "Parameter --python only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --matplotlib)
        DO_MATLIBPLOT=true
        ;;
      --matplotlib=*)
        WITH_MATLIBPLOT=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_MATLIBPLOT}" == "true" ]; then
          DO_MATLIBPLOT=true
        elif [ "${WITH_MATLIBPLOT}" == "false" ]
          DO_MATLIBPLOT=false
        else
          echoRed "Parameter --matplotlib only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --mpi4py)
        DO_PYMPI=true
        ;;
      --mpi4py=*)
        WITH_PYMPI=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_PYMPI}" == "true" ]; then
          DO_PYMPI=true
        elif [ "${WITH_PYMPI}" == "false" ]
          DO_PYMPI=false
        else
          echoRed "Parameter --mpi4py only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --numpy)
        DO_NUMPY=true
        ;;
      --numpy=*)
        WITH_NUMPY=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_NUMPY}" == "true" ]; then
          DO_NUMPY=true
        elif [ "${WITH_NUMPY}" == "false" ]
          DO_NUMPY=false
        else
          echoRed "Parameter --numpy only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --scipy)
        DO_SCIPY=true
        ;;
      --scipy=*)
        WITH_SCIPY=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_SCIPY}" == "true" ]; then
          DO_SCIPY=true
        elif [ "${WITH_SCIPY}" == "false" ]
          DO_SCIPY=false
        else
          echoRed "Parameter --scipy only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --tcl-tk)
        DO_TCLTK=true
        ;;
      --tcl-tk=*)
        WITH_TCLTK=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_TCLTK}" == "true" ]; then
          DO_TCLTK=true
        elif [ "${WITH_TCLTK}" == "false" ]
          DO_TCLTK=false
        else
          echoRed "Parameter --tcl-tk only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --tcl)
        DO_TCL=true
        ;;
      --tcl=*)
        WITH_TCL=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_TCL}" == "true" ]; then
          DO_TCL=true
        elif [ "${WITH_TCL}" == "false" ]
          DO_TCL=false
        else
          echoRed "Parameter --tcl only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --tk)
        DO_TK=true
        ;;
      --tk=*)
        WITH_TK=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_TK}" == "true" ]; then
          DO_TK=true
        elif [ "${WITH_TK}" == "false" ]
          DO_TK=false
        else
          echoRed "Parameter --tk only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --pymodules)
        DO_PYMOD=true
        ;;
      --pymodules=*)
        WITH_PYMOD=$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_PYMOD}" == "true" ]; then
          DO_PYMOD=true
        elif [ "${WITH_PYMOD}" == "false" ]
          DO_PYMOD=false
        else
          echoRed "Parameter --pymodules only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      --gui=*)
        WITH_GUI==$(echo "$1"|cut -d '=' -f2)
        if [ "${WITH_GUI}" == "true" ]; then
          WITH_GUI=true
        elif [ "${WITH_GUI}" == "false" ]
          WITH_GUI=false
        else
          echoRed "Parameter --gui only accept true or false values. Ignored and assuming default value."
        fi
        ;;
      *)
        echoRed "Error: Unrecognized option $1, exiting..."
        helpMessage
        exitGracefully
        ;;
    esac
    shift
  done

#        "clean=true")         DO_CLEAN=true;;
#        "clean=false")        DO_CLEAN=false;;
#        "static=true")        DO_STATIC=true;;
#        "static=false")       DO_STATIC=false;;
#        "gui=false")          GUI_ARGS="";;
#        "setup=true")         DO_SETUP=true;;

}

#function that searchs in an array for an element and returns the position of that element in the array
indexOf(){
  local i=1 S=$1; shift
  while [ $S != $1 ]
  do    ((i++)); shift
        [ -z "$1" ] && { i=0; break; }
  done
  return $i
}

create_bashrc_file()
{
  INC_FILE=$1
  echo "export XMIPP_HOME=$PWD" > $INC_FILE
  echo 'export PATH=$XMIPP_HOME/bin:$PATH' >> $INC_FILE
  echo 'export LD_LIBRARY_PATH=$XMIPP_HOME/lib:$LD_LIBRARY_PATH' >> $INC_FILE
  echo 'if [ "$BASH" != "" ]; then' >> $INC_FILE
  echo '# Load global autocomplete file ' >> $INC_FILE
  echo 'test -s $XMIPP_HOME/.xmipp.autocomplete && . $XMIPP_HOME/.xmipp.autocomplete || true' >> $INC_FILE
  echo '# Load programs autocomplete file ' >> $INC_FILE
  echo 'test -s $XMIPP_HOME/.xmipp_programs.autocomplete && . $XMIPP_HOME/.xmipp_programs.autocomplete || true' >> $INC_FILE
  echo 'fi' >> $INC_FILE
  
  if $IS_MAC; then
    echo 'export DYLD_FALLBACK_LIBRARY_PATH=$XMIPP_HOME/lib:$DYLD_FALLBACK_LIBRARY_PATH' >> $INC_FILE
  fi
  echo " "    >> $INC_FILE
  echo " "    >> $INC_FILE
  
  echo "# Xmipp Aliases 						 "    >> $INC_FILE
  echo "## Setup ##                        "    >> $INC_FILE
  echo "alias xconfigure='./setup.py -j $NUMBER_OF_CPU configure compile ' " >> $INC_FILE
  echo "alias xcompile='./setup.py -j $NUMBER_OF_CPU compile ' "             >> $INC_FILE
  echo "alias xupdate='./setup.py -j $NUMBER_OF_CPU update compile ' "       >> $INC_FILE
  echo "## Interface ##                        "    >> $INC_FILE
  echo "alias xa='xmipp_apropos'               "    >> $INC_FILE
  echo "alias xb='xmipp_browser'               "    >> $INC_FILE
  echo "alias xp='xmipp_protocols'             "    >> $INC_FILE
  echo "alias xmipp='xmipp_protocols'          "    >> $INC_FILE
  echo "alias xs='xmipp_showj'                 "    >> $INC_FILE
  echo "alias xmipp_show='xmipp_showj'         "    >> $INC_FILE
  echo "alias xsj='xmipp_showj'                "    >> $INC_FILE
  echo "alias xij='xmipp_imagej'               "    >> $INC_FILE
  echo "## Image ##                            "    >> $INC_FILE
  echo "alias xic='xmipp_image_convert'        "    >> $INC_FILE
  echo "alias xih='xmipp_image_header'         "    >> $INC_FILE
  echo "alias xio='xmipp_image_operate'        "    >> $INC_FILE
  echo "alias xis='xmipp_image_statistics'     "    >> $INC_FILE
  echo "## Metadata ##                         "    >> $INC_FILE
  echo "alias xmu='xmipp_metadata_utilities'   "    >> $INC_FILE
  echo "alias xmp='xmipp_metadata_plot'        "    >> $INC_FILE
  echo "## Transformation ##                   "    >> $INC_FILE
  echo "alias xtg='xmipp_transform_geometry'   "    >> $INC_FILE
  echo "alias xtf='xmipp_transform_filter'     "    >> $INC_FILE
  echo "alias xtn='xmipp_transform_normalize'  "    >> $INC_FILE
  echo "## Other ##                            "    >> $INC_FILE
  echo "alias xrf='xmipp_resolution_fsc'       "    >> $INC_FILE
  echo "alias xrs='xmipp_resolution_ssnr'      "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "## Configuration ##                                                          "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "# This file will serve to customize some settings of you Xmipp installation  "    >> $INC_FILE
  echo "# Each setting will be in the form o a shell variable set to some value      "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "#---------- GUI ----------                                                   "    >> $INC_FILE
  echo "# If you set to 1 the value of this variable, by default the programs        "    >> $INC_FILE
  echo "# will launch the gui when call without argments, default is print the help  "    >> $INC_FILE
  echo "export XMIPP_GUI_DEFAULT=0                                                   "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "# If you set to 0 the value of this variable the script generated            "    >> $INC_FILE
  echo "# by programs gui will not be erased and you can use the same parameters     "    >> $INC_FILE
  echo "export XMIPP_GUI_CLEAN=1                                                     "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "#---------- Parallel ----------                                              "    >> $INC_FILE
  echo "# This variable will point to your job submition template file               "    >> $INC_FILE
  echo "export XMIPP_PARALLEL_LAUNCH=config_launch.py                                "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "# If you have .xmipp.cfg in your home folder it will override                "    >> $INC_FILE
  echo "# this configurations                                                        "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "test -s ~/.xmipp.cfg && . ~/.xmipp.cfg || true                               "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE

  chmod a+x $INC_FILE
}

create_tcsh_file()
{
  INC_FILE=$1
  echo "setenv XMIPP_HOME $PWD" > $INC_FILE
  echo 'setenv PATH $XMIPP_HOME/bin:$PATH' >> $INC_FILE
  echo 'if($?LD_LIBRARY_PATH) then' >> $INC_FILE
  echo '  setenv LD_LIBRARY_PATH $XMIPP_HOME/lib:$LD_LIBRARY_PATH' >> $INC_FILE
  echo 'else' >> $INC_FILE
  echo '  setenv LD_LIBRARY_PATH $XMIPP_HOME/lib' >> $INC_FILE
  echo 'endif' >> $INC_FILE
  
  if $IS_MAC; then
    echo 'if($?DYLD_FALLBACK_LIBRARY_PATH) then' >> $INC_FILE
    echo '  setenv DYLD_FALLBACK_LIBRARY_PATH $XMIPP_HOME/lib:$DYLD_FALLBACK_LIBRARY_PATH' >> $INC_FILE
    echo 'else' >> $INC_FILE
    echo '  setenv DYLD_FALLBACK_LIBRARY_PATH $XMIPP_HOME/lib' >> $INC_FILE
    echo 'endif' >> $INC_FILE
  fi
  echo 'test -s $XMIPP_HOME/.xmipp.alias && source $XMIPP_HOME/.xmipp.alias || true' >> $INC_FILE
  
  echo " "    >> $INC_FILE
  echo " "    >> $INC_FILE

  echo "# Xmipp Aliases 						 "    >> $INC_FILE
  echo "## Setup ##                        "    >> $INC_FILE
  echo "alias xconfigure './setup.py -j $NUMBER_OF_CPU configure compile ' " >> $INC_FILE
  echo "alias xcompile './setup.py -j $NUMBER_OF_CPU compile ' "             >> $INC_FILE
  echo "alias xupdate './setup.py -j $NUMBER_OF_CPU update compile ' "       >> $INC_FILE
  echo "## Interface ##                        "    >> $INC_FILE
  echo "alias xa 'xmipp_apropos'               "    >> $INC_FILE
  echo "alias xb 'xmipp_browser'               "    >> $INC_FILE
  echo "alias xp 'xmipp_protocols'             "    >> $INC_FILE
  echo "alias xmipp 'xmipp_protocols'          "    >> $INC_FILE
  echo "alias xs 'xmipp_showj'                 "    >> $INC_FILE
  echo "alias xmipp_show 'xmipp_showj'         "    >> $INC_FILE
  echo "alias xsj 'xmipp_showj'                "    >> $INC_FILE
  echo "## Image ##                            "    >> $INC_FILE
  echo "alias xic 'xmipp_image_convert'        "    >> $INC_FILE
  echo "alias xih 'xmipp_image_header'         "    >> $INC_FILE
  echo "alias xio 'xmipp_image_operate'        "    >> $INC_FILE
  echo "alias xis 'xmipp_image_statistics'     "    >> $INC_FILE
  echo "## Metadata ##                         "    >> $INC_FILE
  echo "alias xmu 'xmipp_metadata_utilities'   "    >> $INC_FILE
  echo "alias xmp 'xmipp_metadata_plot'        "    >> $INC_FILE
  echo "## Transformation ##                   "    >> $INC_FILE
  echo "alias xtg 'xmipp_transform_geometry'   "    >> $INC_FILE
  echo "alias xtf 'xmipp_transform_filter'     "    >> $INC_FILE
  echo "alias xtn 'xmipp_transform_normalize'  "    >> $INC_FILE
  echo "## Other ##                            "    >> $INC_FILE
  echo "alias xrf 'xmipp_resolution_fsc'       "    >> $INC_FILE
  echo "alias xrs 'xmipp_resolution_ssnr'      "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "## Configuration ##                                                          "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "# This file will serve to customize some settings of you Xmipp installation  "    >> $INC_FILE
  echo "# Each setting will be in the form o a shell variable set to some value      "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "#---------- GUI ----------                                                   "    >> $INC_FILE
  echo "# If you set to 1 the value of this variable, by default the programs        "    >> $INC_FILE
  echo "# will launch the gui when call without argments, default is print the help  "    >> $INC_FILE
  echo "setenv XMIPP_GUI_DEFAULT 0                                                   "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "# If you set to 0 the value of this variable the script generated            "    >> $INC_FILE
  echo "# by programs gui will not be erased and you can use the same parameters     "    >> $INC_FILE
  echo "setenv XMIPP_GUI_CLEAN 1                                                     "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "#---------- Parallel ----------                                              "    >> $INC_FILE
  echo "# This variable will point to your job submition template file               "    >> $INC_FILE
  echo "setenv XMIPP_PARALLEL_LAUNCH config_launch.py                                "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "# If you have .xmipp.cfg in your home folder it will override                "    >> $INC_FILE
  echo "# this configurations                                                        "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  echo "test -s ~/.xmipp.cfg && source ~/.xmipp.cfg || true                          "    >> $INC_FILE
  echo "                                                                             "    >> $INC_FILE
  
  chmod a+x $INC_FILE
}
 
compile_library()
  {
  tic
  LIB=$1
  PREFIX_PATH=$2
  SUFFIX_PATH=$3
  CONFIGFLAGS=$4
  LIBS_PATH=$5
  _PATH=${EXT_PATH}/${PREFIX_PATH}/${LIB}/${SUFFIX_PATH}
  echo
  echoGreen "*** Compiling ${LIB} ..."
  echoExec "cd ${_PATH}"

  if ! $DO_STATIC; then
    echo "--> Enabling shared libraries..."
    CONFIGFLAGS="--enable-shared ${CONFIGFLAGS}"
  fi

  if $DO_CLEAN; then
    echoRedirectEverything "make distclean" "/dev/null"
  fi

  echoRedirectEverything "./configure ${CONFIGFLAGS}" "${BUILD_PATH}/${LIB}_configure.log"
  echoRedirectEverything "make -j $NUMBER_OF_CPU" "$BUILD_PATH/${LIB}_make.log"
  toc
}

compile_pymodule()
{
   MOD=$1
   _PATH=$EXT_PATH/python/$MOD
   #_PYTHON=$EXT_PATH/python/$PYTHON_FOLDER/python
   echoRedirectEverything "cd $_PATH" "/dev/null"
   echoRedirectEverything "xmipp_python setup.py install --prefix $XMIPP_HOME" "$BUILD_PATH/${MOD}_setup_install.log"
}

#This function should be called from XMIPP_HOME
# Parameter: Library_Path Library_name Lib_Version_Number 
install_libs()
{
  cd $XMIPP_HOME
  LIBPATH=external/$1; shift
  COMMON="$1"; shift
  VERSION=$1; shift
  COPY=$1
  SUFFIXES=".a .la "
  if $IS_MAC; then
	SUFFIXES="$SUFFIXES .dylib .$VERSION.dylib"
  elif $IS_MINGW; then
        SUFFIXES="$SUFFIXES .dll.a -$VERSION.dll"
  else 
	SUFFIXES="$SUFFIXES .so .so.$VERSION"
  fi
  
  for suffix in $SUFFIXES; do
     LIBNAME=$COMMON$suffix
     if $COPY; then
     	     if [ -e lib/$LIBNAME ]; then
	         echoRedirectEverything "rm -f lib/$LIBNAME" "/dev/null"
             fi
	     echoRedirectEverything "cp -f $LIBPATH/$LIBNAME lib/$LIBNAME" "/dev/null"
     else
	     echoRedirectEverything "ln -sf ../$LIBPATH/$LIBNAME lib/$LIBNAME " "/dev/null"
     fi
  done
}

install_bin()
{
  echoRedirectEverything "cd $XMIPP_HOME" "/dev/null"
  BINPATH=../external/$1
  LINKNAME=bin/$2
  echoRedirectEverything "ln -sf $BINPATH $LINKNAME" "/dev/null"
}

create_dir()
{
  DIR=$1
  if [ -d $DIR ]; then 
    echoRed "--> Dir $DIR exists."
  else
    echoRedirectEverything "mkdir $DIR" "/dev/null"
  fi
}

initial_definitions()
{
  export XMIPP_HOME=$PWD
  export PATH=$XMIPP_HOME/bin:$PATH
  export LD_LIBRARY_PATH=$XMIPP_HOME/lib:$LD_LIBRARY_PATH
  if $IS_MAC; then
    export DYLD_FALLBACK_LIBRARY_PATH=$XMIPP_HOME/lib:$DYLD_FALLBACK_LIBRARY_PATH
  fi
  EXT_PATH=$XMIPP_HOME/external
  BUILD_PATH=$XMIPP_HOME/build
}

decompressExternals()
{
  DELETE_ANSWER="n"
  tic
  echo
  echoRedirectEverything "cd ${EXT_PATH}" "/dev/null"
  echoGreen "*** Decompressing external libraries ..."
  lib=0
  while [ ${lib} -le ${#EXTERNAL_LIBRARIES[@]} ]; do
    if [ ${EXTERNAL_LIBRARIES_DO[$lib]} ]; then
      if [ -d ${EXTERNAL_LIBRARIES[$lib]} ]; then
        if [ ! $DO_UNATTENDED -a ${DELETE_ANSWER} != "Y" -a ${DELETE_ANSWER} != "N"]; then
          echo "${EXTERNAL_LIBRARIES[$lib]} folder exists, do you want to permanently remove it? (y)es/(n)o/(Y)es-to-all/(N)o-to-all"
          read DELETE_ANSWER
        else
          DELETE_ANSWER="Y"
        fi
        if [ ${DELETE_ANSWER} == "y" -o ${DELETE_ANSWER} == "Y" ]; then
          echoRedirectEverything "rm -rf ${EXTERNAL_LIBRARIES[$lib]}"
        else
          echoRed "Library ${EXTERNAL_LIBRARIES[$lib]} folder remains untouched."
        fi
      fi
      echoRedirectEverything "tar -xvzf ${EXTERNAL_LIBRARIES_TAR[$lib]}" "/dev/null"
    fi
    lib=$(expr "$lib + 1")
  done
  echoRedirectEverything "cd -" "/dev/null"
  toc
}

decompressPython()
{
  DELETE_ANSWER="n"
  tic
  echo
  echoRedirectEverything "cd ${EXT_PATH}/python" "/dev/null"
  echoGreen "*** Decompressing Python ***"
  if [ -d ${PYTHON_FOLDER} ]; then
    if [ ! $DO_UNATTENDED -a ${DELETE_ANSWER} != "Y" -a ${DELETE_ANSWER} != "N"]; then
      echo "${PYTHON_FOLDER} folder exists, do you want to permanently remove it? (y)es/(n)o/(Y)es-to-all/(N)o-to-all"
      read DELETE_ANSWER
    else
      DELETE_ANSWER="Y"
    fi
    if [ ${DELETE_ANSWER} == "y" -o ${DELETE_ANSWER} == "Y" ]; then
      echoRedirectEverything "rm -rf ${PYTHON_FOLDER}" "/dev/null"
    else
      echoRed "${PYTHON_FOLDER} folder remains untouched."
    fi   
  fi
  echoRedirectEverything "tar -xvzf ${PYTHON_FOLDER}" "/dev/null"
  echoRedirectEverything "cd -" "/dev/null"
  toc
}

decompressPythonModules()
{
  DELETE_ANSWER="n"
  tic
  echo
  echoRedirectEverything "cd ${EXT_PATH}/python" "/dev/null"
  echoGreen "*** Decompressing Python modules ..."
  lib=0
  while [ ${lib} -le ${#PYTHON_MODULES[@]} ]; do
    if [ ${PYTHON_MODULES_DO[$lib]} ]; then
      if [ -d ${PYTHON_MODULES[$lib]} ]; then
        if [ ! $DO_UNATTENDED -a ${DELETE_ANSWER} != "Y" -a ${DELETE_ANSWER} != "N"]; then
          echo "${PYTHON_MODULES[$lib]} folder exists, do you want to permanently remove it? (y)es/(n)o/(Y)es-to-all/(N)o-to-all"
          read DELETE_ANSWER
        else
          DELETE_ANSWER="Y"
        fi
        if [ ${DELETE_ANSWER} == "y" -o ${DELETE_ANSWER} == "Y" ]; then
          echoRedirectEverything "rm -rf ${PYTHON_MODULES[$lib]}" "/dev/null"
        else
          echoRed "Library ${PYTHON_MODULES[$lib]} folder remains untouched."
        fi
      fi
      echoRedirectEverything "tar -xvzf ${PYTHON_MODULES_TAR[$lib]}" "/dev/null"
    fi
    lib=$(expr "$lib + 1")
  done
  echo "--> cd - > /dev/null"
  cd - > /dev/null 2>&1
  toc
}

exitGracefully()
{
  if [ $# -eq 1 ]; then
    GLOB_STATE=$1
    GLOB_COMMAND="Unknown"
  elif [ $# -eq 2 ]; then
    GLOB_STATE=$1
    GLOB_COMMAND="$2"
  elif [ $# -gt 2 ]; then
    echoRed "Error: too much parameters passed to exitGracefully function."
  fi

  if [ ${GLOB_STATE} -eq 0 ]; then
    echoGreen "Program exited succesfully."
  else
    echoRed "Program exited with non-zero status (${GLOB_STATE}), produced by command ${GLOB_COMMAND}."
  fi
}

##################################################################################
#################### INITIAL TASKS ###############################################
##################################################################################

welcomeMessage
initial_definitions
takeArguments $@
decideOS
create_bashrc_file .xmipp.bashrc # Create file to include from BASH this Xmipp installation
create_tcsh_file .xmipp.csh      # for CSH or TCSH

if $DO_UNATTENDED; then
  CONFIGURE_ARGS="$CONFIGURE_ARGS unattended "
fi


##################################################################################
#################### NEEDED FOLDERS: bin lib build ###############################
##################################################################################

echoGreen "*** Checking needed folders ..."
create_dir build
create_dir bin
create_dir lib


##################################################################################
#################### DECOMPRESSING EVERYTHING ####################################
##################################################################################

if $DO_UNTAR; then 
  decompressExternal
  decompressPython
  decompressPythonModules
fi

##################################################################################
#################### COMPILING EXTERNAL LIBRARIES ################################
##################################################################################

#################### SQLITE ###########################
if $DO_SQLITE; then
  if $IS_MAC; then
    #compile_library $SQLITE_FOLDER "." "." "CPPFLAGS=-w CFLAGS=-DSQLITE_ENABLE_UPDATE_DELETE_LIMIT=1 -I/opt/local/include -I/opt/local/lib -I/sw/include -I/sw/lib -lsqlite3" ".libs"
    compile_library ${SQLITE_FOLDER} "." "." "CPPFLAGS=-w CFLAGS=-DSQLITE_ENABLE_UPDATE_DELETE_LIMIT=1" ".libs"
  else
    compile_library ${SQLITE_FOLDER} "." "." "CPPFLAGS=-w CFLAGS=-DSQLITE_ENABLE_UPDATE_DELETE_LIMIT=1" ".libs"
  fi
  #execute sqlite to avoid relinking in the future
  echo "select 1+1 ;" | ${XMIPP_HOME}/external/${SQLITE_FOLDER}/sqlite3
  install_bin ${SQLITE_FOLDER}/sqlite3 xmipp_sqlite3
  install_libs ${SQLITE_FOLDER}/.libs libsqlite3 0 false
  #compile math library for sqlite
  cd ${EXT_PATH}/${SQLITE_EXT_FOLDER}
  if $IS_MINGW; then
    gcc -shared -I. -o libsqlitefunctions.dll extension-functions.c
    cp libsqlitefunctions.dll ${XMIPP_HOME}/lib/libXmippSqliteExt.dll
  elif $IS_MAC; then
    gcc -fno-common -dynamiclib extension-functions.c -o libsqlitefunctions.dylib
    cp libsqlitefunctions.dylib ${XMIPP_HOME}/lib/libXmippSqliteExt.dylib
  else  
    gcc -fPIC  -shared  extension-functions.c -o libsqlitefunctions.so -lm
    cp libsqlitefunctions.so ${XMIPP_HOME}/lib/libXmippSqliteExt.so
  fi
fi

#################### FFTW ###########################
if $DO_FFTW; then
  if $IS_MINGW; then
    FFTWFLAGS=" CPPFLAGS=-I/c/MinGW/include CFLAGS=-I/c/MinGW/include"
  else
    FFTWFLAGS=""
  fi
  FLAGS="${FFTWFLAGS} --enable-threads"
  compile_library ${VFFTW} "." "." ${FLAGS}
  install_libs ${VFFTW}/.libs libfftw3 3 true
  install_libs ${VFFTW}/threads/.libs libfftw3_threads 3 true

  FLAGS="${FFTWFLAGS} --enable-float"
  compile_library ${VFFTW} "." "." ${FLAGS}
  install_libs ${VFFTW}/.libs libfftw3f 3 true
fi

#################### JPEG ###########################
if $DO_JPEG; then
  compile_library ${VJPEG} "." "." "CPPFLAGS=-w"
  install_libs ${VJPEG}/.libs libjpeg 8 false
fi

#################### TIFF ###########################
if $DO_TIFF; then
  compile_library ${VTIFF} "." "." "CPPFLAGS=-w --with-jpeg-include-dir=${EXT_PATH}/${VJPEG} --with-jpeg-lib-dir=${XMIPP_HOME}/lib"
  install_libs ${VTIFF}/libtiff/.libs libtiff 3 false
fi

#################### HDF5 ###########################
if $DO_HDF5; then
  compile_library ${VHDF5} "." "." "CPPFLAGS=-w --enable-cxx"
  install_libs ${VHDF5}/src/.libs libhdf5 7 false
  install_libs ${VHDF5}/c++/src/.libs libhdf5_cpp 7 false
fi

#################### TCL/TK ###########################
if $DO_TCLTK; then
  if $IS_MAC; then
    compile_library ${TCL_FOLDER} python macosx "--disable-xft"
    compile_library ${TK_FOLDER} python macosx "--disable-xft"
  elif $IS_MINGW; then
    compile_library ${TCL_FOLDER} python win "--disable-xft CFLAGS=-I/c/MinGW/include CPPFLAGS=-I/c/MinGW/include"
    compile_library ${TK_FOLDER} python win "--disable-xft --with-tcl=../../${TCL_FOLDER}/win CFLAGS=-I/c/MinGW/include CPPFLAGS=-I/c/MinGW/include"
  else
    compile_library ${TCL_FOLDER} python unix "--enable-threads"
    compile_library ${TK_FOLDER} python unix "--enable-threads"
  fi
fi

#################### NMA ###########################
if $DO_NMA; then
    echoExec "cd ${XMIPP_HOME}/external/NMA/ElNemo"
    echoExec "make" 
    echoExec "cp nma_* ${XMIPP_HOME}/bin"
    echoExec "cd ${XMIPP_HOME}/external/NMA/NMA_cart"
    echoExec "make" 
    echoExec "cp nma_* ${XMIPP_HOME}/bin"
    echoExec "cd ${XMIPP_HOME}"
    echoExec "cp ${XMIPP_HOME}/external/NMA/nma_* ${XMIPP_HOME}/bin"
    echoExec "cp ${XMIPP_HOME}/external/NMA/m_inout_Bfact.py ${XMIPP_HOME}/bin"
    echoExec "cp -"
fi

##################################################################################
#################### COMPILING PYTHON ############################################
##################################################################################

EXT_PYTHON=${EXT_PATH}/python

if $DO_PYTHON; then
  echoGreen "PYTHON SETUP"
  export CPPFLAGS="-I${EXT_PATH}/${SQLITE_FOLDER} -I${EXT_PYTHON}/${TK_FOLDER}/generic -I${EXT_PYTHON}/${TCL_FOLDER}/generic"
  if $IS_CYGWIN; then
    export CPPFLAGS="-I/usr/include -I/usr/include/ncurses ${CPPFLAGS}"
    export LDFLAGS="-L${EXT_PYTHON}/${PYTHON_FOLDER} -L${XMIPP_HOME}/lib -L${EXT_PYTHON}/${TK_FOLDER}/unix -L${EXT_PYTHON}/${TCL_FOLDER}/unix"
    export LD_LIBRARY_PATH="${EXT_PYTHON}/${PYTHON_FOLDER}:${EXT_PYTHON}/${TK_FOLDER}/unix:${EXT_PYTHON}/${TCL_VERSION}/unix:${LD_LIBRARY_PATH}"
    echo "--> export CPPFLAGS=${CPPFLAGS}"
    echo "--> export LDFLAGS=${LDFLAGS}"
    echo "--> export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"	 
  elif $IS_MAC; then
    export LDFLAGS="-L${EXT_PYTHON}/${PYTHON_FOLDER} -L${XMIPP_HOME}/lib -L${EXT_PYTHON}/${TK_FOLDER}/macosx -L${EXT_PYTHON}/${TCL_FOLDER}/macosx"
    export LD_LIBRARY_PATH="${EXT_PYTHON}/${PYTHON_FOLDER}:${EXT_PYTHON}/${TK_FOLDER}/macosx:${EXT_PYTHON}/${TCL_FOLDER}/macosx:${LD_LIBRARY_PATH}"
    export DYLD_FALLBACK_LIBRARY_PATH="${EXT_PYTHON}/${PYTHON_FOLDER}:${EXT_PYTHON}/${TK_FOLDER}/macosx:${EXT_PYTHON}/${TCL_FOLDER}/macosx:${DYLD_FALLBACK_LIBRARY_PATH}"
    echo "--> export CPPFLAGS=${CPPFLAGS}"
    echo "--> export LDFLAGS=${LDFLAGS}"
    echo "--> export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
    echo "--> export DYLD_FALLBACK_LIBRARY_PATH=${DYLD_FALLBACK_LIBRARY_PATH}"
  elif $IS_MINGW; then
    export CPPFLAGS="-I/usr/include -I/usr/include/ncurses -I/c/MinGW/include ${CPPFLAGS} -D__MINGW32__ -I${EXT_PYTHON}/${PYTHON_FOLDER}/Include -I${EXT_PYTHON}/${PYTHON_FOLDER}/PC "
    export CFLAGS="${CPPFLAGS}"
    export LDFLAGS="-L${EXT_PYTHON}/${PYTHON_FOLDER} -L${XMIPP_HOME}/lib -L${EXT_PYTHON}/${TK_FOLDER}/win -L${EXT_PYTHON}/${TCL_FOLDER}/win"
    export LD_LIBRARY_PATH="${EXT_PYTHON}/${PYTHON_FOLDER}:${EXT_PYTHON}/${TK_FOLDER}/win:${EXT_PYTHON}/${TCL_FOLDER}/win:${LD_LIBRARY_PATH}"
    echo "--> export CPPFLAGS=${CPPFLAGS}"
    echo "--> export LDFLAGS=${LDFLAGS}"
    echo "--> export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
  else
    export LDFLAGS="-L${EXT_PYTHON/$PYTHON_FOLDER} -L${XMIPP_HOME}/lib -L${EXT_PYTHON}/${TK_FOLDER}/unix -L${EXT_PYTHON}/${TCL_FOLDER}/unix"
    export LD_LIBRARY_PATH="${EXT_PYTHON}/${PYTHON_FOLDER}:${EXT_PYTHON}/${TK_FOLDER}/unix:${EXT_PYTHON}/${TCL_FOLDER}/unix:${LD_LIBRARY_PATH}"
    echo "--> export CPPFLAGS=${CPPFLAGS}"
    echo "--> export LDFLAGS=${LDFLAGS}"
    echo "--> export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
  fi
  echoGreen "Copying our custom python files ..."
  echoExec "cd ${EXT_PYTHON}"
  echoExec "cp ./xmipp_setup.py ${PYTHON_FOLDER}/setup.py"
  echoExec "chmod a+x ${PYTHON_FOLDER}/setup.py"
  #cp ./xmipp_setup.py $PYTHON_FOLDER/setup.py
  #I thick these two are not needed
  #cp ./xmipp__iomodule.h $PYTHON_FOLDER/Modules/_io/_iomodule.h
  #echo "--> cp ./xmipp__iomodule.h $PYTHON_FOLDER/Modules/_io/_iomodule.h"
    
  compile_library ${PYTHON_FOLDER} python "." ""

  # Create the python launch script with necessary environment variable settings
  PYTHON_BIN=${XMIPP_HOME}/bin/xmipp_python
  echo "--> Creating python launch script $PYTHON_BIN ..."
  printf "#!/bin/sh\n\n" > $PYTHON_BIN
  if $IS_CYGWIN; then
    printf 'PYTHON_FOLDER=%b \n' "${PYTHON_FOLDER}" >> $PYTHON_BIN
    printf 'VTCLTK=%b \n\n' "${VTCLTK}" >> $PYTHON_BIN
    printf 'EXT_PYTHON=$XMIPP_HOME/external/python \n' >> $PYTHON_BIN
    printf 'export LD_LIBRARY_PATH=$EXT_PYTHON/$PYTHON_FOLDER:$EXT_PYTHON/tcl$VTCLTK/unix:$EXT_PYTHON/tk$VTCLTK/unix:$LD_LIBRARY_PATH \n' >> $PYTHON_BIN
    printf 'export PYTHONPATH=$XMIPP_HOME/lib:$XMIPP_HOME/protocols:$XMIPP_HOME/applications/tests/pythonlib:$XMIPP_HOME/lib/python2.7/site-packages:$PYTHONPATH \n' >> $PYTHON_BIN
    printf 'export TCL_LIBRARY=$EXT_PYTHON/tcl$VTCLTK/library \n' >> $PYTHON_BIN
    printf 'export TK_LIBRARY=$EXT_PYTHON/tk$VTCLTK/library \n\n' >> $PYTHON_BIN
    printf 'PYTHONCYGWINLIB=`find $EXT_PYTHON/$PYTHON_FOLDER/build -name "lib.cygwin*" -type d`\n' >> $PYTHON_BIN
    printf 'export LD_LIBRARY_PATH=$PYTHONCYGWINLIB:$LD_LIBRARY_PATH\n' >> $PYTHON_BIN
    printf 'export PYTHONPATH=$PYTHONCYGWINLIB:$PYTHONPATH\n' >> $PYTHON_BIN
    printf '$EXT_PYTHON/$PYTHON_FOLDER/python.exe "$@"\n' >> $PYTHON_BIN
  elif $IS_MINGW; then
    printf 'PYTHON_FOLDER=Python27 \n' >> $PYTHON_BIN
    printf 'VTCLTK=8.5 \n\n' >> $PYTHON_BIN
    printf 'EXT_PYTHON=/c \n' >> $PYTHON_BIN
    printf 'export LD_LIBRARY_PATH=$EXT_PYTHON/$PYTHON_FOLDER:$EXT_PYTHON/$PYTHON_FOLDER/tcl/tcl$VTCLTK:$EXT_PYTHON/$PYTHON_FOLDER/tcl/tk$VTCLTK:$LD_LIBRARY_PATH \n' >> $PYTHON_BIN
    printf 'export PYTHONPATH=$XMIPP_HOME/lib:$XMIPP_HOME/protocols:$XMIPP_HOME/applications/tests/pythonlib:$EXT_PYTHON/$PYTHON_FOLDER:$XMIPP_HOME/lib:$PYTHONPATH \n' >> $PYTHON_BIN
    printf 'export TCL_LIBRARY=$EXT_PYTHON/$PYTHON_FOLDER/tcl/tcl$VTCLTK \n' >> $PYTHON_BIN
    printf 'export TK_LIBRARY=$EXT_PYTHON/$PYTHON_FOLDER/tcl/tk$VTCLTK \n\n' >> $PYTHON_BIN
    printf '$EXT_PYTHON/$PYTHON_FOLDER/python.exe "$@"\n' >> $PYTHON_BIN
  elif $IS_MAC; then
    printf 'PYTHON_FOLDER=%b \n' "${PYTHON_FOLDER}" >> $PYTHON_BIN
    printf 'VTCLTK=%b \n\n' "${VTCLTK}" >> $PYTHON_BIN
    printf 'EXT_PYTHON=$XMIPP_HOME/external/python \n' >> $PYTHON_BIN
    printf 'export LD_LIBRARY_PATH=$EXT_PYTHON/$PYTHON_FOLDER:$EXT_PYTHON/tcl$VTCLTK/macosx:$EXT_PYTHON/tk$VTCLTK/macosx:$LD_LIBRARY_PATH \n' >> $PYTHON_BIN
    printf 'export PYTHONPATH=$XMIPP_HOME/lib:$XMIPP_HOME/protocols:$XMIPP_HOME/applications/tests/pythonlib:$XMIPP_HOME/lib/python2.7/site-packages:$PYTHONPATH \n' >> $PYTHON_BIN
    printf 'export TCL_LIBRARY=$EXT_PYTHON/tcl$VTCLTK/library \n' >> $PYTHON_BIN
    printf 'export TK_LIBRARY=$EXT_PYTHON/tk$VTCLTK/library \n\n' >> $PYTHON_BIN
    printf 'export DYLD_FALLBACK_LIBRARY_PATH=$EXT_PYTHON/$PYTHON_FOLDER:$EXT_PYTHON/tcl$VTCLTK/macosx:$EXT_PYTHON/tk$VTCLTK/macosx:$DYLD_FALLBACK_LIBRARY_PATH \n' >> $PYTHON_BIN	
    printf '$EXT_PYTHON/$PYTHON_FOLDER/python.exe "$@"\n' >> $PYTHON_BIN
  else
    printf 'PYTHON_FOLDER=%b \n' "${PYTHON_FOLDER}" >> $PYTHON_BIN
    printf 'VTCLTK=%b \n\n' "${VTCLTK}" >> $PYTHON_BIN
    printf 'EXT_PYTHON=$XMIPP_HOME/external/python \n' >> $PYTHON_BIN
    printf 'export LD_LIBRARY_PATH=$EXT_PYTHON/$PYTHON_FOLDER:$EXT_PYTHON/tcl$VTCLTK/unix:$EXT_PYTHON/tk$VTCLTK/unix:$LD_LIBRARY_PATH \n' >> $PYTHON_BIN
    printf 'export PYTHONPATH=$XMIPP_HOME/lib:$XMIPP_HOME/protocols:$XMIPP_HOME/applications/tests/pythonlib:$XMIPP_HOME/lib/python2.7/site-packages:$PYTHONPATH \n' >> $PYTHON_BIN
    printf 'export TCL_LIBRARY=$EXT_PYTHON/tcl$VTCLTK/library \n' >> $PYTHON_BIN
    printf 'export TK_LIBRARY=$EXT_PYTHON/tk$VTCLTK/library \n\n' >> $PYTHON_BIN
#    printf 'source ${XMIPP_HOME}/bin/activate' >> $PYTHON_BIN
#    printf '$EXT_PYTHON/$PYTHON_FOLDER/python "$@"\n' >> $PYTHON_BIN
  fi
  echoExec "chmod a+x ${PYTHON_BIN}"
  #make python directory accesible by anybody
  echoExec "chmod -R a+x ${XMIPP_HOME}/external/python/Python-2.7.2"

fi


##################################################################################
#################### COMPILING PYTHON MODULES ####################################
##################################################################################

if $DO_PYMOD; then
  compile_pymodule ${VNUMPY}
  export CPPFLAGS="-I${EXT_PATH}/${SQLITE_FOLDER}/ -I${EXT_PYTHON}/${TK_FOLDER}/generic -I${EXT_PYTHON}/${TCL_FOLDER}/generic"
  if $IS_MAC; then
    export LDFLAGS="-L${EXT_PYTHON}/${PYTHON_FOLDER} -L${XMIPP_HOME}/lib -L${EXT_PYTHON}/${TK_FOLDER}/macosx -L${EXT_PYTHON}/${TCL_FOLDER}/macosx"
    export LD_LIBRARY_PATH="${EXT_PYTHON}/${PYTHON_FOLDER}:${EXT_PYTHON}/${TK_FOLDER}/macosx:${EXT_PYTHON}/${TCL_FOLDER}/macosx:${LD_LIBRARY_PATH}"
    export DYLD_FALLBACK_LIBRARY_PATH="${EXT_PYTHON}/${PYTHON_FOLDER}:${EXT_PYTHON}/${TK_FOLDER}/macosx:${EXT_PYTHON}/${TCL_FOLDER}/macosx:${DYLD_FALLBACK_LIBRARY_PATH}"
    echoExec "ln -s ${XMIPP_HOME}/bin/xmipp_python ${XMIPP_HOME}/bin/python2.7"
    echoExec "cd ${EXT_PYTHON}/${VMATLIBPLOT}"
    echoExec "ln -s ${XMIPP_HOME}/bin/xmipp_python ${XMIPP_HOME}/bin/pythonXmipp" 
    echoExec "make -f make.osx clean"
    echoExec "make -f make.osx PREFIX=${XMIPP_HOME} PYVERSION=Xmipp fetch deps mpl_install"
    echoExec "rm ${XMIPP_HOME}/bin/pythonXmipp"
    echoExec "rm ${XMIPP_HOME}/bin/python2.7"
  elif $IS_MINGW; then
    export LDFLAGS="-L${EXT_PYTHON}/${PYTHON_FOLDER} -L${XMIPP_HOME}/lib -L${EXT_PYTHON}/${TK_FOLDER}/win -L${EXT_PYTHON}/${TCL_FOLDER}/win"
    export LD_LIBRARY_PATH="${EXT_PYTHON}/${PYTHON_FOLDER}:${EXT_PYTHON}/${TK_FOLDER}/win:${EXT_PYTHON}/${TCL_FOLDER}/win:${LD_LIBRARY_PATH}"
    echoExec "ln -s ${XMIPP_HOME}/bin/xmipp_python ${XMIPP_HOME}/bin/python2.7"
    echoExec "cd ${EXT_PYTHON}/${VMATLIBPLOT}"
    echoExec "ln -s ${XMIPP_HOME}/bin/xmipp_python ${XMIPP_HOME}/bin/pythonXmipp"
  else
    export LDFLAGS="-L${EXT_PYTHON}/${PYTHON_FOLDER} -L${XMIPP_HOME}/lib -L${EXT_PYTHON}/${TK_FOLDER}/unix -L${EXT_PYTHON}/${TCL_FOLDER}/unix"
    export LD_LIBRARY_PATH="${EXT_PYTHON}/${PYTHON_FOLDER}:${EXT_PYTHON}/${TK_FOLDER}/unix:${EXT_PYTHON}/${TCL_FOLDER}/unix:${LD_LIBRARY_PATH}"
    echoExec "cp ${EXT_PYTHON}/matplotlib_setupext.py ${EXT_PYTHON}/${VMATLIBPLOT}/setupext.py"
    #The following is needed from matplotlib to works
    echoExec "cd ${EXT_PYTHON}/${TK_FOLDER}/unix/"
    echoExec "ln -sf libtk8.5.so  libtk.so"
    echoExec "cd ${EXT_PYTHON}/${TCL_FOLDER}/unix/"
    echoExec "ln -sf libtcl8.5.so  libtcl.so"
  fi
  compile_pymodule ${VMATLIBPLOT}
  compile_pymodule ${VPYMPI}
  
  if $DO_CLTOMO; then
    # Fast Rotational Matching
    export LDFLAGS="-shared ${LDFLAGS}"
    compile_pymodule ${VSCIPY}
    cd ${EXT_PATH}/sh_alignment
    ./compile.sh
    GLOB_STATE=$?
    check_state 1
  fi
fi

# Launch the configure/compile python script 
cd ${XMIPP_HOME}

#echoGreen "Compiling XMIPP ..."
#echoGreen "CONFIGURE: $CONFIGURE_ARGS"
#echoGreen "COMPILE: $COMPILE_ARGS"
#echoGreen "GUI: $GUI_ARGS"

if $DO_SETUP; then
  echoExec "./setup.py -j ${NUMBER_OF_CPU} configure ${CONFIGURE_ARGS} compile ${COMPILE_ARGS} ${GUI_ARGS} install"
fi

exit 0

