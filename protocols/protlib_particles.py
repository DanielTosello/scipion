#!/usr/bin/env xmipp_python
'''
#/***************************************************************************
# * Authors:     J.M. de la Rosa Trevin (jmdelarosa@cnb.csic.es)
# *
# *
# * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
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
# *  e-mail address 'xmipp@cnb.csic.es'
# ***************************************************************************
'''
# This library contains some common utilities 
# for all particles related protocols: Extract, Import
from protlib_base import *
from xmipp import MetaData, MetaDataInfo, MDL_ZSCORE, getBlocksInMetaDataFile
from protlib_utils import runJob, runShowJ
from protlib_filesystem import moveFile
#MDL_CTF_SAMPLING_RATE, MDL_CTF_VOLTAGE, MDL_CTF_DEFOCUSU, MDL_CTF_DEFOCUSV, \
#MDL_CTF_DEFOCUS_ANGLE, MDL_CTF_CS, MDL_CTF_CA, MDL_CTF_Q0, MDL_CTF_K, label2Str, MetaData,\
#MDL_XCOOR, MDL_YCOOR, MDL_PICKING_FAMILY, MDL_PICKING_MICROGRAPH_FAMILY_STATE, MD_APPEND

class ProtParticlesBase(XmippProtocol):
    '''This class will serve as base for Particles related protocols'''
    def __init__(self, protocolName, scriptname, project):
        XmippProtocol.__init__(self, protocolName, scriptname, project)        
        self.Import = 'from protlib_particles import *; '
        self.Steps = [] # This list will be used for summary of steps applied
        
    def visualize(self):
        fn = self.getFilename('images')        
        if exists(fn):
            from protlib_utils import runShowJ
            if getattr(self, 'TiltPairs', False):
                runShowJ(fn,extraParams="--mode metadata --render first")
            else:
                runShowJ(fn)

            from protlib_gui_figure import XmippPlotter
            from xmipp import MDL_ZSCORE
            md = MetaData(fn)
            if md.containsLabel(MDL_ZSCORE):
                #MD.sort(MDL_ZSCORE)
                xplotter = XmippPlotter(windowTitle="Zscore particles sorting")
                xplotter.createSubPlot("Particle sorting", "Particle number", "Zscore")
                xplotter.plotMd(md, False, mdLabelY=MDL_ZSCORE)
                xplotter.show() 
                
    def addBasicPreprocessSteps(self):
        self.addSummaryStep(self.DoInvert, "Constrast inversion")
        self.addSummaryStep(self.DoRemoveDust, "Dust removal")
        self.addSummaryStep(self.DoNorm, "Ramp normalization")
        
    def addSummaryStep(self, condition, stepMsg):
        if condition:
            self.Steps.append(stepMsg)
            
    def addStepsSummary(self, messages):
        '''Add all steps applied to images '''
        if len(self.Steps):
            messages.append("Steps applied:")
            for i, m in enumerate(self.Steps):
                messages.append("  %d -> %s" % (i+1, m % self.ParamsDict))

def runThreshold(log, stack, selectionMode, threshold, substituteBy, substituteValue, Nproc):
    args="-i %(stack)s --select %(selectionMode)s %(threshold)f --substitute %(substituteBy)s" % locals()
    if substituteBy=='value':
        args+=" %f"%substituteValue
    runJob(log,"xmipp_transform_threshold",args,Nproc)

def runFourierFilter(log,stack,freq_low,freq_high,freq_decay,Nproc):
    program = "xmipp_transform_filter"
    args = "-i %(stack)s --fourier "
    if freq_low == 0:
        args += "low_pass %(freq_high)f %(freq_decay)f"
    elif freq_high == 0.5:
        args += "high_pass %(freq_low)f %(freq_decay)f"
    else:
        args += "band_pass %(freq_low)f %(freq_high)f %(freq_decay)f"
    runJob(log, program, args % locals())

def runGaussianFilter(log, stack, freq_sigma, Nproc):
    runJob(log,"xmipp_transform_filter","-i %(stack)s --fourier gaussian %(freq_sigma)f" % locals(),Nproc)

def runRealGaussianFilter(log, stack, real_sigma, Nproc):
    runJob(log,"xmipp_transform_filter","-i %(stack)s --fourier real_gaussian %(real_sigma)f" % locals(),Nproc)

def runCrop(log, stack, cropSize, tmpStack):
    runJob(log,"xmipp_transform_window","-i %(stack)s --size %(cropSize)d -o %(tmpStack)s" % locals())
    moveFile(log, tmpStack, stack)

def runResize(log,stack,new_size,Nproc):
    runJob(log,"xmipp_image_resize","-i %(stack)s --fourier %(new_size)d" % locals(),Nproc)

def invert(log,ImagesMd,Nproc):
    runJob(log,'xmipp_image_operate','-i %(ImagesMd)s --mult -1' % locals(),Nproc)

def removeDust(log,ImagesMd,threshold,Nproc):
    runJob(log,'xmipp_transform_filter','-i %(ImagesMd)s --bad_pixels outliers %(threshold)f' % locals(),Nproc)

def runNormalize(log,stack,normType,bgRadius,Nproc):
    program = "xmipp_transform_normalize"
    args = "-i %(stack)s "
    
    if bgRadius <= 0:
        particleSize = MetaDataInfo(stack)[0]
        bgRadius = int(particleSize/2)
    
    if normType=="OldXmipp":
        args += "--method OldXmipp"
    elif normType=="NewXmipp":
        args += "--method NewXmipp --background circle %(bgRadius)d"
    else:
        args += "--method Ramp --background circle %(bgRadius)d"
    runJob(log, program, args % locals(), Nproc)

def doMask(log,stack,maskFile,substitute,Nproc):
    runJob(log,"xmipp_transform_mask","-i %(stack)s --mask binary_file %(maskFile)s %(substitute)s" % locals(),Nproc)

def sortImages(log, ImagesFn, rejectionMethod='None', maxZscore=3, percentage=5):    
    md = MetaData(ImagesFn)
    if not md.isEmpty():
        args=""
        if rejectionMethod=='MaxZscore':
            args+=" --zcut "+str(maxZscore)
        elif rejectionMethod=='Percentage':
            args+=" --percent "+str(percentage)
        runJob(log, "xmipp_image_sort_by_statistics","-i %(ImagesFn)s --addToInput" % locals()+args)
        md.read(ImagesFn) # Should have ZScore label after runJob
        md.sort(MDL_ZSCORE)
        md.write(ImagesFn)        

def readPosCoordinates(posFile):
    """ Read the coordinates in .pos file and return corresponding metadata.
    There are two possible blocks with particles:
    particles: with manual/supervised particles
    particles_auto: with automatically picked particles.
    If posFile doesn't exist, the metadata will be empty 
    """
    md = MetaData()
    
    if exists(posFile):
        blocks = getBlocksInMetaDataFile(posFile)
        
        for b in ['particles', 'particles_auto']:
            if b in blocks:
                mdAux = MetaData('%(b)s@%(posFile)s' % locals())
                md.unionAll(mdAux)
        md.removeDisabled()
    
    return md

def countParticles(directory, pattern='*.pos'):
    '''Return the number of picked micrographs and particles '''
    particles = 0
    micrographs = 0
    import glob    
    for fnPos in glob.glob(os.path.join(directory,pattern)):
        md = readPosCoordinates(fnPos)
        pos_particles = md.size()
        if pos_particles > 0:
            particles += pos_particles
            micrographs += 1
    return micrographs, particles

