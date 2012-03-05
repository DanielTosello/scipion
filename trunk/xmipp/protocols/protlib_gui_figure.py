'''
/***************************************************************************
 * Authors:     J.M. de la Rosa Trevin (jmdelarosa@cnb.csic.es)
 *
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 *
 *  All comments concerning this program package may be sent to the
 *  e-mail address 'xmipp@cnb.csic.es'
 ***************************************************************************/
 '''

import Tkinter as tk
import ttk
import matplotlib
matplotlib.use('TkAgg')
import numpy as np
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.cm as cm
from mpl_toolkits.axes_grid.axislines import SubplotZero
from matplotlib.patches import Wedge

def createBgImage(dim):
    from numpy import ones
    return ones((dim, dim, 3))

class Preview():
    def __init__(self, parent, dim, dpi=36, label=None, col=0, row=0):
        self.dim = dim
        self.bg = np.zeros((dim, dim), float)
        ddim = dim/dpi
        self.figure = Figure(figsize=(ddim, ddim), dpi=dpi, frameon=False)
        self.frame = tk.Frame(parent)
        self.canvas = FigureCanvasTkAgg(self.figure, master=self.frame)
        self.canvas.get_tk_widget().grid(column=0, row=0)#, sticky=(N, W, E, S))
        if label:
            tk.Label(self.frame, text=label).grid(column=0, row=1)
        self.frame.grid(column=col, row=row)
        self._create_axes()
        
    def _create_axes(self):
        pass
    
    def _update(self):
        pass
    
    def clear(self):
        self._update(self.bg)
        
    def updateData(self, Z):
        self.clear()
        self._update(Z)
    
    
class ImagePreview(Preview):
    def __init__(self, parent, dim, dpi=36, label=None, col=0):
        Preview.__init__(self, parent, dim, dpi, label, col)
    
    def _create_axes(self):
        ax = self.figure.add_axes([0,0,1,1], frameon=False)       
        self.figureimg = ax.imshow(self.bg, cmap=cm.gray)#, extent=[-h, h, -h, h])
        ax.set_axis_off()
        
    def _update(self, Z, *args):
        self.figureimg.set_data(Z)
        self.figureimg.autoscale()
        self.figureimg.set(extent=[0, Z.shape[1], 0, Z.shape[0]])
        self.canvas.draw()
        
class PsdPreview(Preview):
    def __init__(self, master, dim, lf, hf, dpi=72, Z=None, col=0):
        Preview.__init__(self, master, dim, dpi, label="PSD", col=col)
        self.lf = lf
        self.hf = hf
        if self.ring:
            self.createRing()
        else:
            self.canvas.draw()
                            
    def _create_axes(self):
        #axdef = SubplotZero(self.figure, 111)
        #ax = self.figure.add_subplot(axdef)
        ax = self.figure.add_axes([0.1,0.1,0.8,0.8], frameon=False)
        #ax.xaxis.set_offset_position(0.5)       
        #ax.set_position([0, 0, 1, 1])
        h = 0.5
        ax.set_xlim(-h, h)
        ax.set_ylim(-h, h)
        ax.grid(True)
        self.ring = None
        self.img = ax.imshow(self.bg, cmap=cm.gray, extent=[-h, h, -h, h])
#        for direction in ["xzero", "yzero"]:
#            ax.axis[direction].set_visible(True)
        
#        print "minor", ax.axis['xzero'].minor_ticklabels
#        print 'major', ax.axis['xzero'].major_ticklabels
        
#        for direction in ["left", "right", "bottom", "top"]:
#            ax.axis[direction].set_visible(False)
        self.ax = ax
        
    def createRing(self):
        radius = float(self.hf)
        width = radius - float(self.lf)
        self.ring = Wedge((0,0), radius, 0, 360, width=width, alpha=0.15) # Full ring
        self.ax.add_patch(self.ring)
        self.canvas.draw()
        
    def updateFreq(self, lf, hf):
        self.lf = lf
        self.hf = hf
        if self.ring:
            self.ring.remove()
            self.ring = None
        if self.hf:
            self.createRing()
    
    def _update(self, Z):
        if self.ring:
            self.ring.remove()
            self.ring = None
        if self.hf:
            self.createRing()
        self.img.set_data(Z)
        self.img.autoscale()
        self.canvas.draw()        
    
