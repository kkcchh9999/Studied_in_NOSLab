package org.tensorflow.lite.examples.transfer;

import android.app.Application;

import org.tensorflow.lite.examples.transfer.Database.SampleRepository;

public class SubApplication extends Application {
	@Override
	public void onCreate() {
		super.onCreate();
		SampleRepository.init(this);
	}
}
