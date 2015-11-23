package es.jmgoncalves.pseudo.netflix.test;

import junit.framework.Assert;

import org.junit.Test;

import es.jmgoncalv.pseudo.netflix.EuclideanDistanceSimilarity;

public class EuclideanDistanceSimilarityTest {
	
	private static final double[][] FIXED_CLUSTER = {{1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
	private static final double[][] TIGHER_CLUSTER = {{1, 1}, {0, 0}, {1, 0}, {0, 1}};
	private static final double[][] LOOSER_CLUSTER = {{2, 2}, {-2, -2}, {2, -2}, {-2, 2}};
	private static final int[] INDEXES_ALL = {0, 1, 2 , 3};
	private static final int[] INDEXES_PARTIAL = {0, 1, 2};
 	private static final double MAX_ERROR = 0.0000001;
 	private static final double RESULT = (1/((2*Math.sqrt(8))+8))/Math.pow(4, 2);
 	private static final double RESULT_PARTIAL = (1/(Math.sqrt(8)+4))/Math.pow(3, 2);
	
	// TESTS
	@Test
	public void testStaticCluster() {
		double fs = EuclideanDistanceSimilarity.getAvgSelfSimilarity(FIXED_CLUSTER);
		double ts = EuclideanDistanceSimilarity.getAvgSelfSimilarity(TIGHER_CLUSTER);
		double ls = EuclideanDistanceSimilarity.getAvgSelfSimilarity(LOOSER_CLUSTER);
		
		Assert.assertEquals(RESULT, fs, MAX_ERROR);
		Assert.assertTrue(fs>ls);
		Assert.assertTrue(fs<ts);
		
		double afs = EuclideanDistanceSimilarity.getAvgSelfSimilarity(FIXED_CLUSTER, INDEXES_ALL);
		Assert.assertEquals(afs, fs);
		
		double pfs = EuclideanDistanceSimilarity.getAvgSelfSimilarity(FIXED_CLUSTER, INDEXES_PARTIAL);
		Assert.assertEquals(RESULT_PARTIAL, pfs, MAX_ERROR);
	}
	
	
}
