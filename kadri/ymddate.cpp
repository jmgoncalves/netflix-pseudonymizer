/**
 * Copyright (C) 2009 Saqib Kadri (kadence[at]trahald.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the packaged disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the packaged disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Source code must be provided to the author.
 */

#pragma once

#ifndef YMDDATE_CPP
#define YMDDATE_CPP

#include <iostream>
#ifndef UTIL_CPP
	#include "util.cpp"
#endif

using namespace std;
using namespace util;

//	Earliest vote date for training set is 11/11/99, 10906 unix days
//	Earliest vote date for probe and qualifying sets is 1-6-2000, 10962 unix days
//	Latest vote date on all sets is 12/31/05, 13148 unix days

time_t make_day(int year, int month, int day){
	struct tm timeinfo;		//	See http://www.cplusplus.com/reference/clibrary/ctime/tm.html
	timeinfo.tm_year = year - 1900;
	timeinfo.tm_mon = month - 1;	//	Struct tm months are from 0 to 11
	timeinfo.tm_mday = day;
	timeinfo.tm_hour = 0;
	timeinfo.tm_min = 0;
	timeinfo.tm_sec = 0;
	return mktime(&timeinfo);
}

//	The month number - number of months elapsed, ranging from 0-73
inline int calc_monthnum(int unix_days, bool quarter=false){
	time_t monthstart;
	if(quarter) monthstart = make_day(1999, 10, 1);
	else monthstart = make_day(1999, 11, 1);
	struct tm *monthstartinfo = gmtime(&monthstart);
	int startyear = monthstartinfo->tm_year;
	int startmonth = monthstartinfo->tm_mon;
	time_t month = unix_days*60*60*24;
	struct tm *monthinfo = gmtime(&month);
	int endyear = monthinfo->tm_year;
	int endmonth = monthinfo->tm_mon;
	int monthnum = (endyear - startyear)*12 + endmonth - startmonth;
	return monthnum;
}

//	The quarter number - number of yearly quarters elapsed, ranging from 0-24
//	It makes a *big* difference if this is inline or not - 300 in 25 seconds or 2000 in 3.5 seconds
inline int calc_quarternum(int unix_days){
	return calc_monthnum(unix_days, true) / 3;
}

class YMDDATE{
public:
	time_t rawtime;
	YMDDATE(int year, int month, int day){
		rawtime = make_day(year, month, day);
	}
	YMDDATE(string s){
		int year = atoi(s.substr(0, 4).c_str());
		int month = atoi(s.substr(5, 2).c_str());
		int day = atoi(s.substr(8, 2).c_str());
		rawtime = make_day(year, month, day);
	}
	YMDDATE(int unixdays){
		rawtime = unixdays*60*60*24;
	}
	//	Return the number of days since January 1, 1970
	int unix_days(){
		return rawtime / (60*60*24);
	}
	static int date_diff(YMDDATE ymd1, YMDDATE ymd2){
		int secdiff = ymd1.rawtime - ymd2.rawtime;
		return secdiff / (60*60*24);
	}
	string toString(){
		tm  *t = localtime(&rawtime);
		char arr[10];
		sprintf(arr, "%d-%d-%d", t->tm_year+1900, t->tm_mon+1, t->tm_mday);
		string retval(arr);
		return retval;
	}
};

#endif
