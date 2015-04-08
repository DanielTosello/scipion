#!/usr/bin/env python
'''
#/***************************************************************************
# * Authors:     Airen Zaldivar (azaldivar@cnb.csic.es)
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

from xmipp import *
from multiprocessing.connection import Client
from protlib_xmipp import getImageData
from protlib_gui_figure import ImageWindow
from threading import Thread
from os import system
from numpy import array, ndarray, flipud
from time import gmtime, strftime
from datetime import datetime
from os.path import exists
from decimal import *
import sys

class XmippChimeraClient:
    
    def __init__(self, volfile, port, angulardist=[], voxelSize=None):
        
        if volfile is None:
            raise ValueError(volfile)
        if '@' in volfile:
            [index, file] = volfile.split('@'); 
        else :
            file = volfile 

        self.volfile = volfile
        self.voxelSize = voxelSize
        self.address = ''
        self.port = port #6000
        #is port available?
        self.authkey = 'test'
        self.client = Client((self.address, self.port), authkey=self.authkey)
        
        printCmd('initVolumeData')
        self.initVolumeData()   
        
        if angulardist:
            self.angulardistfile = angulardist[0]
            self.spheres_color = angulardist[1]
            spheres_distance = angulardist[2]
            spheres_maxradius = angulardist[3]
            self.spheres_distance = float(spheres_distance) if not spheres_distance == 'default' else 0.75 * max(self.xdim, self.ydim, self.zdim)
            self.spheres_maxradius = float(spheres_maxradius) if not spheres_maxradius == 'default' else 0.02 * self.spheres_distance
               
        printCmd('openVolumeOnServer')
        self.openVolumeOnServer(self.vol)
        self.initListenThread()
    
    
    def loadAngularDist(self):

        md = MetaData(self.angulardistfile)
        angleRotLabel = MDL_ANGLE_ROT
        angleTiltLabel = MDL_ANGLE_TILT
        anglePsiLabel = MDL_ANGLE_PSI
        if not md.containsLabel(MDL_ANGLE_ROT):
            angleRotLabel = RLN_ORIENT_ROT
            angleTiltLabel = RLN_ORIENT_TILT
            anglePsiLabel = RLN_ORIENT_PSI
        if not md.containsLabel(MDL_WEIGHT):
            md.fillConstant(MDL_WEIGHT, 1.)
            
        maxweight = md.aggregateSingle(AGGR_MAX, MDL_WEIGHT)
        minweight = md.aggregateSingle(AGGR_MIN, MDL_WEIGHT)
        interval = maxweight - minweight
        if interval < 1:
            interval = 1
        minweight = minweight - 1#to avoid 0 on normalized weight
        
        self.angulardist = []  
        x2=self.xdim/2
        y2=self.ydim/2
        z2=self.zdim/2
        #cofr does not seem to work!
        #self.angulardist.append('cofr %d,%d,%d'%(x2,y2,z2))
        
        for id in md:
            
            rot = md.getValue(angleRotLabel, id)
            tilt = md.getValue(angleTiltLabel, id)
            psi = md.getValue(anglePsiLabel, id)
            weight = md.getValue(MDL_WEIGHT, id)
            weight = (weight - minweight)/interval
            x, y, z = Euler_direction(rot, tilt, psi)
            radius = weight * self.spheres_maxradius

            x = x * self.spheres_distance + x2
            y = y * self.spheres_distance + y2
            z = z * self.spheres_distance + z2
            command = 'shape sphere radius %s center %s,%s,%s color %s '%(radius, x, y, z, self.spheres_color)

            self.angulardist.append(command)    
            printCmd(command)
            
    def send(self, cmd, data):
        #print cmd
        self.client.send(cmd)
        self.client.send(data)
        
        
    def openVolumeOnServer(self, volume):
         self.send('open_volume', volume)
         if not self.voxelSize is None:
             self.send('voxelSize', self.voxelSize)
         if not self.angulardistfile is None:
             self.loadAngularDist()
             self.send('draw_angular_distribution', self.angulardist)
         self.client.send('end')
        

    def initListenThread(self):
            self.listen_thread = Thread(target=self.listen)
            #self.listen_thread.daemon = True
            self.listen_thread.start()
         
    
    def listen(self):
        
        self.listen = True
        try:
            while self.listen:
                #print 'on client loop'
                msg = self.client.recv()
                self.answer(msg)
                            
        except EOFError:
            print 'Lost connection to server'
        finally:
            self.exit()
            
            
    def exit(self):
            self.client.close()#close connection


    def initVolumeData(self):
        self.image = Image(self.volfile)
        self.image.convert2DataType(DT_DOUBLE)
        self.xdim, self.ydim, self.zdim, self.n = self.image.getDimensions()
        printCmd("size %dx %dx %d"%(self.xdim, self.ydim, self.zdim))
        self.vol = getImageData(self.image)
        
    def answer(self, msg):
        if msg == 'exit_server':
            self.listen = False



class XmippProjectionExplorer(XmippChimeraClient):
    
    def __init__(self, volfile, port, angulardist=[], size='default', padding_factor=1, max_freq=0.5, spline_degree=2, voxelSize=None):
        XmippChimeraClient.__init__(self, volfile, port, angulardist, voxelSize)
        
        self.projection = Image()
        self.projection.setDataType(DT_DOUBLE)
        #0.5 ->  Niquiest frequency
        #2 -> bspline interpolation
        #print 'creating Fourier Projector'
        
        self.fourierprojector = FourierProjector(self.image, padding_factor, max_freq, spline_degree)
        self.fourierprojector.projectVolume(self.projection, 0, 0, 0)
        
        if size == 'default':
            self.size = self.xdim if self.xdim > 128 else 128
        else:
            self.size = float(size) 

        import ntpath
        self.iw = ImageWindow(filename=ntpath.basename(volfile),image=self.projection, dim=self.size, label="Projection")
        
        self.iw.root.protocol("WM_DELETE_WINDOW", self.exitClient)
        self.iw.root.mainloop()

    def rotate(self, rot, tilt, psi):
        printCmd('image.projectVolumeDouble')
        self.fourierprojector.projectVolume(self.projection, rot, tilt, psi)
        printCmd('flipud')
        self.vol = flipud(getImageData(self.projection))
        printCmd('iw.updateData')
        self.iw.updateData(self.vol)
        printCmd('end rotate')
    
        
    def exit(self):
        XmippChimeraClient.exit(self)
        if hasattr(self, "iw"):
            self.iw.root.destroy()
        
            
    def answer(self, msg):
        XmippChimeraClient.answer(self, msg)
        if msg == 'motion_stop':
            data = self.client.recv()#wait for data
            printCmd('reading motion')
            self.motion = array(data)
            printCmd('getting euler angles')
            rot, tilt, psi = Euler_matrix2angles(self.motion)
            printCmd('calling rotate')  
            self.rotate(rot, tilt, psi)
            
            
    def exitClient(self):
        if not self.listen:
            sys.exit(0)
    
    def initListenThread(self):
            self.listen_thread = Thread(target=self.listen)
            self.listen_thread.daemon = True
            self.listen_thread.start() 
            
def printCmd(cmd):
        timeformat = "%S.%f" 
        #print datetime.now().strftime(timeformat) + ' %s'%cmd
