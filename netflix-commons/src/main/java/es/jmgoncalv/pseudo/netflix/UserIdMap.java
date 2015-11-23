package es.jmgoncalv.pseudo.netflix;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;


public class UserIdMap {

	// 0: original user id
	// 1: pseudo range start (inclusive)
	// 2: pseudo range finish (inclusive)
	private int[][] userIdRanges;
	private boolean original;

	public UserIdMap(File mappingFile, Dataset ds) throws IOException {
		if (mappingFile.exists()) {
			userIdRanges = new int[ds.getNumRows()][3];
			
			BufferedReader br = new BufferedReader(new FileReader(mappingFile));
			String line = br.readLine();
			for (int i=0; line!=null && i<ds.getNumRows(); i++) {
				 int spaceIndex = line.indexOf(" ");
				 int colIndex = line.indexOf(":");
				 
				 userIdRanges[i][0] = Integer.parseInt(line.substring(0,spaceIndex));
				 userIdRanges[i][1] = Integer.parseInt(line.substring(spaceIndex+1,colIndex));
				 userIdRanges[i][2] = Integer.parseInt(line.substring(colIndex+1));
				 line = br.readLine();
			}
			br.close();
			
			// checks
			if (userIdRanges[0][0]!=ds.rows()[0].getUserId() || userIdRanges[ds.getNumRows()-1][0]!=ds.rows()[ds.getNumRows()-1].getUserId())
				throw new RuntimeException("Unexpected user id values in UserIdMap: userIdRanges[0][0]="+userIdRanges[0][0]+" userIdRanges["+(ds.getNumRows()-1)+"][0]="+userIdRanges[ds.getNumRows()-1][0]);
			
			original = false;
		}
		else {
			original = true;
			System.out.println("No mapping file found @ "+mappingFile.getAbsolutePath()+" Assuming direct mapping...");
		}
			
	}


	public boolean match(int userIndex, int pseudo, int originalUserId) {
		if (original) {
			if (pseudo==originalUserId)
				return true;
			return false;
		} else {
			// verification
			if (userIdRanges[userIndex][0]!=originalUserId)
				throw new RuntimeException("Mapping and aux user index-id missmatch: userIndex="+userIndex+" mappingId="+userIdRanges[userIndex][0]+" auxId="+originalUserId);
			
			if (pseudo>=userIdRanges[userIndex][1] && pseudo<=userIdRanges[userIndex][2])
				return true;
			return false;
		}
	}
	
	public int[][] getUserIdRanges() {
		return userIdRanges;
	}
}
