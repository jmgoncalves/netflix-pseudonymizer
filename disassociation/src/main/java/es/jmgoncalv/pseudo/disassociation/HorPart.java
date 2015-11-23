package es.jmgoncalv.pseudo.disassociation;

import java.util.Arrays;

import es.jmgoncalv.pseudo.clustering.RareMoviesClustering;
import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.ProgressMonitor;
import es.jmgoncalv.pseudo.netflix.Row;

public class HorPart extends ProgressMonitor {
	
	private int maxClusterSize, nextCluster;
	
	private long progress, total;
	private String msg;

	private int[][] partitions;
	
	//Algorithm: HORPART
	//Input : Dataset D, set of terms ignore (initially empty)
	//Output : A HORizontal PARTitioning of D
	//Param. : The maximum cluster size maxClusterSize
	//1 if |D| < maxClusterSize then return {{D}};
	//2 Let T be the set of terms of D;
	//3 Find the most frequent term a in T − ignore;
	//4 D1 = all records of D having term a;
	//5 D2 = D − D1;
	//6 return HORPART(D1, ignore U a) U HORPART(D2, ignore)
	public HorPart(Dataset ds, int maxClusterSize) {
		this.maxClusterSize = maxClusterSize;

		total = 1;
		progress = 0;
		msg = "Ordering movies by support...";
		
		int[][] orderedMovies = new int[ds.getNumColumns()][2];
		for (int i=0; i<orderedMovies.length;  i++) {
			orderedMovies[i][0] = i+1; // movieId
			orderedMovies[i][1] = ds.supp(i+1); // support
		}
		RareMoviesClustering.quicksort(orderedMovies, 0, orderedMovies.length-1); // sorts ascending based on value of [][1]
		VerPart.reverseArray(orderedMovies);
		
		msg = "Recursive horizontal partitioning...";
		int[] rowPartition = new int[ds.getNumRows()];
		nextCluster = 1;
		horPart(ds, orderedMovies, rowPartition, 0, 0);
		// linear version
//		for (int i=0; i<orderedMovies.length;  i++) {
//			for (int j=0; j<rowPartition.length; j++) {
//				if (rowPartition[j]==0) { // not attributed yet
//					Row r = ds.rows()[j];
//					int midIndex = r.findMovieId(0, orderedMovies[i][0]);
//					if (r.getMovieId(midIndex) == orderedMovies[i][0]) { // if has movie set
//						rowPartition[j] = orderedMovies[i][0]; // allocate to horizontal partition
//						allocatedRows++;
//					}
//				}
//			}
//			if (allocatedRows>orderedMovies[0][1]) // if unallocated are fewer than the first partition
//				continue;
//		}

		msg = "Building partition array...";
		
		partitions = new int[nextCluster][maxClusterSize];
		for (int i=0; i<partitions.length; i++)
			Arrays.fill(partitions[i], -1);
		
		int[] partitionsCount = new int[nextCluster];
		for (int j=0; j<rowPartition.length; j++) {
			int partition = rowPartition[j];
			partitions[partition][partitionsCount[partition]] = j;
			partitionsCount[partition]++;
		}
		
		msg = "Done! Created "+nextCluster+" horizontal partitions with max cluster size of "+maxClusterSize+"!";
		progress = 1;
	}
	
	//recursive version
	private void horPart(Dataset ds, int[][] orderedMovies, int[] rowPartition,
			int currentCluster, int currentMovieIndex) {
		int allocatedRows = 0;
		int notAllocatedRows = 0;
		for (int j=0; j<rowPartition.length; j++) {
			if (rowPartition[j]==currentCluster) {
				Row r = ds.rows()[j];
				int midIndex = r.findMovieId(0, orderedMovies[currentMovieIndex][0]);
				if (r.getMovieId(midIndex) == orderedMovies[currentMovieIndex][0]) { // if has movie set
					rowPartition[j] = nextCluster; // place in next cluster
					allocatedRows++;
				}
				else {
					notAllocatedRows++;
				}
			}
		}
		nextCluster++;
		if (allocatedRows>maxClusterSize)
			horPart(ds,orderedMovies,rowPartition,nextCluster-1,currentMovieIndex+1);
		if (notAllocatedRows>maxClusterSize)
			horPart(ds,orderedMovies,rowPartition,currentCluster,currentMovieIndex+1);
	}

	public int[][] getPartitions() { // indexes of rows that belong to each cluster
		return partitions;
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
