# **************************************************************************
# *
# * Authors:  Ruben Sanchez (rsanchez@cnb.csic.es), April 2017
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
# *  e-mail address 'scipion@cnb.csic.es'
# *
# **************************************************************************

from __future__ import print_function

from six.moves import range
import numpy as np
import sys, os

import numpy as np
import scipy
import random
from math import ceil

import time
from os.path import expanduser
from pyworkflow.utils import Environ
from sklearn.metrics import roc_auc_score, accuracy_score
import xmipp
import pyworkflow.em.metadata as md
import joblib

DEBUG=False
if DEBUG: print("Debug MODE")

def updateEnviron():
  """ Create the needed environment for TensorFlow programs. """
  environ = Environ(os.environ)
  if  'CUDA' in os.environ and not os.environ['CUDA']=="False":
    environ.update({'LD_LIBRARY_PATH': os.environ['CUDA_LIB']}, position=Environ.BEGIN)
    environ.update({'LD_LIBRARY_PATH': os.environ['CUDA_HOME']+"/extras/CUPTI/lib64"}, position=Environ.BEGIN)

    os.environ['CUDA_VISIBLE_DEVICES']="2"  #THIS IS FOR USING JUST GPU:2 must be changed to select desired gpu
#    print(">>Environment", os.environ)
updateEnviron()

import tensorflow as tf
import tflearn
from pyworkflow.em.packages.xmipp3.networkDef import main_network

BATCH_SIZE= 128

EVALUATE_AT= 10
CHECK_POINT_AT= 100


def printAndOverride(msg):
  print("\r%s"%(msg), end="")
    
