package org.tensorflow.lite.examples.transfer;

import androidx.room.Entity;
import androidx.room.PrimaryKey;

@Entity
public class TrainingSample {
	float[] bottleneck;
	float[] label;
	@PrimaryKey(autoGenerate = true)int ID;

	TrainingSample(float[] bottleneck, float[] label) {
		this.bottleneck = bottleneck;
		this.label = label;
	}
}