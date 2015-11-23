package es.jmgoncalv.pseudo.scoreboard;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintWriter;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.OnePassStdDev;
import es.jmgoncalv.pseudo.netflix.ProgressMonitor;
import es.jmgoncalv.pseudo.netflix.Row;
import es.jmgoncalv.pseudo.netflix.SparseRow;
import es.jmgoncalv.pseudo.netflix.UserIdMap;

public class ScoreboardSampling extends ProgressMonitor {
	
	private static final int MIN_AUX = 2;
	private static final int MAX_AUX = 200;
	private static final int ITERATION_STEP = 100;
	private static final int MIN_ITERATIONS = 3;
	private static final double CONFIDENCE_LEVEL_FACTOR = 1.96; // for 95% confidence level
	private static final int NUM_VALUES = 4;
	private static final double DEFAULT_ERROR_MARGIN = 0.02; // 2% margin of error
	
	private int progress;
	private String msg;
	private int total;
	
	private OnePassStdDev[][] results;
	
	public ScoreboardSampling(Dataset auxSourceDataset, Dataset targetDataset, UserIdMap uim) throws IOException {
		this(auxSourceDataset, targetDataset, uim, DEFAULT_ERROR_MARGIN, null);
	}
	
	public ScoreboardSampling(Dataset auxSourceDataset, Dataset targetDataset, UserIdMap uim, double errorMargin, String outputFile) throws IOException {
		// check
		if (auxSourceDataset.getNumColumns()!=targetDataset.getNumColumns())
			throw new RuntimeException("Compared datasets have different number of columns!");
		
		// initialize outputFile if plan is to output "on the fly"
		PrintWriter pw = null;
		if (outputFile!=null)
			pw = new PrintWriter(outputFile);
		
		total = MAX_AUX-MIN_AUX;
		progress = 0;
		
		// 0: match
		// 1: no match
		// 2: wrong match
		// 3: num attributes
		results = new OnePassStdDev[MAX_AUX-MIN_AUX][NUM_VALUES];
		
		for (int numAux = MIN_AUX; numAux<MAX_AUX; numAux++) {
			// for a certain number of auxiliary information
			for (int i=0; i<NUM_VALUES; i++)
				results[progress][i] = new OnePassStdDev();
			
			// do min iterations
			for (int iterations=1; iterations<=MIN_ITERATIONS; iterations++) {
				msg = "Calculating ScoreboardRH success for auxiliary information with size "+numAux+": doing minimum iterations ("+iterations+" of "+MIN_ITERATIONS+")...";
				iteration(auxSourceDataset, targetDataset, numAux, results[progress], uim);
			}
			
			// do comparing iterations until confidence interval
			double error[] = new double[3];
			binomialMarginOfError(results[progress],error);
			while (anyGreaterThan(error,errorMargin)) {
				msg = "Calculating ScoreboardRH success for auxiliary information with size "+numAux+": reducing error margins ("+error[0]+","+error[1]+","+error[2]+")...";
				iteration(auxSourceDataset, targetDataset, numAux, results[progress], uim);
				binomialMarginOfError(results[progress], error);
			}
			
			pw.println((progress+MIN_AUX)+","+results[progress][0].getNumberOfValues()+","+results[progress][0].getAvgerage()+","+results[progress][0].getVariance()+","+results[progress][1].getAvgerage()+","+results[progress][1].getVariance()+","+results[progress][2].getAvgerage()+","+results[progress][2].getVariance()+","+results[progress][3].getAvgerage()+","+results[progress][3].getVariance());
			pw.flush();
			progress++;
		}
		
		pw.close();
	}
	
	private boolean anyGreaterThan(double[] error, double errorMargin) {
		for (double d : error)
			if (d>errorMargin)
				return true;
		
		return false;
	}
	
	// http://en.wikipedia.org/wiki/Binomial_proportion_confidence_interval#Normal_approximation_interval
	private static void binomialMarginOfError(OnePassStdDev[] values, double error[]) {
		for (int i=0; i<error.length; i++)
			error[i] = CONFIDENCE_LEVEL_FACTOR * Math.sqrt((values[i].getAvgerage()*(1-values[i].getAvgerage())/values[i].getNumberOfValues()));
	}

	// below this is same code as old version
	public void outputResults(String outputFile) throws FileNotFoundException {
		PrintWriter p = new PrintWriter(outputFile);
		for (int i=0; i<results.length; i++)
			p.println((i+MIN_AUX)+","+results[i][0].getNumberOfValues()+","+results[i][0].getAvgerage()+","+results[i][0].getVariance()+","+results[i][1].getAvgerage()+","+results[i][1].getVariance()+","+results[i][2].getAvgerage()+","+results[i][2].getVariance()+","+results[i][3].getAvgerage()+","+results[i][3].getVariance());
		p.close();
	}

	private static void iteration(Dataset auxSourceDataset, Dataset targetDataset, int numAux, OnePassStdDev[] opsd, UserIdMap uim) {
		for (int iteration = 0; iteration<ITERATION_STEP; iteration++) {
			// compute an iteration
			// find a big enough target row
			int randomRow = -1;
			do { randomRow = (int) Math.floor(Math.random()*auxSourceDataset.getNumRows());
			} while (auxSourceDataset.rows()[randomRow].getNumVotes()<numAux);
			Row originalRow = auxSourceDataset.rows()[randomRow];
			
			// build random aux for that row
			Row auxRow = new SparseRow(numAux);
			int[] randomNums = RandomUtils.randomNums(numAux,originalRow.getNumVotes());
			for (int i : randomNums)
				auxRow.newPoint(originalRow.getMovieId(i), originalRow.getRating(i));
			
			// process match
			Row match = ScoreboardRH.scoreboardRH(targetDataset, auxRow);
			
			if (match!=null) { // there is a match!
				opsd[1].processNewValue(0);
				if (uim.match(randomRow,match.getUserId(),originalRow.getUserId())) { // right match!
					opsd[0].processNewValue(1);
					opsd[2].processNewValue(0);
					opsd[3].processNewValue(newAttrs(match,auxRow,auxSourceDataset.getNumColumns()));
					
				} else { // wrong match
					opsd[0].processNewValue(0);
					opsd[2].processNewValue(1);
					opsd[3].processNewValue(0); // negative?
				}
			} else { // no match
				opsd[1].processNewValue(1);
				opsd[0].processNewValue(0);
				opsd[2].processNewValue(0);
				opsd[3].processNewValue(0);
			}
		}
	}

	// counts common attributes set in both rows and subtracts from match row total attributes to calculate new attributes
	private static int newAttrs(Row match, Row auxRow, int numColumns) {
		int commonAttrs = 0;
		
		int ai = 0;
		int ri = 0; 
		int apmid = auxRow.getMovieId(ai);
		int rpmid = match.getMovieId(ri);
		int i = Math.max(apmid,rpmid);
		
		while (i<=numColumns) {
			if (apmid==rpmid) {
				// match
				commonAttrs++;
				ri++; 
				ai++;
				rpmid = match.getMovieId(ri);
				apmid = auxRow.getMovieId(ai);
			} else if (apmid==i) {
				// rpmid must be updated
				ri = match.findMovieId(ri+1,i);
				rpmid = match.getMovieId(ri);
			} else if (rpmid==i) {
				// apmid must be updated
				ai = auxRow.findMovieId(ai+1,i);
				apmid = auxRow.getMovieId(ai);
			}
			
			i = Math.max(apmid,rpmid);
		}
		
		return match.getNumVotes()-commonAttrs;
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
