package es.jmgoncalv.pseudo.clustering;

import es.jmgoncalv.pseudo.pseudonymizer.ClusteringMethod;
import es.jmgoncalv.pseudo.pseudonymizer.ClusterizedRow;
import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.pseudonymizer.Factorization;
import es.jmgoncalv.pseudo.netflix.Row;

public class NoClustering implements ClusteringMethod {

	@Override
	public void clusterRow(Dataset ds, Row r, ClusterizedRow cr, Factorization f) {
		for (int i=0; i<r.getNumVotes(); i++)
			cr.addToCluster(i, 0); // each cr only has 1 cluster - no pseudos!
	}

}
