package com.example.testproject;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;

public class ModelLoader {

	private AssetManager assetManager;
	private String directoryName;

	/**
	 * Create a loader for a transfer learning model under given directory.
	 *
	 * @param directoryName path to model directory in assets tree.
	 */
	public ModelLoader(Context context, String directoryName) {
		this.directoryName = directoryName;
		this.assetManager = context.getAssets();
	}

	protected ByteBuffer loadMappedFile(String filePath) throws IOException {
		AssetFileDescriptor fileDescriptor = assetManager.openFd(this.directoryName + "/" + filePath);

		FileInputStream inputStream = new FileInputStream(fileDescriptor.getFileDescriptor());
		FileChannel fileChannel = inputStream.getChannel();
		long startOffset = fileDescriptor.getStartOffset();
		long declaredLength = fileDescriptor.getDeclaredLength();
		return fileChannel.map(FileChannel.MapMode.READ_ONLY, startOffset, declaredLength);
	}
}

