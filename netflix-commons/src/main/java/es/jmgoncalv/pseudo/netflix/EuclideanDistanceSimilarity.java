package es.jmgoncalv.pseudo.netflix;

public class EuclideanDistanceSimilarity {
	
	public static double getAvgSelfSimilarity(double[][] c1) {
		double disTotal = 0;
		for (int i=0; i<c1.length; i++)
			for (int j=i+1; j<c1.length; j++) {
				disTotal = disTotal + getEuclideanDistance(c1[i],c1[j]);
			}
		return (1/disTotal)/Math.pow(c1.length,2);
	}
	
	public static double getAvgSelfSimilarity(double[][] c1, int[] indexes) {
		double disTotal = 0;
		for (int i=0; i<indexes.length; i++)
			for (int j=i+1; j<indexes.length; j++) {
				disTotal = disTotal + getEuclideanDistance(c1[indexes[i]],c1[indexes[j]]);
			}
		return (1/disTotal)/Math.pow(indexes.length,2);
	}
	
	public static double getEuclideanDistance(double[] p1, double[] p2) {
		double total = 0;
		for (int i=0; i<p1.length; i++)
			total = total + Math.pow(p1[i]-p2[i],2);
		return Math.sqrt(total);
	}

	// euclidean distance that weighs a specific dimension as much as the others combined
	public static double getSkewedDistance(double[] p1, double[] p2, int index) {
		double total = 0;
		for (int i=0; i<p1.length; i++)
			if (i==index)
				total = total + Math.pow(p1[i]-p2[i],2)*(p1.length-1);
			else
				total = total + Math.pow(p1[i]-p2[i],2);
		return Math.sqrt(total);
	}
}
