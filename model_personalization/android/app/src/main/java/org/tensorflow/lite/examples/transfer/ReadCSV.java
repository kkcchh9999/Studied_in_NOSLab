package org.tensorflow.lite.examples.transfer;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

public class ReadCSV {
	InputStream inputStream;

	public ReadCSV(InputStream inputStream) {
		this.inputStream = inputStream;
	}

	public List<String[]> read() {
		List<String[]> result = new ArrayList<>();
		BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));

		try {
			String csvLine;
			while((csvLine = reader.readLine()) != null) {
				String[] row = csvLine.split(",");
				result.add(row);
			}
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			try {
				inputStream.close();
			} catch (IOException e) {

			}
		}
		return result;
	}
}
