#!/usr/bin/env python
#------------------------------------------------------------------------------------------------
#
# General script for Xmipp-based pre-processing of single-particles: 
# Author: Carlos Oscar, August 2011
#
# {begin_of_header}
#------------------------------------------------------------------------------------------
# {section}{has_question} Comment
#------------------------------------------------------------------------------------------
# Display comment
DisplayComment = False

# {text} Write a comment:
""" 
Describe your run here...
"""

#-----------------------------------------------------------------------------
# {section} Run parameters
#-----------------------------------------------------------------------------
# Run name:
""" This will identify your protocol run. It need to be unique for each protocol. You could have run1, run2 for protocol X, but not two
run1 for it. This name together with the protocol output folder will determine the working dir for this run.
"""
RunName = "run_001"

# {list}(Resume, Restart) Run behavior
""" Resume from the last step, restart the whole process
"""
Behavior = "Restart"

#-----------------------------------------------------------------------------
# {section} Filter parameters
#-----------------------------------------------------------------------------
# {run}(extract_particles) Particle extraction run
ExtractionRun=''

# {wizard}(wizardChooseFamily) Family to preprocess
Family=''

# Fourier bandpass filter
""" You may do a lowpass filter by setting Freq_low to 0. You may do a high pass filter
    by setting Freq_high to 0.5."""
DoFourier=False 

#{condition}(DoFourier) Freq_low (0<f<0.5)
""" Set it to 0 for low pass filters """
Freq_low=0.02

#{condition}(DoFourier) Freq_high (0<f<0.5)
""" Set it to 0.5 for high pass filters """
Freq_low=0.35

# Fourier Gaussian
""" Gaussian filter defined in Fourier space"""
DoGaussian=False

#{condition}(DoGaussian) Frequency sigma
""" Remind that the Fourier frequency is normalized between 0 and 0.5"""
Freq_sigma=0.04

# Remove dust
""" Sets pixels with unusually large values to random values from a Gaussian with zero-mean and unity-standard deviation.
    It requires a previous normalization, i.e., Normalization must be set to Yes.
"""
DoRemoveDust=False

# {condition}(DoRemoveDust)  Threshold for dust removal:
""" Pixels with a signal higher or lower than this value times the standard deviation of the image will be affected. For cryo, 3.5 is a good value.
    For high-contrast negative stain, the signal itself may be affected so that a higher value may be preferable.
"""
DustRemovalThreshold=3.5

# Normalize
""" It subtract a ramp in the gray values and normalizes so that in the background
    there is 0 mean and standard deviation 1 """
DoNorm=False

# {list_combo}(OldXmipp,NewXmipp,Ramp){condition}(DoNorm) Normalization type
""" OldXmipp (mean(Image)=0, stddev(Image)=1). NewXmipp (mean(background)=0, stddev(background)=1), Ramp (subtract background+NewXmipp)"""
NormType="Ramp"

# {condition}(DoNorm) Background radius
"""Pixels outside this circle are assumed to be noise and their stddev is set to 1.
   Radius for background circle definition (in pix.).
   If this value is 0, then the same as the particle radius is used. """
BackGroundRadius=0

#------------------------------------------------------------------------------------------
# {section} Parallelization
#------------------------------------------------------------------------------------------
# Number of MPI processes
""" Set to 1 if you do not have MPI installed"""
NumberOfMpi = 8

# Submit to queue
"""Submit to queue
"""
SubmitToQueue = False

# {condition}(SubmitToQueue) Queue name
"""Name of the queue to submit the job
"""
QueueName = "default"

# {condition}(SubmitToQueue) Queue hours
"""This establish a maximum number of hours the job will
be running, after that time it will be killed by the
queue system
"""
QueueHours = 6

# {hidden} Show expert options
"""If True, expert options will be displayed
"""
ShowExpertOptions = False
#
#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------
# {end_of_header} USUALLY YOU DO NOT NEED TO MODIFY ANYTHING BELOW THIS LINE ...
#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------
from protocol_preprocess_particles import *
#        
# Main
#     
 
if __name__ == '__main__':
       # create preprocess_particles_class object
    protocolMain(ProtPreprocessParticles)
