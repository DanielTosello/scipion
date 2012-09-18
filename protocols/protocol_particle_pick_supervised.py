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

from protocol_particle_pick import getPosFiles, validateMicrographs, \
    launchParticlePickingGUI, PM_READONLY, PM_SUPERVISED, countParticles

# Create a GUI automatically from a selfile of micrographs
class ProtParticlePickingSupervised(XmippProtocol):
    def __init__(self, scriptname, project):
        XmippProtocol.__init__(self, protDict.particle_pick_supervised.name, scriptname, project)
        self.Import = "from protocol_particle_pick_supervised import *"
        self.setPreviousRun(self.PickingRun)
        self.pickingDir = self.PrevRun.WorkingDir
        self.inputProperty('TiltPairs', 'MicrographsMd')
        self.keysToImport = ['micrographs', 'families', 'acquisition']
        if xmippExists(self.PrevRun.getFilename('macros')):
            self.keysToImport.append('macros')
        self.inputFilename(*self.keysToImport)
        self.Input['micrographs'] = self.MicrographsMd
        self.micrographsMd = self.getEquivalentFilename(self.PrevRun, self.MicrographsMd)
        
    def createFilenameTemplates(self):
        return {
                'training': join('%(WorkingDir)s', '%(family)s_training.txt'),
                     'pos': join('%(WorkingDir)s', '%(micrograph)s.pos'),
                    'mask': join('%(WorkingDir)s', '%(family)s_mask.xmp'),
                    'pca': join('%(WorkingDir)s', '%(family)s_pca_model.stk'),
                    'maxmin': join('%(WorkingDir)s', '%(family)s_maxmin_dataset.txt'),
                    'svm': join('%(WorkingDir)s', '%(family)s_svm.txt'),
                    'svm2': join('%(WorkingDir)s', '%(family)s_svm2.txt'),
                    'average': join('%(WorkingDir)s', '%(family)s_particle_avg.xmp')
                }

    def defineSteps(self):
        filesToImport = [self.Input[k] for k in self.keysToImport]
        filesToImport += getPosFiles(self.PrevRun)
        self.insertImportOfFiles(filesToImport)
        modeWithArgs = PM_SUPERVISED + " %(NumberOfThreads)d %(Fast)s %(InCore)s" % self.ParamsDict
        self.insertStep('launchParticlePickingGUI', execution_mode=SqliteDb.EXEC_ALWAYS,
                           InputMicrographs=self.micrographsMd, WorkingDir=self.WorkingDir,
                           PickingMode=modeWithArgs, Memory=self.Memory, Family=self.Family)       
        
    def summary(self):
        md = xmipp.MetaData(self.micrographsMd)
        micrographs, particles, familiesDict = countParticles(self, 'auto')
        suffix = "micrographs"        
        summary = ["Manual picking RUN: <%s> " % self.PickingRun,
                   "Input picking: [%s] with <%u> %s" % (self.pickingDir, md.size(), suffix)]        

        return summary
    
    def validate(self):
        if self.TiltPairs:
            return "Supervised picking can't be done on tilted pairs run"
        return validateMicrographs(self.Input['micrographs'])
    
    def visualize(self):
        launchParticlePickingGUI(None, self.micrographsMd, self.WorkingDir, PM_READONLY)

# Main
#     
if __name__ == '__main__':
    protocolMain(ProtParticlePickingSupervised)
