package es.jmgoncalv.pseudo.netflix;

import java.io.PrintWriter;

public class RatingEntry {
	
	private PrintWriter pw;
	private int userId,rating;
	private String tail;

	public RatingEntry(String nextLine, PrintWriter pw) {
		this.pw = pw;
		int commaIndex = nextLine.indexOf(",");
		if (commaIndex>-1) {
			userId = Integer.parseInt(nextLine.substring(0,commaIndex));
			tail = nextLine.substring(commaIndex);
			rating = Integer.parseInt(tail.substring(1,2));
		}
		else
			userId = Integer.parseInt(nextLine);
	}

	public int getUserId() {
		return userId;
	}
	
	public int getRating() {
		return rating;
	}

	public void replaceUserId(int p) {
		if (tail!=null)
			pw.println(p+tail);
		else
			pw.println(p);
	}
	
	public void keepUserId() {
		if (tail!=null)
			pw.println(userId+tail);
		else
			pw.println(userId);
	}

	public void removeEntry() {
		// do nothing
	}
	
	public String getTail() {
		return tail;
	}

}
