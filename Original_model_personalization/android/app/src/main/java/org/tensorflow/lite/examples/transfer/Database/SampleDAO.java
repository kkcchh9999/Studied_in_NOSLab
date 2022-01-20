package org.tensorflow.lite.examples.transfer.Database;

import androidx.room.Delete;
import androidx.room.Insert;
import androidx.room.Query;

import org.tensorflow.lite.examples.transfer.TrainingSample;

import java.util.List;

public interface SampleDAO {
	@Query("Select * FROM TrainingSample")
	List<TrainingSample> getSamples();

	@Query("Select * FROM TrainingSample WHERE ID IN (:ID)")
	List<TrainingSample> selectSample(int ID);

	@Insert
	void addSample(TrainingSample trainingSample);

	@Query("DELETE FROM TrainingSample")
	void delete();
}