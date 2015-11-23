package es.jmgoncalv.pseudo.pseudonymizer;

import java.io.File;
import java.io.IOException;

import es.jmgoncalv.pseudo.clustering.RareMoviesClustering;
import es.jmgoncalv.pseudo.netflix.Dataset;

public class PseudoMain {
	
	public static final String USERS_INDEX_FILE = "users.index.txt";
	public static final String DATASET_DIR = "training_set";
	public static final String USERS_FEATURES_FILE = "mf.users.cache";
	public static final String MOVIES_FEATURES_FILE = "mf.movies.cache";
	public static final String PROBE_FILE = "probe.txt";
	public static final String MAPPING_FILE = "pseudo-mapping.txt";
	
	public static final int FIRST_PSEUDO_ID = 6; // OR ELSE KADRI GOES MAD!
	
	public static void main(String args[]) throws IOException, InterruptedException {
		double logp = 50;
		double linp = 40;

		// Try Read args
		try {
			if (args.length>0) {
				logp = Double.parseDouble(args[0]);
				if (args.length>1) {
					linp = Double.parseDouble(args[1]);
				}
			}
		} catch (NumberFormatException e) {
			System.out.println("Invalid parameters!");
		}
		System.out.println("Running Pseudonymizer with logp="+logp+" and linp="+linp+" ...");
		
		// Initialize dataset files
		File[] fileList = new File(DATASET_DIR).listFiles();
		File userIndexFile = new File(USERS_INDEX_FILE);
		
		// Read Dataset
		Dataset ds = new Dataset(fileList, userIndexFile);
		Thread.sleep(2000);
		
		// Read Factor Matrixes
		Factorization factorization = new Factorization(ds.getNumRows(), ds.getNumColumns(), USERS_FEATURES_FILE, MOVIES_FEATURES_FILE);
		Thread.sleep(2000);
		
		// For each row create a clusterized row and attribute pseudonyms
		ClusteringMethod cm = new RareMoviesClustering(factorization, 500, 5000, logp, linp);
		ClusterizedRow[] crs = new ClusterizedRow[ds.rows().length];
		Clustering c = new Clustering(cm, ds, crs, factorization, FIRST_PSEUDO_ID);
		Thread.sleep(2000);
		
		// output new dataset and probe
		OutputPseudoDataset opd = new OutputPseudoDataset(fileList, PROBE_FILE, ds, crs);
		Thread.sleep(2000);
		
		// output mapping file
		OutputPseudoMap opm = new OutputPseudoMap(new File(MAPPING_FILE), ds, crs);
		
	}

}
