# from scipion.models import *
import pyworkflow as pw
import os
import xmipp
from django.shortcuts import render_to_response
from django.core.context_processors import csrf
from django.http import HttpResponse
from django.template import RequestContext
import json
from pyworkflow.manager import Manager
from pyworkflow.project import Project
from pyworkflow.gui.tree import TreeProvider
from pyworkflow.utils.path import findResource
from pyworkflow.utils.utils import prettyDate
from pyworkflow.web.pages import settings
from pyworkflow.apps.config import *
from pyworkflow.em import *
from django.http.request import HttpRequest
from pyworkflow.hosts import ExecutionHostMapper

from pyworkflow.tests import getInputPath 
from forms import HostForm

def getResource(request):
    if request == 'logoScipion':
        img = 'scipion_logo.png'
    elif request == 'favicon':
        img = 'scipion_bn.png'
    elif request == 'help':
        img = 'contents24.png'
    elif request == 'browse':
        img = 'zoom.png'
    elif request == 'edit_toolbar':
        img = 'edit.gif'
    elif request == 'copy_toolbar':
        img = 'copy.gif'
    elif request == 'delete_toolbar':
        img = 'delete.gif'
    elif request == 'browse_toolbar':
        img = 'run_steps.gif'
    elif request == 'new_toolbar':
        img = 'new_object.gif'
        
    path = os.path.join(settings.MEDIA_URL, img)
    return path

# Resources #
new_tool_path = getResource('new_toolbar')
edit_tool_path = getResource('edit_toolbar')
copy_tool_path = getResource('copy_toolbar')
delete_tool_path = getResource('delete_toolbar')
browse_tool_path = getResource('browse_toolbar')

def projects(request):
    manager = Manager()
#    logo_path = findResource('scipion_logo.png')
    
    # Resources #
    css_path = os.path.join(settings.STATIC_URL, 'css/projects_style.css')
    
    #############
    projectForm_path = os.path.join(settings.STATIC_URL, 'js/projectForm.js')
#    jquery_path = os.path.join(settings.STATIC_URL, 'js/jquery.js')
    
    # Messi Plugin #
#    messi_path = os.path.join(settings.STATIC_URL, 'js/messi.js')
    messi_css_path = os.path.join(settings.STATIC_URL, 'css/messi.css')
    #############
    
    projects = manager.listProjects()
    for p in projects:
        p.pTime = prettyDate(p.mTime)

    context = {
#              'jquery':jquery_path,
               'projects': projects,
               'css': css_path,
#               'messi': messi_path,
               'messi_css': messi_css_path,
               'projectForm':projectForm_path}
    
    return render_to_response('projects.html', context)

def create_project(request):
    from django.http import HttpResponse    
    
    manager = Manager()
    
    if request.is_ajax():
        projectName = request.GET.get('projectName')
        manager.createProject(projectName)       
        
    return HttpResponse(mimetype='application/javascript')

def delete_project(request):
    from django.http import HttpResponse    
    
    manager = Manager()
    
    if request.is_ajax():
        projectName = request.GET.get('projectName')
        manager.deleteProject(projectName)       
        
    return HttpResponse(mimetype='application/javascript')

class TreeItem():
    def __init__(self, name, tag, protClass=None):
        self.name = name
        self.tag = tag
        self.protClass = protClass
        self.childs = []
        
def populateTree(tree, obj):    
    for sub in obj:
        text = sub.text.get()
        value = sub.value.get(text)
        tag = sub.tag.get('')
        item = TreeItem(text, tag)
        tree.childs.append(item)
        # If have tag 'protocol_base', fill dynamically with protocol sub-classes
        if sub.value.hasValue() and tag == 'protocol_base':
            protClassName = value.split('.')[-1]  # Take last part
            prot = emProtocolsDict.get(protClassName, None)
            if prot is not None:
                for k, v in emProtocolsDict.iteritems():
                    if not v is prot and issubclass(v, prot):
                        protItem = TreeItem(k, 'protocol_class', protClassName)
                        item.childs.append(protItem)
        else:
            populateTree(item, sub)                
       
