package es.jmgoncalv.pseudo.netflix;

public interface Row {
	
	public void newPoint(long movieId, long rating);
	public void setUserId(int uid);
	public int getUserId();	
	public int getMovieId(int i);
	public int getRating(int i);
	public int getNumVotes();
	public int findMovieId(int startIndex, int movieId);
}
