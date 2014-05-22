# **************************************************************************
# *
# * Authors:     J.M. De la Rosa Trevin (jmdelarosa@cnb.csic.es)
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
"""
This module implement the wrappers around xmipp_showj
visualization program.
"""

import Tkinter as tk
from Tkinter import *

from pyworkflow.em import ProtUserSubSet, Class2D, Particle, SetOfClasses2D
from pyworkflow.protocol.params import IntParam, FloatParam, BooleanParam
from pyworkflow.protocol.constants import STATUS_FINISHED
from pyworkflow.viewer import Viewer, ProtocolViewer, DESKTOP_TKINTER, WEB_DJANGO
from pyworkflow.utils.graph import Graph
from pyworkflow.utils.path import cleanPath
from pyworkflow.gui import Window
from pyworkflow.gui.widgets import HotButton
from pyworkflow.gui.graph import LevelTree
from pyworkflow.gui.canvas import Canvas, ImageBox
from pyworkflow.em.viewer import DataView, ObjectView
from pyworkflow.em.packages.xmipp3.viewer import XmippViewer
from protocol import SpiderProtClassifyWard, SpiderProtClassifyDiday

from spider import SpiderDocFile


class SpiderViewerClassify(ProtocolViewer):
    """ Visualization of some Classification protocols in Spider. """
           
    _environments = [DESKTOP_TKINTER, WEB_DJANGO]
    
    def _defineParams(self, form):
        form.addSection(label='Visualization')
        group1 = form.addGroup('Dendogram')
        group1.addParam('doShowDendrogram', BooleanParam, label="Show dendrogram?", default=True,
                      help='')
        group1.addParam('minHeight', FloatParam, default=0.5,
                      label='Minimum height',
                      help='The dendrogram will be show until that height')
        self.groupClass = form.addGroup('Classes')
        self.groupClass.addParam('doShowClasses', BooleanParam, label="Visualize class averages?", default=True, 
                      help='')

    def _getVisualizeDict(self):
        return {'doShowDendrogram': self._plotDendrogram,
                'doShowClasses': self.visualizeClasses
                }

    def _plotDendrogram(self, e=None):
        from pyworkflow.em.packages.xmipp3.plotter import XmippPlotter
        xplotter = XmippPlotter()
        self.plt = xplotter.createSubPlot("Dendrogram", "", "")
        self.step = 0.25
        self.rightMost = 0.0 # Used to arrange leaf nodes at the bottom
        
        node = self.protocol.buildDendrogram()
        self.plotNode(node, self.minHeight.get())    
        self.plt.set_xlim(0., self.rightMost + self.step)
        self.plt.set_ylim(-10, 105)
        
        return [xplotter]
    
    def plotNode(self, node, minHeight=-1):
        childs = node.getChilds()
        h = node.height
        if h > minHeight and len(childs) > 1:
            x1, y1 = self.plotNode(childs[0], minHeight)
            x2, y2 = self.plotNode(childs[1], minHeight)
            xm = (x1 + x2)/2
            x = [x1, x1, x2, x2]
            y = [y1, h, h, y2]
            self.plt.plot(x, y, color='b')
            point = (xm, h)
        else:
            self.rightMost += self.step
            point = (self.rightMost, 0.)
        
        length = node.length
        index = node.index
        self.plt.annotate("%d(%d)" % (index, length), point, xytext=(0, -5),
                     textcoords='offset points', va='top', ha='center', size='x-small')
        self.plt.plot(point[0], point[1], 'ro')
        
        return point
    
    def _createNode(self, canvas, node, y):
        node.selected = False
        node.box = SpiderImageBox(canvas, node, y)
        return node.box
            
            
