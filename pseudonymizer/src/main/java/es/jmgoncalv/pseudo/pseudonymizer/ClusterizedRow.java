package es.jmgoncalv.pseudo.pseudonymizer;

public class ClusterizedRow {
	
	// supports up to 256 clusters (8bits per cluster id)
//	public static final int filter = 0b11111111;
//	public static final int MAX_CLUSTER_ID = 255;
//	public static final int slots = 8;
//	public static final int bitsPerCluster = 8;
	
	// supports up to 65536 clusters (16bits per cluster id)
	public static final int filter = 0b1111111111111111;
	public static final int MAX_CLUSTER_ID = 65536;
	public static final int slots = 4;
	public static final int bitsPerCluster = 16;
	
	private long[] clusters;
	private int numClusters;
	private int numMovies;
	private int pseudoBase;
	
	public ClusterizedRow(int numMovies, int pseudoBase) {
		this.numClusters = 0;
		this.numMovies = numMovies;
		this.pseudoBase = pseudoBase;
		clusters = new long[(numMovies/slots)+1];
	}

	public void addToCluster(int i, int clusterId) { // does not support updates
		if (clusterId>MAX_CLUSTER_ID)
			throw new RuntimeException("Invalid clusterId! Passed "+clusterId+" and max id is "+MAX_CLUSTER_ID);
		if (clusterId>numClusters)
			throw new RuntimeException("Invalid clusterId! Passed "+clusterId+" and still on "+numClusters);
		if (clusterId==numClusters)
			numClusters++;
		
		clusters[i/slots] = clusters[i/slots] + ((long)clusterId << ((i%slots)*bitsPerCluster));
	}
	
	public int getCluster(int i) {
		return (int) (clusters[i/slots] >> (bitsPerCluster * (i%slots)) & filter);
	}
	
	public int getNumClusters() {
		return numClusters;
	}
	
	public int getNumMovies() {
		return numMovies;
	}
	
	public int getPseudoBase() {
		return pseudoBase;
	}
	
	/**
	 * 
	 * @param i - movieIndex
	 * @return pseudonym for this user-movie pair
	 */
	public int getPseudo(int i) {
		return pseudoBase+getCluster(i);
	}
}
