#!/usr/bin/env python

# **************************************************************************
# *
# * Authors:     I. Foche Perez (ifoche@cnb.csic.es)
# *              J. Burguet Castell (jburguet@cnb.csic.es)
# *
# * Unidad de Bioinformatica of Centro Nacional de Biotecnologia, CSIC
# *
# * This program is free software; you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation; either version 2 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with this program; if not, write to the Free Software
# * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# * 02111-1307  USA
# *
# *  All comments concerning this program package may be sent to the
# *  e-mail address 'ifoche@cnb.csic.es'
# *
# **************************************************************************

# Builders and pseudobuilders used by SConscript to install things.


import os
from os.path import join, abspath
import platform
import subprocess
import SCons.Script


# URL where we have most of our tgz files for libraries, modules and packages.
URL_BASE = 'http://scipionwiki.cnb.csic.es/files/scipion/software'

# Define our builders.
Download = Builder(action='wget -nv $SOURCE -c -O $TARGET')
Untar = Builder(action='tar -C $cdir --recursive-unlink -xzf $SOURCE')

# Create the environment the whole build will use.
env = Environment(ENV=os.environ,
                  BUILDERS=Environment()['BUILDERS'],
                  tools=['Make', 'AutoConfig'],
                  toolpath=[join('software', 'install', 'scons-tools')])
# TODO: BUILDERS var added from the tricky creation of a new environment.
# If not, they lose default builders like "Program", which are needed later
# (by CheckLib and so on). See http://www.scons.org/doc/2.0.1/HTML/scons-user/x3516.html
# See how to change it into a cleaner way (not doing BUILDERS=Environment()['BUILDERS']!)

# Message from autoconf and make, so we don't see all its verbosity.
env['AUTOCONFIGCOMSTR'] = "Configuring $TARGET from $SOURCES"
env['MAKECOMSTR'] = "Compiling & installing $TARGET from $SOURCES"


def progInPath(env, prog):
    "Is program prog in PATH?"
    return any(os.path.exists('%s/%s' % (base, prog)) for base in
               os.environ.get('PATH', '').split(os.pathsep))


def checkConfigLib(target, source, env):
    "See if we have library <name> from software/log/lib_<name>.log"

    # This is used to create the CheckConfigLib builder.

    # source is ignored and must be ''
    # target must look like 'software/log/lib_<name>.log'

    for tg in map(str, target):  # "target" is a list of SCons.Node.FS.File
        name = tg[len('software/log/lib_'):-len('.log')]
        try:
            subprocess.check_call(['pkg-config', '--cflags', '--libs', name],
                                  stdout=open(os.devnull, 'w'),
                                  stderr=subprocess.STDOUT)
        except (subprocess.CalledProcessError, OSError) as e:
            try:
                subprocess.check_call(['%s-config' % name, '--cflags'])
            except (subprocess.CalledProcessError, OSError) as e:
                print """
  ************************************************************************
    Warning: %s not found. Please consider installing it first.
  ************************************************************************

Continue anyway? (y/n)""" % name
                if raw_input().upper() != 'Y':
                    Exit(2)

                # If we continue without the lib, we write the target log
                # so scons knows not to ask again.
                open(tg, 'w').write('%s off\n' % name)
        # If we do have it, write the tg so we do not ask again either.
        open(tg, 'w').write('%s on\n' % name)


CheckConfigLib = Builder(action=checkConfigLib)


# Add the path to dynamic libraries so the linker can find them.

if platform.system() == 'Linux':
    env.AppendUnique(LIBPATH=os.environ.get('LD_LIBRARY_PATH'))
elif platform.system() == 'Darwin':
    print "OS not tested yet"
    env.AppendUnique(LIBPATH=os.environ.get('DYLD_FALLBACK_LIBRARY_PATH'))
elif platform.system() == 'Windows':
    print "OS not tested yet"
else:
    print "Unknown system: %s\nPlease tell the developers." % platform.system()


#  ************************************************************************
#  *                                                                      *
#  *                           Pseudobuilders                             *
#  *                                                                      *
#  ************************************************************************

