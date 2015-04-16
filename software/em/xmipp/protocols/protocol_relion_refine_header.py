#!/usr/bin/env python
#------------------------------------------------------------------------------------------------
# Protocol for Relion-based 3D classification
#
# Author: Roberto Marabini            roberto@cnb.csic.es    May 2013
#         J. M. de la Rosa Trevin     jmdelarosa@cnb.csic.es
#
#------------------------------------------------------------------------------------------------
# {begin_of_header}

# {eval} expandCommentRun()

# {eval} expandRelion(classify=False)

# {eval} expandParallel()

#------------------------------------------------------------------------------------------
# {end_of_header} USUALLY YOU DO NOT NEED TO MODIFY ANYTHING BELOW THIS LINE
#------------------------------------------------------------------------------------------------

from protocol_relion_refine import *

if __name__ == '__main__':
    protocolMain(ProtRelionRefinner)

