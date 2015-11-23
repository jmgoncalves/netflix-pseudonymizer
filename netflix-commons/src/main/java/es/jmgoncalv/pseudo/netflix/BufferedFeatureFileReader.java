package es.jmgoncalv.pseudo.netflix;

import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;

public class BufferedFeatureFileReader extends AbstractFeatureFileReader {
	
	public static final int BUFFER_SIZE = 1024;

	private InputStreamReader isr;
	
	private char[] c;
	private int i;
	private int start;
	private int readBytes;
	private int limit;

	public BufferedFeatureFileReader(String usersFeaturesFile) throws IOException {
		super(usersFeaturesFile);
		init();
	}
	
	public BufferedFeatureFileReader(File usersFeaturesFile) throws IOException {
		super(usersFeaturesFile);
		init();
	}
	
	private void init() throws IOException {
		isr = new InputStreamReader(fis);
		c = new char[BUFFER_SIZE];
		refreshByteArray(BUFFER_SIZE);
	}

	@Override
	public Double nextDouble() throws IOException {
		Double value = null;
		
		while (readBytes>-1) {
			if (c[i] == ' ' || c[i] == '\n') {
				if (c[i-1] == ':') {
					// new feature
					currentFeature = ParseUtils.charArrayToInt(c,start,i-1);
					currentIndex = -1;
				}
				else {
					// new value
					value = ParseUtils.charArrayToDouble(c,start,i);
					currentIndex++;
				}
				start = i+1;
			}
			
			i++;
			if (i==limit) {
				refreshByteArray(start);
			}
			
			if (value!=null)
				break;
		}
		return value;
	}
	
	// TODO rewrite this (repeated code!)
	@Override
	public Integer nextInteger() throws IOException {
		Integer value = null;
		
		while (readBytes>-1) {
			if (c[i] == ' ' || c[i] == '\n') {
				if (c[i-1] == ':') {
					// new feature
					currentFeature = ParseUtils.charArrayToInt(c,start,i-1);
					currentIndex = -1;
				}
				else {
					// new value
					value = ParseUtils.charArrayToInt(c,start,i);
					currentIndex++;
				}
				start = i+1;
			}
			
			i++;
			if (i==limit) {
				refreshByteArray(start);
			}
			
			if (value!=null)
				break;
		}
		return value;
	}

	private void refreshByteArray(int s) throws IOException {
		System.arraycopy(c, s, c, 0, c.length-s);
		i = c.length-s;
		readBytes = isr.read(c, c.length-s, s);
		totalBytesRead = totalBytesRead + readBytes;
		limit = c.length-s+readBytes;
		start = 0;
	}

	

}