# We have 4 "Pseudo-Builders" http://www.scons.org/doc/HTML/scons-user/ch20.html
#
# They are:
#   addLibrary    - install a library
#   addModule     - install a Python module
#   addPackage    - install an EM package
#   manualInstall - install by manually running commands
#
# Their structure is similar:
#   * Define reasonable defaults
#   * Set --with-<name> option as appropriate
#   * Concatenate builders
#
# For the last step we concatenate the builders this way:
#   target1 = Builder1(env, target1, source1)
#   SideEffect('dummy', target1)
#   target2 = Builder2(env, target2, source2=target1)
#   SideEffect('dummy', target2)
#   ...
#
# So each target becomes the source of the next builder, and the
# dependency is solved. Also, we use SideEffect('dummy', ...) to
# ensure it works in a parallel build (see
# http://www.scons.org/wiki/SConsMethods/SideEffect), and does not try
# to do one step while the previous one is still running in the background.

def addLibrary(env, name, tar=None, buildDir=None, targets=None,
               makeTargets=None, libChecks=[],
               url=None, flags=[], addPath=True, autoConfigTarget='Makefile',
               deps=[], clean=[], default=True):
    """Add library <name> to the construction process.

    This pseudobuilder checks that the needed programs are in PATH,
    downloads the given url, untars the resulting tar file, configures
    the library with the given flags, compiles it (in the given
    buildDir) and installs it. It also tells SCons about the proper
    dependencies (deps).

    If addPath=False, we will not pass the variables PATH and
    LD_LIBRARY_PATH pointing to our local installation directory.

    If default=False, the library will not be built unless the option
    --with-<name> is used.

    Returns the final targets, the ones that Make will create.

    """
    # Use reasonable defaults.
    tar = tar or ('%s.tgz' % name)
    url = url or ('%s/external/%s' % (URL_BASE, tar))
    buildDir = buildDir or tar.rsplit('.tar.gz', 1)[0].rsplit('.tgz', 1)[0]
    targets = targets or ['lib/lib%s.so' % name]

    # Add "software/lib" and "software/bin" to LD_LIBRARY_PATH and PATH.
    def pathAppend(var, value):
        valueOld = os.environ.get(var, '')
        i = 0  # so if flags is empty, we put it at the beginning too
        for i in range(len(flags)):
            if flags[i].startswith('%s=' % var):
                valueOld = flags.pop(i).split('=', 1)[1] + ':' + valueOld
                break
        flags.insert(i, '%s=%s:%s' % (var, value, valueOld))
    if addPath:
        pathAppend('LD_LIBRARY_PATH', abspath('software/lib'))
        pathAppend('PATH', abspath('software/bin'))

    flags += ['CFLAGS=-I%s' % abspath('software/include'),
              'LDFLAGS=-L%s' %  abspath('software/lib')]

    # Install everything in the appropriate place.
    flags += ['--prefix=%s' % abspath('software'),
              '--libdir=%s' % abspath('software/lib')]  # not lib64

    # Add the option --with-name, so the user can call SCons with this
    # to activate the library even if it is not on by default.
    if not default:
        AddOption('--with-%s' % name, dest=name, action='store_true',
                  help='Activate library %s' % name)

    # Add all the checks.
    checksTargets = ['software/log/lib_%s.log' % name for name in libChecks]
    for tg in checksTargets:
        CheckConfigLib(env, tg, '')

    # Create and concatenate the builders.
    tDownload = Download(env, 'software/tmp/%s' % tar,
                         [Value(url)] + checksTargets)
    SideEffect('dummy', tDownload)  # so it works fine in parallel builds
    tUntar = Untar(env, 'software/tmp/%s/configure' % buildDir, tDownload,
                   cdir='software/tmp')
    SideEffect('dummy', tUntar)  # so it works fine in parallel builds
    Clean(tUntar, 'software/tmp/%s' % buildDir)
    tConfig = env.AutoConfig(
        source=Dir('software/tmp/%s' % buildDir),
        AutoConfigTarget=autoConfigTarget,
        AutoConfigSource='configure',
        AutoConfigParams=flags,
        AutoConfigStdOut='software/log/%s_config.log' % name)
    SideEffect('dummy', tConfig)  # so it works fine in parallel builds

    if not makeTargets:
        # This should be the normal case
        lastTarget = tConfig
    else:
        # Some libraries (like swig) need to call "make" before "make
        # install", and so we have to complicate things. Thank you, swig.
        tMakePreInstall = env.Make(
            source=tConfig,
            target=['software/tmp/%s' % t for t in makeTargets],
            MakePath='software/tmp/%s' % buildDir,
            MakeEnv=os.environ,
            MakeStdOut='software/log/%s_make_pre-install.log' % name)
        SideEffect('dummy', tMakePreInstall)
        lastTarget = tMakePreInstall

    tMake = env.Make(
        source=lastTarget,
        target=['software/%s' % t for t in targets],
        MakePath='software/tmp/%s' % buildDir,
        MakeEnv=os.environ,
        MakeTargets='install',
        MakeStdOut='software/log/%s_make.log' % name)
    SideEffect('dummy', tMake)  # so it works fine in parallel builds

    # Clean the special generated files
    for cFile in clean:
        Clean(tMake, cFile)

    # Add the dependencies.
    for dep in deps:
        Depends(tConfig, dep)

    if default or GetOption(name):
        Default(tMake)

    return tMake


