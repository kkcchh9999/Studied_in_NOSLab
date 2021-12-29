import pandas as pd 
import tensorflow as tf
from tensorflow.python.framework.constant_op import constant 

mnist = pd.read_csv("/home/kch/Study/TFLClassify/mnist.csv", index_col=0)
padding = tf.constant([2,2],[2,2])


print(tf.pad(mnist, padding, constant))