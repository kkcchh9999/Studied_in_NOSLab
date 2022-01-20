package org.tensorflow.lite.examples.transfer.Database;

import androidx.room.Database;
import androidx.room.RoomDatabase;

import org.tensorflow.lite.examples.transfer.TrainingSample;

@Database(entities = TrainingSample.class, version = 1)
public abstract class SampleDatabase extends RoomDatabase {
	public abstract SampleDAO sampleDAO();
}
