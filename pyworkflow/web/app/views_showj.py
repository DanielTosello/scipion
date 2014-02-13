# **************************************************************************
# *
# * Authors:    Adrian Quintana (aquintana@cnb.csic.es)
# *             Jose Gutierrez (jose.gutierrez@cnb.csic.es)
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
# *  e-mail address 'jmdelarosa@cnb.csic.es'
# *
# **************************************************************************


import os
import xmipp
from django.http import HttpResponse
from pyworkflow.web.pages import settings
from django.shortcuts import render_to_response
from pyworkflow.tests import getInputPath
from django.template import RequestContext
from pyworkflow.web.app.views_util import *
from forms import VolVisualizationForm, ShowjForm
from pyworkflow.em import *
from pyworkflow.em.packages.xmipp3.convert import *
from layout_configuration import *

from views_base import * 


def showj_init_dataset(request, inputParameters, extraParameters):
    #Init Dataset
    dataset = loadDatasetXmipp(inputParameters['path']) 
    
    #Check type of Dataset 
    if inputParameters['blockComboBox'] == '':
        #If empty, get the first one
        inputParameters['blockComboBox'] = dataset.listTables()[0]
        
    request.session['dataset'] = dataset

def showj_table_dataset(request, inputParameters, extraParameters):
    #Getting Dataset from session
    dataset = request.session['dataset']
    
    #Get table from block name
    dataset.setTableName(inputParameters['blockComboBox'])
    
    #Table used in the Dataset
    tableDataset = dataset.getTable()
    
    #Put extra parameter into session (just first time)
    request.session['defaultColumnsLayoutProperties'] = getExtraParameters(extraParameters, tableDataset)
    
    #Load table layout configuration. How to display columns and attributes (visible, render, editable)  
    inputParameters['tableLayoutConfiguration'] = TableLayoutConfiguration(dataset, tableDataset, inputParameters['allowRender'], request.session['defaultColumnsLayoutProperties'])
    
    request.session['dataset'] = dataset
    request.session['tableDataset'] = tableDataset
    request.session['blockComboBox'] = inputParameters['blockComboBox']

#    return dataset, tableDataset, inputParameters
    return inputParameters

def showj_block(request, inputParameters, extraParameters):
    #Getting variables from session
    dataset = request.session['dataset']
    tableDataset = request.session['tableDataset']
    
#    If no label is set to render, set the first one if exists
    if 'labelsToRenderComboBox' not in inputParameters or inputParameters['labelsToRenderComboBox'] == '' or request.session['blockComboBox'] != inputParameters['blockComboBox']:
        labelsToRenderComboBoxValues = inputParameters['tableLayoutConfiguration'].getLabelsToRenderComboBoxValues()
        if len(labelsToRenderComboBoxValues) > 0:   
            inputParameters['labelsToRenderComboBox'] = labelsToRenderComboBoxValues[0][0]
        else:
            # If there is no image to display and it is initial load, switch to table mode 
            if request.method == 'GET' and inputParameters['mode']!='table':