def loadConfig(config, name):
    c = getattr(config, name) 
    fn = getConfigPath(c.get())
    if not os.path.exists(fn):
        raise Exception('loadMenuConfig: menu file "%s" not found' % fn)
    mapper = ConfigMapper(getConfigPath(fn), globals())
    menuConfig = mapper.getConfig()
    return menuConfig

def loadProtTree():
    configMapper = ConfigMapper(getConfigPath('configuration.xml'), globals())
    generalCfg = configMapper.getConfig()
    protCfg = loadConfig(generalCfg, 'protocols')    
    root = TreeItem('root', 'root')
    populateTree(root, protCfg)
    return root

# to do a module from pw_project
class RunsTreeProvider(TreeProvider):
    """Provide runs info to populate tree"""
    def __init__(self, mapper, actionFunc=None):
        self.actionFunc = actionFunc
        self.getObjects = lambda: mapper.selectAll()
        
    def getColumns(self):
        return [('Run', 250), ('State', 100), ('Modified', 100)]
    
    def getObjectInfo(self, obj):
        return {'key': obj.getObjId(),
                'text': '%s.%s' % (obj.getClassName(), obj.strId()),
                'values': (obj.status.get(), obj.endTime.get())}
      
    def getObjectActions(self, obj):
        prot = obj  # Object should be a protocol
        actionsList = [(ACTION_EDIT, 'Edit     '),
                       # (ACTION_COPY, 'Duplicate   '),
                       (ACTION_DELETE, 'Delete    '),
                       # (None, None),
                       # (ACTION_STOP, 'Stop'),
                       (ACTION_STEPS, 'Browse data')
                       ]
        status = prot.status.get()
        if status == STATUS_RUNNING:
            actionsList.insert(0, (ACTION_STOP, 'Stop execution'))
            actionsList.insert(1, None)
        elif status == STATUS_WAITING_APPROVAL:
            actionsList.insert(0, (ACTION_CONTINUE, 'Approve continue'))
            actionsList.insert(1, None)
        
        actions = []
        def appendAction(a):
            v = a
            if v is not None:
                action = a[0]
                text = a[1]
                v = (text, lambda: self.actionFunc(action), ActionIcons[action])
            actions.append(v)
            
        for a in actionsList:
            appendAction(a)
            
        return actions 
    
def loadProject(projectName):
    manager = Manager()
    projPath = manager.getProjectPath(projectName)
    project = Project(projPath)
    project.load()
    return project
    
def project_content(request):        
    css_path = os.path.join(settings.STATIC_URL, 'css/project_content_style.css')
#    jquery_path = os.path.join(settings.STATIC_URL, 'js/jquery.js')
    jquery_cookie = os.path.join(settings.STATIC_URL, 'js/jquery.cookie.js')
    jquery_treeview = os.path.join(settings.STATIC_URL, 'js/jquery.treeview.js')
    launchTreeview = os.path.join(settings.STATIC_URL, 'js/launchTreeview.js')
    utils_path = os.path.join(settings.STATIC_URL, 'js/utils.js')
    #############
     # Messi Plugin
#    messi_path = os.path.join(settings.STATIC_URL, 'js/messi.js')
    messi_css_path = os.path.join(settings.STATIC_URL, 'css/messi.css')
    #############
    
    projectName = request.GET.get('projectName', None)
    if projectName is None:
        projectName = request.POST.get('projectName', None)
        
    request.session['projectName'] = projectName
        
    project = loadProject(projectName)    
    provider = RunsTreeProvider(project.mapper)
    
    root = loadProtTree()
    
    context = {'projectName':projectName,
               'editTool': edit_tool_path,
               'copyTool': copy_tool_path,
               'deleteTool': delete_tool_path,
               'browseTool': browse_tool_path,
#               'jquery': jquery_path,
#               'jquery_ui':jquery_ui_path,
               'utils': utils_path,
               'jquery_cookie': jquery_cookie,
               'jquery_treeview': jquery_treeview,
               'launchTreeview': launchTreeview,
               'css':css_path,
               'sections': root.childs,
               'provider':provider,
#               'messi': messi_path,
               'messi_css': messi_css_path,
               'view': 'protocols'}
    
    return render_to_response('project_content.html', context)

