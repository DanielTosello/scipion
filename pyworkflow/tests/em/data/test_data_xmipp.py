'''
Created on May 20, 2013

@author: laura
'''

import unittest

from pyworkflow.em.packages.xmipp3 import *
from pyworkflow.tests import *


class TestXmippSetOfMicrographs(BaseTest):
        
    @classmethod
    def setUpClass(cls):
        setupTestOutput(cls)
        cls.dataset = DataSet.getDataSet('xmipp_tutorial')  
        cls.dbGold = cls.dataset.getFile( 'micsGoldSqlite')
        cls.mdGold = cls.dataset.getFile('micsGoldXmd')
    
    def testConvert(self):
        """ Test the convertion of a SetOfMicrographs to Xmipp metadata"""
        micSet = SetOfMicrographs(filename=self.dbGold)
        mdFn = self.getOutputPath('micrographs.xmd')
        
        writeSetOfMicrographs(micSet, mdFn)
                
        self.assertEqual(xmipp.MetaData(mdFn), xmipp.MetaData(self.mdGold), "xmipp metadata wrong")
                  
    
class TestXmippSetOfCoordinates(BaseTest):  
    
    @classmethod
    def setUpClass(cls):
        setupTestOutput(cls)
        cls.dataset = DataSet.getDataSet('xmipp_tutorial')  
        cls.dbGold = cls.dataset.getFile( 'micsGoldSqlite')
        cls.posDir = cls.dataset.getFile('posDir')
        cls.dbCoord = cls.dataset.getFile('coordsGoldSqlite')
        
        
    def testConvert(self):
        """ Test converting a SetOfCoordinates to Xmipp """
        micSet = SetOfMicrographs(filename=self.dbGold)
        
        # Test reading a set of coordinates
        fn = self.getOutputPath('coordinates.sqlite')
        coordSet = SetOfCoordinates(filename=fn)
        readSetOfCoordinates(self.posDir, micSet, coordSet)
        coordSet.write()
        
        # Test writing a set of coordinates to xmipp format
        coordSet = SetOfCoordinates(filename=self.dbCoord)
        coordSet.setBoxSize(512)
        coordSet.setMicrographs(micSet)
        writeSetOfCoordinates(self.outputPath, coordSet)
        
    
class TestXmippCTFModel(BaseTest):
    
    @classmethod
    def setUpClass(cls):
        setupTestOutput(cls)
        cls.dataset = DataSet.getDataSet('xmipp_tutorial')  

                    
    def testConvert(self):
        """ Test converting a CTFModel to Xmipp """
        pass




