#!/usr/bin/env xmipp_python

import os
from protlib_xmipp import ScriptAppIJ
from xmipp import Program, FileName
import xmipp
#512 is too low memory, should be 1024 the default.
class ScriptTrainingPicking(ScriptAppIJ):
    def __init__(self):
        ScriptAppIJ.__init__(self, 'xmipp.viewer.particlepicker.training.Main')
        
    def defineOtherParams(self):
        self.addParamsLine(" -o <directory>                                       : Output directory for load/save session data without updating model.");
        self.addParamsLine("    alias --output;                                   ");
        self.addParamsLine(" [--family <family=\"\">]                              : Specifies family to work with. Not mandatory");
        self.addParamsLine(" --mode <pick_mode=manual>                            : Mode in wich Particle Picker will be used");
        self.addParamsLine("    where <pick_mode>");
        self.addParamsLine("       manual                                         : Enables manual mode. User will pick particles manually.");
        self.addParamsLine("       supervised <thr=1> <fast=True> <incore=False>  : Enables supervised mode. User will use autopicking. Then review/correct particles selected."); 
        self.addParamsLine("                                                      : Particles from manual mode will be used to train software and switch to supervised mode");
        self.addParamsLine("                                                      : Autopicker will use number of threads, fast and incore modes provided");
        self.addParamsLine("       review <file>                                  : Enables review mode. User reviews/corrects particles set provided on specified file");
        self.addParamsLine("                                                      : without updating model. ");
        self.addParamsLine("       readonly                                       : Enables readonly mode. User can see the picked particles, but cannot modify them.");
        
    def readOtherParams(self):
        input = self.getParam('-i')
        output = self.getParam('-o')
        family = self.getParam('--family')
        mode = self.getParam('--mode')

        if (not os.path.exists(output)):
            os.makedirs(output)
        supervised = (mode == 'supervised')
        if supervised:
            
            numberOfThreads = self.getIntParam('--mode', 1)
            fastMode = self.getParam('--mode', 2)
            incore = self.getParam('--mode', 3)
            
        review = (mode == 'review')
        if review:
            file = self.getParam('--mode', 1)

        if family is None:
            self.args = "%(input)s %(output)s %(mode)s" %locals()
        else:
            self.args = "%(input)s %(output)s %(family)s %(mode)s" %locals()
        if supervised:
            self.args += " %(numberOfThreads)d %(fastMode)s %(incore)s" %locals()
        if review:
            self.args += " %(file)s" %locals()
        
if __name__ == '__main__':
    ScriptTrainingPicking().tryRun()

       

