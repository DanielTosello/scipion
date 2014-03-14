#!/usr/bin/env xmipp_python
#------------------------------------------------------------------------------------------------
# Protocol for screening a number of classes comparing them to a volume
#
# Author: Carlos Oscar Sorzano, March 2013 
#
# {begin_of_header}

#{eval} expandCommentRun()

#-----------------------------------------------------------------------------
# {section} Input
#-----------------------------------------------------------------------------

# {file}(classes*.xmd){validate}(PathExists) Set of remaining classes:
""" 
Provide a metadata or stack with classes to construct the model
"""
RemainingClasses = ""

# {file}(classes*.xmd) Set of complementary classes:
""" 
Images from this set will be used to complement the model
"""
Classes = ""

# Symmetry group
""" See [http://xmipp.cnb.csic.es/twiki/bin/view/Xmipp/Symmetry]
    for a description of the symmetry groups format
    If no symmetry is present, give c1
"""
SymmetryGroup ='c1'

#{expert} Angular sampling
""" In degrees. This sampling defines how fine the projection gallery from the volume is explored.
"""
AngularSampling = 5

#{expert} Number of RANSAC iterations
""" Number of initial volumes to test by RANSAC
"""
NRansac = 100

#{expert} Number of random samples
NumSamples = 8

#{expert} Inlier threshold
""" correlation value threshold to determine if an experimental projection is an
inlier or outlier
"""
CorrThresh = 0.77

#{expert} Number of best volumes to refine
""" Number of best volumes to refine 
"""
NumVolumes =10

#{expert} Number of iterations to perform to refine the volumes
""" Number of iterations to perform to refine the volumes 
"""
NumIter = 10

#{file}(*.vol) Initial Volume:
""" You may provide a very rough initial volume as a way to constraint the angular search.
    For instance, when reconstructing a fiber, you may provide a cylinder so that side views 
    are assigned to the correct tilt angle, although the rotational angle may be completely wrong
"""
InitialVolume = ''

#{expert} Max frequency of the initial volume
""" Max frequency of the initial volume in Angstroms
"""
MaxFreq = 5

# Sampling Rate
""" Sampling rate (A/px)
"""
Ts = '1'

#{expert}CorePercentile:
""" Percentile of coocurrence to be in the core """
CorePercentile=95

# {eval} expandParallel(threads=0,hours=12)

#------------------------------------------------------------------------------------------------
# {section}{visualize} Visualization
#------------------------------------------------------------------------------------------------
# Show volume list
DoShowList=True

# Show in projection explorer
"""Create a list of volumes like: 0,1,3 or 0-3 """
VolumesToShow=""

#------------------------------------------------------------------------------------------------
# {end_of_header} USUALLY YOU DO NOT NEED TO MODIFY ANYTHING BELOW THIS LINE ...
#------------------------------------------------------------------------------------------------

from protocol_hg3d import *
#        
# Main
#     
if __name__ == '__main__':
    protocolMain(ProtHG3D)