#            if inputParameters['mode']!='table':
                inputParameters['mode']='table'
                showj(request, inputParameters, extraParameters) 
            inputParameters['labelsToRenderComboBox'] = ''

    dataset.setLabelToRender(inputParameters['labelsToRenderComboBox']) 
    
    if inputParameters['labelsToRenderComboBox'] == '':
            inputParameters['zoom']=0
            _imageDimensions = None
            dataset.setNumberSlices(0)
    else:
        _imageVolName = inputParameters['volumesToRenderComboBox'] if ("volumesToRenderComboBox" in inputParameters and inputParameters['volumesToRenderComboBox'] != '') else tableDataset.getElementById(0,inputParameters['labelsToRenderComboBox'])
        
        _typeOfColumnToRender = inputParameters['tableLayoutConfiguration'].columnsLayout[inputParameters['labelsToRenderComboBox']].typeOfColumn
        
        _imageDimensions = readDimensions(request, _imageVolName, _typeOfColumnToRender)
        
        dataset.setNumberSlices(_imageDimensions[2])
        
        if _typeOfColumnToRender == "image":
            _convert = dataset.getNumberSlices()>1 and (inputParameters['mode']=='gallery' or inputParameters['mode']=='volume_astex' or inputParameters['mode']=='volume_chimera')
            _reslice = dataset.getNumberSlices()>1 and inputParameters['mode']=='gallery'
            _getStats = dataset.getNumberSlices()>1 and (inputParameters['mode']=='volume_astex' or inputParameters['mode']=='volume_chimera')
            _dataType = xmipp.DT_FLOAT if dataset.getNumberSlices()>1 and inputParameters['mode']=='volume_astex' else xmipp.DT_UCHAR
                
            _imageVolName, _stats = readImageVolume(request, _imageVolName, _convert, _dataType, _reslice, int(inputParameters['resliceComboBox']), _getStats)
            
            if dataset.getNumberSlices()>1:
                inputParameters['tableLayoutConfiguration'].columnsLayout[inputParameters['labelsToRenderComboBox']].columnLayoutProperties.renderFunc = "get_slice"
                dataset.setVolumeName(_imageVolName)
                
        volPath = os.path.join(request.session['projectPath'], _imageVolName)


    #Store multiple variables in session
    request.session['dataset'] = dataset
    request.session['labelsToRenderComboBox'] = inputParameters['labelsToRenderComboBox']
    request.session['imageDimensions'] = _imageDimensions
    #Additional variables
    request.session['volPath'] = volPath
    
    return _stats


def create_showj_context(request, inputParameters, params_stats):
    
    dataset = request.session['dataset']
    tableDataset = request.session['tableDataset']
    volPath = request.session['volPath']
    
    showjForm = ShowjForm(dataset,
                          inputParameters['tableLayoutConfiguration'],
                          inputParameters) # A form bound for the POST data and unbound for the GET
        
    if showjForm.is_valid() is False:
        print showjForm.errors

    context = createContext(dataset, tableDataset, inputParameters['tableLayoutConfiguration'], request, showjForm)

    if inputParameters['mode']=='volume_astex' or inputParameters['mode']=='volume_chimera':
        context.update(create_context_volume(request, inputParameters, volPath, params_stats))
               
    elif inputParameters['mode']=='gallery' or inputParameters['mode']=='table' or inputParameters['mode']=='column':
        context.update({"showj_alt_js": getResourceJs('showj_' + inputParameters['mode'] + '_utils')})
        
    return context


def showj(request, inputParameters=None, extraParameters=None):
    
    print "LOADING SHOWJ WEB...."
    
    #############
    # WEB INPUT PARAMETERS
    _imageVolName = ''
    _stats = None
    _imageDimensions = ''
    
    if inputParameters is None:
        inputParameters = request.POST.copy()
        
    if 'dataset' not in request.session:
        #Init Dataset first time
        showj_init_dataset(request, inputParameters, extraParameters)

    if inputParameters["typeVolume"] == 'pdb':
        inputParameters['tableLayoutConfiguration'] = None
        volPath = inputParameters['path']
    else:
        #Init the table dataset
        inputParameters = showj_table_dataset(request, inputParameters, extraParameters)
        #Init Block and Label
        _stats = showj_block(request, inputParameters, extraParameters)
    
#    print "inputParameters: ", inputParameters

    return_page = 'showj/showj_base.html'
    context = create_showj_context(request, inputParameters, _stats)
    
    print "FINISHED RENDER SHOWJ WEB...."
    print "CONTEXT!!!!! ", context
    
    return render_to_response(return_page, RequestContext(request, context))
    