class DeepTFSupervised(object):
  def __init__(self, rootPath, learningRate= 1e-4, batch_size= BATCH_SIZE):
    '''
      @param rootPath: str. Root directory where neural net data will be saved.
                            Generally "extra/nnetData/"
                                                      tfchkpoints/
                                                      tflogs/
      @param learningRate: float. Learning rate for net training
      @param batch_size: integer. The number of elemets that will be fed to the net at each
                                  step          
    '''
    self.lRate= learningRate
    self.rootPath= rootPath

    self.checkPointsNames= os.path.join(rootPath,"tfchkpoints")

    self.checkPointsNames= os.path.join(self.checkPointsNames,"screening")

    self.logsSaveName= os.path.join(rootPath,"tflogs")
        
    self.batch_size= batch_size
    self.num_labels=2
    self.num_channels=None
    self.image_size=None
    
    # Tensorflow objects
    self.X= None
    self.Y= None
    self.saver = None
    self.session = None
    self.train_writer = None
    self.test_writer = None
    self.global_step= None
    self.loss= None
    self.optimizer= None
                                              
  def createNet(self, xdim, ydim, num_chan):
    '''
      @param xdim: int. height of images
      @param ydim: int. width of images
      @param num_chan: int. number of channels of images      
    '''
    print ("Creating net. Learning rate %.1e"%self.lRate)
    ##############################################################
    # INTIALIZE INPUT PLACEHOLDERS
    ##############################################################

    self.num_channels= num_chan
    self.image_size= (xdim, ydim)
    num_labels= self.num_labels
    tflearn.config.init_training_mode()
    self.X= tf.placeholder( tf.float32, shape=(None, self.image_size[0], self.image_size[1], num_chan), name="X")
    self.Y= tf.placeholder(tf.float32, shape=(None, num_labels), name="Y")
    
    ######################
    # NEURAL NET CREATION
    ######################
    self.global_step = tf.Variable(initial_value=0, name='global_step', trainable=False)
    (self.y_pred, self.merged_summaries, self.optimizer,
     self.loss, self.accuracy)= main_network(self.X,self.Y, num_labels, learningRate= self.lRate, globalStep= self.global_step)
  
  def startSessionAndInitialize(self, numberOfThreads=8):
    '''
      @param numberOfThreads. Number of threads to use in cpu mode. If numberOfThreads==None 
                              then default behaviour is expected (use GPU if available otherwise
                              one thread per cpu)
    '''
    print("Initializing tf session")
    
    save_dir, prefixName = os.path.split(self.checkPointsNames)
    if not os.path.exists(save_dir):
      os.makedirs(save_dir)

    self.saver = tf.train.Saver()  
    if numberOfThreads is None:
      self.session = tf.Session()
    else:
      self.session= tf.Session(config=tf.ConfigProto(intra_op_parallelism_threads=numberOfThreads))

    try:
      print("Trying to restore last checkpoint from "+ os.path.abspath(save_dir))
      # Use TensorFlow to find the latest checkpoint - if any.
      last_chk_path = tf.train.latest_checkpoint(checkpoint_dir= save_dir)

      # Try and load the data in the checkpoint.
      
      self.saver.restore(self.session, save_path=last_chk_path)
      # If we get to this point, the checkpoint was successfully loaded.
      print("Restored checkpoint from:", last_chk_path)
    except Exception as e:
      print("last chk_point path:",last_chk_path)
      print(e)
      # If the above failed for some reason, simply
      # initialize all the variables for the TensorFlow graph.
      print("Failed to restore checkpoint. Initializing variables instead.")
      if( os.path.isdir(self.logsSaveName)):
        tf.gfile.DeleteRecursively(self.logsSaveName)
      os.makedirs(os.path.join(self.logsSaveName,"train"))
      os.makedirs(os.path.join(self.logsSaveName,"test"))
      self.session.run( tf.global_variables_initializer() )
      
    self.train_writer = tf.summary.FileWriter(os.path.join(self.logsSaveName,"train"), self.session.graph)
    self.test_writer = tf.summary.FileWriter(os.path.join(self.logsSaveName,"test"), self.session.graph)
    return self.session
  
  def close(self, saveModel= False):
    '''
      Closes a tensorflow connection and related objects.
      @param. saveModel: boolean. If True, model will be saved prior closing model
    
    '''
    if saveModel:
      self.saver.save(self.session, save_path= self.checkPointsNames, global_step= self.global_step) 
      print("\nSaved checkpoint.")
    self.train_writer.close()
    self.test_writer.close()
    tf.reset_default_graph()
    self.session.close()


  def reset(self):
    '''
      Resets a tensorflow connection and related objects.
      Needed if 2 independent trains want to be done in the same program.
    
    '''
    self.train_writer.close()
    self.test_writer.close()
    self.session.close()
    tf.reset_default_graph()
    self.createNet()
    self.startSessionAndInitialize()


  def trainNet(self, numberOfBatches, dataManagerTrain, dataManagerTest=None):
    '''
      @param numberOfBatches: int. The number of batches that will be used for training 
      @param dataManagerTrain: DataManager. Object that will provide training batches (Xs and labels)
      @param dataManagerTest:  DataManager. Optional Object that will provide testing batches (Xs and labels)
                                            If not provided, no testing will be done
    '''
    
    ########################
    # TENSOR FLOW RUN
    ########################
    trainDataBatch= dataManagerTrain.getRandomBatch()
    x_batchTrain, labels_batchTrain= trainDataBatch
    feed_dict_train= {self.X : x_batchTrain, self.Y: labels_batchTrain}
    tflearn.is_training(False, session=self.session)
    i_global,stepLoss= self.session.run( [self.global_step, self.loss], feed_dict= feed_dict_train)
    numberOfRemainingBatches= max(0, numberOfBatches- i_global)
    bestStepLoss= 2^30
    currentLoss=[]
    if numberOfBatches >0:
      print("Training net for %d batches of size %d"%(numberOfRemainingBatches, dataManagerTrain.getBatchSize()))
      print("Initial loss %f"%stepLoss)
      self.testPerformance(i_global,trainDataBatch,dataManagerTest)
    else:
      return
    time0 = time.time()
    tflearn.is_training(True, session=self.session)
    for iterNum in range(numberOfRemainingBatches):
      trainDataBatch= dataManagerTrain.getRandomBatch()
      x_batchTrain, labels_batchTrain = trainDataBatch
      feed_dict_train= {self.X : x_batchTrain, self.Y: labels_batchTrain }
      i_global, __, stepLoss,= self.session.run( [self.global_step, self.optimizer, self.loss],
                                                 feed_dict=feed_dict_train )
      currentLoss.append( stepLoss)

      print("iterNum %d/%d trainLoss: %3.4f"%((i_global), numberOfBatches, stepLoss))
      sys.stdout.flush()      
      if i_global % EVALUATE_AT ==0:
        timeBatches= time.time() -time0
        timeTest=time.time()
        self.testPerformance(i_global,trainDataBatch,dataManagerTest)
        print("%d batches time: %f s. time for test %f s"%(EVALUATE_AT, timeBatches, time.time() -timeTest))
        time0 = time.time()
        sys.stdout.flush()
          
      if (i_global + 1 ) % CHECK_POINT_AT==0:
        stepLossMean= np.mean(currentLoss)
        currentLoss=[]
        improvement= bestStepLoss-stepLossMean
        print("Improvement since last checkpoint %.5f"%(improvement))
        if improvement>0:
          bestStepLoss= stepLossMean
          self.saver.save(self.session, save_path= self.checkPointsNames, global_step= self.global_step) 
          print("\nSaved checkpoint.")   

    self.testPerformance(i_global,trainDataBatch, dataManagerTest)
    
  def accuracy_score(self, labels, predictions ):
    return (100.0 * np.sum(np.argmax(predictions, 1) == np.argmax(labels, 1))
            / predictions.shape[0])    

  def testPerformance(self, stepNum, trainDataBatch, testDataManager=None):
    tflearn.is_training(False, session=self.session)      

    batch_x, batch_y= trainDataBatch
    feed_dict_train= {self.X : batch_x, self.Y: batch_y}
    c_e_train, y_pred_train, merged = self.session.run( [self.loss, self.y_pred, self.merged_summaries], 
                                                 feed_dict = feed_dict_train )
    train_accuracy=  self.accuracy_score(batch_y, y_pred_train)
    train_auc= roc_auc_score(batch_y, y_pred_train)
    self.train_writer.add_summary(merged, stepNum)
    if not testDataManager is None:
      y_pred_list=[]
      labels_list=[]
      for images, labels in testDataManager.getIteratorTestBatch(20):
        feed_dict_train= {self.X : images, self.Y : labels }
        y_pred, merged= self.session.run( [self.y_pred,self.merged_summaries], feed_dict=feed_dict_train )
        y_pred_list.append(y_pred)
        labels_list.append(labels)

      y_pred= np.concatenate(y_pred_list)
      labels= np.concatenate(labels_list)
      test_accuracy= self.accuracy_score(labels, y_pred)
      test_auc= roc_auc_score(labels, y_pred)

      self.test_writer.add_summary(merged, stepNum)
      print("iterNum %d accuracy(train/test) %f / %f   auc  %f / %f"%(stepNum,train_accuracy,test_accuracy,
                                                                      train_auc, test_auc))
    tflearn.is_training(True, session=self.session)      
    

  def predictNet(self, dataManger):
    tflearn.is_training(False, session=self.session)      
    y_pred_list=[]
    labels_list=[]
    metadataId_list=[]
    for images, labels, metadataIdTuple in dataManger.getIteratorPredictBatch():
      feed_dict_train= {self.X : images}
      y_pred= self.session.run( self.y_pred, feed_dict=feed_dict_train )
      y_pred_list.append(y_pred)
      labels_list.append(labels)
      metadataId_list+= metadataIdTuple
    y_pred= np.concatenate(y_pred_list)
    labels= np.concatenate(labels_list)
    accuracy= self.accuracy_score(labels, y_pred)
    auc= roc_auc_score(labels, y_pred)
    y_pred=y_pred[:,1]
    pos_labels= labels[:,1]
    print("GLOBAL test accuracy: %f  auc: %f"%(accuracy,auc))
    posScores= y_pred[pos_labels==1]
    negScores= y_pred[pos_labels==0]
    print("pos_mean %f pos_sd %f neg_mean %f neg_perc90 %f neg_perc95 %f"%(np.mean(posScores),np.std(posScores),
                                                         np.mean(negScores), np.percentile(negScores,90),
                                                         np.percentile(negScores,95)))
    print("number of y_pred , labels, metadataIdTuple %d %d %d"%(len(y_pred) , len(labels), len(metadataIdTuple)))                                                         
