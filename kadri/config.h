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

using namespace std;

string configPath = "..";

//	LAPACK routines
extern "C" void sgels_ ( ... );

bool debug = false;
bool full_output = true;

//	Users are accessed from 1 to totalUsers if userStartIndex=1
int userStartIndex = 1;
//	Movies are accessed from 1 to totalMovies if movieStartIndex=1
int movieStartIndex = 1;
int voteStartIndex = 0;

float cross_val = 0.0;

//	This srand() seed value is used in Blend_Probe() and Algorithm::runProbe_partial()
int blend_probe_seed = 100;
//	Use 50% of the probe set in Algorithm::runProbe_partial()
float probe_partial_percent = 0.50;

string statsFile = "stats.txt";
bool writeStats = true;
