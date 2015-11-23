package es.jmgoncalv.pseudo.scoreboard;

import java.io.File;
import java.io.IOException;
import java.util.NoSuchElementException;
import java.util.Scanner;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.UserIdMap;

public class ScoreboardMain {

	public static final String USERS_INDEX_FILE = "users.index.txt";
	public static final String DATASET_DIR = "training_set";
	public static final String MAPPING_FILE = "pseudo-mapping.txt";
//	public static final String ORIGINAL_DIR = "pseudo";
//	public static final String PSEUDO_DIR = "verify";
//	public static final String OUTPUT_ORIGINAL = "scoreboard-output-original.txt";
//	public static final String OUTPUT_PSEUDO = "scoreboard-output-pseudo.txt";
	
	public static void main(String[] args) throws IOException, InterruptedException {
		// default args
		String tDir = ".";
		String aDir = "~"+File.separator+"pseudo";
		String out = "scoreboard-output.txt";
		double errorMargin = 0.02;
		
		// get args
		if (args.length>0) {
			tDir = args[0];
			if (args.length>1) {
				aDir = args[1];
				if (args.length>2) {
					try { errorMargin = Double.parseDouble(args[2]); }
					catch (NumberFormatException e) { System.out.println("Error: Could not parse error margin '"+args[2]+"'!"); }
					if (args.length>3)
						out = args[3];
				}
			}
		}
		
		// initialize target dataset
		File[] tFileList = new File(tDir+File.separator+DATASET_DIR).listFiles();
		File tUserIndexFile = new File(tDir+File.separator+USERS_INDEX_FILE);
		if (tFileList==null)
			System.out.println("Directory "+(new File(tDir+File.separator+DATASET_DIR)).getAbsolutePath()+" does not exist!");
		if (!tUserIndexFile.exists())
			System.out.println("File "+tUserIndexFile.getAbsolutePath()+" does not exist!");
		Dataset tds = new Dataset(tFileList, tUserIndexFile);
		Thread.sleep(2000);
		
		// initialize aux source dataset
		File[] aFileList = new File(aDir+File.separator+DATASET_DIR).listFiles();
		File aUserIndexFile = new File(aDir+File.separator+USERS_INDEX_FILE);
		if (aFileList==null)
			System.out.println("Directory "+(new File(aDir+File.separator+DATASET_DIR)).getAbsolutePath()+" does not exist!");
		if (!aUserIndexFile.exists())
			System.out.println("File "+aUserIndexFile.getAbsolutePath()+" does not exist!");
		Dataset ads = new Dataset(aFileList, aUserIndexFile);
		
		// load mapping between target and source ids
		File mappingFile = new File(tDir+File.separator+MAPPING_FILE);
		UserIdMap uim = new UserIdMap(mappingFile, ads); // assumes aux source is the original 
		
		// calculation
		ScoreboardSampling smc = new ScoreboardSampling(ads, tds, uim, errorMargin, tDir+File.separator+out);
		//smc.outputResults(tDir+File.separator+out);
	}

}
