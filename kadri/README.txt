Icefox's original code Copyright 2007, Benjamin Meyer (Icefox - icefox.net/benjamin-meyer.blogspot.com)
New contributions copyright 2009, Saqib Kadri (kadence[at]trahald.com)

REQUIREMENTS
------------
For Icefox's code, you will need to download the Qt framework (qtsoftware.com).
For my code, you will need LAPACK (netlib.org/lapack). This is actually only necessary for the blending code, so you can get by without it if you don't include the blending files.
To properly use my code, you will need a 64-bit OS and a 64-bit compiler (e.g. Visual Studio Pro's 64-bit add-in/MinGW-64)
	To remove the 64-bit requirement, you will probably have to comment out the mmaping of DataBase::datesmap in database.cpp, and edit the Movie.h:votedate() and User.h:votedate() functions to return a dummy value like 0.
	Windows 32-bit: You will still need the 64-bit compiler (which I believe will still compile on your system), but you should be able my framework if you increase your process RAM limit to 3GB by adding the '/3GB' switch to your boot.ini (edited via 'msconfig' in Start>Run).
		Further reading: Google 'windows 32 bit boot.ini /3gb switch', and http://support.microsoft.com/kb/316739

INSTRUCTIONS
------------
Place the directory that contains this readme file in your Netflix/ folder. Your directory structure should be like:

Netflix/qualifying.txt
Netflix/probe.txt
Netflix/training_set/
Netflix/kadri

Basic steps to generate data files:
1) Run Netflix/kadri/icefox/average
2) Run Netflix/kadri/icefox/scrubprobedata
3) Delete Netflix/movies.index, movies.data, users.index, users.data
4) Again run Netflix/kadri/icefox/average
5) Run Netflix/kadri/icefox/dates

The main program (kadri/main.cpp) can then be run as desired. main.cpp can be compiled directly, e.g. using "g++ main.cpp"
To save output, set the second parameter to "true" in Algorithm::runQualifying() and send the output of the compiled program to some file via command line. Example:
	In main.cpp: avg.runQualifying("none", true);
	From command line (if compiled to "progname"): progname > qualpreds.txt
You can have the Algorithm::runProbe() output in submission style format as well by setting the first parameter to "stdout". Example:
	In main.cpp: avg.runProbe("stdout");
	From command line (if compiled to "progname"): progname > probepreds.txt

If you wish to train models on the entire training set including probe data, instead of deleting: movies.data, movies.index, users.data, users.index
	Simply rename those four files, run all the steps above, then restore their original names. Run icefox/dates again. After this edit the main.cpp value for NUM_VOTES to 100480507, and run your models normally.

Stats are stored in the stats.txt file. You can control stat writing in config.h, or with the Algorithm::full_output Boolean variable.

More detailed steps to build data files ('>' is the command prompt):

>cd Netflix/kadri/icefox/average
>qmake -recursive
>make
>average
>cd Netflix/kadri/icefox/scrubprobedata
>qmake -recursive
>make
>scrubprobedata
>cd Netflix
>del movies.data
>del movies.index
>del users.data
>del users.index
>cd Netflix/kadri/icefox/average
>average
>cd Netflix/kadri/icefox/dates
>qmake -recursive
>make
>dates

MD5 Sums for Data Files
-----------------------
movies.data		D51798C548BDD91BA1C941C709B07C3B
movies.index		7B96599263FFD3FDDEBFCBA09DE2B157
users.data		83F33D423B6BB5DF2F534648D68DE664
users.index		0BF8AB2302FCCD815975B0D0FD96C165
probe.data		38A03FA9BB60584003B273B77FFA0AE3
dates.data		3AE1CF3941667CA8359FE2C9F3A89AA0
qualifying.data		A8A01CFFF06CD3D54A43443C5253249A
probe.dates.data	54099D5257835472520B623F18E1D0A9
qualifying.dates.data	7561FC6AE0BD9704056ABF4D408AB87B

MD5's above are post-scrubbing. MD5's for the data files before scrubbing:
movies.data (full)	E85D3FC81120C7B5091AE35952B1CA07
movies.index (full)	1EBB410592BBC8EC88FB0F8E96B07057
users.data (full)	1A592B76FCBA4A264F4ABEB636E0F345
users.index (full)	BC782BC5926F437569724DE67ACB7335
dates.data (full)	5F5ECC9B641C0FC5DFB226487D642BF6