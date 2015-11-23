package es.jmgoncalv.pseudo.netflix;

import java.io.File;
import java.util.Comparator;

public class ProbeOrderComparator implements Comparator<File> {
	
	@Override
	public int compare(File f1, File f2) {
		String s1 = f1.getName().replaceFirst("^mv_0*", "");
		String s2 = f2.getName().replaceFirst("^mv_0*", "");
        return s1.compareTo(s2);
	}

}
