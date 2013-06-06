'''
Created on May 8, 2013

@author: laura
'''
import unittest, sys
from pyworkflow.manager import Manager
from pyworkflow.em import *
from pyworkflow.tests import *
from pyworkflow.em.packages.xmipp3 import *
from PIL import Image


# Some utility functions to import micrographs that are used
# in several tests.
class TestXmippBase(unittest.TestCase):
    
    @classmethod
    def setUpClass(cls):    
        # Create a new project for the tests
        setupProject(cls)
        if hasattr(cls, '_setupClass'):
            cls._setupClass()
    
    @classmethod
    def runImportMicrograph(cls, pattern, samplingRate, voltage):
        """ Run an Import micrograph protocol. """
        cls.protImport = ProtImportMicrographs(pattern=pattern, samplingRate=samplingRate, voltage=voltage)
        cls.proj.launchProtocol(cls.protImport, wait=True)
        # check that input micrographs have been imported (a better way to do this?)
        if cls.protImport.outputMicrographs is None:
            raise Exception('Import of micrograph: %s, failed. outputMicrographs is None.' % pattern)
        return cls.protImport
        
    @classmethod
    def runImportMicrographBPV1(cls):
        """ Run an Import micrograph protocol. """
        pattern = getInputPath('Micrographs_BPV1', '*.mrc')
        return cls.runImportMicrograph(pattern, samplingRate=1.237, voltage=300)
    
    @classmethod
    def runFakedPicking(cls, mics):
        """ Run a faked particle picking. Coordinates already existing. """
        coordsFolder = getInputPath('Picking_XmippBPV1')
        cls.protPP = XmippProtParticlePicking(importFolder=coordsFolder)                
        cls.protPP.inputMicrographs.set(mics)        
        cls.proj.launchProtocol(cls.protPP, wait=True)
        # check that faked picking has run ok
        if cls.protPP.outputCoordinates is None:
            raise Exception('Faked particle picking: %s, failed. outputCoordinates is None.' % coordsFolder)
        return cls.protPP       
        
 
class TestXmippPreprocessMicrographs(TestXmippBase):

    @classmethod
    def _setupClass(cls):
        cls.protImport = cls.runImportMicrographBPV1()

    def testDownsampling(self):
        # test downsampling a set of micrographs
        downFactor = 2
        protDown = XmippProtPreprocessMicrographs(doDownsample=True, downFactor=downFactor)
        protDown.inputMicrographs.set(self.protImport.outputMicrographs)
        self.proj.launchProtocol(protDown, wait=True)
        
        # check that output micrographs have double sampling rate than input micrographs
        self.assertTrue(protDown.outputMicrographs.samplingRate.get() == self.protImport.outputMicrographs.samplingRate.get()*downFactor, "Micrographs uncorrectly downsampled")
        
    def testCrop(self):
        # test crop on a set of micrographs
        cropPixels = 100
        protCrop = XmippProtPreprocessMicrographs(doCrop=True, cropPixels=cropPixels)
        protCrop.inputMicrographs.set(self.protImport.outputMicrographs)
        self.proj.launchProtocol(protCrop, wait=True)
        
        
class TestXmippCTFEstimation(TestXmippBase):
    
    def doCTF(self, pattern):
        #First, import a set of micrographs
        protImport = self.runImportMicrograph(pattern, samplingRate=3.711, voltage=300)
        
        # Now estimate CTF on the downsampled micrographs
        print "Performing CTF..."   
        protCTF = XmippProtCTFMicrographs()                
        protCTF.inputMicrographs.set(protImport.outputMicrographs)        
        self.proj.launchProtocol(protCTF, wait=True)
        
        self.assertTrue(protCTF.outputMicrographs.hasCTF(), "CTF estimation has not been performed.")
        
    def test_Micrographs_BPV1_Down3(self):
        self.doCTF(pattern = getInputPath('Micrographs_BPV1_Down3', '*.mrc'))
        
    def test_Micrographs_BPV3_Down3(self):
        self.doCTF(pattern = getInputPath('Micrographs_BPV3_Down3', '*.mrc')) 
    
    
class TestXmippExtractParticles(TestXmippBase):
    
    SAME_AS_PICKING = 1
        
    def doExtract(self, boxSize, downsampleType):
        print "Run extract particles"
        pattern = getInputPath('Micrographs_BPV1', '*.mrc')
        protImport = self.runImportMicrograph(pattern, samplingRate=1.237, voltage=300)
        protPP = self.runFakedPicking(protImport.outputMicrographs)
        protExtract = XmippProtExtractParticles(boxSize=boxSize, downsampleType=downsampleType)
        protExtract.inputCoordinates.set(protPP.outputCoordinates)
        self.proj.launchProtocol(protExtract, wait=True)
        
        self.assertIsNotNone(protExtract.outputImages, "There was a problem with the extract particles")
        
    def testExtractSameAsPicking(self):
        print "Run extract particles with Same as picking"
        self.doExtract(512, self.SAME_AS_PICKING)
        
class TestXmippML2D(TestXmippBase):
    
    def testML2D(self):
        pass
        

if __name__ == "__main__":
    suite = unittest.TestLoader().loadTestsFromTestCase(TestXmippExtractParticles)
    #suite = unittest.TestLoader().loadTestsFromName('test_protocols_xmipp.TestXmippTiltedMicrographs.testPreprocess')
    unittest.TextTestRunner(verbosity=2).run(suite)