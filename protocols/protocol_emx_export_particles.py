#!/usr/bin/env python

#------------------------------------------------------------------------------------------------
#   General script for exporting particles to EMX format
#
#   Authors: J.M. de la Rosa Trevin, Sept 2013
#            Roberto Marabini
# 

from os.path import relpath, join, abspath, dirname, exists
from glob import glob
import math

import xmipp
from emx import *

from protlib_base import *
from protlib_filesystem import replaceBasenameExt, renameFile, copyFile
from protlib_utils import runJob
from protlib_xmipp import redStr, RowMetaData
from protlib_emx import *


class ProtEmxExportParticles(XmippProtocol):
    
    def __init__(self, scriptname, project):
        XmippProtocol.__init__(self, protDict.emx_export_particles.name, scriptname, project)
        self.Import = "from protocol_emx_export_particles import *"

            
    def createFilenameTemplates(self):
        return {
                 'emxDir': join('%(WorkingDir)s','emxData')
            } 
        
    def defineSteps(self):
        emxDir = self.getFilename('emxDir')
        self.insertStep("createDir", verifyfiles=[emxDir], 
                        path=emxDir)
        
        emxParticles = join(emxDir, 'particles.emx')
        emxParticlesMrc = join(emxDir, 'particles.mrc')
        self.insertStep("exportParticles"
                        , verifyfiles=[emxParticles, emxParticlesMrc]
                        , emxDir=emxDir
                        , inputMd=self.ImagesMd
                        , doAlign=self.DoAlign
                        )
        
    def validate(self):
        errors = []
        
        return errors

    def summary(self):
        emxDir = self.getFilename('emxDir')
        emxFile = join(emxDir, 'particles.emx')
        message=[]
        message.append ('Images to export: [%s]' % self.ImagesMd)
        if os.path.exists(emxFile):
            message.append ('EMX file: [%s]' % emxFile)
            message.append ('Directory with Binary files: [%s]' % emxDir)
        return message

    
    def papers(self):
        papers=[]
        papers.append('Marabini, Acta Crys D (2013) [http://www.ncbi.nlm.nih.gov/pubmed/23633578]')
        papers.append('Sorzano, Cap. 1 (2014) [http://www.springer.com/book/978-1-4614-9520-8]')
        return papers

    def visualize(self):
        emxDir = self.getFilename('emxDir')
        emxFile = join(emxDir, 'particles.emx')
        if os.path.exists(emxFile):
            from protlib_gui_ext import showTextfileViewer
            showTextfileViewer(emxFile,[emxFile])



def exportParticles(log, emxDir, inputMd, doAlign):
    print "INSIDE: exportParticles"
    emxData = EmxData()
    xmippParticlesToEmx(inputMd, emxData, emxDir, doAlign)

