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
#ifndef AVERAGE_CPP
#define AVERAGE_CPP

#include <iostream>
#include <math.h>
#include "config.h"
#include "util.cpp"
#include "algorithm.cpp"

/*
	Example:
		Average avg(&db);
		avg.runProbe();
*/

using namespace std;
using namespace util;

class Average : public Algorithm{
public:
	Globals globals;
	Average(DataBase *db, string n="Average") : Algorithm(db), globals(db) {
		name = n;
		globals.getGlobalAverage();
	}
	
	void setMovie(int movieid){
	}
	double determine(int userid){
		return 0;
	}

	float predict(int movieid, int userid, int votedate){
		float pred;
		int u = db->users[userid];
		int m = movieid - 1;
		pred = globals.globalAverage;
		return pred;
	}
};

#endif