def delete_protocol(request):
    from django.http import HttpResponse   
    
    # Project Id(or Name) should be stored in SESSION
    if request.is_ajax():
        projectName = request.GET.get('projectName')
        project = loadProject(projectName)
        protId = request.GET.get('protocolId', None)
        protocol = project.mapper.selectById(int(protId))
     
        project.deleteProtocol(protocol)         
        
    return HttpResponse(mimetype='application/javascript')

def form(request):
    
    # Resources #
    favicon_path = getResource('favicon')
    logo_help = getResource('help')
    logo_browse = getResource('browse')
    jquery_path = os.path.join(settings.STATIC_URL, 'js/jquery.js')
    jsForm_path = os.path.join(settings.STATIC_URL, 'js/form.js')
    utils_path = os.path.join(settings.STATIC_URL, 'js/utils.js')
    css_path = os.path.join(settings.STATIC_URL, 'css/form.css')
    # Messi Plugin
    messi_path = os.path.join(settings.STATIC_URL, 'js/messi.js')
    messi_css_path = os.path.join(settings.STATIC_URL, 'css/messi.css')
    #############
    
    # # Project Id(or Name) should be stored in SESSION
    projectName = request.GET.get('projectName')
    project = loadProject(projectName)        
    protocolName = request.GET.get('protocol', None)
    action = request.GET.get('action', None)
    
    if protocolName is None:
        protId = request.GET.get('protocolId', None)
        protocol = project.mapper.selectById(int(protId))
    else:
        protocolClass = emProtocolsDict.get(protocolName, None)
        protocol = protocolClass()
    
    if action == 'copy':
        protocol = project.copyProtocol(protocol)
    
    # TODO: Add error page validation when protocol is None
    for section in protocol._definition.iterSections():
        for paramName, param in section.iterParams():
            protVar = getattr(protocol, paramName, None)
            if protVar is None:
                raise Exception("_fillSection: param '%s' not found in protocol" % paramName)
                # Create the label
            if protVar.isPointer():
                if protVar.hasValue():
                    param.htmlValue = protVar.get().getNameId()
            else:
                param.htmlValue = protVar.get(param.default.get(""))
                if isinstance(protVar, Boolean):
                    if param.htmlValue:
                        param.htmlValue = 'true'
                    else:
                        param.htmlValue = 'false' 
            param.htmlCond = param.condition.get()
            param.htmlDepend = ','.join(param._dependants)
            param.htmlCondParams = ','.join(param._conditionParams)
#            param.htmlExpertLevel = param.expertLevel.get()
    
    
    context = {'projectName':projectName,
               'protocol':protocol,
               'definition': protocol._definition,
               'favicon': favicon_path,
               'help': logo_help,
               'form': jsForm_path,
               'jquery': jquery_path,
               'browse': logo_browse,
               'utils': utils_path,
               'css':css_path,
               'messi': messi_path,
               'messi_css': messi_css_path}
    
    # Cross Site Request Forgery protection is need it
    context.update(csrf(request))
    
    return render_to_response('form.html', context)

def protocol(request):
    projectName = request.POST.get('projectName')
    protId = request.POST.get("protocolId")
    protClass = request.POST.get("protocolClass")
    
    # Load the project
    project = loadProject(projectName)
    # Create the protocol object
    if protId != 'None':  # Case of new protocol
        protId = request.POST.get('protocolId', None)
        protocol = project.mapper.selectById(int(protId))
    else:
        protocolClass = emProtocolsDict.get(protClass, None)
        protocol = protocolClass() 
    # Update parameter set in the form
    for paramName, attr in protocol.iterDefinitionAttributes():
        value = request.POST.get(paramName)
        if attr.isPointer():
            if len(value.strip()) > 0:
                objId = int(value.split('.')[-1])  # Get the id string for last part after .
                value = project.mapper.selectById(objId)  # Get the object from its id
                if attr.getObjId() == value.getObjId():
                    raise Exception("Param: %s is autoreferencing with id: %d" % (paramName, objId))
            else:
                value = None
        attr.set(value)
    
    errors = protocol.validate()

    if len(errors) == 0:
        # No errors 
        # Finally, launch the protocol
        project.launchProtocol(protocol)
    jsonStr = json.dumps({'errors' : errors},
                     ensure_ascii=False)
    return HttpResponse(jsonStr, mimetype='application/javascript')

