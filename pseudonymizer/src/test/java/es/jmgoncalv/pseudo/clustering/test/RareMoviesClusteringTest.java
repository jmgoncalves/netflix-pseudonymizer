package es.jmgoncalv.pseudo.clustering.test;

import junit.framework.Assert;

import org.junit.Test;

import es.jmgoncalv.pseudo.clustering.RareMoviesClustering;

public class RareMoviesClusteringTest {
	
	private static final int[][] TEST_ARRAY = { { 0, 3},
												{ 1, 6}, 
												{ 2, 1},
												{ 3, 0},
												{ 4, 4},
												{ 5, 5},
												{ 6, 2} };
	private static final int[][] RESULT_ARRAY = { 	{ 3, 0},
													{ 2, 1}, 
													{ 6, 2},
													{ 0, 3},
													{ 4, 4},
													{ 5, 5},
													{ 1, 6} };


	@Test
	public void testQuicksort() {
		int[][] array = new int[TEST_ARRAY.length][2];
		for (int i=0; i<TEST_ARRAY.length; i++) {
			array[i][0] = TEST_ARRAY[i][0];
			array[i][1] = TEST_ARRAY[i][1];
		}
		
		RareMoviesClustering.quicksort(array, 0, array.length-1);
		
		 org.junit.Assert.assertArrayEquals(array,RESULT_ARRAY);
	}
	
	@Test
	public void testMinimumDistanceCalculation() {
		// TODO
		Assert.assertTrue(true);
	}
}