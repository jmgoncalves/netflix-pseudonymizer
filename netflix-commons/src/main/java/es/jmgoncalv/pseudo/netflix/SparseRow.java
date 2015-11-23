package es.jmgoncalv.pseudo.netflix;

public class SparseRow implements Row {
	
//	mId (1-17770: 16bit -> 65536)
//	rat (1-5: 4bit -> 16)
	
	public static final long pointFilter = 0xfffff;
	public static final int movieIdFilter = 0xffff;
	public static final int ratingFilter = 0b1111;
	
	private int userId, currentPoint; // 8Bytes * 480189 = 3841512Bytes = 3752KB = 4MB
	private long[] words; // 1 long = 3 points... worst case 33493503+480189=33973692 * 8Bytes = 271789536Bytes = 265420KB = 260MB
	private double last;
	
	public SparseRow(int numPoints) {
		words = new long[(numPoints/3)+1];
		currentPoint = 0;
	}
	
	public void newPoint(long movieId, long rating) {
		if (movieId<=last)
			throw new IllegalStateException("Passed movieId '"+movieId+"' is less or equal to the last movieId '"+last+"' - newPoint must be called with increasing values of movieId");
		checkRanges(movieId, rating);
		if (currentPoint>=words.length*3)
			throw new IllegalStateException("Adding "+(currentPoint+1)+"th point for user '"+userId+"' which is configured for a maximum of "+(words.length*3)+" points...");
		
		words[currentPoint/3] = words[currentPoint/3] + ((movieId + (rating << 16)) << (20 * (currentPoint%3)));
		currentPoint++;
		last = (double)movieId;
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
		if (i>=currentPoint || i<0)
			return movieIdFilter; // easier to break normal behaviour in scoreboardRH
		return safeGetMovieId(i);
	}
	
	private int safeGetMovieId(int i) {
		return getEncodedPoint(i) & movieIdFilter;
	}

	public int getRating(int i) {
		if (i>=currentPoint || i<0)
			return -1;
		return getEncodedPoint(i) >> 16;
	}
	
	private int getEncodedPoint(int i) {
		return (int) (words[i/3] >> (20 * (i%3)) & pointFilter);
	}
	
	public int getNumVotes() {
		return currentPoint;
	}

	// returns the index of the passed movieId or the one immediately after; in case target movieId is smaller than the startIndex movieId, return startIndex
	public int findMovieId(int startIndex, int movieId) {
		if (startIndex>=currentPoint || startIndex<0)
			return currentPoint;
		
		int sid = safeGetMovieId(startIndex);
		
		// shortcut for small rows
		if (sid>=movieId)
			return startIndex;
		
		// heuristics assume linear distribution of votes
		int estimatedIndex = Math.min(startIndex+(int)(((movieId-sid)/last)*(currentPoint-startIndex)),currentPoint-1);
		int eid = safeGetMovieId(estimatedIndex);

		// linear search after heuristic
		while (eid>movieId && estimatedIndex>startIndex) { // search back
			estimatedIndex--;
			eid = safeGetMovieId(estimatedIndex);
		}
		// now estimatedIndex is 1) on movieId; 2) before movieId; 3) on startIndex and after movieId
		// if 1) then below is skipped, returning the matching index
		// if 2) then below runs once returning the immediately after index
		// 3) doesn't happen, shortcut returned startIndex already
		while (eid<movieId && estimatedIndex<currentPoint) { // search forward
			estimatedIndex++;
			eid = safeGetMovieId(estimatedIndex);
		}
		
		return estimatedIndex;
	}
}
