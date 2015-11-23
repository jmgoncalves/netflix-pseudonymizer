package es.jmgoncalv.pseudo.scoreboard;

import java.util.Arrays;

public abstract class RandomUtils {
	
	// generates num distinct random ints between 0 and range (exclusive)
	//http://stackoverflow.com/questions/196017/unique-random-numbers-in-o1
	public static int[] randomNums(int num, int range) {
		// init array between 0 and range
		int[] a = new int[range];
		for (int i=0; i<a.length; i++)
			a[i] = i;
		
		// modified Fisher-Yates shuffle
		for (int i=0; i<num; i++) {
			int j = i + (int) Math.floor(Math.random()*(range-i));
			int t = a[j];
			a[j] = a[i];
			a[i] = t;
		}
		
		// return
		int[] b = Arrays.copyOf(a, num); // get first elements
		Arrays.sort(b); // sort them ascendingly (requirement for scoreboardRH)
		return b;
	}

}
