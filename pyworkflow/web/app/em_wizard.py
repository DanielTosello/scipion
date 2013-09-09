import os
from os.path import basename
import xmipp
from pyworkflow.web.app.views_util import * 
from pyworkflow.manager import Manager
from pyworkflow.project import Project
from django.shortcuts import render_to_response
from pyworkflow.gui import getImage, getPILImage
from django.http import HttpResponse

def wizard(request):
    _ , protocol = loadProtocolProject(request, 'GET')
    functionName = request.GET.get('function', None)
    function = globals().get(functionName, None)
    param = request.GET.get('param', None)
    
    if function is None:
        pass  # redirect to error: wizard not found
    elif not callable(function):
        pass  # redirect to error: name is not a function
    else:
        return function(protocol, param)

def wiz_downsampling(protocol, param):
    mics = [mic for mic in protocol.inputMicrographs.get()]
    for m in mics:
        m.basename = basename(m.getFileName())
        
    param = param.split('-');
    
    context = {'objects': mics,
               'downFactor': param[1]}
    
    return render_to_response('wiz_downsampling.html', context)

def wiz_ctf(protocol, param):
    mics = [mic for mic in protocol.inputMicrographs.get()]
    for m in mics:
        m.basename = basename(m.getFileName())
    
    param = param.split('-');
    
    if param[0] == "lowRes":
        highRes = 0.25
        lowRes= param[1]
    elif param[0] == 'highRes':
        lowRes = 0.25
        highRes= param[1]
    
    
    context = {'objects': mics,
               'raphael':getResourceJs('rapahel'),
               'high_res' : highRes,
               'low_res': lowRes
               }
    
    return render_to_response('wiz_ctf.html', context)
    
def wiz_fourier(protocol, param):
    mics = [mic for mic in protocol.inputMicrographs.get()]
    for m in mics:
        m.basename = basename(m.getFileName())
    
    param = param.split('-');
    
    if param[0] == "lowRes":
        highRes = 0.25
        lowRes= param[1]
    elif param[0] == 'highRes':
        lowRes = 0.25
        highRes= param[1]
    
    
    context = {'objects': mics,
               'raphael':getResourceJs('rapahel'),
               'high_res' : highRes,
               'low_res': lowRes
               }
    
    return render_to_response('wiz_fourier.html', context)

def wiz_gaussian(protocol, param):
    mics = [mic for mic in protocol.inputMicrographs.get()]
    for m in mics:
        m.basename = basename(m.getFileName())
        
    param = param.split('-');
    
    context = {'objects': mics,
               'downFactor': param[1]}
    
    return render_to_response('wiz_gaussian.html', context)

def get_image_psd(request):
    imagePath = request.GET.get('image', None)
    downsample = request.GET.get('downsample', None)
    dim = request.GET.get('dim', None)
    
    # create a xmipp image empty
    imgXmipp = xmipp.Image()
    
    # compute the PSD image
    xmipp.fastEstimateEnhancedPSD(imgXmipp, str(imagePath), int(downsample), int(dim), 2)
        
    # from PIL import Image
    img = getPILImage(imgXmipp, dim)
        
    response = HttpResponse(mimetype="image/png")
    img.save(response, "PNG")
    return response
