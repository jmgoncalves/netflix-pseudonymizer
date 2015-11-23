package es.jmgoncalv.pseudo.pseudonymizer;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.Row;

public interface ClusteringMethod {
	public void clusterRow(Dataset ds, Row r, ClusterizedRow cr, Factorization f);
}