def addModule(env, name, tar=None, buildDir=None, targets=None,
              url=None, flags=[], deps=[], clean=[], default=True):
    """Add Python module <name> to the construction process.

    This pseudobuilder downloads the given url, untars the resulting
    tar file, configures the module with the given flags, compiles it
    (in the given buildDir) and installs it. It also tells SCons about
    the proper dependencies (deps).

    If default=False, the module will not be built unless the option
    --with-<name> is used.

    Returns the final target (software/lib/python2.7/site-packages/<name>).

    """
    # Use reasonable defaults.
    tar = tar or ('%s.tgz' % name)
    url = url or ('%s/python/%s' % (URL_BASE, tar))
    buildDir = buildDir or tar.rsplit('.tar.gz', 1)[0].rsplit('.tgz', 1)[0]
    targets = targets or [name]
    flags += ['--prefix=%s' % abspath('software')]

    # Add the option --with-name, so the user can call SCons with this
    # to activate the module even if it is not on by default.
    if not default:
        AddOption('--with-%s' % name, dest=name, action='store_true',
                  help='Activate module %s' % name)

    # Create and concatenate the builders.
    tDownload = Download(env, 'software/tmp/%s' % tar, Value(url))
    SideEffect('dummy', tDownload)  # so it works fine in parallel builds
    tUntar = Untar(env, 'software/tmp/%s/setup.py' % buildDir, tDownload,
                   cdir='software/tmp')
    SideEffect('dummy', tUntar)  # so it works fine in parallel builds
    Clean(tUntar, 'software/tmp/%s' % buildDir)
    tInstall = env.Command(
        ['software/lib/python2.7/site-packages/%s' % t for t in targets],
        tUntar,
        Action('PYTHONHOME="%(root)s" LD_LIBRARY_PATH="%(root)s/lib" '
               'PATH="%(root)s/bin:%(PATH)s" '
               'CFLAGS="-I%(root)s/include" LDFLAGS="-L%(root)s/lib" '
               '%(root)s/bin/python setup.py install %(flags)s > '
               '%(root)s/log/%(name)s.log 2>&1' % {'root': abspath('software'),
                                                   'PATH': os.environ['PATH'],
                                                   'flags': ' '.join(flags),
                                                   'name': name},
               'Compiling & installing %s > software/log/%s.log' % (name, name),
               chdir='software/tmp/%s' % buildDir))
    SideEffect('dummy', tInstall)  # so it works fine in parallel builds

    # Clean the special generated files
    for cFile in clean:
        Clean(lastTarget, cFile)

    # Add the dependencies.
    for dep in deps:
        Depends(tInstall, dep)

    if default or GetOption(name):
        Default(tInstall)

    return tInstall


