package es.jmgoncalv.pseudo.inspector;

import java.io.Console;
import java.io.File;
import java.io.IOException;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.Row;

public class DatasetInspectorMain {
	
	public static final String USERS_INDEX_FILE = "users.index.txt";
	public static final String DATASET_DIR = "training_set";
	//public static final String MAPPING_FILE = "pseudo-mapping.txt";

	public static final String NEW_LINE = System.getProperty("line.separator");
	public static final String CMD_QUIT = "q";
	public static final String CMD_RATING = "r";
	public static final String CMD_CARDROW = "cr";
	public static final String CMD_CARDCOL = "cc";
	public static final String CMD_ATTRROW = "ar";
	public static final String CMD_ROWCARDGREATER = "rcg";
	
	/**
	 * @param args
	 * @throws IOException 
	 * @throws InterruptedException 
	 */
	public static void main(String[] args) throws IOException, InterruptedException {
		// default args
		String tDir = ".";
		
		// get args
		if (args.length>0) {
			tDir = args[0];
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
		
		Console console = System.console();
		String cmd = console.readLine("> ");
		String[] cmdArgs;
		
		while (!cmd.startsWith(CMD_QUIT)) {
			cmdArgs = cmd.split(" ");
			switch (cmdArgs[0]) {
				case CMD_RATING:
					 console.printf(rating(tds, cmdArgs)+NEW_LINE);
					break;
				case CMD_CARDROW:
					console.printf(cardinalityOfRow(tds, cmdArgs)+NEW_LINE);
					break;
				case CMD_CARDCOL:
					console.printf(cardinalityOfColumn(tds, cmdArgs)+NEW_LINE);
					break;
				case CMD_ATTRROW:
					console.printf(attributeIdsOfRow(tds, cmdArgs)+NEW_LINE);
					break;
				case CMD_ROWCARDGREATER:
					console.printf(rowsWithCardinalityGreaterThan(tds, cmdArgs)+NEW_LINE);
					break;
				default:
					console.printf(help()+NEW_LINE);
					break;
			}
			cmd = console.readLine("> ");
		}
		
		System.out.println("Quitting...");
	}

	private static String help() {
		return "help!";
	}

	private static String rowsWithCardinalityGreaterThan(Dataset tds,
			String[] cmdArgs) {
		int minCard = Integer.parseInt(cmdArgs[1]);
		StringBuilder response = new StringBuilder();
		int count = 0;
		for (int i=0; i<tds.rows().length; i++) {
			Row r = tds.rows()[i];
			if (r.getNumVotes()>=minCard) {
				response.append(i+" "+r.getUserId()+" "+r.getNumVotes()+NEW_LINE);
				count++;
			}
		}
		response.append("Total of "+count+" rows have cardinality "+minCard+" or higher!"+NEW_LINE);
		return response.toString();
	}

	private static String attributeIdsOfRow(Dataset tds, String[] cmdArgs) {
		return "not implemented!";
	}

	private static String cardinalityOfRow(Dataset tds, String[] cmdArgs) {
		return "not implemented!";
	}

	private static String cardinalityOfColumn(Dataset tds, String[] cmdArgs) {
		return "not implemented!";
	}

	private static String rating(Dataset tds, String[] cmdArgs) {
		return "not implemented!";
	}

}
