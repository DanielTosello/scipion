#!/usr/bin/env python
'''
#/***************************************************************************
# * Authors:     J.M. de la Rosa Trevin (jmdelarosa@cnb.csic.es)
# *
# *
# * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
# *
# * This program is free software; you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation; either version 2 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with this program; if not, write to the Free Software
# * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# * 02111-1307  USA
# *
# *  All comments concerning this program package may be sent to the
# *  e-mail address 'xmipp@cnb.csic.es'
# ***************************************************************************
 '''
 
import os
import shutil
import ConfigParser
from config_protocols import projectDefaults, sections, protDict
from protlib_sql import SqliteDb, XmippProjectDb, XmippProtocolDb
from protlib_utils import XmippLog, loadModule, reportError
from protlib_filesystem import deleteDir, deleteFiles


class XmippProject():
    def __init__(self, projectDir=None):
        if not projectDir:
            self.projectDir = os.getcwd()
        else:
            self.projectDir = projectDir
        self.cfgName = projectDefaults['Cfg']
        self.dbName =  projectDefaults['Db']
        self.logsDir = projectDefaults['LogsDir']
        self.runsDir = projectDefaults['RunsDir']
        
    
    def exists(self):
        ''' a project exists if the data base can be opened and directories Logs and Runs
        exists'''
        from protlib_sql import existsDB
        status = True
        if not os.path.exists(self.logsDir):
            status = False
        elif not os.path.exists(self.runsDir):
            status = False
        elif not os.path.exists(self.cfgName):
            status = False
        elif not existsDB(self.dbName):
            status = False
        return status
    
    def create(self):
        os.chdir(self.projectDir)
        print "Creating new project on directory: '%s'" % self.projectDir
        #==== CREATE CONFIG file
        self.config = ConfigParser.RawConfigParser()            
        self.config.add_section('project')
        self.config.set('project', 'projectdir', self.projectDir)
        self.config.set('project', 'systemflavour', '')
        self.SystemFlavour = self.config.get('project', 'systemflavour')

        self.writeConfig()
        #===== CREATE LOG AND RUN directories
        if not os.path.exists(self.logsDir):
            os.mkdir(self.logsDir)
        if not os.path.exists(self.runsDir):
            os.mkdir(self.runsDir)
        #===== CREATE DATABASE
        self.projectDb  = XmippProjectDb(self.dbName)
        #===== POPULATE SOME TABLES
        for section, groupList in sections:
            for group in groupList:
                groupName = group[0]
                prots = group[1:]
                self.projectDb.insertGroup(groupName)
                for p in prots:
                    self.projectDb.insertProtocol(groupName, p)
        # commit changes
        self.projectDb.connection.commit()
        
    def load(self):
        self.config = ConfigParser.RawConfigParser()
        self.config.read(self.cfgName)
        # Load database
        self.projectDb = XmippProjectDb(self.dbName)
        self.SystemFlavour = self.config.get('project', 'systemflavour')
        

    def writeConfig(self):
        with open(self.cfgName, 'wb') as configfile:
            self.config.write(configfile) 
            
    def clean(self):
        ''' delete files related with a project'''
        if os.path.exists(self.logsDir):
            shutil.rmtree(self.logsDir)
        if os.path.exists(self.runsDir):
            shutil.rmtree(self.runsDir)
        if os.path.exists(self.cfgName):
            os.remove(self.cfgName)
        if os.path.exists(self.dbName):
            os.remove(self.dbName)
        self.create()
            
    
    def cleanRun(self, run):
        script = run['script']
        toDelete = [script.replace(self.runsDir, self.logsDir).replace(".py", ext) for ext in ['.log', '.err', '.out']]
        deleteFiles(None, toDelete, False)
        workingDir = self.getWorkingDir(run['protocol_name'], run['run_name'])
        from distutils.dir_util import remove_tree
        if os.path.exists(workingDir):
            remove_tree(workingDir, True)

    def deleteRun(self, run):
        script = run['script']
        deleteFiles(None, [script, script+'c'], False)
        self.cleanRun(run)
        self.projectDb.deleteRun(run)
        
    def deleteTmpFiles(self):
        for section, groupList in sections:
            for group in groupList:
                groupName = group[0]
                runs = self.projectDb.selectRuns(groupName)
                for run in runs:
                    tmpDir = os.path.join(self.getWorkingDir(run['protocol_name'], run['run_name']), 'tmp')
                    if os.path.exists(tmpDir):
                        shutil.rmtree(tmpDir)

    def getWorkingDir(self,protocol_name,run_name):
        return os.path.join(protDict[protocol_name].dir,run_name)
            
    def getRunScriptFileName(self, protocol_name, runName):
        return os.path.join(self.runsDir, '%s_%s.py' % (protocol_name, runName))
    
    def createRunFromScript(self, protocol_name, script):
        #suggest a new run_name        
        runName = self.projectDb.suggestRunName(protocol_name)
        dstAbsPath = self.getRunScriptFileName(protocol_name, runName)
        run = {
               'protocol_name':protocol_name, 
               'run_name': runName, 
               'script': dstAbsPath, 
               'comment': "",
               'source': script
               }
        return run
    
    def newProtocol(self, protocol_name):
        from protlib_filesystem import getXmippPath
        srcProtAbsPath = os.path.join(getXmippPath('protocols'),
                                       'protocol_%s_header.py' % protocol_name)
        return self.createRunFromScript(protocol_name, srcProtAbsPath)
    
    def copyProtocol(self, protocol_name, script):
        return self.createRunFromScript(protocol_name, script)
        
    def loadProtocol(self, protocol_name, runName):
        run = self.projectDb.selectRunByName(self, protocol_name, runName)
        if not run is None:
            run['source'] = run['script']
        return run
    
    def newOrLoadProtocol(self, protocol_name, runName):
        run = self.loadProtocol(self.project, protocol_name, runName)
        if run is None:
            run = self.newProtocol(protocol_name)
        return run
            