def createContext(dataset, tableDataset, tableLayoutConfiguration, request, showjForm):
    #Create context to be send
    
    context = {'showj_css': getResourceCss('showj'),
            'smoothness': getResourceCss('ui_smoothness'),
            'demo_table_jui': getResourceCss('showj_demo_table_jui'),
           
            'logo': getResourceIcon('logo_scipion'),
            'logo_transparent': getResourceIcon('logo_scipion_transparent'),
           
            'jquery_datatable': getResourceJs('jquery_datatables'),
            'jquerydataTables_colreorder': getResourceJs('jquery_colreorder'),
            'jquerydataTables_colreorder_resize': getResourceJs('jquery_colreorder_resize'),
            'jeditable': getResourceJs('jquery_editable'),
            'jquery_waypoints':getResourceJs('jquery_waypoints'),
            'jquery_hover_intent':getResourceJs('jquery_hover_intent'),
            'showj_js':getResourceJs('showj_utils'),
            'jquery_ui':getResourceJs('jquery_ui'),
            
            'transpose_lib':getResourceJs('transpose'),
           
            'imageDimensions': request.session['imageDimensions'] if 'imageDimensions' in request.session else 0,
            'defaultZoom': request.session['defaultZoom'] if 'defaultZoom' in request.session else 0,
            'projectName': request.session['projectName'],
            
            'form': showjForm
            }
    
    if dataset is not None:
        context.update({'dataset': dataset})
    if tableLayoutConfiguration is not None:
        
        context.update({'tableLayoutConfiguration': json.dumps({'columnsLayout': tableLayoutConfiguration.columnsLayout,
                                                               'colsOrder': tableLayoutConfiguration.colsOrder},
                                                               ensure_ascii=False,
                                                               cls=ColumnLayoutConfigurationEncoder)})
    if tableDataset is not None:
        context.update({'tableDataset': tableDataset})

    context = base_grid(request, context)
    
    return context

def getExtraParameters(extraParameters, tableDataset):
#    print "extraParameters: ",extraParameters 
    defaultColumnsLayoutProperties = None
    
    if extraParameters != None and extraParameters != {}:
        defaultColumnsLayoutProperties = {k.getName(): {} for k in tableDataset.iterColumns()}
        for key, value in extraParameters.iteritems():

            try:
                label, attribute = key.rsplit('___')
            except Exception, ex:
                raise Exception('Showj Web visualizer: Incorrect extra parameter')

            if tableDataset.hasColumn(label):
                defaultColumnsLayoutProperties[label].update({attribute:value})
                
    return defaultColumnsLayoutProperties

###################################################################
########################### BEGIN SAVE & LOAD #####################    
###################################################################
#### Load an Xmipp Dataset ###
def loadDatasetXmipp(path):
    """ Create a table from a metadata. """
    from pyworkflow.em.packages.xmipp3 import XmippDataSet
    mdPath = getInputPath('showj', path)
    return XmippDataSet(str(mdPath))

    
def save_showj_table(request):
    if request.is_ajax():
        changes = request.POST.get('changes')
        jsonChanges = json.loads(changes)
        
        print "jsonChanges",jsonChanges 
        
        dataset=request.session['dataset']
        blockComboBox=request.session['blockComboBox']
#        tableLayoutConfiguration=request.session['tableLayoutConfiguration']
        
        tableDataset=dataset.getTable(blockComboBox)
        
        for key in jsonChanges:
            element_split = key.rsplit('___')
            if len(element_split)!=2: 
                print "this fails and sth has to be done"
            #NAPA de LUXE ahora mismo se realiza una conversion a int pero habria que ver de que tipo de datos se trata 
            #tableLayoutConfiguration.columnsLayout[element_split[0]].typeOfColumn

            dictelement = {element_split[0]:jsonChanges[key]}
            tableDataset.updateRow(int(element_split[1]),**dictelement)
        
        dataset.writeTable(blockComboBox, tableDataset)
        
        return HttpResponse(json.dumps({'message':'Ok'}), mimetype='application/javascript')
