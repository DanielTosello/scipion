'''
/***************************************************************************
 * Authors:     RobertoMarabini (roberto@cnb.csic.es)
 *              Jose Miguel de la Rosa
 *
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
#from emx_data_model import *
from xmipp import *
from emx.emx import *

BINENDING     = '.mrc'
CTFENDING     = '_ctf.param'
FAMILY        = 'DefaultFamily'
FIRSTIMAGE    =  1
MICFILE       = 'micrographs.xmd'
PARTFILE      = 'images.xmd'
POSENDING     = ".pos"
STACKENDING   = '.stk'

def ctfMicXmippToEmx(emxData,xmdFileName,amplitudeContrast):
    
    md    = MetaData()
    mdCTF = MetaData()
    md.read(xmdFileName)
    for objId in md:
        micrographName = md.getValue(MDL_MICROGRAPH, objId)
        ctfModel       = md.getValue(MDL_CTF_MODEL,objId)
        m1             = micrograph(fileName=micrographName)

        mdCTF.read(ctfModel)
        objId2 = mdCTF.firstObject()
        pixelSpacing        = mdCTF.getValue(MDL_CTF_SAMPLING_RATE, objId2)####
        acceleratingVoltage = mdCTF.getValue(MDL_CTF_VOLTAGE, objId2)
        cs                  = mdCTF.getValue(MDL_CTF_CS, objId2)
        defocusU            = mdCTF.getValue(MDL_CTF_DEFOCUSU, objId2)/10.
        defocusV            = mdCTF.getValue(MDL_CTF_DEFOCUSV, objId2)/10.
        defocusUAngle       = mdCTF.getValue(MDL_CTF_DEFOCUS_ANGLE, objId2)
        if defocusUAngle < 0:
            defocusUAngle += 180.
        elif defocusUAngle > 180.:
            defocusUAngle -= 180.;            
            
        m1.acceleratingVoltage.set(acceleratingVoltage)
        m1.amplitudeContrast.set(amplitudeContrast)
        m1.defocusU.set(defocusU)
        m1.defocusV.set(defocusV)
        m1.defocusUAngle.set(defocusUAngle)
        m1.pixelSpacing.X.set(pixelSpacing)
        m1.pixelSpacing.Y.set(pixelSpacing)
        m1.cs.set(cs)
        emxData.addObject(m1)

def ctfMicXmippToEmxChallenge(emxData,xmdFileName,amplitudeContrast):
    
    md    = MetaData()
    mdCTF = MetaData()
    md.read(xmdFileName)
    for objId in md:
        micrographName = md.getValue(MDL_MICROGRAPH, objId)
        ctfModel       = md.getValue(MDL_CTF_MODEL,objId)
        m1             = micrograph(fileName=micrographName)

        mdCTF.read(ctfModel)
        objId2 = mdCTF.firstObject()
        defocusU            = mdCTF.getValue(MDL_CTF_DEFOCUSU, objId2)/10.
        defocusV            = mdCTF.getValue(MDL_CTF_DEFOCUSV, objId2)/10.
        defocusUAngle       = mdCTF.getValue(MDL_CTF_DEFOCUS_ANGLE, objId2)
        if defocusUAngle < 0:
            defocusUAngle += 180.
        elif defocusUAngle > 180.:
            defocusUAngle -= 180.;            
            
        m1.defocusU.set(defocusU)
        m1.defocusV.set(defocusV)
        m1.defocusUAngle.set(defocusUAngle)
        emxData.addObject(m1)


def ctfMicEMXToXmipp(emxData,mode):
    #iterate though emxData
    mdMic     = MetaData()
    for micrograph in emxData.objLists[mode]:
        micIndex    = None
        micFileName = None
        #id = micrograph.getValue('id')
        if micrograph.id.has_key('index'):
            micIndex     = micrograph.id['index']
        if micrograph.id.has_key('fileName'):
            micFileName  = micrograph.id['fileName']
        if micFileName is None:
            if micIndex is None:
                raise Exception("ctfMicEMXToXmipp: Micrograph has neither filename not index")
            else: #only index? for xmipp index should behave as filename
                micFileName = FileName(str(micIndex).zfill(FILENAMENUMBERLENGTH))
                micIndex    = None
                fileName = FileName(micFileName)
        elif micIndex is None:
            fileName = FileName(micFileName)
        #micrograph is a stack. Unlikely but not impossible
        else:
            fileName=FileName()
            fileName.compose(int(micIndex),micFileName)


        mdMicId   = mdMic.addObject()
        mdMic.setValue(MDL_MICROGRAPH, fileName, mdMicId)
        ctfModelFileName = fileName.withoutExtension() + CTFENDING
        mdMic.setValue(MDL_CTF_MODEL, ctfModelFileName, mdMicId)

        acceleratingVoltage = micrograph.acceleratingVoltage.get()
        amplitudeContrast   = micrograph.amplitudeContrast.get()
        cs                  = micrograph.cs.get()

        defocusU            = micrograph.defocusU.get()
        if not defocusU is None:
            defocusU *= 10.
        defocusV            = micrograph.defocusV.get()
        if defocusV is None:
            defocusV = defocusU
            defocusUAngle = 0.
        else:
            defocusUAngle   = micrograph.defocusUAngle.get()
            defocusV       *= 10.
        pixelSpacingX    = micrograph.pixelSpacing.X.get()
        pixelSpacingY    = micrograph.pixelSpacing.Y.get()
        if not pixelSpacingY is None:
            if pixelSpacingX != pixelSpacingY:
                raise Exception ('pixelSpacingX != pixelSpacingY. xmipp does not support it') 

        MD = MetaData()
        MD.setColumnFormat(False)
        objId = MD.addObject()
        if pixelSpacingX:
            MD.setValue(MDL_CTF_SAMPLING_RATE, float(pixelSpacingX), objId)
        if acceleratingVoltage:
            MD.setValue(MDL_CTF_VOLTAGE,       float(acceleratingVoltage), objId)
        if cs:
            MD.setValue(MDL_CTF_CS,            float(cs), objId)
        if defocusU:
            MD.setValue(MDL_CTF_DEFOCUSU,      float(defocusU), objId)
        if defocusV:
            MD.setValue(MDL_CTF_DEFOCUSV,      float(defocusV), objId)
        if defocusUAngle:
            MD.setValue(MDL_CTF_DEFOCUS_ANGLE, float(defocusUAngle), objId)
        if amplitudeContrast:
            MD.setValue(MDL_CTF_Q0,            float(amplitudeContrast), objId)
        MD.setValue(MDL_CTF_K,             1.0, objId)
        MD.write(ctfModelFileName)
    mdMic.sort(MDL_MICROGRAPH)
    mdMic.write(MICFILE)
    
def coorrXmippToEmx(emxData,xmdFileName):
    ''' convert a single file '''
    md    = MetaData()
    md.read(xmdFileName)
    xmdFileNameNoExt = FileName(xmdFileName.withoutExtension())
    xmdFileNameNoExtNoBlock = xmdFileNameNoExt.removeBlockName()
    micrographName = xmdFileNameNoExtNoBlock + BINENDING
    particleName   = xmdFileNameNoExtNoBlock + STACKENDING
    m1 = micrograph(fileName=micrographName)
    emxData.addObject(m1)
    counter = FIRSTIMAGE
    for objId in md:

        coorX = md.getValue(MDL_XCOOR, objId)
        coorY = md.getValue(MDL_YCOOR, objId)
        p1             = particle(fileName=particleName, index=counter)
        p1.centerCoord.X.set(coorX)
        p1.centerCoord.Y.set(coorY)
        p1.setMicrograph(m1)
        emxData.addObject(p1)

        counter += 1

def coorEMXToXmipp(emxData,mode,emxFileName):
    #iterate though emxData
    mdParticle     = MetaData()
    for particle in emxData.objLists[mode]:
        mdPartId   = mdParticle.addObject()
        if particle.id.has_key('index'):
            partIndex     = particle.id['index']
        if particle.id.has_key('fileName'):
            partFileName  = particle.id['fileName']
        if partFileName is None:
            if partIndex is None:
                raise Exception("coorEMXToXmipp: Particle has neither filename not index")
            else: #only index? for xmipp index should behave as filename
                partFileName = FileName(str(partIndex).zfill(FILENAMENUMBERLENGTH))
                partIndex    = None
                fileName = FileName(partFileName)
        elif partIndex is None:
            fileName = FileName(partFileName)
        #micrograph is a stack. Unlikely but not impossible
        else:
            fileName=FileName()
            fileName.compose(int(partIndex),partFileName)

        centerCoordX   = particle.centerCoord.X.get()
        centerCoordY   = particle.centerCoord.Y.get()
        mdParticle.setValue(MDL_IMAGE, fileName, mdPartId)
        mdParticle.setValue(MDL_XCOOR, int(centerCoordX), mdPartId)
        mdParticle.setValue(MDL_YCOOR, int(centerCoordY), mdPartId)
    f = FileName()
    f.compose(FAMILY,emxFileName)
    mdParticle.sort(MDL_IMAGE)
    mdParticle.write(f.withoutExtension() + POSENDING)
    
        
