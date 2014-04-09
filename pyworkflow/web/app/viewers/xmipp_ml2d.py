# **************************************************************************
# *
# * Authors:    Jose Gutierrez (jose.gutierrez@cnb.csic.es)
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

from pyworkflow.em.packages.xmipp3.viewer_ml2d import createPlots
from pyworkflow.web.app.views_util import *


def doShowClasses(request, protocolViewer):
    objId = str(protocolViewer.protocol.outputClasses.getObjId())
    return "showj","/visualize_object/?objectId="+ objId
            
def doAllPlotsML2D(request, protocolViewer):
    protViewerClass = str(protocolViewer.getClassName())
    protId = str(protocolViewer.protocol.getObjId())
    width, height = getSizePlotter(4)
    
    return "plotsComposite","/view_plots/?function=allPlotsML2D&protViewerClass="+ protViewerClass + "&protId="+ protId + "&width=" + str(width) + "&height="+ str(height)

def doSomePlotsML2D(protocolViewer):
    plots=""
    for p in protocolViewer._plotVars:
        if protocolViewer.getAttributeValue(p):
            plots = plots + p + "-"
    
    if plots != "":
        aux = plots.split("-")
        width, height = getSizePlotter(len(aux)-1)
        
        return "plotsComposite","/view_plots/?function=somePlotsML2D&plots="+str(plots)+"&protViewerClass="+ str(protocolViewer.getClassName())+ "&protId="+ str(protocolViewer.protocol.getObjId()) + "&width=" + str(width) + "&height="+ str(height)
    else:
        return "", None

def doShowLLML2D(request, protocolViewer):
    protViewerClass = str(protocolViewer.getClassName())
    protId = str(protocolViewer.protocol.getObjId())
    width, height = getSizePlotter(1)
    return "plot","/view_plots/?function=somePlotsML2D&plots=doShowLL&protViewerClass="+ protViewerClass + "&protId="+ protId + "&width=" + str(width) + "&height="+ str(height)

def doShowPmax(request, protocolViewer):
    protViewerClass = str(protocolViewer.getClassName())
    protId = str(protocolViewer.protocol.getObjId())
    width, height = getSizePlotter(1)
    return "plot","/view_plots/?function=somePlotsML2D&plots=doShowPmax&protViewerClass="+ protViewerClass + "&protId="+ protId + "&width=" + str(width) + "&height="+ str(height)

def doShowSignalChange(request, protocolViewer):
    protViewerClass = str(protocolViewer.getClassName())
    protId = str(protocolViewer.protocol.getObjId())
    width, height = getSizePlotter(1)
    return "plot","/view_plots/?function=somePlotsML2D&plots=doShowSignalChange&protViewerClass="+ protViewerClass + "&protId="+ protId + "&width=" + str(width) + "&height="+ str(height)
    
def doShowMirror(request, protocolViewer):
    protViewerClass = str(protocolViewer.getClassName())
    protId = str(protocolViewer.protocol.getObjId())
    width, height = getSizePlotter(1)
    return "plot","/view_plots/?function=somePlotsML2D&plots=doShowMirror&protViewerClass="+ protViewerClass + "&protId="+ protId + "&width=" + str(width) + "&height="+ str(height)

def allPlotsML2D(request, protocolViewer):
    xplotter = createPlots(protocolViewer.protocol, protocolViewer._plotVars)
    return xplotter

def somePlotsML2D(request, protocolViewer):
    plots = request.GET.get('plots', None)
    plots = str(plots).split("-")
    if len(plots) > 1:
        plots.remove(plots[len(plots)-1])
    
    xplotter = createPlots(protocolViewer.protocol, plots)
    return xplotter