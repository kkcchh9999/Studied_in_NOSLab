import pandas as pd
import tensorflow as tf
import numpy as np
from tensorflow.python.framework.constant_op import constant

mnist = tf.keras.datasets.mnist

(x_train, y_train), (x_test, y_test) = mnist.load_data()
x_train = x_train / 255

data = x_train[0:100]

paddings = tf.constant([[0, 0,], [98, 98], [98, 98]])
padData = tf.pad(data, paddings, "CONSTANT")

label_100 = y_train[0:100]

pad_100 = np.array(padData)
print(pad_100.shape)
pad_100 = pad_100.ravel()
print(pad_100.shape)
pad_100 = pad_100.reshape(100, 50176)
print(pad_100.shape)
print(label_100.shape)
pd.DataFrame(pad_100).to_csv("/home/kch/Study/mnist.csv", index = False)
pd.DataFrame(label_100).to_csv("/home/kch/Study/label.csv", index = False)