w = None
def showImage(filename, dim=512, dpi=96):
    import xmipp
    #from pylab import axes, Slider
    from protlib_xmipp import getImageData
    
    h = 0.5
    lf0 = 0.15
    hf0 = 0.35
    axcolor = 'lightgoldenrodyellow'
    
    img = xmipp.Image()
    img.readPreview(filename, dim)
    xdim, ydim, zdim, n = img.getDimensions()
    Z = getImageData(img)
    xdim += 10
    ydim += 10
    figure = Figure(figsize=(xdim/dpi, ydim/dpi), dpi=dpi, frameon=False)
    # a tk.DrawingArea
    root = tk.Tk()
    root.title(filename)
    canvas = FigureCanvasTkAgg(figure, master=root)
    canvas.get_tk_widget().grid(column=0, row=0)#, sticky=(N, W, E, S))
    #figureimg = figure.figimage(Z, cmap=cm.gray)#, origin='lower')
    #ax = SubplotZero(figure, 111)
    #ax = figure.add_subplot(ax)
    ax = figure.add_axes([0.2,0.2,0.6,0.6], frameon=False)
    ax.set_xlim(-0.5, 0.5)
    ax.set_ylim(-0.5, 0.5)
    #ax.xaxis.set_offset_position(0.5)
    #ax.set_axis_off()
    #axes([0.1, 0.1, 0.9, 0.9])
    #ax.set_aspect(1.0)
    ax.imshow(Z, cmap=cm.gray, extent=[-h, h, -h, h])
    #for direction in ["xzero", "yzero"]:
    #    ax.axis[direction].set_visible(True)
    #for direction in ["left", "right", "bottom", "top"]:
    #    ax.axis[direction].set_visible(False)
    global w
    #w = Wedge((0,0), hf0, 0, 360, width=lf0, alpha=0.15) # Full ring
    #ax.add_patch(w)
    
    def update(hf, lf):
        global w
        w.remove()
        w2 = Wedge((0,0), hf, 0, 360, width=lf, alpha=0.2)
        ax.add_patch(w2)
        w = w2
        canvas.draw()
        
    root.mainloop()
    
def getPngData(filename):  
    import matplotlib.image as mpimg
    return mpimg.imread(filename)

import numpy as np
import matplotlib.ticker as ticker
import matplotlib.gridspec as gridspec
import matplotlib.pyplot as plt

''' Class to create several plots'''
class XmippPlotter():
    def __init__(self, x=1, y=1, mainTitle="", figsize=None, dpi=100):
        
        if figsize is None: # Set some defaults values
            if x == 1 and y == 1:
                figsize = (6, 5)
            elif x == 1 and y == 2:
                figsize = (4, 6)
            elif x == 2 and y == 1:
                figsize = (6, 4)
            else:
                figsize = (8, 6)
        
        # Create grid
        self.grid = gridspec.GridSpec(x, y)#, height_ratios=[7,4])
        self.grid.update(left=0.15, right=0.95, hspace=0.25, wspace=0.4)#, top=0.8, bottom=0.2)  
        self.gridx = x
        self.gridy = y  
        self.figure = plt.figure(figsize=figsize, dpi=dpi)
        self.mainTitle = mainTitle
        if (mainTitle):
            self.figure.suptitle(mainTitle)
        self.plot_count = 0
        self.plot_axis_fontsize = 10
        self.plot_text_fontsize = 8
        self.plot_yformat = '%1.2e'

    def showLegend(self, labels):
        leg = self.last_subplot.legend(tuple(labels))
        for t in leg.get_texts():
            t.set_fontsize(self.plot_axis_fontsize)    # the legend text fontsize
        
    def createSubPlot(self, title, xlabel, ylabel, xpos=None, ypos=None, 
                      yformat=False, projection='rectilinear'):
        '''
        Create a subplot in the figure. 
        You should provide plot title, and x and y axis labels.
        yformat True specified the use of global self.plot_yformat
        Posibles values for projection are: 
            'aitoff', 'hammer', 'lambert', 'mollweide', 'polar', 'rectilinear' 
        
        '''
        if xpos is None:
            self.plot_count += 1
            pos = self.plot_count
        else:
            pos = xpos + (ypos - 1) * self.gridx
        a = self.figure.add_subplot(self.gridx, self.gridy, pos, projection=projection)
        #a.get_label().set_fontsize(12)
        a.set_title(title)
        a.set_xlabel(xlabel)
        a.set_ylabel(ylabel)
            
        if yformat:
            formatter = ticker.FormatStrFormatter(self.plot_yformat)
            a.yaxis.set_major_formatter(formatter)
        a.xaxis.get_label().set_fontsize(self.plot_axis_fontsize)
        a.yaxis.get_label().set_fontsize(self.plot_axis_fontsize)
        labels = a.xaxis.get_ticklabels() + a.yaxis.get_ticklabels()
        for label in labels:
            label.set_fontsize(self.plot_text_fontsize) # Set fontsize
            label.set_text('aa')
                #print label.
                #label.set_visible(False)
        self.last_subplot = a
        self.plot = a.plot
        self.hist = a.hist
        return a
    
    def plotMd(self, md, mdLabelX, mdLabelY, color='g',**args):
        """ plot metadata columns mdLabelX and mdLabelY
            if nbins is in args then and histogram over y data is made
        """
        if mdLabelX:
            xx = []
        else:
            xx = range(1, md.size() + 1)
        yy = []
        for objId in md:
            if mdLabelX:
                xx.append(md.getValue(mdLabelX, objId))
            yy.append(md.getValue(mdLabelY, objId))
        if args.has_key('nbins'):
            print "nnbins",args['nbins']
            nbins = args.pop('nbins', None)
            
            self.hist(yy,nbins, facecolor=color,**args)
        else:
            self.plot(xx, yy, color,**args) #no histogram
        
    def plotMdFile(self, mdFilename, mdLabelX, mdLabelY, color='g', **args):
        """ plot metadataFile columns mdLabelX and mdLabelY
            if nbins is in args then and histogram over y data is made
        """
        from xmipp import MetaData
        md = MetaData(mdFilename)
        self.plotMd(md, mdLabelX, mdLabelY, color='g',**args)
        
    def show(self):
        plt.tight_layout()
        plt.show()

    def draw(self):
        plt.tight_layout()
        plt.draw()
        