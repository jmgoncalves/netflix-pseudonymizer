package es.jmgoncalv.pseudo.analyzer;

import java.io.FileNotFoundException;
import java.io.PrintWriter;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.OnePassStdDev;
import es.jmgoncalv.pseudo.netflix.ProgressMonitor;
import es.jmgoncalv.pseudo.netflix.Row;

public class MovieCardinalityAnalyzer extends ProgressMonitor {
	
	// 0: total
	// 1 to limit-2: exact movieCardinality
	// limit-1: movieCardinality >= limit-1
	private OnePassStdDev[] userCardinality;
	
	private long total, progress;
	private String msg;
	
	public MovieCardinalityAnalyzer(Dataset ds, int limit) {
		total = ds.getNumPoints();
		progress = 0;
		msg = "Processing...";
		userCardinality = new OnePassStdDev[limit];
		for (int i=0; i<userCardinality.length; i++)
			userCardinality[i] = new OnePassStdDev();
		
		// Calculate average cardinality of users per movie, aggregated per movie cardinality
		for (Row r : ds.rows()) {
			for (int i=0; i<r.getNumVotes(); i++) {
				// for each vote
				userCardinality[Math.min(ds.supp(r.getMovieId(i)), limit-1)].processNewValue(r.getNumVotes());
				userCardinality[0].processNewValue(r.getNumVotes());
				progress++;
			}
		}
		
		progress = total;
	}
	
	public void output(String outputFile) throws FileNotFoundException {
		PrintWriter p = new PrintWriter(outputFile);
		for (int i=0; i<userCardinality.length; i++)
			p.println(i+","+userCardinality[i].getNumberOfValues()+","+userCardinality[i].getAvgerage()+","+userCardinality[i].getStandardDeviation());
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