def browse_objects(request):
    """ Browse objects from the database. """
    if request.is_ajax():
        objClass = request.GET.get('objClass')
        projectName = request.GET.get('projectName')
        project = loadProject(projectName)    
        
        objs = []
        for obj in project.mapper.selectByClass(objClass, iterate=True):
            objs.append(obj.getNameId())
        jsonStr = json.dumps({'objects' : objs},
                             ensure_ascii=False)
        return HttpResponse(jsonStr, mimetype='application/javascript')

def getScipionHosts():
    defaultHosts = os.path.join(pw.HOME, 'settings', 'execution_hosts.xml')
    return ExecutionHostMapper(defaultHosts).selectAll()

def openHostsConfig(request):
#     if request.method == 'POST':  # If the form has been submitted...
#         pass
#     else:
    # Resources #
    css_path = os.path.join(settings.STATIC_URL, 'css/general_style.css')
    jquery_path = os.path.join(settings.STATIC_URL, 'js/jquery.js')
    utils_path = os.path.join(settings.STATIC_URL, 'js/utils.js')
    # # Project Id(or Name) should be stored in SESSION
    projectName = request.session['projectName']
    project = loadProject(projectName)
    scipionHosts = getScipionHosts()
    projectHosts = project.getHosts()   
#         hostsKeys = [host.getLabel() for host in projectHosts]      
#         availableHosts = [host for host in scipionHosts if host.getLabel() not in hostsKeys]
    if request.method == 'POST':
        form = HostForm(request.POST)
    else:       
        form = HostForm(auto_id=True)
    scpnHostsChoices = []
    scpnHostsChoices.append(('', ''))
    for executionHostMapper in scipionHosts:
        scpnHostsChoices.append((executionHostMapper.getLabel(), executionHostMapper.getHostName()))
    form.fields['scpnHosts'].choices = scpnHostsChoices
    context = {'projectName' : request.session['projectName'],
               'editTool': edit_tool_path,
               'newTool': new_tool_path,
               'deleteTool': delete_tool_path,
               'browseTool': browse_tool_path,
               'hosts': projectHosts,
               'scipionHosts': scipionHosts,
               'jquery': jquery_path,
               'utils': utils_path,
               'css':css_path,
               'form': form,
               'view': 'hosts'}
        
    return render_to_response('hosts.html', RequestContext(request, context)) # Form Django forms

def getHost(request):
    from django.http import HttpResponse
    import json
    from django.utils import simplejson
    
    if request.is_ajax():
        hostLabel = request.GET.get('hostLabel')
        projectName = request.session['projectName']
        project = loadProject(projectName)
        hostsMapper = ExecutionHostMapper(project.hostsPath)
        executionHostConfig = hostsMapper.selectByLabel(hostLabel)
        jsonStr = json.dumps({'host':executionHostConfig.getDictionary()})
#         jsonStr = json.dumps({'hostConfig' :  executionHostConfig},
#                              ensure_ascii=False)
        return HttpResponse(jsonStr, mimetype='application/javascript')


def updateHostsConfig(request):
    form = HostForm(request.POST) # A form bound to the POST data
    if form.is_valid(): # All validation rules pass
        projectName = request.session['projectName']
        project = loadProject(projectName)
        project.saveHost(form.getHost())
        return openHostsConfig(request)
    else:
        return openHostsConfig(request)

def showj(request):
    # manager = Manager()
