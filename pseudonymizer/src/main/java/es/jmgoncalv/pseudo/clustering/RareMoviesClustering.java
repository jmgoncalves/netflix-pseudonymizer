package es.jmgoncalv.pseudo.clustering;

import es.jmgoncalv.pseudo.pseudonymizer.ClusteringMethod;
import es.jmgoncalv.pseudo.pseudonymizer.ClusterizedRow;
import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.EuclideanDistanceSimilarity;
import es.jmgoncalv.pseudo.pseudonymizer.Factorization;
import es.jmgoncalv.pseudo.netflix.Row;

public class RareMoviesClustering implements ClusteringMethod {

	private static final double ABSOLUTE_NUM_VOTES_THRESHOLD = 5000;
	private static final double NUM_VOTES_THRESHOLD = 1000;
	private static final double TARGET_NUM_CLUSTER_LOG_UNIT = 50;
	private static final double TARGET_NUM_CLUSTER_MULTIPLIER = 20;
	
	private double numVotesThreshold;
	private double absoluteNumVotesThreshold;
	private double targetNumClusterLogUnit;
	private double targetNumClusterMultiplier;
	
	public RareMoviesClustering(Factorization f) {
		this(f, NUM_VOTES_THRESHOLD, ABSOLUTE_NUM_VOTES_THRESHOLD, TARGET_NUM_CLUSTER_LOG_UNIT, TARGET_NUM_CLUSTER_MULTIPLIER);
	}
	
	public RareMoviesClustering(Factorization f, double numVotesThreshold) {
		this(f, numVotesThreshold, ABSOLUTE_NUM_VOTES_THRESHOLD, TARGET_NUM_CLUSTER_LOG_UNIT, TARGET_NUM_CLUSTER_MULTIPLIER);
	}
	
	public RareMoviesClustering(Factorization f, double numVotesThreshold, double absoluteNumVotesThreshold) {
		this(f, numVotesThreshold, absoluteNumVotesThreshold, TARGET_NUM_CLUSTER_LOG_UNIT, TARGET_NUM_CLUSTER_MULTIPLIER);
	}
	
	public RareMoviesClustering(Factorization f, double numVotesThreshold, double absoluteNumVotesThreshold, double targetNumClusterLogUnit, double targetNumClusterMultiplier) {
		this.numVotesThreshold = numVotesThreshold;
		this.absoluteNumVotesThreshold = absoluteNumVotesThreshold;
		this.targetNumClusterLogUnit = targetNumClusterLogUnit;
		this.targetNumClusterMultiplier = targetNumClusterMultiplier;
	}
	
	@Override
	public void clusterRow(Dataset ds, Row r, ClusterizedRow cr, Factorization f) {
		double[][] clusterCenters = clusterByMovieRarity(ds,r,f);
		
		// shortcut for 1 cluster
		if (clusterCenters.length==1) {
			for (int i=0; i<r.getNumVotes(); i++)
				cr.addToCluster(i, 0);
			return;
		}
		
		// build clusters
		int[] clusterIndexToCrIndex = new int[clusterCenters.length];
		int attributedClusters = 0;
		for (int i=0; i<r.getNumVotes(); i++) {
			boolean added = false;
			int clusterIndex = minimumDistance(clusterCenters,f.getMovieFeatures()[r.getMovieId(i)-1]);
			for (int j=0; j<attributedClusters; j++ ) {
				if (clusterIndexToCrIndex[j]==clusterIndex) {
					cr.addToCluster(i, j);
					added = true;
					break;
				}
			}
			if (!added) {
				clusterIndexToCrIndex[attributedClusters] = clusterIndex;
				cr.addToCluster(i, attributedClusters);
				attributedClusters++;
			}
		}
	}
	
