package es.jmgoncalv.pseudo.netflix;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

public class IndexFileReader {
	
	private File f;
	private FileInputStream fis;
	private int currentIndex;
	private long totalBytesRead;

	public IndexFileReader(String usersIndexFile) throws FileNotFoundException {
		f = new File(usersIndexFile);
		init();
	}
	
	public IndexFileReader(File usersIndexFile) throws FileNotFoundException {
		f = usersIndexFile;
		init();
	}

	private void init() throws FileNotFoundException {
		fis = new FileInputStream(f);
		currentIndex = -1;
		totalBytesRead = 0;
		
		//System.out.println("Opening '"+f.getName()+"', "+f.length()+" bytes to read...");
	}

	public int next() throws IOException {
		int value = -1;
		int c = fis.read();
		totalBytesRead++;
		char[] buffer = new char[20];
		int i = 0;
		while (c>-1) {
			if ((char)c == ' ') {
				// return userId
				value = ParseUtils.charArrayToInt(buffer,0,i);
				currentIndex++;
				i = 0;
				break;
			}
			else {
				// add to buffer
				buffer[i] = (char) c;
				i++;
			}
			c = fis.read();
			totalBytesRead++;
		}
		return value;
	}

	public int getIndex() {
		return currentIndex;
	}

	public void destroy() throws IOException {
		//System.out.println("Closing '"+f.getName()+"', "+totalBytesRead+" bytes were read...");
		fis.close();
		fis = null;
	}

	public void switchToVotes() {
		currentIndex = -1;
	}
}
