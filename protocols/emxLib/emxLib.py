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
from numpy import eye
from protlib_filesystem import join, dirname, abspath, replaceBasenameExt

BINENDING     = '.mrc'
CTFENDING     = '_ctf.param'
FAMILY        = 'DefaultFamily'
FIRSTIMAGE    =  1
MICFILE       = 'micrographs.xmd'
PARTFILE      = 'images.xmd'
POSENDING     = ".pos"
STACKENDING   = '.stk'


def ctfMicXmippToEmx(emxData,xmdFileName):
    
    md    = MetaData()
    mdCTF = MetaData()
    md.read(xmdFileName)
    for objId in md:
        micrographName = md.getValue(MDL_MICROGRAPH, objId)
        ctfModel       = md.getValue(MDL_CTF_MODEL,objId)
        m1             = Emxmicrograph(fileName=micrographName)

        mdCTF.read(ctfModel)
        objId2 = mdCTF.firstObject()
        pixelSpacing        = mdCTF.getValue(MDL_CTF_SAMPLING_RATE, objId2)####
        acceleratingVoltage = mdCTF.getValue(MDL_CTF_VOLTAGE, objId2)
        cs                  = mdCTF.getValue(MDL_CTF_CS, objId2)
        defocusU            = mdCTF.getValue(MDL_CTF_DEFOCUSU, objId2)/10.
        defocusV            = mdCTF.getValue(MDL_CTF_DEFOCUSV, objId2)/10.
        defocusUAngle       = mdCTF.getValue(MDL_CTF_DEFOCUS_ANGLE, objId2)
        amplitudeContrast   = mdCTF.getValue(MDL_CTF_Q0, objId2)
        while defocusUAngle < 0:
            defocusUAngle += 180.
        while defocusUAngle > 180.:
            defocusUAngle -= 180.;            
        
        m1.set('acceleratingVoltage',acceleratingVoltage)
        m1.set('amplitudeContrast',amplitudeContrast)
        m1.set('cs',cs)
        m1.set('defocusU',defocusU)
        m1.set('defocusV',defocusV)
        m1.set('defocusUAngle',defocusUAngle)
        m1.set('pixelSpacing__X',pixelSpacing)
        m1.set('pixelSpacing__Y',pixelSpacing)
        emxData.addObject(m1)

def ctfMicXmippToEmxChallenge(emxData,xmdFileName):
    
    md    = MetaData()
    mdCTF = MetaData()
    md.read(xmdFileName)
    for objId in md:
        micrographName = md.getValue(MDL_MICROGRAPH, objId)
        ctfModel       = md.getValue(MDL_CTF_MODEL,objId)
        m1             = Emxmicrograph(fileName=micrographName)

        mdCTF.read(ctfModel)
        objId2 = mdCTF.firstObject()
        defocusU            = mdCTF.getValue(MDL_CTF_DEFOCUSU, objId2)/10.
        defocusV            = mdCTF.getValue(MDL_CTF_DEFOCUSV, objId2)/10.
        defocusUAngle       = mdCTF.getValue(MDL_CTF_DEFOCUS_ANGLE, objId2)
        amplitudeContrast   = mdCTF.getValue(MDL_CTF_Q0, objId2)
        while defocusUAngle < 0:
            defocusUAngle += 180.
        while defocusUAngle > 180.:
            defocusUAngle -= 180.;            
            
        m1.set('defocusU',defocusU)
        m1.set('defocusV',defocusV)
        m1.set('defocusUAngle',defocusUAngle)
        emxData.addObject(m1)

class CTF(object):
    pass

