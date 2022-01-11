import matplotlib.pyplot as plt
import numpy as np
from numpy.core.fromnumeric import shape
import tensorflow as tf
from tensorflow.python.ops.gen_math_ops import Mod
import os

print("TensorFlow version:", tf.__version__)

IMG_SIZE = 28

class Model(tf.Module):
    
    def __init__(self): 
        self.model = tf.keras.Sequential([
            tf.keras.layers.Flatten(input_shape=(IMG_SIZE, IMG_SIZE), name='flatten'),
            tf.keras.layers.Dense(128, activation='relu', name = 'dense_1'),
            tf.keras.layers.Dense(10, name = 'dense_2')
        ])
        
        self.model.compile(
            optimizer='sgd', 
            loss=tf.keras.losses.CategoricalCrossentropy(from_logits=True)
        )
        
    @tf.function(input_signature=[
        tf.TensorSpec([None, IMG_SIZE, IMG_SIZE], tf.float32),
        tf.TensorSpec([None, 10], tf.float32),
    ])
    def train(self, x, y):
        with tf.GradientTape() as tape:
            prediction = self.model(x)
            loss = self.model.loss(y, prediction)
        gradients = tape.gradient(loss, self.model.trainable_variables)
        self.model.optimizer.apply_gradients(
            zip(gradients, self.model.trainable_variables))
        result = {"loss": loss}
        return result

    @tf.function(input_signature=[
        tf.TensorSpec([None, IMG_SIZE, IMG_SIZE], tf.float32),
    ])
    def infer(self, x):
        logits = self.model(x)
        probabilities = tf.nn.softmax(logits, axis=-1)
        return {
            "output": probabilities,
            "logits": logits
        }
    
    @tf.function(input_signature=[tf.TensorSpec(shape = [], dtype=tf.string)])
    def save(self, checkpoint_path):
        tensor_names = [weight.name for weight in self.model.weights]
        tensor_to_save = [weight.read_value() for weight in self.model.weights]
        tf.raw_ops.Save(
            filename = checkpoint_path, tensor_names = tensor_names,
            data = tensor_to_save, name = 'save'
        )
        return {
            "checkpoint_path": checkpoint_path
        }
        
    @tf.function(input_signature=[tf.TensorSpec(shape=[], dtype=tf.string)])
    def restore(self, checkpoint_path):
        restored_tensors = {}
        for var in self.model.weights: 
            restored = tf.raw_ops.Restore(
                file_pattern=checkpoint_path, tensor_name=var.name, dt=var.dtype,
                name='restore'
            )
            var.assign(restored)
            restored_tensors[var.name] = restored
        return restored_tensors
    
# Prepaer data        
fashion_mnist = tf.keras.datasets.fashion_mnist
(train_images, train_labels), (test_images, test_labels) = fashion_mnist.load_data()       

# Preprocess data
train_images = (train_images / 255.0).astype(np.float32)
test_images = (test_images / 255.0).astype(np.float32)
train_labels = tf.keras.utils.to_categorical(train_labels)
test_labels = tf.keras.utils.to_categorical(test_labels)

NUM_EPOCHS = 100
BATCH_SIZE = 100
epochs = np.arange(1, NUM_EPOCHS + 1, 1)
losses = np.zeros([NUM_EPOCHS])
m = Model()

train_ds = tf.data.Dataset.from_tensor_slices((train_images, train_labels))
train_ds = train_ds.batch(BATCH_SIZE)

for i in range(NUM_EPOCHS): 
    for x, y in train_ds:
        result = m.train(x, y)
    losses[i] = result['loss']
    if (i + 1) % 10 == 0:
        print(f"Finished {i+1} epochs")
        print(f"    loss: {losses[i]:.3f}")

m.save('/home/kch/Study/model.ckpt')

SAVED_MODEL_DIR = "saved_model"

tf.saved_model.save(
    m, 
    SAVED_MODEL_DIR,
    signatures={
        'train':
            m.train.get_concrete_function(),
        'infer':
            m.infer.get_concrete_function(),
        'save':
            m.save.get_concrete_function(),
        'restore':
            m.restore.get_concrete_function(),
    }
)

converter = tf.lite.TFLiteConverter.from_saved_model(SAVED_MODEL_DIR)
converter.target_spec.supported_ops = [
    tf.lite.OpsSet.TFLITE_BUILTINS,
    tf.lite.OpsSet.SELECT_TF_OPS
]
converter.experimental_enable_resource_variables = True
tflite_model = converter.convert()

model_file_path = os.path.join('model.tflite')
with open(model_file_path, 'wb') as model_file:
    model_file.write(tflite_model)
