package es.jmgoncalv.pseudo.pseudonymizer;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.ProgressMonitor;

public class Clustering extends ProgressMonitor {
	
	private int pseudo;
	private int firstPseudo;
	
	private long progress, total;
	private String msg;
	
	public Clustering(ClusteringMethod cm, Dataset ds, ClusterizedRow[] crs, Factorization f, int firstPseudo) {
		msg = "Initializing...";
		total = ds.getNumPoints();
		progress = 0;
		
		this.pseudo = firstPseudo;
		this.firstPseudo = firstPseudo;
		
		msg = "Clustering...";
		for (int i=0; i<ds.rows().length; i++) {
			crs[i] = new ClusterizedRow(ds.rows()[i].getNumVotes(), pseudo);
			cm.clusterRow(ds, ds.rows()[i],crs[i],f);
			pseudo = pseudo + crs[i].getNumClusters();
			progress = progress + ds.rows()[i].getNumVotes();
		}
		
		msg = "Finished! Assigned "+getTotalPseudos()+" pseudonyms!";
	}
	
	public int getTotalPseudos() {
		return pseudo-firstPseudo;
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
