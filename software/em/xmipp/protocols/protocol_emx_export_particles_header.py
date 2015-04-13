#!/usr/bin/env python
#------------------------------------------------------------------------------------------------
# {begin_of_header}

#{eval} expandCommentRun()

#------------------------------------------------------------------------------------------------
# {section} Input  
#------------------------------------------------------------------------------------------------
# {file}(images*.xmd) Images to export:
"""Select a metadata containing your images to export
as EMX particles.
"""
ImagesMd = ''

# Is Alignment?
""" Check if you what to align (true) versus reconstruct (false)
"""
DoAlign=True

#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------
# {end_of_header} USUALLY YOU DO NOT NEED TO MODIFY ANYTHING BELOW THIS LINE ...
#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------
from protocol_emx_export_particles import *

if __name__ == '__main__':
    protocolMain(ProtEmxExportParticles)
