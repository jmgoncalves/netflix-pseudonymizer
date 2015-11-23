package es.jmgoncalv.pseudo.scoreboard;

import es.jmgoncalv.pseudo.netflix.Dataset;
import es.jmgoncalv.pseudo.netflix.OnePassStdDev;
import es.jmgoncalv.pseudo.netflix.Row;

public abstract class ScoreboardRH {

	public static Row scoreboardRH(Dataset ds, Row auxRow) {	
		OnePassStdDev opsd = new OnePassStdDev();
		Row bestMatch = null;
		double bmScore = 0;
		double sbmScore = 0;
		
		// calculate score for each row in the dataset
		for (Row r : ds.rows()) {
			double s = scoreRH(auxRow,r,ds);
			if (s>bmScore) {
				sbmScore = bmScore;
				bmScore = s;
				bestMatch = r;
			} else if (s>sbmScore) {
				sbmScore = s;
			}
			opsd.processNewValue(s);
		}
		
		// return best match if eccentric enough
		double e = (bmScore-sbmScore)/opsd.getStandardDeviation();		
		if (e>eccentricity)
			return bestMatch;
		return null;
	}
	
//	The eccentricity parameter was set to φ = 1.5, i.e., the algorithm declares there is no match if and only if the difference	between the highest 
//	and the second highest scores is no more than 1.5 times the standard deviation.
	public static final double eccentricity = 1.5;
	
//	For movie ratings which in the case of Netflix are on the 1-5 scale, we consider the thresholds of 0 (corresponding to exact match) and 1, and
//	for the rating dates, 3 days, 14 days, or ∞. The latter means that the adversary has no information	about the date when the movie was rated.
	private static double exactMatchPointSimilarity(int r1, int r2) {
		if (r1==r2)
			return 1;
		else
			return 0;
	}

	// score of a row
	private static double scoreRH(Row auxRow, Row r, Dataset ds) {
		double s = 0;
		
		int ai = 0;
		int ri = 0; 
		int apmid = auxRow.getMovieId(ai);
		int rpmid = r.getMovieId(ri);
		int i = Math.max(apmid,rpmid);
		
		while (i<=ds.getNumColumns()) {
			if (apmid==rpmid) {
				// match
				s = s + (ds.wt(i) * exactMatchPointSimilarity(auxRow.getRating(ai),r.getRating(ri)));
				ri++; 
				ai++;
				rpmid = r.getMovieId(ri);
				apmid = auxRow.getMovieId(ai);
			} else if (apmid==i) {
				// rpmid must be updated
				ri = r.findMovieId(ri+1,i);
				rpmid = r.getMovieId(ri);
			} else if (rpmid==i) {
				// apmid must be updated
				ai = auxRow.findMovieId(ai+1,i);
				apmid = auxRow.getMovieId(ai);
			}
			
			i = Math.max(apmid,rpmid);
		}
		
		return s;
	}
	
	// OLD LINEAR VERSIONS
	// linear versions are faster for high numbers of votes
	// heuristic version is around 10 times faster for a small number of votes
	public static Row scoreboardRHlinear(Dataset ds, Row auxRow) {	
		OnePassStdDev opsd = new OnePassStdDev();
		Row bestMatch = null;
		double bmScore = 0;
		double sbmScore = 0;
		
		// calculate score for each row in the dataset
		for (Row r : ds.rows()) {
			double s = scoreRHlinear(auxRow,r,ds);
			if (s>bmScore) {
				sbmScore = bmScore;
				bmScore = s;
				bestMatch = r;
			} else if (s>sbmScore) {
				sbmScore = s;
			}
			opsd.processNewValue(s);
		}
		
		// return best match if eccentric enough
		double e = (bmScore-sbmScore)/opsd.getStandardDeviation();		
		if (e>eccentricity)
			return bestMatch;
		return null;
	}
	// score of a row
	private static double scoreRHlinear(Row auxRow, Row r, Dataset ds) {
		double s = 0;
		
		int ai = 0;
		int ri = 0; 
		int apmid = auxRow.getMovieId(ai);
		int rpmid = r.getMovieId(ri);
		int i = Math.max(apmid,rpmid);
		
		while (i<=ds.getNumColumns()) {
			if (apmid==rpmid) {
				// match
				s = s + (ds.wt(i) * exactMatchPointSimilarity(auxRow.getRating(ai),r.getRating(ri)));
				ri++; 
				ai++;
				rpmid = r.getMovieId(ri);
				apmid = auxRow.getMovieId(ai);
			} else if (apmid==i) {
				// rpmid must be updated
				while (rpmid<i) {
					ri++;
					rpmid = r.getMovieId(ri);
				}
			} else if (rpmid==i) {
				// apmid must be updated
				while (apmid<i) {
					ai++;
					apmid = auxRow.getMovieId(ai);
				}
			}
			
			i = Math.max(apmid,rpmid);
		}
		
		return s;
	}
}
