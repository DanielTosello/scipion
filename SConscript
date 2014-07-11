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


# First import the environment (this comes from SConstruct)
Import('env')

from os.path import abspath

#  ************************************************************************
#  *                                                                      *
#  *                              Libraries                               *
#  *                                                                      *
#  ************************************************************************

tcl = env.AddLibrary(
    'tcl',
    tar='tcl8.6.1-src.tgz',
    buildDir='tcl8.6.1/unix',
    targets=['lib/libtcl8.6.so'],
    flags=['--enable-threads',
           '--bindir=%s' % abspath('software/bin')])

tk = env.AddLibrary(
    'tk',
    tar='tk8.6.1-src.tgz',
    buildDir='tk8.6.1/unix',
    targets=['lib/libtk8.6.so'],
    flags=['--enable-threads',
           '--bindir=%s' % abspath('software/bin')],
    deps=[tcl])

sqlite = env.AddLibrary(
    'sqlite',
    tar='sqlite-3.6.23.tgz',
    targets=['lib/libsqlite3.so'],
    flags=['CPPFLAGS=-w',
           'CFLAGS=-DSQLITE_ENABLE_UPDATE_DELETE_LIMIT=1',
           '--bindir=%s' % abspath('software/bin')])

zlib = env.AddLibrary(
    'zlib',
    tar='zlib-1.2.8.tgz',
    buildDir='zlib-1.2.8',
    targets=['lib/libz.so'],
    autoConfigTarget='configure.log')

python = env.AddLibrary(
    'python',
    tar='Python-2.7.8.tgz',
    targets=['lib/libpython2.7.a', 'bin/python'],
    flags=['--enable-shared',
           'CFLAGS=-I/usr/include/ncurses'],
    deps=[sqlite, tk])


#  ************************************************************************
#  *                                                                      *
#  *                           Python Modules                             *
#  *                                                                      *
#  ************************************************************************

# Helper function to include the python dependency automatically.
def addModule(*args, **kwargs):
    kwargs['deps'] = kwargs.get('deps', []) + python
    return env.AddModule(*args, **kwargs)


addModule('numpy',
          tar='numpy-1.8.1.tgz')

addModule('matplotlib',
          tar='matplotlib-1.3.1.tgz')

addModule('psutil',
          tar='psutil-2.1.1.tgz')

addModule('mpi4py',
          tar='mpi4py-1.3.1.tgz')

addModule('scipy',
          tar='scipy-0.14.0.tgz',
          default=False)

addModule('bibtexparser',
          tar='bibtexparser-0.5.tgz')

addModule('django',
          tar='Django-1.5.5.tgz')

addModule('paramiko',
          tar='paramiko-1.14.0.tgz',
          default=False)

addModule('PIL',
          tar='Imaging-1.1.7.tgz',
          deps=[zlib])


#  ************************************************************************
#  *                                                                      *
#  *                       External (EM) Packages                         *
#  *                                                                      *
#  ************************************************************************

env.AddPackage('xmipp',
               tar='Xmipp-3.1-src.tgz',
               instDir='xmipp',
               url='http://xmipp.cnb.csic.es/Downloads/Xmipp-3.1-src.tgz',
               default=False)
# TODO: fix the url of this package!

env.AddPackage('bsoft',
               tar='bsoft1_8_8_Fedora_12.tgz',
               instDir='bsoft',
               default=False)

env.AddPackage('ctffind',
               tar='ctffind_V3.5.tgz',
               instDir='ctffind',
               default=False)
# TODO: check if instDir should be brandeis/ctffind instead

env.AddPackage('eman2',
               tar='eman2.1beta3.linux64.tgz',
               instDir='EMAN2',
               default=False)

env.AddPackage('frealign',
               tar='frealign_v9.07.tgz',
               default=False)

env.AddPackage('relion',
               tar='relion-1.2.tgz',
               default=False)

env.AddPackage('spider',
               tar='spider-web-21.13.tgz',
               instDir='spider-web',
               default=False)


# TODO: check if we have to use the "purge" option below:

# # Purge option
# if GetOption('purge'):
#     env.RemoveInstallation()
