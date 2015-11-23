package es.jmgoncalv.pseudo.analyzer;

import java.io.File;
import java.io.IOException;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.UserIdMap;

public class AnalyzerMain {
	
	public static final String USERS_INDEX_FILE = "users.index.txt";
	public static final String DATASET_DIR = "training_set";
	public static final String MAPPING_FILE = "pseudo-mapping.txt";
	
	public static final int LIMIT = 500;
	

	public static void main(String[] args) throws IOException, InterruptedException {
		// Initialize dataset files
		File[] fileList = new File(DATASET_DIR).listFiles();
		File userIndexFile = new File(USERS_INDEX_FILE);
		
		// Read Dataset
		Dataset ds = new Dataset(fileList, userIndexFile);
		Thread.sleep(2000);
		
		File mappingFile = new File(MAPPING_FILE);
		UserIdMap uim = new UserIdMap(mappingFile, ds); // assumes ds is the original 
		
		
		// Analyze
//		CardinalityAnalyzer ca = new CardinalityAnalyzer(ds,LIMIT);
//		ca.output("cadinality-analysis.txt");
//		FragmentationAnalyzer fa = new FragmentationAnalyzer(ds,uim,LIMIT);
//		fa.output("fragmentation-analysis.txt");
//		MovieCardinalityAnalyzer mca = new MovieCardinalityAnalyzer(ds,LIMIT);
//		mca.output("mc-analysis.txt");
		int nq = Integer.parseInt(args[0]);
		QuadrantAnalyzer qa = new QuadrantAnalyzer(ds,nq);
		qa.output("quadrant-analysis"+nq+".txt");
	}
}