#class TestXmippSetOfMicrographs(unittest.TestCase):
#        
#    @classmethod
#    def setUpClass(cls):
#        cls.outputPath = getOutputPath('test_data_xmipp')
#        
#        cls.mdGold = getGoldPath('Micrographs_TiltedPhantom', 'micrographs_gold.xmd')
#        cls.dbGold = getGoldPath('Micrographs_TiltedPhantom', 'micrographs_gold.sqlite')
#        cls.dbGoldTilted = getGoldPath('Micrographs_TiltedPhantom', 'micrographs_tilted_gold.sqlite')
#        
#        cls.micsPattern = getInputPath('Micrographs_TiltedPhantom', '*.mrc')
#        
#        cls.mdFn = getOutputPath(cls.outputPath, 'micrographs.xmd')
#        cls.dbFn = getOutputPath(cls.outputPath, 'micrographs.sqlite')
#        cls.dbFnTilted = getOutputPath(cls.outputPath, 'micrographs_tilted.sqlite')
#        
#        #cls.mics = glob(cls.micsPattern)
#        cls.mics = []
#        for mic in iglob(cls.micsPattern):
#            cls.mics.append(getRelPath(mic))
#        
#        if len(cls.mics) == 0:
#            raise Exception('There are not micrographs matching pattern')
#        cls.mics.sort()
#    
#        """ Create tilted pairs """
#        cls.tiltedDict = {}
#        for i, fn in enumerate(cls.mics):
#            if i%2==0:
#                fn_u = fn
#            else:
#                cls.tiltedDict[fn] = fn_u  
#                
#    def setUp(self):
#        cleanPath(self.outputPath)
#        makePath(self.outputPath)
#                    
#    def testWrite(self):
#        """ Test creating and writing a XmippSetOfMicrographs from a list of micrographs """
#        
#        xmippSet = XmippSetOfMicrographs(self.mdFn, tiltPairs=False)
#        xmippSet.setSamplingRate(1.2)
#        for fn in self.mics:
#            mic = XmippMicrograph(fn)
#            xmippSet.append(mic)
#            
##        for u, t in self.tiltedDict.iteritems():
##            xmippSet.appendPair(u, t)  
#        
#        # Write the metadata
#        xmippSet.write()
#        
#        self.checkXmippSet(xmippSet)
#
#        
#    def testRead(self):
#        """ Test reading an XmippSetOfMicrographs from an existing  metadata """
#        xmippSet = XmippSetOfMicrographs(self.mdGold, tiltPairs=False)
#        
#        #Check that micrographs on metadata corresponds to the input ones (same order too)
#        for i, mic in enumerate(xmippSet):
#            self.assertEqual(self.mics[i], mic.getFileName(), "Micrograph %d in set is different from expected" % i)
#
##        for (iU, iT) in xmippSet.iterTiltPairs():
##            self.assertEqual(self.tiltedDict[iU], iT, "")
#            
#    def testConvert(self):
#        """ Test converting a SetOfMicrographs to a XmippSetOfMicrographs """
#        setMics = self.createSetOfMicrographs()
#                
#        xmippSet = XmippSetOfMicrographs.convert(setMics, self.mdFn)
#        
#        self.checkXmippSet(xmippSet)
#        
#        
#    def testCopy(self):
#        """ Test copying from a SetOfMicrographs to a XmippSetOFMicrographs """
#        setMics = self.createSetOfMicrographs()
#        
#        xmippSet = XmippSetOfMicrographs(self.mdFn)
#        
#        mapsId = {}
#        
#        for mic in setMics:
#            xmic = XmippMicrograph(mic.getFileName())
#            xmippSet.append(xmic)
#            mapsId[mic.getObjId()] = xmic.getObjId()
#            
#        xmippSet.copyInfo(setMics)
#        
#        # If input micrographs have tilt pairs copy the relation
#        if setMics.hasTiltPairs():
#            xmippSet.copyTiltPairs(setMics, mapsId.get)
#            
#        xmippSet.write()
#       
#        self.checkXmippSet(xmippSet)
#        
#    def checkXmippSet(self, setMics):
#        idCount = 1
#        
#        for fn, mic in zip(self.mics, setMics):            
#            self.assertEqual(fn, mic.getFileName(), "micrograph name in the set is wrong")
#            self.assertEqual(idCount, mic.getObjId(), "micrograph id in the set is wrong")
#            mic2 = setMics[idCount] # Test getitem
#            self.assertTrue(((mic.getFileName() == mic2.getFileName()) and (mic.getObjId() == mic2.getObjId())), "micrograph got from id is wrong")
#            idCount += 1     
#        
#        
#    def createSetOfMicrographs(self):
#        """ Create a SetOfMicrographs from a list of micrographs """
#        setMics = SetOfMicrographs(self.dbFn, tiltPairs=False)
#        setMics.setSamplingRate(1.2)
#        for fn in self.mics:
#            mic = Micrograph(fn)
#            setMics.append(mic)
##            if fn in self.tiltedDict.values():
##                mic_t = mic
##            else:
##                setMics.appendPair(mic.getObjId(), mic_t.getObjId()) 
#            
#        setMics.write()
#        
#        self.assertTrue(self.checkMicrographsDb(setMics), "micrographs database does not exist")
#        
#        return setMics
#        
#            
#    def checkMicrographsMetaData(self, xmippSet):
#        """ Check that a metadata is equal to the gold one """
#        #TODO: Implement how to check that two metadatas are equal
#        md_gold = xmipp.FileName(self.mdGold)
#        md_fn = xmipp.FileName(xmippSet.getFileName())
#        return (sys.getsizeof(md_fn) == sys.getsizeof(md_gold))
#
#    def checkMicrographsDb(self, setMics):
#        """ Check that a database is equal to the gold one """
#        #TODO: Implement how to check that two databases are equal
#        return (os.path.getsize(setMics.getFileName()) == os.path.getsize(self.dbGold))  
#    
#    
#class TestXmippSetOfCoordinates(unittest.TestCase):  
#    
#    @classmethod
#    def setUpClass(cls):
#        cls.outputPath = getOutputPath('test_data_xmipp')   
#        cls.posDir = getInputPath('Picking_XmippBPV1', 'extra/scipion_micrographs_coordinates.xmd') 
#        cls.micMd = getInputPath('Micrographs_BPV1', 'micrographs.xmd')
#        
#    def setUp(self):
#        cleanPath(self.outputPath)
#        makePath(self.outputPath)
#        
#    def testIterate(self):
#        """ Test reading an XmippSetOfCoordinates from an existing  directory """
#        xmippSetCoords = XmippSetOfCoordinates(self.posDir)
#        #Set micrographs associated to coordinates
#        xmippSetMics = XmippSetOfMicrographs(self.micMd)
#        xmippSetCoords.setMicrographs(xmippSetMics)
#        for coord in xmippSetCoords.iterCoordinates():
#            (x, y) = coord.getPosition()
#            #print ("Coordinate: x=%d y=%d" %(x,y))
#            #TODO: How to check coordinates, against what?
#            #self.assertTrue(x = , "micrographs database does not exist")
#    
#class TestXmippCTFModel(unittest.TestCase):
#    
#    @classmethod
#    def setUpClass(cls):
#        cls.outputPath = getOutputPath('test_data_xmipp')   
#        cls.ctfFn = getOutputPath(cls.outputPath, 'xmipp_ctf.ctfparam')
#        
#    def setUp(self):
#        cleanPath(self.outputPath)
#        makePath(self.outputPath)
#                    
#    def testConvertCtfFind(self):
#        """ Test converting a CTFModel to a XmippCTFModel """
#        ctfModel = CTFModel()
#        ctfModel.samplingRate.set(1.237)
#        ctfModel.voltage.set(300)
#        ctfModel.sphericalAberration.set(2.26)
#        ctfModel.defocusU.set(25039.42)
#        ctfModel.defocusV.set(25276.29)
#        ctfModel.defocusAngle.set(27.3)
#        ctfModel.ampContrast.set(0.1)
#        ctfModel.psdFile.set('input/CTF_BPV1/ctffind_psd.mrc')
#                                
#        xmippCTFModel = XmippCTFModel.convert(ctfModel, self.ctfFn)
#        
#        self.assertTrue(xmippCTFModel.getPsdFile() == 'input/CTF_BPV1/ctffind_psd.mrc', "psd file does not exists")
#
#    def testConvertXmippCtf(self):
#        """ Test converting a XmippCTFModel to a XmippCTFModel """
#        ctfParam = getInputPath('CTF_BPV1', 'xmipp_ctf.ctfparam')
#        ctfModel = XmippCTFModel(ctfParam)
#
#        xmippCTFModel = XmippCTFModel.convert(ctfModel, self.ctfFn)
#                
#        self.assertTrue(xmippCTFModel.get() == ctfModel.get(), 'conversion did not work')

        
if __name__ == '__main__':
#    suite = unittest.TestLoader().loadTestsFromName('test_data_xmipp.TestXmippSetOfMicrographs.testCopy')
#    unittest.TextTestRunner(verbosity=2).run(suite)
    
    unittest.main()
