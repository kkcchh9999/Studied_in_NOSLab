
import time
import datetime
import tensorflow as tf
from tensorflow.keras.layers import Dense, Flatten, Conv2D, ConvLSTM2D
from tensorflow.keras import Model
#from tensorflow.python.ops.gen_dataset_ops import interleave_dataset
import os
os.environ["CUDA_DEVICE_ORDER"] = "PCI_BUS_ID"
os.environ['CUDA_VISIBLE_DEVICES'] = '-1'
import time
import math
import numpy as np
import pandas as pd
from tensorflow.python.client import device_lib
from tensorflow.python.eager.context import graph_mode
from tensorflow.python.ops.gen_math_ops import mul
device_lib.list_local_devices()
import threading
from concurrent.futures import ThreadPoolExecutor
import concurrent.futures
import glob
import random

import copy
#from tensorflow.python.framework.ops import disable_eager_execution
import cProfile
import re
#disable_eager_execution()

# df_train = pd.read_csv("/home/js/Mnist/integrated_train.csv") #mnist 읽기
# df_test = pd.read_csv("/home/js/Mnist/integrated_test.csv")   #mnist 읽기

df_train = tf.keras.datasets.mnist
(x_train, y_train), (x_test, y_test) = df_train.load_data()
x_train, x_test = x_train / 255.0, x_test / 255.0

time0 = time.time()
# y_train = df_train['label']  #라벨
# x_train = df_train.drop(['label'],axis=1) #라벨 제외 데이터값
# x_train = x_train.to_numpy()  #numpy 배열
# y_train = y_train.to_numpy()  #numpy 배열
# y_test = df_test['label']     #테스트 라벨
# x_test = df_test.drop(['label'], axis=1)  #라벨 제외 데이터값
# x_test = x_test.to_numpy()    #numpy 배열
# y_test = y_test.to_numpy()    #numpy 배열
print(x_train.shape)
print(y_train.shape)
print(x_test.shape)
print(y_test.shape)
x_train, x_test = x_train / 255.0, x_test / 255.0 #정규화
x_train = x_train[..., tf.newaxis]    #데이터 차원 변경
x_test = x_test[..., tf.newaxis]      #데이터 차원 변경

print(x_train.shape)
print(x_test.shape)

random.seed(1000) #난수 발생?

'''tf.data'''
time1 = time.time()
#pr = cProfile.Profile()
#pr.enable()
#train_ds = tf.data.Dataset.from_tensor_slices((batching_img, batching_lab)).batch(32)#.shuffle(10000)
#test_ds = tf.data.Dataset.from_tensor_slices((batching_testimg,batching_testlab)).batch(32)
print("shuffling time",time.time() - time1)
'''tf.data'''
#cProfile.run('re.compile("foo|bar")')

class MyModel(Model): #모델 클래스
  def __init__(self): 
      super(MyModel, self).__init__()
      self.conv1 = Conv2D(32, 3, activation='relu') #합성곱 연산망, 필터의 수, 커널사이즈, 활성화함수
      self.flatten = Flatten()  #다차원 배열 평평하게
      self.d1 = Dense(128, activation='relu')   #dense 레이어 = 완전연결  relu함수를 통해 
      self.d2 = Dense(10, activation='softmax') #dense 레이어 softmax를 통해 분류 - 0 ~ 9 
  def call(self, x):
      x = self.conv1(x)
      x = self.flatten(x)
      x = self.d1(x)
      return self.d2(x) #결과값 리턴 
  @tf.function(input_signature=[
        tf.TensorSpec([None, 28, 28], tf.float32),
    ])
  def train_step(images, labels):
    with tf.GradientTape() as tape:
      predictions = model(images)
      loss = loss_object(labels, predictions)
    gradients = tape.gradient(loss, model.trainable_variables)
    optimizer.apply_gradients(zip(gradients, model.trainable_variables))
    train_loss(loss)
    train_accuracy(labels, predictions)
  @tf.function(input_signature=[
    tf.TensorSpec([None, 28, 28], tf.float32),
    ])
  def test_step(images, labels):
    predictions = self.images
    t_loss = loss_object(labels, predictions)
    test_loss(t_loss)
    test_accuracy(labels, predictions)
    
model = MyModel() #모델 생성
loss_object = tf.keras.losses.SparseCategoricalCrossentropy() #손실함수?
optimizer = tf.keras.optimizers.Adam()  #옵티마이저
train_loss = tf.keras.metrics.Mean(name='train_loss')
train_accuracy = tf.keras.metrics.SparseCategoricalAccuracy(name='train_accuracy')  
test_loss = tf.keras.metrics.Mean(name='test_loss') 
test_accuracy = tf.keras.metrics.SparseCategoricalAccuracy(name='test_accuracy')
'''
current_time = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
train_log_dir = 'logs/gradient_tape/' + current_time + '/train'
test_log_dir = 'logs/gradient_tape/' + current_time + '/test'
tf.summary.trace_on(graph=False, profiler=True)
train_summary_writer = tf.summary.create_file_writer(train_log_dir)
test_summary_writer = tf.summary.create_file_writer(test_log_dir)
'''

  
EPOCHS = 10
for epoch in range(EPOCHS):
  s = np.arange(x_train.shape[0])
  np.random.shuffle(s)# 6만라인 범위 랜덤셔플링 0.35
  train_dx = x_train[s]
  train_dy = y_train[s]
  batching_img = x_train.reshape(-1,25,28,28,1)
  batching_lab = y_train.reshape(-1,25)
  batching_testimg = x_test.reshape(-1,25,28,28,1)
  batching_testlab = y_test.reshape(-1,25)
  for images, labels in zip(batching_img, batching_lab):
    model.train_step(images, labels)
#    '''tf.data 사용 시 주석해제 '''
#  for images, labels in train_ds:
#        train_step(images, labels)
  for test_images, test_labels in zip(batching_testimg,batching_testlab):
    model.test_step(test_images, test_labels)
#   '''tf.data 사용 시 주석해제 '''
#  for test_images, test_labels in test_ds:
#        test_step(test_images, test_labels)
  template = '에포크: {}, 손실: {}, 정확도: {}, 테스트 손실: {}, 테스트 정확도: {}'
  print (template.format(epoch+1,
                      train_loss.result(),
                      train_accuracy.result()*100,
                      test_loss.result(),
                      test_accuracy.result()*100))
#pr.disable()
#pr.dump_stats('logs.txt')
#with train_summary_writer.as_default(): #텐서 보드 (그래프 추가)
#  tf.summary.trace_export(
#      name='graph',
#      step=0,
#      profiler_outdir='logs/gradient_tape/' + current_time + '/train')
print("Execution time")
print(time.time() - time0)