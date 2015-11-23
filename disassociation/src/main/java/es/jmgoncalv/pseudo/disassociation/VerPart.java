package es.jmgoncalv.pseudo.disassociation;

import java.util.Arrays;

import es.jmgoncalv.pseudo.clustering.RareMoviesClustering;
import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.ProgressMonitor;
import es.jmgoncalv.pseudo.netflix.Row;
import es.jmgoncalv.pseudo.pseudonymizer.ClusterizedRow;

public class VerPart extends ProgressMonitor {
	
	private int pseudoBase;
	
	private long progress, total;
	private String msg;

	//Algorithm: VERPART
	//Input : A cluster P, integers k and m
	//Output : A km-anonymous VERtical PARTitioning of P
	public VerPart(Dataset ds, int[] rowIndexes, ClusterizedRow[] crs, int m, int pseudoBaseStart) {
		msg = "Calculating local support...";
		total = rowIndexes.length+1;
		progress = 0;
		
		//1 Let TP be the set of terms of P;
		//2 for every term t in TP do
		//3 Compute the number of appearances s(t);
		int[] localSupport = new int[ds.getNumColumns()];
		for (int i=0; i<rowIndexes.length;  i++) {
			if (rowIndexes[i]<0)
				break;
			Row r = ds.rows()[rowIndexes[i]];
			for (int j=0; j<r.getNumVotes(); j++)
				localSupport[r.getMovieId(j)-1]++;
			progress++;
		}
		
		msg = "Ordering local support...";
		total = total+localSupport.length+localSupport.length;
		
		//4 Sort TP with decreasing s(t);
		//5 Move all terms with s(t) < k into TT; //TT is finalized
		//6 i = 0;
		//7 Tremain = TP − TT; //Tremain has the ordering of TP
		int itemCount = 0;
		for (int i=0; i<localSupport.length;  i++) {
			if (localSupport[i] >= DisassociationMain.K) {
				itemCount++;
			}
			progress++;
		}
		
		int[][] orderedMovies = new int[itemCount][3];
		itemCount = 0;
		for (int i=0; i<localSupport.length;  i++) {
			if (localSupport[i] >= DisassociationMain.K) {
				orderedMovies[itemCount][0] = i+1; // movieId
				orderedMovies[itemCount][1] = localSupport[i]; // local support
				orderedMovies[itemCount][2] = 0; // not processed; 1 to N assigned to that chunk
				itemCount++;
			}
			progress++;
		}

		if (orderedMovies.length>1) {
			RareMoviesClustering.quicksort(orderedMovies, 0, orderedMovies.length-1); // sorts ascending based on value of [][1]
			reverseArray(orderedMovies);
		}
		localSupport = null; // to become available for gc
		
		msg = "Creating chunks...";
		total = total+orderedMovies.length;
		
		//8 while Tremain ̸= empty do
		int doneCount = 0;
		int chunkCount = 1;
		while (doneCount < orderedMovies.length) {
			//9 Tcur = empty;
			int[] currentChunk = new int[orderedMovies.length];
			int currentChunkLength = 0;
			//10 for every term t in Tremain do
			for (int i=0; i<orderedMovies.length;  i++) {
				if (orderedMovies[i][2] == 0) {
					//11 Create a chunk Ctest by projecting to Tcur ∪ {t};
					//12 if Ctest is km-anonymous then Tcur = Tcur ∪ {t};
					if (currentChunkLength == 0 || isKmanonymous(ds, rowIndexes, orderedMovies, currentChunk, currentChunkLength, i, m)) {
						orderedMovies[i][2] = chunkCount;
						doneCount++;
						currentChunk[currentChunkLength] = i;
						currentChunkLength++;
					}
				}
			}
			
			//13 i++;
			chunkCount++;
			progress++;
			//14 Ti = Tcur;
			//15 Tremain = Tremain − Tcur;
		}
		
		//16 Create record chunks C1, . . . ,Cv by projecting to T1, . . . , Tv;
		//17 Create term chunk CT using TT ;
		
		msg = "Assigning chunks...";
		total = total+orderedMovies.length;
		
		// chunks saved and filtered using orderedMovies[i][2]: 0 remaining, positive indicates chunk number
		// make chunks addressable by movieId
		int[] assignedChunk = new int[ds.getNumColumns()];
		for (int i=0; i<orderedMovies.length;  i++) {
			assignedChunk[orderedMovies[i][0]-1] = orderedMovies[i][2];
			progress++;
		}
		
		msg = "Allocating chunks...";
		total = total+rowIndexes.length;
		
		// allocate chunks to row clusters
		pseudoBase = pseudoBaseStart;
		for (int i=0; i<rowIndexes.length;  i++) {
			if (rowIndexes[i]<0)
				break;
			Row r = ds.rows()[rowIndexes[i]];
			crs[rowIndexes[i]] = new ClusterizedRow(r.getNumVotes(), pseudoBase);
			ClusterizedRow cr = crs[rowIndexes[i]];
			int[] chunkToCluster = new int[chunkCount-1];
			Arrays.fill(chunkToCluster, -1);
			int clusterCount = 0;
			for (int j=0; j<r.getNumVotes(); j++) {
				int mid = r.getMovieId(j);
				if (assignedChunk[mid-1]==0) { // unclustered movie
					cr.addToCluster(j, clusterCount);
					clusterCount++;
				} else { // clustered movie
					if (chunkToCluster[assignedChunk[mid-1]-1]<0) { // first movie of a cluster (assign clusterId)
						chunkToCluster[assignedChunk[mid-1]-1] = clusterCount;
						clusterCount++;
					}
					cr.addToCluster(j, chunkToCluster[assignedChunk[mid-1]-1]);
				}
			}
			
			pseudoBase = pseudoBase+cr.getNumClusters();
			progress++;
		}
		
		msg = "Done!";
		progress = total;
		//18 return C1, . . . ,Cv,CT
	}
	
