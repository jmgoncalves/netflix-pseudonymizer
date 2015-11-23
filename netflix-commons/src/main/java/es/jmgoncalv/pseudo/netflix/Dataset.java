package es.jmgoncalv.pseudo.netflix;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;

public class Dataset extends ProgressMonitor { // ~300MB per loaded dataset
	
	private static final int NUM_COLUMNS = 17770;
	private static final int NUM_COLUMNS_MOVIELENS = 10677;
//	private static final int UNSCRUBBED_VOTES = 100480507;
//	private static final int LAST_USER_ID = 2649429;
//	private static final int NUM_USERS = 480189;
	
	private int numRows, maxUserId, numColumns, numPoints;
	private int[] suppColumn; // 4Bytes * 17770 = 71080Bytes = 72KB
	private double[] wt; // 8Bytes * 17770 = 142160Bytes = 139KB
	private int[] idToIndex; // 4Bytes * 2649429 = 10597716Bytes = 10350KB = 11MB
	private Row[] rows; // 8Bytes (64bit) * 480189 = 3841512Bytes = 3752KB = 4MB
	
	private long progress, total;
	private String msg;
	
	public Dataset(File[] fileList, File userIndexFile) throws IOException {
		if (fileList.length!=NUM_COLUMNS && fileList.length!=NUM_COLUMNS_MOVIELENS) {
			System.out.println("Expected "+NUM_COLUMNS+" or "+NUM_COLUMNS_MOVIELENS+" movie files, "+fileList.length+" found instead!");
		}
		
		// global initializations
		numColumns = fileList.length;
		suppColumn = new int[numColumns];
		wt = new double[numColumns];
		numRows = 0;
		maxUserId = 0;
		numPoints = 0;
		
		// monitor initializations
		progress = 0;
		total = numColumns+4;
		
		// get numUsers and maxUserId
		msg = "Getting number of users and max user id...";
		IndexFileReader userIfr = new IndexFileReader(userIndexFile);
		int ui = userIfr.next();
		while (ui > 0) {
			numRows++;
			if (ui>maxUserId)
				maxUserId=ui;
			ui = userIfr.next();
		}
		progress++;
		
		// more global initializations
		idToIndex = new int[maxUserId];
		Arrays.fill(idToIndex,-1); // makes debug easier
		rows = new Row[numRows];

		// get user votes that follow the ids in the index file
		msg = "Getting number of votes per user and initializing rows...";
		userIfr.switchToVotes();
		int lastUi = 0;
		ui = userIfr.next();
		while (ui > 0) {
			int numVotes = ui-lastUi;
			if (numVotes>1) 
				rows[userIfr.getIndex()] = new SparseRow(numVotes);
			else
				rows[userIfr.getIndex()] = new TinyRow();
			lastUi = ui;
			ui = userIfr.next();
		}
		userIfr.destroy();
		numPoints = lastUi;
		progress++;
		
		// create idToIndex map and set userId in rows
		msg = "Mapping user ids to row indexes...";
		userIfr = new IndexFileReader(userIndexFile);
		ui = userIfr.next();
		while (ui > 0) {
			idToIndex[ui-1] = userIfr.getIndex();
			rows[userIfr.getIndex()].setUserId(ui);
			ui = userIfr.next();
		}
		userIfr.destroy();
		userIfr = null;
		progress++;
		
		// impose movie id order
		Arrays.sort(fileList);
		// process data files
		for (File f : fileList) {
			msg = "Processing file "+f.getName()+"...";
			TrainingSetFileReader tsfr = new TrainingSetFileReader(f,false);
			RatingEntry re = tsfr.next();
			while (re!=null) {
				suppColumn[tsfr.getMovieId()-1]++;
				try {
					rows[idToIndex[re.getUserId()-1]].newPoint(tsfr.getMovieId(),re.getRating());
				} catch (IllegalStateException e) {
					msg = "IllegalStateException while adding new point for userId '"+re.getUserId()+"', userIndex '"+idToIndex[re.getUserId()-1]+"' movieId '"+tsfr.getMovieId()+"' and rating '"+re.getRating()+"'";
					throw e;
				}
				re = tsfr.next();
			}
			tsfr.finalize();
			progress++;
		}
		
		// calculate wt
		msg = "Calculating wt(i)...";
//		wt(i) = 1 / log |supp(i)| - (|supp(i)| is the number of subscribers who have rated movie i
		for (int i=0; i<suppColumn.length; i++) 
			wt[i] =  1 / Math.log((double)suppColumn[i]);
		progress++;
		
		msg = "Done! Imported database with "+numRows+" rows, "+numColumns+" columns and "+numPoints+" points.";
		progress = total;
	}
	
	public Row[] rows() {
		return rows;
	}


	public double wt(int movieId) { 
		return wt[movieId-1];
	}
	
	public int supp(int movieId) { 
		return suppColumn[movieId-1];
	}
	
	public int getNumRows() {
		return numRows;
	}
	
	public int getNumColumns() {
		return numColumns;
	}
	
	public int getNumPoints() {
		return numPoints;
	}
	
	public int getUserIndex(int userId) {
		return idToIndex[userId-1];
	}
	
	public int getRating(int userId, int movieId) {
		Row r = rows[idToIndex[userId-1]];
		for (int i=0; i<r.getNumVotes(); i++) {
			if (r.getMovieId(i)==movieId)
				return r.getRating(i);
		}
		return 0;
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
