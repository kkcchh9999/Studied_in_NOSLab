package org.tensorflow.lite.examples.transfer.Database;

import android.annotation.SuppressLint;
import android.content.Context;

import androidx.room.Room;

import org.tensorflow.lite.examples.transfer.TrainingSample;

import java.util.List;

public class SampleRepository {

	//-----싱글톤 유지 위함-----
	@SuppressLint("StaticFieldLeak")
	static SampleRepository INSTANCE = null;

	SampleRepository(Context context) {
		this.context = context;
	}
	Context context = null;

	public static void init(Context context) {
		if (INSTANCE == null) {
			INSTANCE = new SampleRepository(context);
		}
	}

	public static SampleRepository get() {
		if (INSTANCE == null) {
			throw new IllegalStateException("Repository must be Init");
		}
		return INSTANCE;
	}
	//-------------------------

	private final SampleDatabase database = Room.databaseBuilder(
		context.getApplicationContext(),
		SampleDatabase.class,
		"SampleDatabase"
	).build();

	private final SampleDAO sampleDAO = database.sampleDAO();

	public List<TrainingSample> getTrainingSamples() {
		return sampleDAO.getSamples();
	}

	public void deleteTrainingSamples() {
		sampleDAO.delete();
	}

	public void addTrainingSample(TrainingSample trainingSample) {
		sampleDAO.addSample(trainingSample);
	}

	public List<TrainingSample> getTrainingSample(int id) {
		return sampleDAO.selectSample(id);
	}
}