def addPackage(env, name, tar=None, buildDir=None, url=None, neededProgs=[],
               extraActions=[], deps=[], clean=[], default=True):
    """Add external (EM) package <name> to the construction process.

    This pseudobuilder checks that the needed programs are in PATH,
    downloads the given url, untars the resulting tar file and copies
    its content from buildDir into the installation directory
    <name>. It also tells SCons about the proper dependencies (deps).

    extraActions is a list of (target, command) that should be
    executed after the package is properly installed.

    If default=False, the package will not be built unless the option
    --with-<name> or --with-all-packages is used.

    Returns the final target (software/em/<name>).

    """
    # Use reasonable defaults.
    tar = tar or ('%s.tgz' % name)
    url = url or ('%s/em/%s' % (URL_BASE, tar))
    buildDir = buildDir or tar.rsplit('.tar.gz', 1)[0].rsplit('.tgz', 1)[0]

    # Add the option --with-<name>, so the user can call SCons with this
    # to get the package even if it is not on by default.
    AddOption('--with-%s' % name, dest=name, metavar='%s_HOME' % name.upper(),
              nargs='?', const='unset',
              help=("Get package %s. With no argument, download and "
                    "install it. To use an existing installation, pass "
                    "the package's directory." % name))
    # So GetOption(name) will be...
    #   None      if we did *not* pass --with-<name>
    #   'unset'   if we passed --with-<name> (nargs=0)
    #   PKG_HOME  if we passed --with-<name>=PKG_HOME (nargs=1)

    # See if we have used the --with-<package> option and exit if appropriate.
    if GetOption('withAllPackages'):
        defaultPackageHome = 'unset'
        # we asked to install all packages, so it is at least as if we
        # also did --with-<name>
    else:
        defaultPackageHome = None
        # by default it is as if we did not use --with-<name>

    packageHome = GetOption(name) or defaultPackageHome

    # If we do have a local installation, link to it and exit.
    if packageHome != 'unset':  # default value when calling only --with-package
        # Just link to it and do nothing more.
        if packageHome is not None:
            Default('software/em/%s/bin' % name)
        return env.Command(
            Dir('software/em/%s/bin' % name),
            Dir(packageHome),
            Action('rm -rf %s && ln -v -s %s %s' % (name, packageHome, name),
                   'Linking package %s to software/em/%s' % (name, name),
                   chdir='software/em'))

    # Check that all needed programs are there.
    for p in neededProgs:
        if not progInPath(env, p):
            print """
  ************************************************************************
    Warning: Cannot find program "%s" needed by %s
  ************************************************************************

Continue anyway? (y/n)""" % (p, name)
            if raw_input().upper() != 'Y':
                Exit(2)

    # Donload, untar, link to it and execute any extra actions.
    tDownload = Download(env, 'software/tmp/%s' % tar, Value(url))
    SideEffect('dummy', tDownload)  # so it works fine in parallel builds
    tUntar = Untar(env, Dir('software/em/%s/bin' % buildDir), tDownload,
                   cdir='software/em')
    SideEffect('dummy', tUntar)  # so it works fine in parallel builds
    Clean(tUntar, 'software/em/%s' % buildDir)
    if buildDir != name:
        # Yep, some packages untar to the same directory as the package
        # name (hello Xmipp), and that is not so great. No link to it.
        tLink = env.Command(
            'software/em/%s/bin' % name,  # TODO: find smtg better than "/bin"
            Dir('software/em/%s' % buildDir),
            Action('rm -rf %s && ln -v -s %s %s' % (name, buildDir, name),
                   'Linking package %s to software/em/%s' % (name, name),
                   chdir='software/em'))
    else:
        tLink = tUntar  # just so the targets are properly connected later on
    SideEffect('dummy', tLink)  # so it works fine in parallel builds
    lastTarget = tLink

    for target, command in extraActions:
        lastTarget = env.Command('software/em/%s/%s' % (name, target),
                                 lastTarget,
                                 Action(command, chdir='software/em/%s' % name))
        SideEffect('dummy', lastTarget)  # so it works fine in parallel builds

    # Clean the special generated files
    for cFile in clean:
        Clean(lastTarget, cFile)
    # Add the dependencies. Do it to the "link target" (tLink), so any
    # extra actions (like setup scripts) have everything in place.
    for dep in deps:
        Depends(tLink, dep)

    if default or packageHome:
        Default(lastTarget)

    return lastTarget


def manualInstall(env, name, tar=None, buildDir=None, url=None, neededProgs=[],
                  extraActions=[], deps=[], clean=[], default=True):
    """Just download and run extraActions.

    This pseudobuilder downloads the given url, untars the resulting
    tar file and runs extraActions on it. It also tells SCons about
    the proper dependencies (deps).

    extraActions is a list of (target, command) that should be
    executed after the package is properly installed.

    If default=False, the package will not be built unless the option
    --with-<name> is used.

    Returns the final target in extraActions.

    """
    # Use reasonable defaults.
    tar = tar or ('%s.tgz' % name)
    url = url or ('%s/external/%s' % (URL_BASE, tar))
    buildDir = buildDir or tar.rsplit('.tar.gz', 1)[0].rsplit('.tgz', 1)[0]

    # Add the option --with-name, so the user can call SCons with this
    # to activate it even if it is not on by default.
    if not default:
        AddOption('--with-%s' % name, dest=name, action='store_true',
                  help='Activate %s' % name)

    # Check that all needed programs are there.
    for p in neededProgs:
        if not progInPath(env, p):
            print """
  ************************************************************************
    Warning: Cannot find program "%s" needed by %s
  ************************************************************************

Continue anyway? (y/n)""" % (p, name)
            if raw_input().upper() != 'Y':
                Exit(2)

    # Donload, untar, and execute any extra actions.
    tDownload = Download(env, 'software/tmp/%s' % tar, Value(url))
    SideEffect('dummy', tDownload)  # so it works fine in parallel builds
    tUntar = Untar(env, 'software/tmp/%s/README' % buildDir, tDownload,
                   cdir='software/tmp')
    SideEffect('dummy', tUntar)  # so it works fine in parallel builds
    Clean(tUntar, 'software/tmp/%s' % buildDir)
    lastTarget = tUntar

    for target, command in extraActions:
        lastTarget = env.Command(
            target,
            lastTarget,
            Action(command, chdir='software/tmp/%s' % buildDir))
        SideEffect('dummy', lastTarget)  # so it works fine in parallel builds

    # Clean the special generated files.
    for cFile in clean:
        Clean(lastTarget, cFile)

    # Add the dependencies.
    for dep in deps:
        Depends(tUntar, dep)

    if default or GetOption(name):
        Default(lastTarget)

    return lastTarget


