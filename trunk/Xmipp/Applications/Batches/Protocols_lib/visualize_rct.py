#!/usr/bin/env python
#------------------------------------------------------------------------------------------------
# Protocol for visualization of the results of protocol_rct.py
#
# Example use:
# ./visualize_rct.py
#
# This script requires that protocol_rct.py is in the current directory
#
# Author: Sjors Scheres, March 2007
#
#------------------------------------------------------------------------------------------------
# {section} Global parameters
#------------------------------------------------------------------------------------------------
# For which classes do you want to perform the visualization?
SelectClasses="1,2"
# Visualize volumes in slices along Z?
VisualizeVolZ=False
# Visualize volumes in slices along X?
VisualizeVolX=False
# Visualize volumes in slices along Y?
VisualizeVolY=False
# Visualize volumes in UCSF Chimera?
""" For this to work, you need to have chimera installed!
"""
VisualizeVolChimera=True
# {expert} Width of selfile visualizations (preferably an even value):
MatrixWidth=10
#------------------------------------------------------------------------------------------------
# {section} Step-by-step visualization
#------------------------------------------------------------------------------------------------
# Visualize untilted average images?
VisualizeUntiltedAverages=True
# Visualize aligned untilted images?
VisualizeUntiltedImages=True
# Visualize aligned tilted images?
VisualizeTiltedImages=True
# Visualize ART reconstructions?
VisualizeArtVols=True
# Visualize WBP reconstructions?
VisualizeWbpVols=True
#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------
# {end-of-header} USUALLY YOU DO NOT NEED TO MODIFY ANYTHING BELOW THIS LINE ...
#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------
#
import protocol_rct
#
class visualize_RCT_class:

    #init variables
    def __init__(self,
                 SelectClasses,
                 VisualizeVolZ,
                 VisualizeVolX,
                 VisualizeVolY,
                 VisualizeVolChimera,
                 MatrixWidth,
                 VisualizeUntiltedAverages,
                 VisualizeUntiltedImages,
                 VisualizeTiltedImages,
                 VisualizeArtVols,
                 VisualizeWbpVols):
	     
        import os,sys
        scriptdir=os.path.expanduser('~')+'/scripts/'
        sys.path.append(scriptdir) # add default search path
        import visualization
     
        print '*  cd '+str(protocol_rct.WorkingDir)
        os.chdir(protocol_rct.WorkingDir)
        ShowVolumes=[]
        ShowSelfiles=[]
        ShowImages=[]

        refs=SelectClasses.split(',')
        for ref in refs:

            basename='rct_ref'+str(ref).zfill(5)
            if VisualizeUntiltedAverages:
                ShowImages.append(basename+'_untilted.med.xmp')

            if VisualizeUntiltedImages:
                ShowSelfiles.append(basename+'_untilted.sel')

            if VisualizeTiltedImages:
                ShowSelfiles.append(basename+'_tilted.sel')

            if VisualizeArtVols:
                ShowVolumes.append('art_'+basename+'_tilted.vol')

            if VisualizeWbpVols:
                ShowVolumes.append('wbp_'+basename+'_tilted.vol')

        visualization.visualize_volumes(ShowVolumes,
                                        VisualizeVolZ,
                                        VisualizeVolX,
                                        VisualizeVolY,
                                        VisualizeVolChimera)

        visualization.visualize_images(ShowSelfiles,True,MatrixWidth)

        visualization.visualize_images(ShowImages)
 
        # Return to parent dir
        print '*  cd ..'
        os.chdir(os.pardir)

    def getname(self,pattern):
        import glob,sys
        names=glob.glob(pattern)
        if len(names)<1:
            print 'No file '+pattern+' exists, ignore...'
            names=[""]
        elif len(names)>1:
            print 'Multiple files '+pattern+\
                  ' exist, taking only first one into account'
        return str(names[0])

    def close(self):
        message='Done!'
        print '*',message
        print '*********************************************************************'
#		
# Main
#     
if __name__ == '__main__':

    visualize_RCT=visualize_RCT_class(SelectClasses,
                                      VisualizeVolZ,
                                      VisualizeVolX,
                                      VisualizeVolY,
                                      VisualizeVolChimera,
                                      MatrixWidth,
                                      VisualizeUntiltedAverages,
                                      VisualizeUntiltedImages,
                                      VisualizeTiltedImages,
                                      VisualizeArtVols,
                                      VisualizeWbpVols)
    visualize_RCT.close()
