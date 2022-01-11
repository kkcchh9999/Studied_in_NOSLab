package com.example.testproject;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;

import org.tensorflow.lite.Interpreter;

import java.io.IOException;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		ModelLoader modelLoader = new ModelLoader(getApplicationContext(), "model");

		try (Interpreter interpreter = new Interpreter(modelLoader.loadMappedFile("model.tflite"))) {
			int NUM_EPOCHS = 100;
			int BATCH_SIZE = 100;
			int IMG_HEIGHT = 28;
			int IMG_WIDTH = 28;
			int NUM_TRAININGS = 60000;
			int NUM_BATCHES = NUM_TRAININGS / BATCH_SIZE;

			List<FloatBuffer> trainImageBatches = new ArrayList<>(NUM_BATCHES);
			List<FloatBuffer> trainLabelBatches = new ArrayList<>(NUM_BATCHES);

			// Prepare training batches.
			for (int i = 0; i < NUM_BATCHES; ++i) {
				FloatBuffer trainImages = FloatBuffer.allocateDirect(BATCH_SIZE * IMG_HEIGHT * IMG_WIDTH).order(ByteOrder.nativeOrder());
				FloatBuffer trainLabels = FloatBuffer.allocateDirect(BATCH_SIZE * 10).order(ByteOrder.nativeOrder());

				// Fill the data values...
				trainImageBatches.add(trainImages.rewind());
				trainImageLabels.add(trainLabels.rewind());
			}

			// Run training for a few steps.
			float[] losses = new float[NUM_EPOCHS];
			for (int epoch = 0; epoch < NUM_EPOCHS; ++epoch) {
				for (int batchIdx = 0; batchIdx < NUM_BATCHES; ++batchIdx) {
					Map<String, Object> inputs = new HashMap<>();
					inputs.put("x", trainImageBatches.get(batchIdx));
					inputs.put("y", trainLabelBatches.get(batchIdx));

					Map<String, Object> outputs = new HashMap<>();
					FloatBuffer loss = FloatBuffer.allocate(1);
					outputs.put("loss", loss);

					interpreter.runSignature(inputs, outputs, "train");

					// Record the last loss.
					if (batchIdx == NUM_BATCHES - 1) losses[epoch] = loss.get(0);
				}

				// Print the loss output for every 10 epochs.
				if ((epoch + 1) % 10 == 0) {
					System.out.println(
							"Finished " + (epoch + 1) + " epochs, current loss: " + loss.get(0));
				}
			}

			// ...
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}