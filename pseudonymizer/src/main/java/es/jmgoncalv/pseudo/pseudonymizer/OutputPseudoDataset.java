package es.jmgoncalv.pseudo.pseudonymizer;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Arrays;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.ProbeOrderComparator;
import es.jmgoncalv.pseudo.netflix.ProgressMonitor;
import es.jmgoncalv.pseudo.netflix.RatingEntry;
import es.jmgoncalv.pseudo.netflix.TrainingSetFileReader;

public class OutputPseudoDataset extends ProgressMonitor {
	
	private long progress, total;
	private String msg;

	public OutputPseudoDataset(File[] fileList, String probe, Dataset ds, ClusterizedRow[] crs) throws FileNotFoundException {
		msg = "Going to apply pseudonym algorithm...";
		progress = 0;
		total = fileList.length;
		
		// for the probe file
		TrainingSetFileReader ptsfr = new TrainingSetFileReader(probe,true);
		RatingEntry pre = ptsfr.next();
		// for each file in the training set check the map and apply find and replace
		Arrays.sort(fileList,new ProbeOrderComparator());
		for (File f : fileList) {
			if (f.getName().startsWith("mv_") && f.getName().endsWith(".txt")) { // skip new files
				TrainingSetFileReader tsfr = new TrainingSetFileReader(f,true);
				msg = "Processing "+f.getName()+" ...";
				RatingEntry re = tsfr.next();
				while (re!=null) {
					int userIndex = ds.getUserIndex(re.getUserId());
					int movieIndex = ds.rows()[userIndex].findMovieId(0,tsfr.getMovieId());
					// verification! movieId must exist in row!
					if (ds.rows()[userIndex].getMovieId(movieIndex)!=tsfr.getMovieId())
						throw new RuntimeException("ds.rows()[re.getUserId()].getMovieId(i)="+ds.rows()[userIndex].getMovieId(movieIndex)+" tsfr.getMovieId()="+tsfr.getMovieId());
					int p = crs[userIndex].getPseudo(movieIndex);
					re.replaceUserId(p);
					if (pre!=null && pre.getUserId()==re.getUserId() && ptsfr.getMovieId()==tsfr.getMovieId()) {
						// replace in probe also
						pre.replaceUserId(p);
						pre = ptsfr.next();
					}
					re = tsfr.next();
				}
				progress++;
				tsfr.finalize();
			}
		}
		ptsfr.finalize();
		
		msg = "Finished! Wrote new dataset and probe file!";
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
