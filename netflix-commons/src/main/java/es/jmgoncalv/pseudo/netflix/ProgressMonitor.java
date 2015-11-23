package es.jmgoncalv.pseudo.netflix;

public abstract class ProgressMonitor extends Thread {
	
	private static final long SLEEP = 2000;
	
	private long start, current;
	private int printLenght;

	public ProgressMonitor() {
		start();
	}
	
	public abstract long getProgress();
	public abstract long getTotal();
	public abstract String getMessage();
	
	@Override
	public void run() {
		start = current = System.currentTimeMillis();
		System.out.println("Processing "+this.getClass().getCanonicalName()+"...");
		
		long p = getProgress();
		long t = getTotal();
		updateProgress(p,t,(current-start));
		while (p<t || t==0) {
			try {
		        Thread.sleep(SLEEP);
		    } catch (InterruptedException e) {}
			p = getProgress();
			t = getTotal();
			current = System.currentTimeMillis();
			updateProgress(p,t,(current-start));
		}
		System.out.println();
		current = System.currentTimeMillis();
		//System.out.println("Finished processing in "+(current-start)+" ms...");
	}
	
	public void updateProgress(long progress, long total, long time) {
		String msg = getMessage();
		
		int p = 0;
		if (total>0)
			p = (int)(progress*100/(double)total);
		if (msg==null)
			msg = "";
		
		Runtime run = Runtime.getRuntime();
	    long um = run.totalMemory()-run.freeMemory();
	    
	    String printStr = "Progress: "+p+"% ("+progress+"/"+total+") " +
				"| Memory: " + megabyteString(um) + "MB " +
	            "| Time: "+ longToTime(time) +
	            " | " + msg;
		System.out.print("\r"+printStr+padding(printLenght-printStr.length())); // padding due to variable length
		printLenght = printStr.length();
	}

	private static String padding(int l) {
		if (l>0) {
			StringBuffer outputBuffer = new StringBuffer(l);
			for (int i = 0; i < l; i++)
			   outputBuffer.append(" ");
			return outputBuffer.toString();
		}
		return "";
	}

	private static String longToTime(long time) {
		int s = (int) ((time/1000)%60);
		int m = (int) ((time/60000)%60);
		int h = (int) (time/3600000);
		return String.format("%d:%02d:%02d", h, m, s);
	}

	private static String megabyteString(long bytes) {
	    return String.format("%.1f", ((float)bytes) / 1024 / 1024); 
	}

}
