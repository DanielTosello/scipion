#!/usr/bin/env xmipp_python
#------------------------------------------------------------------------------------------------
# General script for Xmipp-based particle picking
#
# Author: Carlos Oscar Sorzano, August, 2011
#
#------------------------------------------------------------------------------------------------

from config_protocols import protDict
from protlib_base import *
from protlib_utils import runJob
from protlib_filesystem import createLink, copyFile
import xmipp
from glob import glob
from os.path import exists, join

# Picking modes
PM_MANUAL, PM_SUPERVISED, PM_READONLY, PM_REVIEW = ('manual', 'supervised', 'readonly', 'review')

# Create a GUI automatically from a selfile of micrographs
class ProtParticlePicking(XmippProtocol):
    def __init__(self, scriptname, project):
        XmippProtocol.__init__(self, protDict.particle_pick.name, scriptname, project)
        self.Import = "from protocol_particle_pick import *"
        self.setPreviousRun(self.ImportRun)
        self.importDir = self.PrevRun.WorkingDir
        self.inputProperty('TiltPairs', 'MicrographsMd')
        self.Input['micrographs'] = self.MicrographsMd
        self.inputFilename('microscope', 'acquisition')

    def defineSteps(self):
        self.insertImportOfFiles(self.MicrographsMd, self.Input['acquisition'])
        self.insertStep('launchParticlePickingGUI',execution_mode=SqliteDb.EXEC_ALWAYS,
                           InputMicrographs=self.MicrographsMd, WorkingDir=self.WorkingDir,
                           TiltPairs=self.TiltPairs, Memory=self.Memory)       
        
    def summary(self):
        md = xmipp.MetaData(self.MicrographsMd)
        micrographs, particles, familiesDict = countParticles(self)
        if self.TiltPairs: 
            suffix = "tilt pairs"
            items = "pairs"
            micrographs /= 2
            particles /= 2
        else: 
            suffix = "micrographs"
            items = "particles"        
        
        summary = ["Input: [%s] with <%u> %s" % (self.importDir, md.size(), suffix),         
                   "Number of %(items)s manually picked: <%(particles)d> (from <%(micrographs)d> micrographs)" % locals()]
        
        families = len(familiesDict)
        if families > 1:
            summary.append("Number of families: <%u>" % families)
        
        for family, particles in familiesDict.iteritems():
            if self.TiltPairs:
                particles /= 2
            summary.append("Family <%(family)s>: <%(particles)u> %(items)s" % locals())
        
        return summary
    
    def validate(self):
        return validateMicrographs(self.Input['micrographs'], self.TiltPairs)
    
    def visualize(self):
        launchParticlePickingGUI(None, self.MicrographsMd, self.WorkingDir, PM_READONLY, self.TiltPairs)


def getPosFiles(prot, pattern=''):
    '''Return the .pos files of this picking protocol'''
    return glob(prot.workingDirPath('*%s.pos' % pattern))

def validateMicrographs(inputMicrographs, tiltPairs=False):
    ''' Validate the existence of input micrographs metadata file 
    and also each of the micrographs, return an error list if some 
    file is missing '''
    # Check that there is a valid list of micrographs
    if not exists(inputMicrographs):
        return ["Cannot find input micrographs: \n   <%s>" % inputMicrographs]
    # Check that all micrographs exist
    errors = []
    md = xmipp.MetaData(inputMicrographs)
    missingMicrographs = []        
    
    def checkMicrograph(label, objId): # Check if micrograph exists
        micrograph = md.getValue(label, objId)
        if not exists(micrograph):
            missingMicrographs.append(micrograph)
    for objId in md:
        checkMicrograph(xmipp.MDL_MICROGRAPH, objId)
        if tiltPairs:
            checkMicrograph(xmipp.MDL_MICROGRAPH_TILTED, objId)
    
    if len(missingMicrographs):
        errors.append("Cannot find the following micrographs: " + "\n".join(missingMicrographs))
    return errors    

def countParticles(prot, pattern=''):
    '''Return the number of picked micrographs, particles and 
    a dictionary with particles per family'''
    particles = 0
    micrographs = 0
    familiesDict = {}
    
    for posfile in getPosFiles(prot, pattern):
        blockList = xmipp.getBlocksInMetaDataFile(posfile)
        pos_particles = 0
        for block in blockList:
            if block != 'families':
                md = xmipp.MetaData("%(block)s@%(posfile)s" % locals());
                md.removeDisabled();
                block_particles = md.size()
                pos_particles += block_particles
                if block in familiesDict:
                    familiesDict[block] += block_particles
                else:
                    familiesDict[block] = block_particles
        if pos_particles > 0:
            particles += pos_particles
            micrographs += 1
    return micrographs, particles, familiesDict

def launchParticlePickingGUI(log, InputMicrographs, WorkingDir, PickingMode=PM_MANUAL,
                             TiltPairs=False, Memory=2):
    ''' Utility function to launch the Particle Picking application '''
    args = "-i %(InputMicrographs)s -o %(WorkingDir)s --mode %(PickingMode)s --memory %(Memory)dg"
    if TiltPairs:
        program = "xmipp_micrograph_tiltpair_picking"
    else:
        program = "xmipp_micrograph_particle_picking"
    runJob(log, program, args % locals(), RunInBackground=True)
#		
# Main
#     
if __name__ == '__main__':
    protocolMain(ProtParticlePicking)