def ctfMicEMXToXmipp(emxData, outputFileName=MICFILE, filesPrefix=None):
    #iterate though emxData
    mdMic = MetaData()
    root = dirname(outputFileName)
    
    samplingRate = 0.
    
    for micrograph in emxData.iterClasses(MICROGRAPH):
        micIndex = micrograph.get('index')
        micFileName = micrograph.get('fileName')

        if micFileName is None:
            raise Exception("ctfMicEMXToXmipp: Xmipp doesn't support Micrograph without filename")
        
        if filesPrefix is not None:
            micFileName = join(filesPrefix, micFileName)
        # micIndex is ignored now, in a more general solution
        # Xmipp should be able to handle micrographs in a stack
        # where the index has sense....

        mdMicId = mdMic.addObject()
        mdMic.setValue(MDL_MICROGRAPH, micFileName, mdMicId)
        ctfModelFileName = join(root, replaceBasenameExt(micFileName, CTFENDING))
        mdMic.setValue(MDL_CTF_MODEL, ctfModelFileName, mdMicId)

        # Dictionary for matching between EMX var and Xmipp labes
        ctfVarLabels = {'acceleratingVoltage': MDL_CTF_VOLTAGE,
                        'amplitudeContrast': MDL_CTF_Q0,
                        'cs': MDL_CTF_CS,
                        'defocusU': MDL_CTF_DEFOCUSU,
                        'defocusV': MDL_CTF_DEFOCUSV,
                        'defocusUAngle': MDL_CTF_DEFOCUS_ANGLE,
                        'pixelSpacing__X': MDL_CTF_SAMPLING_RATE,
                        }
        ctf = CTF()
        ctf.pixelSpacing__Y = micrograph.get('pixelSpacing__Y')
        
        # Set the variables in the dict
        for var in ctfVarLabels.keys():
            setattr(ctf, var, micrograph.get(var))

        # Now do some adjusments to vars        
        if ctf.defocusU is not None:
            ctf.defocusU *= 10.
        
        if ctf.defocusV is None:
            ctf.defocusV = ctf.defocusU
            ctf.defocusUAngle = 0.
        else:
            ctf.defocusV *= 10.
            
        if ctf.pixelSpacing__Y is not None:
            if ctf.pixelSpacing__X != ctf.pixelSpacing__Y:
                raise Exception ('pixelSpacingX != pixelSpacingY. Xmipp does not support it') 
            
        samplingRate = ctf.pixelSpacing__X
        # Create the .ctfparam, replacing the micrograph name
        mdCtf = MetaData()
        mdCtf.setColumnFormat(False)
        objId = mdCtf.addObject()
        
        for var, label in ctfVarLabels.iteritems():
            v = getattr(ctf, var)
            if v is not None:
                mdCtf.setValue(label, float(v), objId)
        
        mdCtf.setValue(MDL_CTF_K, 1.0, objId)
        mdCtf.write(ctfModelFileName)
        
        
        
    # Sort metadata by micrograph name
    mdMic.sort(MDL_MICROGRAPH)
    # Write micrographs metadata
    mdMic.write('Micrographs@' + outputFileName)
    # Create the acquisition info file
    acqFn = join(root, 'acquisition_info.xmd')
    mdAcq = MetaData()
    mdAcq.setColumnFormat(False)
    objId = mdAcq.addObject()
    mdAcq.setValue(MDL_SAMPLINGRATE, float(1), objId)
    mdAcq.write(acqFn)
    
    
def coorrXmippToEmx(emxData,xmdFileName):
    ''' convert a single file '''
    md    = MetaData()
    md.read(xmdFileName)
    xmdFileNameNoExt = FileName(xmdFileName.withoutExtension())
    xmdFileNameNoExtNoBlock = xmdFileNameNoExt.removeBlockName()
    micrographName = xmdFileNameNoExtNoBlock + BINENDING
    particleName   = xmdFileNameNoExtNoBlock + STACKENDING
    m1 = Emxmicrograph(fileName=micrographName)
    emxData.addObject(m1)
    counter = FIRSTIMAGE
    #check if MDL_XCOOR column exists if not 
    #print error no coordenates found. did you rememebr to include the block name in the filename
    for objId in md:

        coorX = md.getValue(MDL_XCOOR, objId)
        coorY = md.getValue(MDL_YCOOR, objId)
        p1    = Emxparticle(fileName=particleName, index=counter)
        p1.set('centerCoord__X',coorX)
        p1.set('centerCoord__Y',coorY)
        p1.setForeignKey(m1)
        emxData.addObject(p1)

        counter += 1

def coorEMXToXmipp(emxData,mode,emxFileName):
    #iterate though emxData
    mdParticle     = MetaData()
    for particle in emxData.objLists[mode]:
        mdPartId   = mdParticle.addObject()

        partIndex     = particle.get('index')
        partFileName  = particle.get('fileName')
        if partFileName is None:
            if partIndex is None:
                raise Exception("coorEMXToXmipp: Particle has neither filename not index")
            else: #only index? for xmipp index should behave as filename
                partFileName = FileName(str(partIndex).zfill(FILENAMENUMBERLENGTH))
                partIndex    = None
                fileName = FileName(partFileName)
        elif partIndex is None:
            fileName = FileName(partFileName)
        #particle is a stack. 
        else:
            fileName=FileName()
            fileName.compose(int(partIndex),partFileName)

        centerCoordX   = particle.get('centerCoord__X')
        centerCoordY   = particle.get('centerCoord__Y')
        mdParticle.setValue(MDL_IMAGE, fileName, mdPartId)
        mdParticle.setValue(MDL_XCOOR, int(centerCoordX), mdPartId)
        mdParticle.setValue(MDL_YCOOR, int(centerCoordY), mdPartId)
    f = FileName()
    f.compose(FAMILY,emxFileName)
    mdParticle.sort(MDL_IMAGE)
    mdParticle.write(f.withoutExtension() + POSENDING)
    
