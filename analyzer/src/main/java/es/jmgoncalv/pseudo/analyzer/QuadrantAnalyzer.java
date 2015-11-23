package es.jmgoncalv.pseudo.analyzer;

import java.io.FileNotFoundException;
import java.io.PrintWriter;
import java.util.Arrays;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.ProgressMonitor;
import es.jmgoncalv.pseudo.netflix.Row;

public class QuadrantAnalyzer extends ProgressMonitor {
	
	// user X movie
	private int[][] quadrants;
	private int[][] splitPoints;
	
	private long total, progress;
	private String msg;
	
	public QuadrantAnalyzer(Dataset ds, int quadrantNum) {
		total = ds.getNumPoints()+ds.getNumRows()+ds.getNumColumns();
		progress = 0;
		msg = "Calculating cardinality distributions...";
		quadrants = new int[quadrantNum][quadrantNum];
		splitPoints = new int[quadrantNum-1][2];
		
		// Get Cardinality Distributions
		int[] uc = new int[ds.getNumColumns()]; // userCardinality[numUserVotes]=numUsers
		int[] mc = new int[ds.getNumRows()]; //movieCardinality[numMovieVotes]=numMovies
		for (Row r : ds.rows()) {
			uc[r.getNumVotes()-1]++;
			progress++;
		}
		for (int i=1; i<=ds.getNumColumns(); i++) {
			mc[ds.supp(i)-1]++;
			progress++;
		}
		
		msg = "Calculating split points...";
		// Calculate splitPoints rows
		double split = ds.getNumRows()/quadrantNum;
		double currSplit = split;
		int aggregate = 0;
		int currPoint = 0;
		for (int i=0; i<ds.getNumColumns(); i++) {
			aggregate += uc[i];
			if (aggregate>currSplit) {
				// allocate split point
				splitPoints[currPoint][0] = i;
				currPoint++;
				currSplit = currSplit+split;
				if (currPoint>=splitPoints.length) {
					break;
				}
			}
		}
		
		// Calculate splitPoints columns
		split = ds.getNumColumns()/quadrantNum;
		currSplit = split;
		aggregate = 0;
		currPoint = 0;
		for (int i=0; i<ds.getNumRows(); i++) {
			aggregate += mc[i];
			if (aggregate>currSplit) {
				// allocate split point
				splitPoints[currPoint][1] = i;
				currPoint++;
				currSplit = currSplit+split;
				if (currPoint>=splitPoints.length) {
					break;
				}
			}
		}
		
		// Print split points
		StringBuilder sb = new StringBuilder("\nSplit Points User Cardinality: (0,) ");
		for (int i=0; i<splitPoints.length-1; i++)
			sb.append(splitPoints[i][0]+", ");
		sb.append(splitPoints[splitPoints.length-1][0]+" (, "+ds.getNumRows()+")\n");
		sb.append("Split Points Movie Cardinality: (0,) ");
		for (int i=0; i<splitPoints.length-1; i++)
			sb.append(splitPoints[i][1]+", ");
		sb.append(splitPoints[splitPoints.length-1][1]+" (, "+ds.getNumColumns()+")\n");
		synchronized (this) { System.out.println(sb.toString()); }
		
		msg = "Distributing votes...";
		// Calculate quadrant of vote
		for (Row r : ds.rows()) {
			for (int i=0; i<r.getNumVotes(); i++) {
				// for each vote
				int uci = quadrantNum-1;
				int mci = quadrantNum-1;
				for(int a=0; a<quadrantNum-1; a++) {
					if (splitPoints[a][0]>=r.getNumVotes()) {
						uci = a;
						break;
					}
				}
				for(int a=0; a<quadrantNum-1; a++) {
					if (splitPoints[a][1]>=ds.supp(r.getMovieId(i))) {
						mci = a;
						break;
					}
				}
				quadrants[uci][mci]++;
				progress++;
			}
		}
				
		msg = "Done!";
		progress = total;
	}
	
	public void output(String outputFile) throws FileNotFoundException {
		PrintWriter p = new PrintWriter(outputFile);
		for (int i=0; i<quadrants.length; i++) {
			for (int j=0; j<quadrants.length-1; j++)
				p.print(quadrants[i][j]+",");
			p.println(quadrants[i][quadrants.length-1]);
		}
		p.close(); 
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
