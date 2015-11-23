package es.jmgoncalv.pseudo.netflix;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintWriter;
import java.util.NoSuchElementException;
import java.util.Scanner;

public class TrainingSetFileReader {
	
	private Scanner scanner;
	private File f;
	private PrintWriter pw;
	private int movieId;
	private File outFile;
	private boolean transform;
	
	public TrainingSetFileReader(String fPath, boolean transform) throws FileNotFoundException {
		init(new File(fPath), transform);
	}

	public TrainingSetFileReader(File f, boolean transform) throws FileNotFoundException {
		init(f, transform);
	}

	private void init(File f, boolean transform) throws FileNotFoundException {
		this.transform = transform;
		this.f = f;
		scanner = new Scanner(f);
		if (transform) {
			outFile = new File(f.getAbsolutePath()+".new");
			pw = new PrintWriter(outFile);
		}
		
		if (f.getName().startsWith("mv_")) {
			// movieId verification and initialization... a waste?
			int dotIndex = f.getName().indexOf(".");
			int scoreIndex = f.getName().indexOf("_");
			movieId = Integer.parseInt(f.getName().substring(scoreIndex+1,dotIndex));
			String firstLine = scanner.nextLine();
			int flId = Integer.parseInt(firstLine.substring(0,firstLine.length()-1));
			
			if (movieId != flId) {
				System.out.println("filenameMovieId="+movieId);
				System.out.println("firstLineMovieId="+flId);
				throw new RuntimeException("Movie ID mismatch!");
			}
			
			if (transform)
				pw.println(firstLine);
		}
		
		//System.out.println("Processing "+f.getName()+"...");
	}

	public int getMovieId() {
		return movieId;
	}

	public RatingEntry next() {
		try {
			String line = scanner.nextLine();
			if (line!=null) {
				if (line.endsWith(":")) {
					movieId = Integer.parseInt(line.substring(0,line.length()-1));
					if (transform)
						pw.println(line);
					return next();
				}
				else
					return new RatingEntry(line,pw);
			}
		} catch (NoSuchElementException e) {
			// silent catch
		}
		return null;
	}

	public void finalize() {
		if (transform) {
			pw.flush();
			pw.close();
		}
		scanner.close();
		scanner = null;
		f = null;
//		f.delete();
//		outFile.renameTo(f);
//		System.out.println("Done processing "+f.getName()+"!");
	}

}
