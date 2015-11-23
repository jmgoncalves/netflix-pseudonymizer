package es.jmgoncalves.pseudo.netflix.test;

import junit.framework.Assert;

import org.junit.Test;

import es.jmgoncalv.pseudo.netflix.Row;
import es.jmgoncalv.pseudo.netflix.SparseRow;

public class PointTest {
	
	public static final int[] ratingTests = { 1, 2, 3, 4, 5, 4, 3, 2 };
	public static final int[] movieIdTests = { 1, 127, 128, 255, 256, 257, 500, 17770 };
	public static final int[] findMovieIdTests = { 1, 127, 200, 255, 256, 257, 500, 17770 };

	// TESTS
	@Test
	public void testByteTranslations() {
		Row r = new SparseRow(ratingTests.length);
		
		for (int i=0; i<ratingTests.length; i++) {
			r.newPoint(movieIdTests[i], ratingTests[i]);
			Assert.assertEquals(ratingTests[i], r.getRating(i));
			Assert.assertEquals(movieIdTests[i], r.getMovieId(i));
		}
	}
	
	@Test
	public void testFindMovieId() {
		Row r = new SparseRow(ratingTests.length);
		for (int i=0; i<ratingTests.length; i++) 
			r.newPoint(movieIdTests[i], ratingTests[i]);
		
		// TODO
//		for (int i=0; i<findMovieIdTests.length; i++)
//			r.findMovieId(0, findMovieIdTests[i]);
		Assert.assertTrue(true);
	}
}
