package es.jmgoncalv.pseudo.pseudonymizer;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintWriter;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.ProgressMonitor;

public class OutputPseudoMap extends ProgressMonitor {
	
	private long progress, total;
	private String msg;

	public OutputPseudoMap(File outputFile, Dataset ds, ClusterizedRow[] crs) throws FileNotFoundException {
		msg = "Outputing pseudonym map...";
		progress = 0;
		total = crs.length;
		
		PrintWriter pw = new PrintWriter(outputFile);
		for (int progress=0; progress<crs.length; progress++)
			pw.println(ds.rows()[progress].getUserId()+" "+crs[progress].getPseudoBase()+":"+(crs[progress].getPseudoBase()+crs[progress].getNumClusters()-1));
		pw.close();
		
		msg = "Finished! Wrote pseudonym map file!";
		progress = total;
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