	public double[][] clusterByMovieRarity(Dataset ds, Row r, Factorization f) {
		// calculate target number of clusters (iteration at which the movie support is directly compared with the numVotesThreshold)
		double targetNumCluster = Math.log(1+(r.getNumVotes()/targetNumClusterLogUnit))*targetNumClusterMultiplier;
		
		// order movies in row by support
		// 0: movieId
		// 1: support of movie
		// initialize movie array
		int[][] orderedMovies = new int[r.getNumVotes()][2];
		int numRiskMovies = 0;
		for (int i=0; i<r.getNumVotes(); i++) {
			int mid = r.getMovieId(i);
			if (ds.supp(mid)<absoluteNumVotesThreshold) {
				orderedMovies[numRiskMovies][0] = mid;
				orderedMovies[numRiskMovies][1] = ds.supp(orderedMovies[numRiskMovies][0]);
				numRiskMovies++;
			}
		}
		
		// shortcut for no risky movies
		if (numRiskMovies==0) {
			return new double[1][Factorization.FEATURES];
		}
		
		// sort movies by support
		quicksort(orderedMovies, 0, numRiskMovies-1);
		 
		// cap number of clusters to the max allowed by ClusterizedRow
		numRiskMovies = Math.min(numRiskMovies, ClusterizedRow.MAX_CLUSTER_ID+1);
		
		// iterate from rarest to most common
		int clusterCount = 1;
		for (int i=1; i<numRiskMovies; i++) {
			if (orderedMovies[i][1]*(i/targetNumCluster)>numVotesThreshold)
				break;
			clusterCount++;
		}
		
		// build return array
		double[][] returnArray = new double[clusterCount][Factorization.FEATURES];
		for (int i=0; i<clusterCount; i++) {
			System.arraycopy(f.getMovieFeatures()[orderedMovies[i][0]-1], 0, returnArray[i], 0, Factorization.FEATURES);
		}
		
		return returnArray;
	}

	public static void quicksort(int[][] numbers, int low, int high) {
	    int i = low, j = high;
	    // Get the pivot element from the middle of the list
	    int pivot = numbers[low + (high-low)/2][1];

	    // Divide into two lists
	    while (i <= j) {
	      // If the current value from the left list is smaller then the pivot
	      // element then get the next element from the left list
	      while (numbers[i][1] < pivot) {
	        i++;
	      }
	      // If the current value from the right list is larger then the pivot
	      // element then get the next element from the right list
	      while (numbers[j][1] > pivot) {
	        j--;
	      }

	      // If we have found a values in the left list which is larger then
	      // the pivot element and if we have found a value in the right list
	      // which is smaller then the pivot element then we exchange the
	      // values.
	      // As we are done we can increase i and j
	      if (i <= j) {
	        exchange(numbers, i, j);
	        i++;
	        j--;
	      }
	    }
	    
	    // Recursion
	    if (low < j)
	      quicksort(numbers, low, j);
	    if (i < high)
	      quicksort(numbers, i, high);
	  }

	private static void exchange(int[][] numbers, int i, int j) {
		int temp = numbers[i][0];
	    numbers[i][0] = numbers[j][0];
	    numbers[j][0] = temp;
	    temp = numbers[i][1];
	    numbers[i][1] = numbers[j][1];
	    numbers[j][1] = temp;
	    
	    // added to support disassociation VerPart
	    if (numbers[0].length>2) {
	    	temp = numbers[i][2];
		    numbers[i][2] = numbers[j][2];
		    numbers[j][2] = temp;
	    }
	}

	public int minimumDistance(double[][] clusterCenters, double[] mf) {
		int minIndex = 0;
		double minDist = EuclideanDistanceSimilarity.getEuclideanDistance(clusterCenters[0], mf); 
		for (int i=1; i<clusterCenters.length; i++) {
			double dist = EuclideanDistanceSimilarity.getEuclideanDistance(clusterCenters[i], mf);
			if (dist<minDist) {
				minDist = dist;
				minIndex = i;
			}
		}
		return minIndex;
	}
}