class XmippProtocol(object):
    '''This class will serve as base for all Xmipp Protocols'''
    def __init__(self, protocolName, scriptname, project):
        '''Basic constructor of the Protocol
        protocolName -- the name of the protocol, should be unique
        scriptname,     file containing the protocol instance
        runName      -- the name of the run,  should be unique for one protocol
        project      -- project instance
        '''
        #Import all variables in the protocol header
        self.Header = loadModule(scriptname)
        for k, v in self.Header.__dict__.iteritems():
            self.__dict__[k] = v
            
        self.DoParallel = 'NumberOfMpi' in dir() and self.NumberOfMpi > 1
        self.Name = protocolName
        self.scriptName = scriptname
        #A protocol must be able to find its own project
        self.project = project
        self.Import = '' # this can be used by database for import modules
        self.WorkingDir = project.getWorkingDir(protocolName, self.RunName)
        self.TmpDir = os.path.join(self.WorkingDir, 'tmp')
        self.projectDir = project.projectDir  
        #Setup the Log for the Protocol
        self.LogDir = project.logsDir
        self.uniquePrefix = self.WorkingDir.replace('/', '_')
        self.LogPrefix = os.path.join(self.LogDir, self.uniquePrefix)       
        self.Err = self.LogPrefix+".err"
        self.Out = self.LogPrefix+".out"
        self.SystemFlavour = project.SystemFlavour
        
    def getProjectId(self):
        pass
        #self.project = project.getId(launchDict['Projection Matching'],runName,)
    def validateBase(self):
        '''Validate if the protocols is ready to be run
        in this function will be implemented all common
        validations to all protocols and the function
        validate will be called for specific validation
        of each protocol'''
        errors=[]
        #check if there is a valid project, otherwise abort
        if not self.project.exists():
            errors.append("Not valid project available")
        # Check if there is runname
        if self.RunName == "":
            errors.append("No run name given")
            
        #Check that number of threads and mpi are int and greater than 0
        if 'NumberOfThreads' in dir(self):
            if self.NumberOfThreads<1:
                errors.append("Number of threads has to be >=1")
        if 'NumberOfMpi' in dir(self):
            if self.NumberOfMpi<2:
                errors.append("Number of MPI processes has to be >=2")
        
        #specific protocols validations
        errors += self.validate()
        
        return errors
    
    def validate(self):
        '''Validate if the protocols is ready to be run
        it may be redefined in derived protocol classes but do not forget
        to call the main class with
        super(ProtProjMatch, self).validate()
        '''
        return []
    
    def summary(self):
        '''Produces a summary with the most relevant information of the protocol run'''
        return []
    
    def visualize(self):
        '''Visualizes the results of this run'''
        pass
    
    def warningsBase(self):
        '''Output some warnings that can be errors and require user confirmation to proceed'''
        warningList=[]
        if self.Behavior=="Restart":
            warningList.append("Restarting a protocol will delete previous results")
        warningList += self.warnings()
        return warningList

    def warnings(self):
        '''Output some warnings that can be errors and require user confirmation to proceed'''
        return []
    
    def postRun(self):
        '''This function will be called after the run function is executed'''
        pass   
    
    def defineSteps(self):
        '''In this function the actions to be performed by the protocol will be added to db.
        each particular protocol need to add its specific actions. Thing to be performed before 
        "run" should be added here'''
        pass
    
    def runSetup(self, isMainLoop=True):
        import sys
        #Redirecting standard output and error to files
        self.fOut = open(self.Out, 'a')
        self.fErr = open(self.Err, 'a')
        sys.stdout = self.fOut
        sys.stderr = self.fErr
        self.Log = XmippLog(self.LogPrefix + ".log")
        self.Db  = XmippProtocolDb(self, isMainLoop)

    def run(self):
        '''Run of the protocols
        if the other functions have been correctly implemented, this not need to be
        touched in derived class, since the run of protocols should be the same'''
        
        #Change to project dir
        os.chdir(self.projectDir)

        errors = self.validateBase()
        if len(errors) > 0:
            raise Exception('\n'.join(errors))
        
        #insert basic operations for all scripts
        if self.Behavior=="Restart":
            run = {
               'protocol_name':self.Name, 
               'run_name': self.RunName, 
               'script': self.scriptName, 
               'source': self.scriptName
               }
            self.project.cleanRun(run)
        #Remove temporaly files
        if os.path.exists(self.TmpDir):
            shutil.rmtree(self.TmpDir)
        #Initialization of log and db
        retcode = 0
        try:
            self.runSetup()
            self.Db.insertStep('createDir', path = self.WorkingDir)
            self.Db.insertStep('createDir', path = self.TmpDir)
            self.defineSteps()
            self.Db.runSteps()
            self.postRun()
        except Exception, e:
            retcode = 1;
            print e
        finally:
            self.fOut.close()
            self.fErr.close()  
                            
        return retcode
    
