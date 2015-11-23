package es.jmgoncalv.pseudo.netflix;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

public abstract class AbstractFeatureFileReader {
	
	protected File f;
	protected FileInputStream fis;
	protected int currentFeature;
	protected int currentIndex;
	protected long totalBytesRead;

	public AbstractFeatureFileReader(String usersFeaturesFile) throws IOException {
		f = new File(usersFeaturesFile);
		init();
	}
	
	public AbstractFeatureFileReader(File usersFeaturesFile) throws IOException {
		f = usersFeaturesFile;
		init();
	}

	private void init() throws IOException {
		fis = new FileInputStream(f);
		currentFeature = -1;
		currentIndex = -1;
		totalBytesRead = 0;
		
		//System.out.println("Opening '"+f.getName()+"', "+f.length()+" bytes to read...");
	}

	public abstract Double nextDouble() throws IOException;
	
	public abstract Integer nextInteger() throws IOException;

	public int getFeatureNumber() {
		return currentFeature;
	}

	public int getIndex() { 
		return currentIndex;
	}

	public void destroy() throws IOException {
		//System.out.println("Closing '"+f.getName()+"', "+totalBytesRead+" bytes were read...");
		
		fis.close();
		fis = null;
	}

}