##    with open("/home/rsanchez/app/scipion/pyworkflow/em/packages/xmipp3/backupDeepLearning/scores.tab","w") as f:
##      best_accuracy= 0
##      thr=0
##      for score, label in zip(y_pred, pos_labels):
##        f.write("%f\t%d\n"%(float(score),float(label)))
##        tmpY=[ 1 if elem>=score else 0 for elem in y_pred]
##        tmp_accu=accuracy_score(pos_labels, tmpY)
##        if tmp_accu> best_accuracy:
##          best_accuracy= tmp_accu
##          thr= score
##    print("best thr %f --> accuracy %f"%(thr,best_accuracy))
    return y_pred , labels, metadataId_list

class DataManager(object):

  def __init__(self, posImagesXMDFname, posImagesSetOfParticles,  negImagesXMDFname=None, negImagesSetOfParticles=None):

    self.mdFalse=None
    self.nFalse=0 #Number of negative particles in dataManager
    
    self.mdTrue  = md.MetaData(posImagesXMDFname)
    self.fnListTrue =self.mdTrue.getColumnValues(md.MDL_IMAGE)
    

    xdim, ydim, _   = posImagesSetOfParticles.getDim()
    self.shape= (xdim,ydim,1)
    self.nTrue= posImagesSetOfParticles.getSize()
    
    self.batchSize= BATCH_SIZE

    self.splitPoint= self.batchSize//2

    self.batchStack = np.zeros((self.batchSize, xdim,ydim,1))
    if not ( negImagesXMDFname is None or negImagesSetOfParticles is None):
      self.mdFalse  = md.MetaData(negImagesXMDFname)
      self.fnListFalse  = self.mdFalse.getColumnValues(md.MDL_IMAGE)
      xdim_1, ydim_1, _ = negImagesSetOfParticles.getDim()
      self.nFalse= negImagesSetOfParticles.getSize()
      assert xdim==xdim_1 and ydim==ydim_1
      self.getRandomBatch= self.getRandomBatchWorker
    else:
      self.getRandomBatch= self.NOgetRandomBatchWorker
    
    if DEBUG:
      self.nTrue=  2**9
      self.nFalse= 2**9

  def getMetadata(self) :
    return  self.mdTrue, self.mdFalse

  def getNBatches(self, Nepochs):
    return  int(ceil(2*self.nTrue*Nepochs/self.batchSize))

  def getBatchSize(self):
    return self.batchSize

  def _random_flip_leftright(self, batch):
    for i in range(len(batch)):
      if bool(random.getrandbits(1)):
        batch[i] = np.fliplr(batch[i])
    return batch

  def _random_flip_updown(self, batch):
    for i in range(len(batch)):
      if bool(random.getrandbits(1)):
        batch[i] = np.flipud(batch[i])
    return batch

  def _random_90degrees_rotation(self, batch, rotations=[0, 1, 2, 3]):
    for i in range(len(batch)):
      num_rotations = random.choice(rotations)
      batch[i] = np.rot90(batch[i], num_rotations)
    return batch

  def _random_rotation(self, batch, max_angle):
    for i in range(len(batch)):
      if bool(random.getrandbits(1)):
        # Random angle
        angle = random.uniform(-max_angle, max_angle)
        batch[i] = scipy.ndimage.interpolation.rotate(batch[i], angle,reshape=False, mode="reflect")
    return batch

  def _random_blur(self, batch, sigma_max):
    for i in range(len(batch)):
      if bool(random.getrandbits(1)):
        # Random sigma
        sigma = random.uniform(0., sigma_max)
        batch[i] =scipy.ndimage.filters.gaussian_filter(batch[i], sigma)
    return batch
  
  def augmentBatch(self, batch):
    if bool(random.getrandbits(1)):
      batch= self._random_flip_leftright(batch)
      batch= self._random_flip_updown(batch)
    if bool(random.getrandbits(1)):
      batch= self._random_90degrees_rotation(batch)
    if bool(random.getrandbits(1)):
      batch= self._random_rotation(batch, 10.0)
    return batch

  def NOgetRandomBatchWorker(self):
    raise ValueError("needs positive and negative images to compute random batch. Just pos provided")
    
  def getRandomBatchWorker(self):

    batchSize = self.batchSize
    splitPoint = self.splitPoint
    batchStack   = self.batchStack
    batchLabels  = np.zeros((batchSize, 2))

    idxListTrue =  np.random.choice( self.nTrue,  splitPoint, False)
    idxListFalse = np.random.choice( self.nFalse, splitPoint, False)

    I = xmipp.Image()
    n = 0
    for idx in idxListTrue:
      fnImage = self.fnListTrue[idx]
      I.read(fnImage)
      batchStack[n,...]= np.expand_dims(I.getData(),-1)
      batchLabels[n, 1]= 1
      n+=1
      if n>=splitPoint:
          break
    for idx in idxListFalse:
      fnImage = self.fnListFalse[idx]
      I.read(fnImage)
      batchStack[n,...]= np.expand_dims(I.getData(),-1)
      batchLabels[n, 0]= 1
      n+=1
      if n>=batchSize:
          break

    shuffInd= np.random.choice(n,n, replace=False)
    batchStack= batchStack[shuffInd, ...]
    batchLabels= batchLabels[shuffInd, ...]

    return self.augmentBatch(batchStack), batchLabels

  def getDataAsNp(self):
    allData= self.getIteratorPredictBatch()
    x, labels, __ = zip(* allData)
    x= np.concatenate(x)
    y= np.concatenate(labels)
    print(x.shape, y.shape)
    print(np.min(x), np.mean(x), np.max(x))
