'''
/***************************************************************************
 * Authors:     J.M. de la Rosa Trevin (jmdelarosa@cnb.csic.es)
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
 '''

#---------------------------------------------------------------------------
# Filesystem utilities
#---------------------------------------------------------------------------
from distutils.errors   import DistutilsInternalError
import os
from protlib_utils import printLogError
from xmipp import *

# The following are Wrappers to be used from Protocols
# provinding filesystem utitities

def createDir(_log, path):
    """ Create directory no add workingdir"""
    from distutils.dir_util import mkpath
    from distutils.errors import DistutilsFileError
    _log.info("Creating directory " + path)
    try:
        mkpath(path, 0777, True)
    except DistutilsFileError, e:
        printLogError(_log, "could not create '%s': %s" % (os.path.abspath(path), e))

def changeDir(_log, path):
    """ Change to Directory """
    _log.info("Changing to directory " + path)
    try:
        os.chdir(path)
    except os.error, (errno, errstr):
        printLogError(_log, "could not change to directory '%s'\nError (%d): %s" % (path, errno, errstr))

def deleteDir(_log, path):
    from distutils.dir_util import remove_tree
    _log.info("Deleting directory " + path)
    if os.path.exists(path):
        remove_tree(path, True)
           
def deleteWorkingDirectory(_log, WorkingDir, DoDeleteWorkingDir):
    if DoDeleteWorkingDir:
        deleteDir(_log,  WorkingDir)

def deleteFile(_mylog, FileName, Verbose):
    if os.path.exists(Filename):
        os.remove(Filename)
        if Verbose:
            printLog( 'Deleted file %s' % Filename )
    else:
        if Verbose:
            printLog( 'Do not need to delete %s; already gone' % Filename )

#--------------------------- Xmipp specific tools ---------------------------------
def getXmippPath(subfolder=''):
    '''Return the path the the Xmipp installation folder
    if a subfoder is provided, will be concatenated to the path'''
    import os
    protdir = os.popen('which xmipp_protocols', 'r').read()
    xmippdir = os.path.dirname(os.path.dirname(protdir))
    return os.path.join(xmippdir, subfolder)

def includeProtocolsDir():
    protDir = getXmippPath('protocols')
    import sys
    sys.path.append(protDir)
    