###################################################################
########################### END SAVE & LOAD #######################    
###################################################################    



###################################################################
######################## DISPATCHER ###############################    
###################################################################  
def visualizeObject(request):
    #Initialize default values
    inputParameters = {'objectId': '',
                       'path': '',
                       'allowRender': True,                 # Image can be displayed, depending on column layout 
                       'mode': 'gallery',                   # Mode Options: gallery, table, column, volume_astex, volume_chimera
                       'zoom': '150px',                     # Zoom set by default
                       'blockComboBox': '',                 # Metadata Block to display. If None the first one will be displayed
                       'labelsToRenderComboBox': '',        # Column to be displayed in gallery mode. If None the first one will be displayed
                       'volumesToRenderComboBox': '',       # If 3D, Volume to be displayed in gallery, volume_astex and volume_chimera mode. If None the first one will be displayed
#                       'dims': '2d',                        # Object Dimensions
                       'goto': 1,                           # Element selected (metadata record) by default. It can be a row in table mode or an image in gallery mode
                       'colRowMode': 'Off',                 # In gallery mode 'On' means columns can be adjust manually by the user. When 'Off' columns are adjusted automatically to screen width.
                       'cols':  '',                         # In gallery mode (and colRowMode set to 'On') cols define number of columns to be displayed
                       'rows': '',                          # In gallery mode (and colRowMode set to 'On') rows define number of columns to be displayed
                       'mirrorY': False,                    # When 'True' image are mirrored in Y Axis 
                       'applyTransformMatrix': False,       # When 'True' if there is transform matrix, it will be applied
                       'onlyShifts': False,                 # When 'True' if there is transform matrix, only shifts will be applied
                       'wrap': False,                       # When 'True' if there is transform matrix, only shifts will be applied
                       'resliceComboBox': xmipp.VIEW_Z_NEG, # If 3D, axis to slice volume 
                       'imageMaxWidth': 512,                # Maximum image width (in pixels)
                       'imageMinWidth': 30,                 # Minimum image width (in pixels)
                       'imageMaxHeight': 512,               # Maximum image height (in pixels)
                       'imageMinHeight': 30,                # Minimum image height (in pixels)
                       'typeVolume': 'map'}                 # If map, it will be displayed normally, else if pdb only astexViewer and chimera display will be available


    # Extra parameters can be used to configure table layout and set render function for a column
    # Default layout configuration is set in ColumnLayoutProperties method in layout_configuration.py
    # 
    # Parameters are formed by: [label]___[property]: [value]. E.g.: id___visible:True or micrograph___renderFunc:"get_image_psd"
    # Properties to be configured are:
    #    visible: Defines if this column is displayed
    #    allowSetVisible: Defines if user can change visible property (show/hide this column).
    #    editable: Defines if this column is editable, ie user can change field value.
    #    allowSetEditable: Defines if user can change editable property (allow editing this column).
    #    renderable: Defines if this column is renderizable, ie it renders data column using renderFunc
    #    allowSetRenderable: Defines if user can change renderable property.
    #    renderFunc: Function to be used when this field is rendered. (it has to be inserted in render_column method)
    #    extraRenderFunc: Any extra parameters needed for rendering. Parameters are passed like in a url ie downsample=2&lowPass=3.5
    #
    # Example:
    # extraParameters["id___visible"]=True
    # extraParameters["micrograph___renderFunc"]="get_image_psd"
    # extraParameters["micrograph___extraRenderFunc"]="downsample=2"
            
    extraParameters = {}
    