# Add methods so SConscript can call them.
env.AddMethod(addLibrary, "AddLibrary")
env.AddMethod(addModule, "AddModule")
env.AddMethod(addPackage, "AddPackage")
env.AddMethod(manualInstall, "ManualInstall")
env.AddMethod(progInPath, "ProgInPath")


#  ************************************************************************
#  *                                                                      *
#  *                            Extra options                             *
#  *                                                                      *
#  ************************************************************************


opts = Variables(None, ARGUMENTS)

opts.Add('SCIPION_HOME', 'Scipion base directory', abspath('.'))

opts.Add('MPI_CC', 'MPI C compiler', 'mpicc')
opts.Add('MPI_CXX', 'MPI C++ compiler', 'mpiCC')
opts.Add('MPI_LINKERFORPROGRAMS', 'MPI Linker for programs', 'mpiCC')
opts.Add('MPI_INCLUDE', 'MPI headers dir ', '/usr/include')
opts.Add('MPI_LIBDIR', 'MPI libraries dir ', '/usr/lib')
opts.Add('MPI_LIB', 'MPI library', 'mpi')
opts.Add('MPI_BINDIR', 'MPI binaries', '/usr/bin')

opts.Update(env)

Help('\nVariables that can be set:\n')
Help(opts.GenerateHelpText(env))
Help('\n')


# Check that we have a working installation of MPI.
def WorkingMPI(context, mpi_inc, mpi_libpath, mpi_lib, mpi_cc, mpi_cxx, mpi_link):
    "Return 1 if a working installation of MPI is found, 0 otherwise"

    context.Message('* Checking for MPI ... ')

    oldValues = dict([(v, context.env.get(v, [])) for v in
                      ['LIBS', 'LIBPATH', 'CPPPATH', 'CC', 'CXX']])

    context.env.Prepend(LIBS=[mpi_lib], LIBPATH=[mpi_libpath], CPPPATH=[mpi_inc])
    context.env.Replace(LINK=mpi_link)
    context.env.Replace(CC=mpi_cc, CXX=mpi_cxx)

    # Test only to compile with C++.
    ret = context.TryLink("""
    #include <mpi.h>
    int main(int argc, char** argv)
    {
        MPI_Init(0, 0);
        MPI_Finalize();
        return 0;
    }""", '.cpp')  # scons, te odio

    # Put back the old values, we don't want MPI flags for non-MPI programs.
    context.env.Replace(**oldValues)

    context.Result(ret)
    return ret

# TODO: maybe change and put the proper thing here, like if we are compiling with mpi.
if not GetOption('help'):
    conf = Configure(env, {'WorkingMPI' : WorkingMPI}, 'config.tests', 'config.log')

    if conf.WorkingMPI(env['MPI_INCLUDE'], env['MPI_LIBDIR'],
                       env['MPI_LIB'], env['MPI_CC'], env['MPI_CXX'],
                       env['MPI_LINKERFORPROGRAMS']):
        print 'MPI seems to work.'
    else:
        print 'Warning: MPI seems NOT to work.'
# If you call "scipion install" it may get some cached result and you
# get surprising results that don't seem to make sense at all.
# If you call "scipion install --config=force" you have better
# chances, but then also other things may be rebuilt. Thanks scons...


AddOption('--with-all-packages', dest='withAllPackages', action='store_true',
          help='Get all EM packages')


Export('env')

env.SConscript('SConscript')

# Add original help (the one that we would have if we didn't use
# Help() before). But remove the "usage:" part (first line).
phelp = SCons.Script.Main.OptionsParser.format_help().split('\n')
Help('\n'.join(phelp[1:]))
# This is kind of a hack, because the #@!^ scons doesn't give you easy
# access to the original help message.
