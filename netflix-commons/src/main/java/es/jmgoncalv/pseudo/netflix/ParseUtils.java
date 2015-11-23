package es.jmgoncalv.pseudo.netflix;

public abstract class ParseUtils {

	public static double charArrayToDouble(char[] buffer, int start, int end) throws NumberFormatException {
		boolean scientific = false;
		boolean positive = true;
		double result = 0;
		int decimal = -1;
	    for (int i = start; i < end; i++)
	    {
	    	// handle dot
	    	if (buffer[i] == '.') {
	    		decimal = i;
	    		continue;
	    	}
	    	
	    	// handle minus
	    	if (buffer[i] == '-') {
	    		positive = false;
	    		continue;
	    	}
	    	
	    	// handle e
	    	if (buffer[i] == 'e') {
//	    		System.out.print("Rounding scientific notation to zero: ");
//	    		System.out.println(buffer);
	    		return 0; // it is acceptable to round it as is only used for values under 0.0001
	    	}
	    	
	    	// handle digit
	        int digit = (int)buffer[i] - (int)'0';
	        if ((digit < 0) || (digit > 9))
	        	throwException(digit, buffer[i], buffer, start, end);
	        if (decimal>-1) {
	        	double d = digit;
	        	for (int j=decimal; j<i; j++)
	        		d = d * 0.1;
	        	result = result + d;
	        }
	        else {
		        result = result * 10;
		        result = result + digit;
	        }
	    }
	    
	    if (positive)
	    	return result;
	    else
	    	return -result;
	}

	// only supports positives
	public static int charArrayToInt(char[] buffer, int start, int end) throws NumberFormatException
	{
	    int result = 0;
	    for (int i = start; i < end; i++)
	    {
	        int digit = (int)buffer[i] - (int)'0';
	        if ((digit < 0) || (digit > 9))
	        	throwException(digit, buffer[i], buffer, start, end);
	        result *= 10;
	        result += digit;
	    }
	    return result;
	}
	
	private static void throwException(int digit, char c, char[] buffer, int start,
			int end) throws NumberFormatException {
		String b = new String(buffer);
		throw new NumberFormatException(
        		"Character '"+c+"' converted to digit '"+digit+"' found in token '"+b.substring(start,end)
        		+"' is not a digit... Underlying string is '"+b+"'");
	}
}