#    logo_path = findResource('scipion_logo.png')

    # Resources #
    css_path = os.path.join(settings.STATIC_URL, 'css/showj_style.css')
    favicon_path = getResource('favicon')
    jquery_path = os.path.join(settings.STATIC_URL, 'js/jquery.js')
    jquery_cookie = os.path.join(settings.STATIC_URL, 'js/jquery.cookie.js')
    jquery_treeview = os.path.join(settings.STATIC_URL, 'js/jquery.treeview.js')
    launchTreeview = os.path.join(settings.STATIC_URL, 'js/launchTreeview.js')
    utils_path = os.path.join(settings.STATIC_URL, 'js/utils.js')
    
    powertable_path = os.path.join(settings.STATIC_URL, 'js/jquery-powertable.js')
    powertable_resources_path = os.path.join(settings.STATIC_URL, 'js/jquery-litelighter.js')
    
    jquerydataTables_path = os.path.join(settings.STATIC_URL, 'js/jquery.dataTables.js')
    jquerydataTables_colreorder_path = os.path.join(settings.STATIC_URL, 'js/ColReorder.js')
        
    
    #############
    # WEB INPUT PARAMETERS
    inputParameters = {'path': request.GET.get('path', 'tux_vol.xmd'),
                     'block': request.GET.get('block', ''),
                     'allowRender': 'render' in request.GET,
                     'imageDim' : request.GET.get('dim', None),
                     'mode': request.GET.get('mode', 'gallery'),
                     'metadataComboBox': request.GET.get('metadataComboBox', 'image')}
    
    md = loadMetaData(inputParameters['path'], inputParameters['block'], inputParameters['allowRender'], inputParameters['imageDim'])    

    menuLayoutConfig = loadMenuLayoutConfig(inputParameters['mode'], inputParameters['path'], inputParameters['block'], inputParameters['allowRender'], inputParameters['imageDim']);
   
    
    context = {'jquery': jquery_path,
               'utils': utils_path,
               'jquery_cookie': jquery_cookie,
               'jquery_treeview': jquery_treeview,
               'launchTreeview': launchTreeview,
               'jquery_datatable': jquerydataTables_path,
               'jquerydataTables_colreorder': jquerydataTables_colreorder_path,
               'css': css_path,
               'metadata': md,
               'inputParameters': inputParameters,
               'menuLayoutConfig': menuLayoutConfig}
    
    return_page = '%s%s%s' % ('showj_', inputParameters['mode'], '.html')
    return render_to_response(return_page, context)

AT = '__at__'

class MdObj():
    pass
    
class MdValue():
    def __init__(self, md, label, objId, allowRender, imageDim=None):
        self.strValue = str(md.getValue(label, objId))        

        self.label = xmipp.label2Str(label)
        
        self.allowRender = allowRender

        # check if enabled label
        self.displayCheckbox = (label == xmipp.MDL_ENABLED)

        # Prepare path for image
        self.imgValue = self.strValue
        if allowRender and '@' in self.strValue:
            self.imgValue = self.imgValue.replace('@', AT)
#            if imageDim:
#                self.imgValue += '&dim=%s' % imageDim

class MdData():
    def __init__(self, path, allowRender=True, imageDim=None):        
        md = xmipp.MetaData(path)
        
        labels = md.getActiveLabels()
        self.labels = []
        self.labelsToRender = []
        for l in labels:
            labelName = xmipp.label2Str(l)
            self.labels.append(labelName)
            if (xmipp.labelIsImage(l) and allowRender):
                self.labelsToRender.append(labelName)
                
        self.colsOrder = defineColsLayout(self.labels);
        self.objects = []
        for objId in md:
            obj = MdObj()
            obj.id = objId
            obj.values = [MdValue(md, l, objId, (xmipp.labelIsImage(l) and allowRender), imageDim) for l in labels]
            self.objects.append(obj)

