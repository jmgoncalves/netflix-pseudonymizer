package es.jmgoncalv.pseudo.netflix;

public class OnePassStdDev {

	private double sum;
	private double squareSumAvg;
	private double avg;
	private double sd;
	private double pAvg;
	private double min;
	private double max;
	private int n;
	
	// http://www.cs.berkeley.edu/~mhoemmen/cs194/Tutorials/variance.pdf
	public OnePassStdDev() {
		sum = 0;
		squareSumAvg = 0;
		avg = 0;
		sd = 0;
		n = 0;
		pAvg = 0;
		min = Double.MAX_VALUE;
		max = Double.MIN_VALUE;
	}

	public void processNewValue(int x) {
		n++;
		sum = sum+x;
		squareSumAvg = squareSumAvg + (((x*x)-squareSumAvg)/n);
		pAvg = avg;
		avg = pAvg + ((x-pAvg)/n);
		sd = sd + (((n-1)*Math.pow((x-pAvg), 2))/n);
		if (x<min)
			min = x;
		if (x>max)
			max = x;
	}
	
	public void processNewValue(double x) {
		n++;
		sum = sum+x;
		squareSumAvg = squareSumAvg + (((x*x)-squareSumAvg)/n);
		pAvg = avg;
		avg = pAvg + ((x-pAvg)/n);
		sd = sd + (((n-1)*Math.pow((x-pAvg), 2))/n);
		if (x<min)
			min = x;
		if (x>max)
			max = x;
	}

	public double getAvgerage() {
		return avg;
	}

	public double getStandardDeviation() {
		return Math.sqrt(sd/n);
	}
	
	public double getVariance() {
		return sd/n;
	}

	public int getNumberOfValues() {
		return n;
	}
	
	public double getSum() {
		return sum;
	}

	public double getMin() {
		return min;
	}

	public double getMax() {
		return max;
	}
	
	public double getSquareSumAvg() {
		return squareSumAvg;
	}

	@Override
	public String toString() {
		return "Average: "+avg+"; Standard Deviation: "+getStandardDeviation()+"; Min: "+min+"; Max: "+max+"; N: "+n;
	}
}