def alignXmippToEmx(emxData,xmdFileName):
    ''' convert a set of particles including geometric information '''
    md       = MetaData(xmdFileName)
    for objId in md:
        #get fileName
        fileName = FileName(md.getValue(MDL_IMAGE, objId))
        (index,fileName)=fileName.decompose()
        if index==0:
            index= None
        p1=Emxparticle(fileName=fileName, index=index)
        rot  = md.getValue(MDL_ANGLE_ROT ,objId)
        tilt = md.getValue(MDL_ANGLE_TILT,objId)
        psi  = md.getValue(MDL_ANGLE_PSI ,objId)
        if rot is  None:
            rot = 0
        if tilt is  None:
            tilt = 0
        if psi is  None:
            psi = 0
        tMatrix=Euler_angles2matrix(rot,tilt,psi)

        p1.set('transformationMatrix__t11', tMatrix[0][0])
        p1.set('transformationMatrix__t12', tMatrix[0][1])
        p1.set('transformationMatrix__t13', tMatrix[0][2])
        x = md.getValue(MDL_SHIFT_X, objId)
        p1.set('transformationMatrix__t14',x if x is not None else 0.)

        p1.set('transformationMatrix__t21', tMatrix[1][0])
        p1.set('transformationMatrix__t22', tMatrix[1][1])
        p1.set('transformationMatrix__t23', tMatrix[1][2])
        y = md.getValue(MDL_SHIFT_Y, objId)
        p1.set('transformationMatrix__t24',y if y is not None else 0.)

        p1.set('transformationMatrix__t31', tMatrix[2][0])
        p1.set('transformationMatrix__t32', tMatrix[2][1])
        p1.set('transformationMatrix__t33', tMatrix[2][2])
        z = md.getValue(MDL_SHIFT_Z, objId)
        p1.set('transformationMatrix__t34',z if z is not None else 0.)

        emxData.addObject(p1)
        
def alignEMXToXmipp(emxData,mode,xmdFileName):
    """import align related information
    """
    #iterate though emxData
    mdParticle     = MetaData()
    for particle in emxData.objLists[mode]:
        mdPartId   = mdParticle.addObject()

        partIndex     = particle.get('index')
        partFileName  = particle.get('fileName')
        if partFileName is None:
            if partIndex is None:
                raise Exception("coorEMXToXmipp: Particle has neither filename not index")
            else: #only index? for xmipp index should behave as filename
                partFileName = FileName(str(partIndex).zfill(FILENAMENUMBERLENGTH))
                partIndex    = None
                fileName = FileName(partFileName)
        elif partIndex is None:
            fileName = FileName(partFileName)
        #particle is a stack. Unlikely but not impossible
        else:
            fileName=FileName()
            fileName.compose(int(partIndex),partFileName)
        mdParticle.setValue(MDL_IMAGE , fileName , mdPartId)
        #eulerangle2matrix
        _array = eye(3)# unitary matrix

        t = particle.get('transformationMatrix__t11')
        _array[0][0]   = t if t is not None else 1.
        t = particle.get('transformationMatrix__t12')
        _array[0][1]   = t if t is not None else 0.
        t = particle.get('transformationMatrix__t13')
        _array[0][2]   = t if t is not None else 0.
        
        t = particle.get('transformationMatrix__t21')
        _array[1][0]   = t if t is not None else 0.
        t = particle.get('transformationMatrix__t22')
        _array[1][1]   = t if t is not None else 1.
        t = particle.get('transformationMatrix__t23')
        _array[1][2]   = t if t is not None else 0.

        t = particle.get('transformationMatrix__t31')
        _array[2][0]   = t if t is not None else 0.
        t = particle.get('transformationMatrix__t32')
        _array[2][1]   = t if t is not None else 0.
        t = particle.get('transformationMatrix__t33')
        _array[2][2]   = t if t is not None else 1.

        rot,tilt,psi = Euler_matrix2angles(_array)
        
        t = particle.get('transformationMatrix__t14')
        _X   = t if t is not None else 0.
        t = particle.get('transformationMatrix__t24')
        _Y   = t if t is not None else 0.
        t = particle.get('transformationMatrix__t34')
        _Z   = t if t is not None else 0.

        mdParticle.setValue(MDL_ANGLE_ROT , rot , mdPartId)
        mdParticle.setValue(MDL_ANGLE_TILT, tilt, mdPartId)
        mdParticle.setValue(MDL_ANGLE_PSI , psi , mdPartId)

        mdParticle.setValue(MDL_SHIFT_X, _X, mdPartId)
        mdParticle.setValue(MDL_SHIFT_Y, _Y, mdPartId)
        mdParticle.setValue(MDL_SHIFT_Z, _Z, mdPartId)
    mdParticle.write(xmdFileName)