# NAPA DE LUXE: PARA probarlo sin sesion iniciada    
#    request.session['projectPath'] = "/home/adrian/Scipion/projects/TestXmippWorkflow/"
#    request.session['projectName'] = "Juanitpo"
    
    if "projectPath" in request.session:
        projectPath = request.session['projectPath']
    else:         
        raise Exception('Showj Web visualizer: No project loaded')
    
    for key, value in request.GET.iteritems():
        if key in inputParameters:
            inputParameters[key]=value
        else:
            extraParameters[key]=value
    
    
    if "objectId" in request.GET: 
        project = Project(projectPath)
        project.load()
        
        obj = project.mapper.selectById(int(inputParameters["objectId"]))
        if obj.isPointer():
            obj = obj.get()

        if isinstance(obj, SetOfMicrographs):
            fn = project.getTmpPath(obj.getName() + '_micrographs.xmd')
            inputParameters['path'] = os.path.join(projectPath, createXmippInputMicrographs(None, obj, micsFn=os.path.join(projectPath, fn)))
#            writeSetOfMicrographs(obj, fn)
#            inputParameters['path']= os.path.join(projectPath, fn)
        elif isinstance(obj, SetOfVolumes):
            fn = project.getTmpPath(obj.getName()+ '_volumes.xmd')
            inputParameters['path'] = os.path.join(projectPath, createXmippInputVolumes(None, obj, volsFn=os.path.join(projectPath, fn))) 
            inputParameters['mode']= 'table'
            
#            writeSetOfVolumes(obj, fn)
#            inputParameters['path']= os.path.join(projectPath, fn)
        elif isinstance(obj, PdbFile):
            inputParameters['path'] = obj.getFileName()
            inputParameters['mode'] = 'volume_astex'
            inputParameters['typeVolume'] = 'pdb'

        elif isinstance(obj, SetOfImages):
            fn = project.getTmpPath(obj.getName() + '_images.xmd')
            inputParameters['path'] = os.path.join(projectPath, createXmippInputImages(None, obj, imagesFn=os.path.join(projectPath, fn)))
#            writeSetOfParticles(obj, fn)
#            inputParameters['path']= os.path.join(projectPath, fn)
        elif isinstance(obj, Image):
            fn = project.getTmpPath(obj.getName() + '_image.xmd')
#            
#            # TODO: Improve this part saving the file
##            from time import time
##            t = time()
##            fnSet = fn.replace('.xmd', str(t) + '.sqlite')
#            fnSet = fn.replace('.xmd', '.sqlite')
#            cleanPath(fn, fnSet)
#                
#            imgSet = SetOfImages(filename=fnSet)
##            print "imgSet:",imgSet
#
#            img = Image()
#            #img.copyInfo(obj)
#            img.copyLocation(obj)
#            imgSet.append(img)
#            inputParameters['path'] = os.path.join(projectPath, createXmippInputImages(None, imgSet, imagesFn=os.path.join(projectPath, fn)))
            md = xmipp.MetaData()
            md.setValue(xmipp.MDL_IMAGE, getImageLocation(obj), md.addObject())
            md.write(fn)
            inputParameters['path'] = os.path.join(projectPath, fn)
#            writeSetOfParticles(imgSet, fn)
#            inputParameters['path']= os.path.join(projectPath, fn)
            
        elif isinstance(obj, SetOfClasses2D):
            fn = project.getTmpPath(obj.getName() + '_classes.xmd')
#            writeSetOfClasses2D(obj, fn)
#            inputParameters['path']= os.path.join(projectPath, fn)
            inputParameters['path'] = os.path.join(projectPath, createXmippInputClasses2D(None, obj, classFn=os.path.join(projectPath, fn)))
        elif isinstance(obj, SetOfCTF):
            fn = project.getTmpPath(obj.getName() + '_ctfs.xmd')
            inputParameters['path'] = os.path.join(projectPath, createXmippInputCTF(None, obj, ctfFn=os.path.join(projectPath, fn)))