class SpiderViewerWard(SpiderViewerClassify):
    _targets = [SpiderProtClassifyWard]
    _label = "viewer ward"
    
    def _defineParams(self, form):
        SpiderViewerClassify._defineParams(self, form)

        self.groupClass.addParam('maxLevel', IntParam, default=4,
                      label='Maximum level',
                      help='Maximum level of classes to show')


    def visualizeClasses(self, e=None):
        classTemplate = "class_%03d"
        averages = '%03d@' + self.protocol._getFileName('averages')
        
        def getInfo2(level, classNo):
            return classTemplate % classNo, averages % classNo
        
        node = self.protocol.buildDendrogram()
        g = Graph(root=node)
        self.graph = g
               
        self.win = Window("Select classes", self.formWindow, minsize=(1000, 600))
        root = self.win.root
        canvas = Canvas(root)
        canvas.grid(row=0, column=0, sticky='nsew')
        root.grid_columnconfigure(0, weight=1)
        root.grid_rowconfigure(0, weight=1) 
        
        self.buttonframe = Frame(root)
        self.buttonframe.grid(row=2, column=0, columnspan=2)  
        self.win.createCloseButton(self.buttonframe).grid(row=0, column=0, sticky='n', padx=5, pady=5) 
        saveparticlesbtn = HotButton(self.buttonframe, "Create Particles", command=self.saveParticles)
        saveparticlesbtn.grid(row=0, column=1, sticky='n', padx=5, pady=5)  
        btn = HotButton(self.buttonframe, "Create Classes", command=self.saveClasses)
        btn.grid(row=0, column=2, sticky='n', padx=5, pady=5)
            
        lt = LevelTree(g)
        lt.DY = 135 # TODO: change in percent of the image size
        lt.setCanvas(canvas)
        lt.paint(self._createNode, maxLevel=self.maxLevel.get()-1)
        canvas.updateScrollRegion()
        
        return [self.win]
        
    def saveClasses(self, e=None):
        """ Store selected classes. """
        try:
            selectedNodes = [node for node in self.graph.getNodes() if node.selected]
            suffix = 'Selection'
            prot = ProtUserSubSet(inputType='Classes', outputType='Classes')
            prot.createInputPointer(self.protocol.outputClasses)
            self._project._setupProtocol(prot)
            prot.makePathsAndClean()
            classes = prot._createSetOfClasses2D(self.protocol.inputParticles.get(), suffix)
            averages = prot._createSetOfParticles(suffix)
            averages.copyInfo(self.protocol.outputClasses.getRepresentatives())
            self.protocol._fillClassesFromNodes(classes, averages, selectedNodes)
            prot.createOutputSet(classes)
            prot.setStatus(STATUS_FINISHED)
            self._project._storeProtocol(prot)
            #self.project.launchProtocol(prot, wait=True)
            self.win.showInfo("Protocol %s created. " % prot.getName())
        except Exception, ex:
            import traceback
            traceback.print_exc()    
            self.win.showError(str(ex))
            #raise
            
    def saveParticles(self, e=None):
        """ Store particles from selected classes. """
        try:
            selectedNodes = [node for node in self.graph.getNodes() if node.selected]
            suffix = 'Selection'
            prot = ProtUserSubSet(inputType='Classes', outputType='Particles')
            prot.createInputPointer(self.protocol.outputClasses)
            self._project._setupProtocol(prot)
            prot.makePathsAndClean()
            
            particles = prot._createSetOfParticles(suffix)
            particles.copyInfo(self.protocol.outputClasses.getImages())
            self.protocol._fillParticlesFromNodes(particles, selectedNodes)
            prot.createOutputSet(particles)
            prot.setStatus(STATUS_FINISHED)
            self._project._storeProtocol(prot)
            #self.project.launchProtocol(prot, wait=True)
            self.win.showInfo("Protocol %s created. " % prot.getName())
        except Exception, ex:
            import traceback
            traceback.print_exc()    
            self.win.showError(str(ex))
        
        
class SpiderImageBox(ImageBox):
    def __init__(self, canvas, node, y):
        ImageBox.__init__(self, canvas, node.path, text=node.getName(), y=y)
        
    def _onClick(self, e=None):
        if self.node.path is None:
            return
        # On click change the selection state
        self.node.selected = not self.node.selected
        if self.node.selected:
            self.label.config(bd=2, bg='green')
            
        else:
            self.label.config(bd=0, bg='grey')
            
            
class SpiderViewerDiday(SpiderViewerClassify):
    _targets = [SpiderProtClassifyDiday]
    _label = "viewer diday"
    
    def _defineParams(self, form):
        SpiderViewerClassify._defineParams(self, form)

        self.groupClass.addParam('numberOfClasses', IntParam, default=4,
                      label='Number of classes',
                      help='Desired number of classes.')


    def visualizeClasses(self, e=None):
        prot = self.protocol
        classDir = prot.getClassDir()
        
        params = {'[class_dir]': classDir,
                  '[desired-classes]': self.numberOfClasses.get(),
                  '[particles]': prot._params['particles'] + '@******',                  
                  }
        
        prot.runScript('mda/classavg.msa', prot.getExt(), params)

        particles = prot.inputParticles.get()
        sampling = particles.getSamplingRate()
        
        setFn = self._getPath('classes2D.sqlite')
        cleanPath(setFn)
        classes2D = SetOfClasses2D(filename=setFn)
        classes2D.setImages(particles)
            
        for classId in range(1, self.numberOfClasses.get()+1):
            class2D = Class2D()
            class2D.setObjId(classId)
            
            avgImg = Particle()
            avgImg.setSamplingRate(sampling)
            avgFn = prot._getPath(classDir, 'classaverage%03d.stk' % classId)
            avgImg.setLocation(1, avgFn)
            
            class2D.setRepresentative(avgImg)
            classes2D.append(class2D)
            
            docClass = prot._getPath(classDir, 'docclass%03d.stk' % classId)
            doc = SpiderDocFile(docClass)
            
            for values in doc.iterValues():
                imgId = int(values[0])
                img = particles[imgId]
                class2D.append(img)
                
        from pyworkflow.em.packages.xmipp3.convert import writeSetOfClasses2D
        fn = self._getPath('classes.xmd')
        writeSetOfClasses2D(classes2D, fn)
        
        return [DataView(fn)]
                              