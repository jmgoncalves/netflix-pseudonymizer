package es.jmgoncalv.pseudo.analyzer;

import java.io.FileNotFoundException;
import java.io.PrintWriter;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.OnePassStdDev;
import es.jmgoncalv.pseudo.netflix.ProgressMonitor;
import es.jmgoncalv.pseudo.netflix.Row;
import es.jmgoncalv.pseudo.netflix.UserIdMap;

public class FragmentationAnalyzer extends ProgressMonitor {
	
	// 0: total
	// 1 to limit-2: exact userCardinality
	// limit-1: userCardinality >= limit-1
	private OnePassStdDev[] pseudosCreated;
	
	private long total, progress;
	private String msg;
	
	public FragmentationAnalyzer(Dataset ds, UserIdMap m, int limit) {
		total = ds.getNumRows();
		progress = 1;
		msg = "Processing...";
		pseudosCreated = new OnePassStdDev[limit];
		for (int i=0; i<pseudosCreated.length; i++)
			pseudosCreated[i] = new OnePassStdDev();
		
		// Calculate average fragmentation, aggregated per user cardinality
		for (int i=0; i<ds.getNumRows(); i++) {
			if (ds.rows()[i].getUserId()!=m.getUserIdRanges()[i][0])
				throw new RuntimeException("User Id Mismatch at index "+i+"!");
			int np = 1+(m.getUserIdRanges()[i][2]-m.getUserIdRanges()[i][1]);
			pseudosCreated[Math.min(ds.rows()[i].getNumVotes(), limit-1)].processNewValue(np);
			pseudosCreated[0].processNewValue(np);
			progress++;
		}
		
		progress = total;
	}
	
	public void output(String outputFile) throws FileNotFoundException {
		PrintWriter p = new PrintWriter(outputFile);
		for (int i=0; i<pseudosCreated.length; i++)
			p.println(i+","+pseudosCreated[i].getNumberOfValues()+","+pseudosCreated[i].getAvgerage()+","+pseudosCreated[i].getStandardDeviation());
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
