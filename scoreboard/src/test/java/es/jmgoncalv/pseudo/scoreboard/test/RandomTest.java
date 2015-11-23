package es.jmgoncalv.pseudo.scoreboard.test;

import junit.framework.Assert;

import org.junit.Test;

import es.jmgoncalv.pseudo.scoreboard.RandomUtils;

public class RandomTest {

	// TESTS
	private static final int RUNS = 50;
	private static final double MAX = 100;

	@Test
	public void testRandomNums() {
		for (int i=0; i<RUNS; i++) {
			// generate random numbers
			int range = 2+ (int)Math.floor(Math.random()*MAX);
			int num = (int)Math.floor(Math.random()*range);
			int[] r = RandomUtils.randomNums(num, range);
			
			// sanity checks
			Assert.assertEquals(num, r.length); // number of numbers is correct
			for (int j=0; j<r.length; j++) {
				Assert.assertTrue((r[j]>=0)); // number is between 0 and range (exclusive)
				Assert.assertTrue((r[j]<range)); 
				for (int k=0; k<j; k++)
					Assert.assertTrue((r[j]!=r[k])); // number is unique
				if (j>0)
					Assert.assertTrue(r[j]>r[j-1]); // is ordered
			}
			
			// TODO test random properties
		}
	}
}
