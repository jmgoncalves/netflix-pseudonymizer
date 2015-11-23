package es.jmgoncalv.pseudo.analyzer;

import java.io.FileNotFoundException;
import java.io.PrintWriter;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.OnePassStdDev;
import es.jmgoncalv.pseudo.netflix.ProgressMonitor;
import es.jmgoncalv.pseudo.netflix.Row;

public class CardinalityAnalyzer extends ProgressMonitor {
	
	// 0: total
	// 1 to limit-2: exact userCardinality
	// limit-1: userCardinality >= limit-1
	private OnePassStdDev[] movieCardinality;
	
	private long total, progress;
	private String msg;
	
	public CardinalityAnalyzer(Dataset ds, int limit) {
		total = ds.getNumPoints();
		progress = 0;
		msg = "Processing...";
		movieCardinality = new OnePassStdDev[limit];
		for (int i=0; i<movieCardinality.length; i++)
			movieCardinality[i] = new OnePassStdDev();
		
		// Calculate average cardinality of movies per user, aggregated per user cardinality
		for (Row r : ds.rows()) {
			double mcSum = 0;
			for (int i=0; i<r.getNumVotes(); i++) {
				mcSum += ds.supp(r.getMovieId(i));
				progress++;
			}
			double mcAvg = mcSum/r.getNumVotes();
			movieCardinality[Math.min(r.getNumVotes(), limit-1)].processNewValue(mcAvg);
			movieCardinality[0].processNewValue(mcAvg);
			msg = "Processed row with "+r.getNumVotes()+" votes and "+mcAvg+" movie cardinality average...";
		}
	}
	
	public void output(String outputFile) throws FileNotFoundException {
		PrintWriter p = new PrintWriter(outputFile);
		for (int i=0; i<movieCardinality.length; i++)
			p.println(i+","+movieCardinality[i].getNumberOfValues()+","+movieCardinality[i].getAvgerage()+","+movieCardinality[i].getStandardDeviation());
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
