package com.example.testproject;

import org.tensorflow.lite.Interpreter;

import java.io.Closeable;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.util.HashMap;
import java.util.Map;

public class LiteMultipleSignatureModel implements Closeable {

	private static final int BOTTLENECK_SIZE = 7 * 7 * 1280;
	private static final int EXPECTED_BATCH_SIZE = 20;

	private final Interpreter interpreter;
	private final int numClasses;

	/**
	 * Constructor for the multiple signature model wrapper.
	 *
	 * @param tfLiteModel byte buffer of the saved flatbuffer model
	 * @param numClasses number of class labels
	 */
	LiteMultipleSignatureModel(ByteBuffer tfLiteModel, int numClasses) {
		this.interpreter = new Interpreter(tfLiteModel);
		this.numClasses = numClasses;
	}

	/**
	 * Loads the bottleneck feature from the given image array.
	 *
	 * @param image 3-D float array of size (IMG_SIZE, IMG_SIZE, 3)
	 * @return 1-D float array containing bottleneck features
	 */
	float[] loadBottleneck(float[][][] image) {
		Map<String, Object> inputs = new HashMap<>();
		inputs.put("feature", new float[][][][] {image});
		Map<String, Object> outputs = new HashMap<>();
		float[][] bottleneck = new float[1][BOTTLENECK_SIZE];
		outputs.put("bottleneck", bottleneck);
		this.interpreter.runSignature(inputs, outputs, "load");
		return bottleneck[0];
	}

	/**
	 * Runs one training step with the given bottleneck batches and labels.
	 *
	 * @param bottlenecks 2-D float array of bottleneck batches of size (BATCH_SIZE, BOTTLENECK_SIZE)
	 * @param labels 2-D float array of label batches of size (BATCH_SIZE, NUM_CLASSES)
	 * @return the training loss
	 */
	float runTraining(float[][] bottlenecks, float[][] labels) {
		Map<String, Object> inputs = new HashMap<>();
		inputs.put("bottleneck", bottlenecks);
		inputs.put("label", labels);

		Map<String, Object> outputs = new HashMap<>();
		FloatBuffer loss = FloatBuffer.allocate(1);
		outputs.put("loss", loss);

		this.interpreter.runSignature(inputs, outputs, "train");

		return loss.get(0);
	}

	/**
	 * Invokes inference on the given image batches.
	 *
	 * @param testImage 3-D float array of image of size (IMG_SIZE, IMG_SIZE, 3)
	 * @return 1-D float array of softmax output of prediction
	 */
	float[] runInference(float[][][] testImage) {
		// Run the inference.
		Map<String, Object> inputs = new HashMap<>();
		inputs.put("feature", new float[][][][] {testImage});

		Map<String, Object> outputs = new HashMap<>();
		float[][] output = new float[1][numClasses];
		outputs.put("output", output);
		this.interpreter.runSignature(inputs, outputs, "infer");
		return output[0];
	}

	int getExpectedBatchSize() {
		return EXPECTED_BATCH_SIZE;
	}

	int getNumBottleneckFeatures() {
		return this.interpreter.getInputTensorFromSignature("bottleneck", "train").shape()[1];
	}

	@Override
	public void close() {
		this.interpreter.close();
	}
}
