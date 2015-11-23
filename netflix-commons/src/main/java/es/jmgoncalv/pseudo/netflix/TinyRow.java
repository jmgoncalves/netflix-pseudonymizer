package es.jmgoncalv.pseudo.netflix;

public class TinyRow implements Row {
	
	// 64bit footprint vs a minimum of 3*64bit in SparseRow
	
	private int userId = 0;
	private int data = 0;
	
	public TinyRow() {} // 1 rating row
	
	public void newPoint(long movieId, long rating) {
		if (data!=0)
			throw new IllegalStateException("newPoint must only be called once for TinyRow");
		checkRanges(movieId, rating);
		
		data = (int)((movieId + (rating << 16)));
	}
	
	private void checkRanges(long movieId, long rating) {
		if (movieId<1 || movieId>65535)
			throw new IllegalStateException("Invalid movieId '"+movieId+"'");
		
		if (rating<1 || rating>5)
			throw new IllegalStateException("Invalid rating '"+rating+"'");
	}

	public void setUserId(int uid) {
		this.userId = uid;
	}

	public int getUserId() {
		return userId;
	}
	
	public int getMovieId(int i) {
		if (i!=0)
			return SparseRow.movieIdFilter; // easier to break normal behaviour in scoreboardRH
		return safeGetMovieId(i);
	}
	
	private int safeGetMovieId(int i) {
		return data & SparseRow.movieIdFilter;
	}

	public int getRating(int i) {
		if (i!=0)
			return -1;
		return data >> 16;
	}
	
	public int getNumVotes() {
		if (data!=0)
			return 1;
		return 0;
	}

	// returns the index of the passed movieId or the one immediately after; in case target movieId is smaller than the startIndex movieId, return startIndex
	public int findMovieId(int startIndex, int movieId) {
		return startIndex;
	}
}
