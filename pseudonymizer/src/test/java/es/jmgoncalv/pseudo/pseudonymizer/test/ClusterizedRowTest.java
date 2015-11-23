package es.jmgoncalv.pseudo.pseudonymizer.test;

import junit.framework.Assert;

import org.junit.Test;

import es.jmgoncalv.pseudo.pseudonymizer.ClusterizedRow;

public class ClusterizedRowTest {
	
	public static final int[] clusterIdTest = { 0, 1, 2, 1, 3, 0, 2, 2 };

	// TESTS
	@Test
	public void testTranslations() {
		int pseudoBase = (int) (Math.random()*100);
		ClusterizedRow r = new ClusterizedRow(clusterIdTest.length,pseudoBase);
		
		for (int i=0; i<clusterIdTest.length; i++) {
			r.addToCluster(i, clusterIdTest[i]);
			Assert.assertEquals(clusterIdTest[i], r.getCluster(i));
			Assert.assertEquals(clusterIdTest[i]+pseudoBase, r.getPseudo(i));
		}
	}
	
	@Test
	public void testSequentialTest() {
		int pseudoBase = (int) (Math.random()*100);
		ClusterizedRow r = new ClusterizedRow(256,pseudoBase);
		
		for (int i=0; i<256; i++) {
			r.addToCluster(i, i);
			Assert.assertEquals(i, r.getCluster(i));
			Assert.assertEquals(i+pseudoBase, r.getPseudo(i));
		}
	}

}