#            writeSetOfCTFs(obj, fn)
#            inputParameters['path']= os.path.join(projectPath, fn)
        elif isinstance(obj, NormalModes):
            inputParameters['path'] = os.path.join(projectPath, obj.getFileName())
        else:
            raise Exception('Showj Web visualizer: can not visualize class: %s' % obj.getClassName())
    
    elif "path" in request.GET:
        inputParameters.update({'path':os.path.join(projectPath, request.GET.get("path"))})
    else:
        raise Exception('Showj Web visualizer: No object identifier or path found')         

    request.session['defaultZoom'] = inputParameters['zoom']
    return showj(request, inputParameters, extraParameters)  


def testingSSH(request):
    context = {}
#    return render_to_response("testing_ssh.html", RequestContext(request, context))
    return render_to_response("scipion.html", RequestContext(request, context))


def create_context_volume(request, inputParameters, volPath, param_stats):
#        volPath = os.path.join(request.session['projectPath'], _imageVolName)

    threshold = calculateThreshold(param_stats)
    
    context = {"threshold":threshold,
                    'minStats':param_stats[2] if param_stats != None else 1,
                    'maxStats':param_stats[3] if param_stats != None else 1 }
    
    if inputParameters['mode'] == 'volume_astex':
        context.update(create_context_astex(request, inputParameters['typeVolume'], volPath))
        
    elif inputParameters['mode'] == 'volume_chimera':   
        context.update(create_context_chimera(volPath))
        
#               'volType': 2, #0->byte, 1 ->Integer, 2-> Float

    return context
        

def create_context_astex(request, typeVolume, volPath):
#   volPath='/home/adrian/Scipion/tests/input/showj/emd_1042.map'        
#   fileName, fileExtension = os.path.splitext(_imageVolName)
            
#    linkName = 'test_link_' + request.session._session_key + '.' + inputParameters["typeVolume"]

    linkName = 'test_link_' + request.session._session_key + '.' + typeVolume
    volLinkPath = os.path.join(pw.WEB_RESOURCES, 'astex', 'tmp', linkName)
    
    from pyworkflow.utils.path import cleanPath, createLink
    cleanPath(volLinkPath)
    createLink(volPath, volLinkPath)
    volLink = os.path.join('/', settings.STATIC_ROOT, 'astex', 'tmp', linkName)
    
    return {"volLink":volLink}
    
    
def create_context_chimera(volPath):
    from subprocess import Popen, PIPE, STDOUT
    
    p = Popen([os.environ.get('CHIMERA_HEADLESS'), volPath], stdout=PIPE, stdin=PIPE, stderr=PIPE)
    outputHtmlFile = os.path.join(pw.WEB_RESOURCES, 'astex', 'tmp', 'test.html')
    #chimeraCommand= 'volume #0 level ' + str(threshold) + '; export format WebGL ' + outputHtmlFile + '; stop'
    chimeraCommand= 'export format WebGL ' + outputHtmlFile + '; stop'
    stdout_data, stderr_data = p.communicate(input=chimeraCommand)
#            print "stdout_data",stdout_data
#            print "stderr_data",stderr_data
    f = open(outputHtmlFile)
    chimeraHtml = f.read().decode('string-escape').decode("utf-8").split("</html>")[1]
    
    return {"chimeraHtml":chimeraHtml}
    
    
def calculateThreshold(params):
    threshold = (params[2] + params[3])/2 if params != None else 1
        
#    print "Threshold:", threshold
#    print "stats:",params
#    print "minStats:", params[2]
#    print "maxStats:", params[3]
    
    return threshold


def showj_gallery(request):
    context = {}
    context = RequestContext(request, context)
    return render_to_response('showj_gallery.html', context)

def showj_table(request):
    context = {}
    context = RequestContext(request, context)
    return render_to_response('showj_table.html', context)

def showj_column(request):
    context = {}
    context = RequestContext(request, context)
    return render_to_response('showj_column.html', context)

def showj_volume_astex(request):
    context = {}
    context = RequestContext(request, context)
    return render_to_response('showj_volume_astex.html', context)

def showj_volume_chimera(request):
    context = {}
    context = RequestContext(request, context)
    return render_to_response('showj_volume_chimera.html', context)



