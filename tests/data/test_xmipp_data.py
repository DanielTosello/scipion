'''
Created on May 20, 2013

@author: laura
'''

import unittest
from glob import glob
from pyworkflow.em.packages.xmipp3 import *
import os, sys

class TestXmippSetOfMicrographs(unittest.TestCase):
    
    def setUp(self):
        self.mdGold = '/home/laura/Scipion_Projects/tests/micrographs_gold.xmd'
        self.dbGold = '/home/laura/Scipion_Projects/tests/micrographs_gold.sqlite'
        
        self.micsDir = '/home/laura/Scipion_Projects/TiltedData/*.mrc'
        self.mdFn = '/home/laura/Scipion_Projects/tests/micrographs.xmd'
        self.dbFn = '/home/laura/Scipion_Projects/tests/micrographs.sqlite'
        
        self.mics = glob(self.micsDir)
        if len(self.mics) == 0:
            raise Exception('There is not mics matching pattern')
        self.mics.sort()
                    
        self.tiltedDict = self.createTiltedDict()
                
    def testWriteMd(self):
        """ Test creating and writing a XmippSetOfMicrographs from a list of micrographs """
        
        xmippSet = XmippSetOfMicrographs(self.mdFn, tiltPairs=True)
        xmippSet.setSamplingRate(1.2)
        for fn in self.mics:
            mic = XmippMicrograph(fn)
            xmippSet.append(mic)
            
        for u, t in self.tiltedDict.iteritems():
            xmippSet.appendPair(u, t)  
        
        # Write the metadata
        xmippSet.write()

        self.assertTrue(self.checkMicrographsMetaData(xmippSet), "micrographs metadata does not exist")
        
    def testRead(self):
        """ Test reading an XmippSetOfMicrographs from a metadata """
        xmippSet = XmippSetOfMicrographs(self.mdFn, tiltPairs=True)
        
        error = False
        #Check that micrographs on metadata corresponds to the input ones (same order too)
        #TODO: Check against gold metadata????
        for i, mic in enumerate(xmippSet):
            if self.mics[i] != mic.getFileName():
                error = True
                break

        self.assertFalse(error, "Error on Micrographs data block")
            
        for (iU, iT) in xmippSet.iterTiltPairs():
            if self.tiltedDict[iU] != iT:
                error = True
                break
            
        self.assertFalse(error, "Error on TiltedPairs data block")
            
    def testConvert(self):
        """ Test converting a SetOfMicrographs to a XmippSetOfMicrographs """
        setMics = self.createSetOfMicrographs()
                
        xmippSet = XmippSetOfMicrographs.convert(setMics, self.mdFn)
        
        self.assertTrue(self.checkMicrographsMetaData(xmippSet), "micrographs metadata does not exist")
        
    def testCopy(self):
        """ Test copying from a SetOfMicrographs to a XmippSetOFMicrographs """
        setMics = self.createSetOfMicrographs()
        
        xmippSet = XmippSetOfMicrographs(self.mdFn)
        
        mapsId = {}
        
        for mic in setMics:
            xmic = XmippMicrograph(mic.getFileName())
            xmippSet.append(xmic)
            mapsId[setMics.getId(mic)] = xmippSet.getId(xmic)
            
        xmippSet.copyInfo(setMics)
        
        # If input micrographs have tilt pairs copy the relation
        if setMics.hasTiltPairs():
            xmippSet.copyTiltPairs(setMics, mapsId)
            
        xmippSet.write()
        
    def testMerge(self):
        
        mdCtfFn = '/home/laura/Scipion_Projects/tests/micrographs_ctf.xmd'
        mdCtfOut = '/home/laura/Scipion_Projects/tests/micrographs_out.xmd'
        
        mdCtf = xmipp.MetaData(mdCtfFn) 
        
        md = xmipp.MetaData('TiltedPairs@' + self.mdFn)
        
        xmippSet = XmippSetOfMicrographs(mdCtfOut)
        
        xmippSet.appendFromMd(md)
        
        xmippSet.write()
        
    def createSetOfMicrographs(self):
        #Remove sqlite db
        os.remove(self.dbFn)
        
        setMics = SetOfMicrographs(self.dbFn, tiltPairs=True)
        setMics.setSamplingRate(1.2)
        for fn in self.mics:
            mic = Micrograph(fn)
            setMics.append(mic)
            if fn in self.tiltedDict:
                mic_u = mic
            else:
                setMics.appendPair(mic_u.getObjId(), mic.getObjId()) 
            
        setMics.write()
        
        self.assertTrue(self.checkMicrographsDb(setMics), "micrographs database does not exist")
        
        return setMics
        
            
    def checkMicrographsMetaData(self, xmippSet):
        """ Check that a metadata is equal to the gold one """
        #TODO: Implement how to check that two metadatas are equal
        md_gold = xmipp.FileName(self.mdGold)
        md_fn = xmipp.FileName(xmippSet.getFileName())
        return (sys.getsizeof(md_fn) == sys.getsizeof(md_gold))

    def checkMicrographsDb(self, setMics):
        """ Check that a database is equal to the gold one """
        #TODO: Implement how to check that two databases are equal
        return (os.path.getsize(setMics.getFileName()) == os.path.getsize(self.dbGold))    
    
    def createTiltedDict(self):
        """ Create tilted pairs """
        tiltedDict = {}
        for i, fn in enumerate(self.mics):
            if i%2==0:
                fn_u = fn
            else:
                tiltedDict[fn_u] = fn  
        return tiltedDict
            
if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromName('test_xmipp_data.TestXmippSetOfMicrographs.testMerge')
    unittest.TextTestRunner(verbosity=2).run(suite)