class MenuLayoutConfig():        
    def __init__(self, mode, path, block, allowRender, imageDim):
        link = "location.href='/showj/?path=" + path
        if len(block):
            link = link + "&block=" + block
        if allowRender:
            link = link + "&render"
        if imageDim is not None:
            link = link + "&dim=" + imageDim
                

        if (mode == "table"):
            self.tableViewLink = "#"
            self.tableViewSrc = "/resources/showj/tableViewOn.gif"
            self.galleryViewLink = link + "&mode=gallery'"
            self.galleryViewSrc = "/resources/showj/galleryViewOff.gif"
            
            self.disabledColRowMode = "disabled"
            
        elif (mode == "gallery"):
            self.tableViewLink = link + "&mode=table'"
            self.tableViewSrc = "/resources/showj/tableViewOff.gif"
            self.galleryViewLink = "#"
            self.galleryViewSrc = "/resources/showj/galleryViewOn.gif"
            
            self.disabledColRowMode = ""
            
            
            
         
        
def defineColsLayout(labels):
    colsOrder = range(len(labels))
    if 'enabled' in labels:
        colsOrder.insert(0, colsOrder.pop(labels.index('enabled')))
    return colsOrder    

def loadMetaData(path, block, allowRender=True, imageDim=None):
    path = getInputPath('showj', path)    
    if len(block):
        path = '%s@%s' % (block, path)
    # path2 = 'Volumes@' + path1
    return MdData(path, allowRender, imageDim)   

def loadMenuLayoutConfig(mode , path, block, allowRender=True, imageDim=None):
    return MenuLayoutConfig(mode, path, block, allowRender, imageDim)
     
def table(request):    
    css_path = os.path.join(settings.STATIC_URL, 'css/project_content_style.css')
    jquery_path = os.path.join(settings.STATIC_URL, 'js/jquery.js')
    jquery_cookie = os.path.join(settings.STATIC_URL, 'js/jquery.cookie.js')
    jquery_treeview = os.path.join(settings.STATIC_URL, 'js/jquery.treeview.js')
    launchTreeview = os.path.join(settings.STATIC_URL, 'js/launchTreeview.js')
    utils_path = os.path.join(settings.STATIC_URL, 'js/utils.js')
    #############
    
    path = request.GET.get('path', 'tux_vol.xmd')
    block = request.GET.get('block', '')
    allowRender = 'render' in request.GET
    imageDim = request.GET.get('dim', None)
    
    md = loadMetaData(path, block, allowRender, imageDim)    
   
    
    context = {
               'jquery': jquery_path,
               'utils': utils_path,
               'jquery_cookie': jquery_cookie,
               'jquery_treeview': jquery_treeview,
               'launchTreeview': launchTreeview,
               'css': css_path,
               'metadata': md}
    
    return render_to_response('table.html', context)

def get_table(request):
    from django.http import HttpResponse
    import json
    response_data={}
    response_data["sEcho"]=1
    response_data["iTotalRecords"]=2
    response_data["iTotalDisplayRecords"]=2
    response_data["aaData"]=[["taka","laka","xaka","jaka","raka"],["taka","laka","xaka","jaka","raka"]]
    
    
    print json.dumps(response_data)
    
    return HttpResponse(json.dumps(response_data), content_type="application/json")

def get_image(request):
    from django.http import HttpResponse
    from pyworkflow.gui import getImage, getPILImage
    imageNo = None
    imagePath = request.GET.get('image')
    imageDim = request.GET.get('dim', 150)
    
    
    # PAJM: Como vamos a gestionar lsa imagen    
    if imagePath.endswith('png') or imagePath.endswith('gif'):
        img = getImage(imagePath, tk=False)
    else:
        if AT in imagePath:
            parts = imagePath.split(AT)
            imageNo = parts[0]
            imagePath = parts[1]
        imagePath = getInputPath('showj', imagePath)
        if imageNo:
            imagePath = '%s@%s' % (imageNo, imagePath) 
        imgXmipp = xmipp.Image(imagePath)
        # from PIL import Image
        img = getPILImage(imgXmipp, imageDim)
        
        
        
    # response = HttpResponse(mimetype="image/png")    
    response = HttpResponse(mimetype="image/png")
    img.save(response, "PNG")
    return response
    
if __name__ == '__main__':
    root = loadProtTree()    
    for s in root.childs:
        print s.name, '-', s.tag
        for p in s.childs:
            print p.name, '-', p.tag