	// for any set of m or less items, there should be at least 2 rows, which contain this set
	private static boolean isKmanonymous(Dataset ds, int[] rowIndexes, int[][] orderedMovies, 
			int[] currentChunk, int currentChunkLength, int candidateIndex, int m) {
		
		// only rows that have the candidate movie need to be tested
		// gather ratings for chunk movie ids and relevant rows in testrows
		int[][] testRows = new int[rowIndexes.length][currentChunkLength+1];
		int testRowCount = 0;
		for (int i=0; i<rowIndexes.length; i++) {
			if (rowIndexes[i]<0)
				break;
			Row testRow = ds.rows()[rowIndexes[i]];
			int midIndex = testRow.findMovieId(0, orderedMovies[candidateIndex][0]);
			if (testRow.getMovieId(midIndex)==orderedMovies[candidateIndex][0]) {
				for (int j=0; j<currentChunkLength; j++) {
					int tempMid = orderedMovies[currentChunk[j]][0];
					int tempMidIndex = testRow.findMovieId(0, tempMid);
					if (testRow.getMovieId(tempMidIndex) == tempMid)
						testRows[testRowCount][j] = testRow.getRating(tempMidIndex);
				}
				testRows[testRowCount][currentChunkLength] = testRow.getRating(midIndex);
				testRowCount++;
			}
		}
		
		// for each row in testRow try to find an indistinguishable row
//		for (int i=0; i<testRowCount; i++) {
//			boolean foundIndistinguishable = false;
//			for (int j=0; j<testRowCount; j++) {
//				if (indistinguishableTo(testRows[i][0], testRows[j][0], ds, rowIndexes, orderedMovies, currentChunk, currentChunkLength, candidateIndex, m)) {
//					foundIndistinguishable = true;
//					continue;
//				}
//			}
//			if (!foundIndistinguishable) // if finished searching and no indistinguishable row found, not k^m-anonymous
//				return false;
//		}
//		return true;
		
		// for each combination of one element of the chunk and the candidate element (M=2)
		for (int i=0; i<currentChunkLength; i++) {
			int[] indistinguishable = new int[testRowCount];
			for (int j=0; j<testRowCount; j++) {
				if (testRows[j][i]>0) { // if a row has that item
					// find a subsequent row that also has it
					for (int k=j+1; k<testRowCount; k++) {
						if (testRows[k][i]>0 && 
								testRows[k][i] == testRows[j][i] && // if found and their ratings match
								testRows[k][currentChunkLength] == testRows[j][currentChunkLength]) { // both ratings
							indistinguishable[k]++; // then they are indistinguishable
							indistinguishable[j]++;
							break;						
						}
					}
					if (indistinguishable[j] == 0) // if no indistinguishable row has been found
						return false; // the candidate chunk is not 2^2 anonymous
				}
			}
		}
		return true;
	}
	
	public int getPseudoBaseEnd() {
		return pseudoBase;
	}
	
	// every m (or less) sized combination of items in i
	// present in the currentChunk and containing candidateIndex
	// see if their ratings match with j
//	private static boolean indistinguishableTo(int i, int j, Dataset ds,
//			int[] rowIndexes, int[][] orderedMovies, int[] currentChunk,
//			int currentChunkLength, int candidateIndex, int m) {
//		
//		return false;
//	}

	public static void reverseArray(int[][] array) {
		boolean third = false;
		if (array[0].length>2)
			third = true;
		
		for(int i = 0; i < array.length/2 ; i++) {
		    int temp = array[i][0];
		    array[i][0] = array[array.length - i - 1][0];
		    array[array.length - i - 1][0] = temp;
		    
		    temp = array[i][1];
		    array[i][1] = array[array.length - i - 1][1];
		    array[array.length - i - 1][1] = temp;
		    
		    if (third) {
			    temp = array[i][2];
			    array[i][2] = array[array.length - i - 1][2];
			    array[array.length - i - 1][2] = temp;
		    }
		}
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
