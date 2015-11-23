package es.jmgoncalv.pseudo.pseudonymizer;

import java.io.IOException;

import es.jmgoncalv.pseudo.netflix.BufferedFeatureFileReader;
import es.jmgoncalv.pseudo.netflix.ProgressMonitor;

public class Factorization extends ProgressMonitor {
	
	public static final int FEATURES = 25;
//	public static final int NUM_USERS = 480189;
//	public static final int NUM_MOVIES = 17770;
	
	private double[][] userFeatures;
	private double[][] movieFeatures;
	
	private long progress, total;
	private String msg;

	public Factorization(int numUsers, int numMovies, String ufFile, String mfFile) {
		total = numUsers*FEATURES+numMovies*FEATURES;
		msg = "Initializing...";
		
		userFeatures = new double[numUsers][FEATURES]; // 480189 * 25 * 8bytes = 96037800bytes = 93787KB = 92MB
		movieFeatures = new double[numMovies][FEATURES]; // 17770 * 25 * 8bytes = 3554000bytes = 3471KB
	
		try {
			// attribute pseudonyms or drop entry for each feature
			msg = "Reading user features @ "+ufFile+"...";
			BufferedFeatureFileReader userFfr = new BufferedFeatureFileReader(ufFile);
			Double du = userFfr.nextDouble();
			while (du != null) {
				progress++;
				userFeatures[userFfr.getIndex()][userFfr.getFeatureNumber()] = du;
				du = userFfr.nextDouble();
			}
			userFfr.destroy();
			userFfr = null;
			
			// attribute a primary feature to each movie
			msg = "Reading movie features @ "+mfFile+"...";
			BufferedFeatureFileReader movieFfr = new BufferedFeatureFileReader(mfFile);
			Double dm = movieFfr.nextDouble();
			while (dm != null) {
				progress++;
				movieFeatures[movieFfr.getIndex()][movieFfr.getFeatureNumber()] = dm;
				dm = movieFfr.nextDouble();
			}
			movieFfr.destroy();
			movieFfr = null;
		} catch (IOException e) {
			userFeatures = null;
			movieFeatures = null;
			msg = e.getMessage();
			total = progress;
		}
		msg = "Done!";
	}
	
	public double[][] getUserFeatures() {
		return userFeatures;
	}

	public double[][] getMovieFeatures() {
		return movieFeatures;
	}

	@Override
	public long getProgress() {
		return progress;
	}

	@Override
	public long getTotal() {
		return total;
	}

	@Override
	public String getMessage() {
		return msg;
	}

}