#    joblib.dump(x, "/home/jsegura/Tesis/DataStack.pckl")
#    joblib.dump(y, "/home/jsegura/Tesis/labelsStack.pckl")
    return x,y

  def getIteratorPredictBatch(self):
    batchSize = self.batchSize
    xdim,ydim,nChann= self.shape
    batchStack = np.zeros((self.batchSize, xdim,ydim,nChann))
    batchLabels  = np.zeros((batchSize, 2))
    mdTrue  = self.mdTrue
    I = xmipp.Image()
    n = 0
    idAndType=[]
    for objId in mdTrue:
#      print("True id", id)
      fnImage = mdTrue.getValue(md.MDL_IMAGE, objId)
      I.read(fnImage)
      batchStack[n,...]= np.expand_dims(I.getData(),-1)
      batchLabels[n, 1]= 1
      idAndType.append((True,objId))
      n+=1
      if n>=batchSize:
        yield batchStack, batchLabels,  idAndType
        n=0
        batchLabels  = np.zeros((batchSize, 2))
        idAndType= []
    if not self.mdFalse is None:
      mdFalse= self.mdFalse
      for objId in mdFalse:
        fnImage = mdFalse.getValue(md.MDL_IMAGE, objId)
        I.read(fnImage)
        batchStack[n,...]= np.expand_dims(I.getData(),-1)
        batchLabels[n, 0]= 1
        idAndType.append((False,objId))
        n+=1
        if n>=batchSize:
          yield batchStack, batchLabels,  idAndType
          n=0
          idAndType= []
          batchLabels  = np.zeros((batchSize, 2))
    if n>0:
      yield batchStack[:n,...], batchLabels[:n,...], idAndType[:n]

  def getIteratorTestBatch(self, nBatches= 10):
    batchSize = self.batchSize
    xdim,ydim,nChann= self.shape
    batchStack = np.zeros((self.batchSize, xdim,ydim,nChann))
    batchLabels  = np.zeros((batchSize, 2))
    I = xmipp.Image()
    n = 0

    currNBatches=0
    for fnImageTrue, fnImageFalse in zip(self.fnListTrue, self.fnListFalse):
      I.read(fnImageTrue)
      batchStack[n,...]= np.expand_dims(I.getData(),-1)
      batchLabels[n, 1]= 1
      n+=1
      if n>=batchSize:
        yield batchStack, batchLabels
        n=0
        batchLabels  = np.zeros((batchSize, 2))
        currNBatches+=1
        if currNBatches>=nBatches:
          break
      I.read(fnImageFalse)
      batchStack[n,...]= np.expand_dims(I.getData(),-1)
      batchLabels[n, 0]= 1
      n+=1
      if n>=batchSize:
        yield batchStack, batchLabels
        n=0
        batchLabels  = np.zeros((batchSize, 2))
        currNBatches+=1
        if currNBatches>=nBatches:
          break
    if n>0:
      yield batchStack[:n,...], batchLabels[:n,...]

