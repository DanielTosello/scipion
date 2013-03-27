#!/usr/bin/env xmipp_python
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

import xmipp
from multiprocessing.connection import Client
from protlib_xmipp import getImageData
from protlib_gui_figure import ImageWindow
from pickle import dumps, loads
from threading import Thread
from os import system
from numpy import array, ndarray, flipud
from time import gmtime, strftime
from datetime import datetime

class XmippChimeraClient:
    
    def __init__(self, volfile):
        self.volfile = volfile
        #print 'volfile: ' + self.volfile
        self.address = ''
        self.port = 6000
        self.authkey = 'test'
        self.client = Client((self.address, self.port), authkey=self.authkey)
        printCmd('initVolumeData')
        self.initVolumeData()
        printCmd('openVolumeOnServer')
        self.openVolumeOnServer(self.vol)
        printCmd('init ended')
        
       
    def send(self, cmd, data):
        print cmd
        self.client.send(cmd)
        self.client.send(data)
        
        
    def openVolumeOnServer(self, volume):
         self.send('open_volume', volume)



        

    def initListen(self):
            self.listen_thread = Thread(target=self.listen)
            self.listen_thread.daemon = True
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
            print 'finally'
            self.exit()
            
    def exit(self):
            self.client.close()#close connection

    def initVolumeData(self):
        self.image = xmipp.Image(self.volfile)
        self.image.convert2DataType(xmipp.DT_DOUBLE)
        xdim, ydim, zdim, n = self.image.getDimensions()
        self.vol = getImageData(self.image)
        
    def answer(self, msg):
        if msg == 'exit_server':
            self.listen = False



class XmippProjectionExplorer(XmippChimeraClient):
    
    def __init__(self, volfile):
        XmippChimeraClient.__init__(self, volfile)
        self.fourierprojector = FourierProjector(self.vol)
        self.initListen()
        self.initProjection()
        self.iw = ImageWindow(image=self.projection, label="Projection")
        self.iw.root.protocol("WM_DELETE_WINDOW", self.exitClient)
        self.iw.root.mainloop()
        
     
        
    def initProjection(self):
        self.projection = self.image.projectVolumeDouble(0, 0, 0)
        

    def rotate(self, rot, tilt, psi):
        printCmd('image.projectVolumeDouble')
        
        self.projection = self.fourier.projector.projectVolumeDouble(rot, tilt, psi)#self.image.projectVolumeDoubleUsingFourier(rot, tilt, psi)
        printCmd('flipud')
        self.vol = flipud(getImageData(self.projection))
        printCmd('iw.updateData')
        self.iw.updateData(self.vol)
        printCmd('end rotate')
    
        
    def exit(self):
        XmippChimeraClient.exit(self)
        if not (self.iw is None):
            self.iw.root.destroy()
            
    def answer(self, msg):
        XmippChimeraClient.answer(self, msg)
        if msg == 'motion_stop':
            data = loads(self.client.recv())#wait for data
            printCmd('reading motion')
            self.motion = array(data)
            printCmd('getting euler angles')
            rot1, tilt1, psi1 = xmipp.Euler_matrix2angles(self.motion)
            printCmd('calling rotate')  
            self.rotate(rot1, tilt1, psi1)
            
    def exitClient(self):
        self.client.send('exit_client')
        self.exit()
            
            
def printCmd(cmd):
        timeformat = "%S.%f" 
        print datetime.now().strftime(timeformat)
        print cmd

