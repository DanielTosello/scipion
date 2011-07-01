#--------------------------------------------------------------------------------
# This is a sample config file for launching jobs to a queue system
# 
# There should be two variables in this file:
#
# 1.- FileTemplate: string holding the template for launch script
#      Following vars are availables to template:
#      - %(jobId)s       : job name or id
#      - %(nodes)d       : number of mpi nodes
#      - %(threads)d     : number of threads
#      - %(hours)d       : limit of running hours
#      - %(memory)d      : limit of memory used
#      - %(command)s     : command to be executed
#
#
# 2.- CommandTemplate: string with the template for the launch command
#      Following vars are availables to template:
#      - %(file)s     : file to be launched 
#--------------------------------------------------------------------------------

FileTemplate = """
#!/bin/bash
### Inherit all current environment variables
#PBS -V
### Job name
#PBS -N %(jobId)s
### Queue name
#PBS -q %(queueName)s
### Standard output and standard error messages
#PBS -k eo
### Specify the number of nodes and thread (ppn) for your job.
#PBS -l nodes=%(nodes)d:ppn=%(threads)d
### Tell PBS the anticipated run-time for your job, where walltime=HH:MM:SS
#PBS -l walltime=%(hours)d:00:00
#################################
### Switch to the working directory;
cd $PBS_O_WORKDIR
echo Working directory is $PBS_O_WORKDIR
# Calculate the number of processors allocated to this run.
NPROCS=`wc -l < $PBS_NODEFILE`
# Calculate the number of nodes allocated.
NNODES=`uniq $PBS_NODEFILE | wc -l`
### Display the job context
echo Running on host `hostname`
echo Time is `date`
echo Directory is `pwd`
echo Using ${NPROCS} processors across ${NNODES} nodes
echo PBS_NODEFILE:
cat $PBS_NODEFILE
#################################

%(command)s
"""

LaunchTemplate = "qsub %(file)s"