def command_line_options():
    '''process protocol command line'''
    import optparse
    parser = optparse.OptionParser()
    parser.add_option('-g', '--gui',
                              dest="gui",
                              default=False,
                              action="store_true",
                              help="use graphic interface to launch protocol "
                              )
    parser.add_option('-c', '--no_check',
                              dest="no_check",
                              default=False,
                              action="store_true",
                              help="do NOT check run checks before execute protocols"
                              )
    parser.add_option('-n', '--no_confirm',
                          dest="no_confirm",
                          default=False,
                          action="store_true",
                          help="do NOT ask confirmation for warnings"
                          )
        
    options = parser.parse_args()[0]
    return options

def getProtocolFromModule(script, project):
    mod = loadModule(script)
    from inspect import isclass
    for v in mod.__dict__.values():
        if isclass(v) and issubclass(v, XmippProtocol) and v != XmippProtocol:
            return v(script, project)
    reportError("Can load protocol from " + script)
            
def protocolMain(ProtocolClass, script=None):
    gui = False
    no_check = False
    no_confirm = False
    
    if script is None:
        import sys
        script  = sys.argv[0]
        options = command_line_options()
        gui = options.gui
        no_check = options.no_check
        no_confirm = options.no_confirm
    
    mod = loadModule(script)
    #init project
    project = XmippProject()
    #load project: read config file and open conection database
    project.load()
    #register run with runName, script, comment=''):
    p = ProtocolClass(script, project)
    run_id = project.projectDb.getRunId(p.Name, mod.RunName)
    
    #1) just call the gui    
    if gui:
        run = {
           'run_id': run_id,
           'protocol_name':p.Name, 
           'run_name': mod.RunName, 
           'script': script, 
           'comment': "",
           'source': script
           }
        from protlib_gui import ProtocolGUI 
        gui = ProtocolGUI()
        gui.createGUI(project, run)
        gui.fillGUI()
        gui.launchGUI()
    else:#2) Run from command line
        
        if no_check: 
            if not run_id:
                reportError("Protocol run '%s' has not been registered in project database" % mod.RunName)
        else:
            from protlib_utils import showWarnings
            if not showWarnings(p.warningsBase(), no_confirm):
                exit(0)
            
            if not run_id:
                _run={
                       'comment' : ''
                      ,'protocol_name': p.Name
                      ,'run_name' : mod.RunName
                      ,'script'  : script 
                      }
                project.projectDb.insertRun(_run)
            if 'SubmitToQueue' in dir(mod) and mod.SubmitToQueue:
                from protlib_utils import submitProtocol
                NumberOfThreads = 1
                if 'NumberOfThreads' in dir(mod):
                    NumberOfThreads=mod.NumberOfThreads
                submitProtocol(script,
                               jobId = p.uniquePrefix,
                               queueName = mod.QueueName,
                               nodes = mod.NumberOfMpi,
                               threads = NumberOfThreads,
                               hours = mod.QueueHours,
                               command = 'python %s --no_check' % script
                               )
                project.projectDb.updateRunState(SqliteDb.RUN_LAUNCHED)
                exit(0)
        
        return p.run()
