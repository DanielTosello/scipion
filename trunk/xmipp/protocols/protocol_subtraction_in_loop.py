from xmipp import *
import os
from protlib_utils import runJob

#No defocus group
def joinImageCTFscale(_log, CTFgroupName,DocFileExp,inputSelfile):
    ctfMD = MetaData(CTFgroupName)
    MDaux = MetaData(inputSelfile)
    outMD = MetaData()
    outMD.join(MDaux, ctfMD, MDL_IMAGE_ORIGINAL, MDL_IMAGE, INNER_JOIN)
    outMD.write(DocFileExp, MD_APPEND)

#No defocus group
def joinImageCTF(_log, CTFgroupName,DocFileExp,inputSelfile):
    ctfMD = MetaData(CTFgroupName)
    MDaux = MetaData(inputSelfile)
    outMD = MetaData()
    outMD.join(MDaux, ctfMD, MDL_IMAGE, MDL_IMAGE, INNER_JOIN)
    outMD.write(DocFileExp, MD_APPEND)

#reconstruct
def reconstructVolume(_log 
                     ,DocFileExp
                     , MpiJobSize
                     , NumberOfMpi
                     , NumberOfThreads
                     , reconstructedVolume
                     , SymmetryGroup
                     , SystemFlavour
                     ):
    #xmipp_reconstruct_fourier -i ctfgroup_1@ctfgroups_Iter_13_current_angles.doc -o rec_ctfg01.vol --sym i3 --weight
    parameters  = ' -i ' +  DocFileExp 
    parameters += ' -o ' +  reconstructedVolume 
    parameters += ' --sym ' + SymmetryGroup +'h'
    if (NumberOfMpi>1):
            parameters += ' --mpi_job_size ' + MpiJobSize
            parameters += ' --thr ' + str(NumberOfThreads)

    runJob(_log,'xmipp_reconstruct_fourier',
                             parameters,
                             NumberOfMpi,
                             NumberOfThreads,
                             SystemFlavour)

def maskVolume(_log
              ,dRradiusMax
              ,dRradiusMin
              ,maskReconstructedVolume
              ,reconstructedVolume
):
    parameters  = ' -i ' +  reconstructedVolume 
    parameters += ' -o ' +  maskReconstructedVolume 
    parameters += ' --mask raised_crown -%d -%d 2' %(dRradiusMin, dRradiusMax)

    runJob( _log,"xmipp_transform_mask",
                             parameters,
                             False,1,1,'')
    
    #project
def createProjections(_log
                      ,AngSamplingRateDeg
                      ,DocFileExp
                      ,maskReconstructedVolume
                      ,MaxChangeInAngles
                      , MpiJobSize
                      ,NumberOfMpi
                      ,NumberOfThreads
                      ,referenceStack
                      ,SymmetryGroup
                      ,SystemFlavour
):

    parameters  = ' -i ' +  maskReconstructedVolume 
    parameters += ' --experimental_images ' +  DocFileExp
    
    parameters += ' -o ' +  referenceStack
    parameters += ' --sampling_rate ' + AngSamplingRateDeg
    parameters += ' --sym ' + SymmetryGroup +'h'
    parameters += ' --compute_neighbors --near_exp_data ' 
    parameters += ' --angular_distance ' + str(MaxChangeInAngles)
    if ((NumberOfMpi *NumberOfThreads)>1):
            parameters += ' --mpi_job_size ' + MpiJobSize

    runJob(_log,'xmipp_angular_project_library',
                             parameters,
                             NumberOfMpi *NumberOfThreads,
                             1,
                             SystemFlavour)
                


def subtractionScript(_log
                      ,DocFileExp
                      ,referenceStackDoc
                      ,subtractedStack
                      ):
    md = MetaData(DocFileExp)#experimental images
    mdRotations = MetaData(md) #rotations
    
    # Save Metadata with just rotations (shifts will be applied when reading)
    mdRotations.setValueCol(MDL_SHIFTX, 0.)
    mdRotations.setValueCol(MDL_SHIFTY, 0.)
    mdRotations.operate('anglePsi=-anglePsi')
    
    #reference projection for a given defocus group
    mdRef = MetaData(referenceStackDoc) #reference library
    
    imgExp = Image()
    imgRef = Image()
    imgSub = Image()
    #apply ctf to a temporary reference    
    for id in md:
        #refNum = md.getValue(MDL_REF, id)
        angRot = md.getValue(MDL_ANGLEROT, id)
        angTilt = md.getValue(MDL_ANGLETILT, id)
        psi = md.getValue(MDL_ANGLEPSI, id)
        
        # Search for the closest idRef
        dist = -1.
        distMin = 999.
        for idRef in mdRef:
            angRotRef  = mdRef.getValue(MDL_ANGLEROT, idRef)
            angTiltRef = mdRef.getValue(MDL_ANGLETILT, idRef)
            
            dist = abs(float(angRotRef) - float(angRot)) +  abs(float(angTiltRef) - float(angTilt))
            if(dist < distMin or dist == -1):
                refNum = idRef
                distMin = dist
                    
        # Apply alignment as follows: shifts firt (while reading), then rotations
        imgExp.readApplyGeo(md, id, True, DATA, ALL_IMAGES, False) # only_apply_shifts = true
        imgExp.applyGeo(mdRotations, id, False, False)  
        
        imgRef.readApplyGeo(mdRef,refNum, False, DATA, ALL_IMAGES,False)
        imgSub = imgExp - imgRef
        imgSub.write('%06d@%s'%(id,subtractedStack))
        imgExp.write('%06d@%s'%(id,subtractedStack+'exp'))
        imgRef.write('%06d@%s'%(id,subtractedStack+'ref'))
        
        mdRotations.setValue(MDL_IMAGE, '%06d@%s'%(id,subtractedStack), id)
        
    
    mdRotations.operate('anglePsi=0')
    mdRotations.write(subtractedStack.replace('.stk','.xmd'))

