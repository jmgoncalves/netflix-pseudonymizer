package es.jmgoncalv.pseudo.disassociation;

import java.io.File;
import java.io.IOException;

import es.jmgoncalv.pseudo.clustering.RareMoviesClustering;
import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.Row;
import es.jmgoncalv.pseudo.pseudonymizer.Clustering;
import es.jmgoncalv.pseudo.pseudonymizer.ClusteringMethod;
import es.jmgoncalv.pseudo.pseudonymizer.ClusterizedRow;
import es.jmgoncalv.pseudo.pseudonymizer.Factorization;
import es.jmgoncalv.pseudo.pseudonymizer.OutputPseudoDataset;
import es.jmgoncalv.pseudo.pseudonymizer.OutputPseudoMap;

public class DisassociationMain {

	public static final String USERS_INDEX_FILE = "users.index.txt";
	public static final String DATASET_DIR = "training_set";
	public static final String PROBE_FILE = "probe.txt";
	public static final String MAPPING_FILE = "pseudo-mapping.txt";
	
	public static final int FIRST_PSEUDO_ID = 6; // OR ELSE KADRI GOES MAD!
	public static final int K = 2; // FIXED! otherwise have to implement refine... and is the minimum anonymity
	public static final int M = 2; // temporarily fixed... minimum anonymity
	
	public static void main(String args[]) throws IOException, InterruptedException {
		int maxClusterSize = 1000;

		// Try Read args
		try {
			if (args.length>0) {
				maxClusterSize = Integer.parseInt(args[0]);
			}
		} catch (NumberFormatException e) {
			System.out.println("Invalid parameters!");
		}
		System.out.println("Running Pseudonymizer with k="+K+", m="+M+" and maxClusterSize="+maxClusterSize+" ...");
		
		// Initialize dataset files
		File[] fileList = new File(DATASET_DIR).listFiles();
		File userIndexFile = new File(USERS_INDEX_FILE);
		
		// Read Dataset
		Dataset ds = new Dataset(fileList, userIndexFile);
		Thread.sleep(2000);
		
		// Horizontal Partitioning
		HorPart hp = new HorPart(ds, maxClusterSize);
		Thread.sleep(2000);
		
		// Vertical Partitioning
		VerPart[] vp = new VerPart[hp.getPartitions().length];
		ClusterizedRow[] crs = new ClusterizedRow[ds.rows().length];
		int pseudoBase = FIRST_PSEUDO_ID;
		for (int i = 0; i<vp.length; i++) {
			System.out.println("Starting VerPart on cluster "+(i+1)+" of "+vp.length+"...");
			vp[i] = new VerPart(ds, hp.getPartitions()[i], crs, M, pseudoBase);
			pseudoBase = vp[i].getPseudoBaseEnd();
			Thread.sleep(2000);
		}
		// Refine isn't needed with K=2!!!
		
		// output new dataset and probe
		OutputPseudoDataset opd = new OutputPseudoDataset(fileList, PROBE_FILE, ds, crs);
		Thread.sleep(2000);
		
		// output mapping file
		OutputPseudoMap opm = new OutputPseudoMap(new File(MAPPING_FILE), ds, crs);
		
	}
}
