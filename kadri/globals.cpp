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
#ifndef GLOBALS_CPP
#define GLOBALS_CPP

#include <iostream>
#include <math.h>
#include "config.h"
#include "util.cpp"
#include "algorithm.cpp"

/*
	Example:
		Globals globals(&db);
		globals.setAverages(10);
		globals.setVariances();
		globals.setThetas();
		globals.runProbe();
*/

using namespace std;
using namespace util;

class Globals : public Algorithm{
public:
	Globals(DataBase *db, string n="Globals") : Algorithm(db) {
		name = n;
		level = 10;	//	Default level
		//	Fill first date vectors with high numbers
		userFirstDates.resize(db->totalUsers());
		movieFirstDates.resize(db->totalMovies());
		userLastDates.resize(db->totalUsers());
		movieLastDates.resize(db->totalMovies());
		fill(userFirstDates.begin(), userFirstDates.end(), 999999);
		fill(movieFirstDates.begin(), movieFirstDates.end(), 999999);
		fill(userLastDates.begin(), userLastDates.end(), 0);
		fill(movieLastDates.begin(), movieLastDates.end(), 0);

		// jmgonc initialize array that is no longer in stack
		userAverageDates = (float*) malloc(db->totalUsers()*sizeof(float));
		userAverageDates = (float*) malloc(db->totalMovies()*sizeof(float));
	}

	float level;
	float globalAverage;
	float sqrtmoviecountaverage;	// The average of sqrt(movie.numVotes())
	float sqrtUserTimeUserAverage;
	float sqrtUserTimeMovieAverage;
	float sqrtMovieTimeMovieAverage;
	float sqrtMovieTimeUserAverage;
	vector<float> movieAverages;
	vector<float> userAverages;
	vector<float> movieVariances;
	vector<float> userVariances;
	vector<float> movieThetas;
	vector<float> userThetas;
	vector<float> monthThetas;
	vector<float> quarterThetas;
	vector<float> userTimeUserThetas;
	vector<float> userTimeMovieThetas;
	vector<float> movieTimeMovieThetas;
	vector<float> movieTimeUserThetas;
	vector<float> userMovieAverageThetas;
	vector<float> userMovieSupportThetas;
	vector<float> movieUserAverageThetas;
	vector<float> movieUserSupportThetas;
	vector<int> userFirstDates;
	vector<int> movieFirstDates;
	vector<int> userLastDates;
	vector<int> movieLastDates;
	float* userAverageDates;
	float* movieAverageDates;
	uint* user_first_dates;
	uint* user_last_dates;
	uint* movie_first_dates;
	uint* movie_last_dates;
	
	void setMovie(int movieid){
	}
	double determine(int userid){
		return 0;
	}

	float getGlobalAverage(){
		double globalsum = 0;
		for(int i=1; i<=db->totalMovies(); i++){
			int count = movies.numVotes(i);
			for(int j=0; j<count; j++){
				float rating = movies.rating(i, j);
				globalsum += rating;
			}
		}
		globalAverage = globalsum / db->totalVotes();
		return globalAverage;
	}

	float getMovieAverage(int mindex){
		int count = movies.numVotes(mindex);
		float sum = 0;
		for(int v=0; v<count; v++){
			float rating = movies.rating(mindex, v);
			sum += rating;
		}
		return sum / count;
	}

	float getUserAverage(int uindex){
		int count = users.numVotes(uindex);
		float sum = 0;
		for(int v=0; v<count; v++){
			float rating = users.rating(uindex, v);
			sum += rating;
		}
		return sum / count;
	}
	
	//	Level 2 or less: movieAverages and userAverages vectors
	//	Level 2.5:	movieFirstDates and userFirstDates vectors, but not the sqrt Time averages
	//	Level 3 or higher: sqrt Time averages
	bool setAverages(float setlevel=10){
		script_timer("setAverages", false);
		level = setlevel;
		stringstream ss;
		ss << level;
		name = name + "_" + ss.str();
		fprintf(stderr, "Setting global averages (level %0.1f)...\n", level);
		movieAverages.clear();
		userAverages.clear();
		float globalsum = 0;
		float sqrtmoviesum = 0;
		for(int i=1; i<=db->totalMovies(); i++){
			int count = movies.numVotes(i);
			sqrtmoviesum += sqrt(count);
			float sum = 0;
			int votedatesum = 0;
			for(int j=0; j<count; j++){
				float rating = movies.rating(i, j);
				sum += rating;
				if(level>2){
					int votedate = movies.votedate(i, j);
					votedatesum += votedate;
					if(votedate<movieFirstDates.at(i-1)) movieFirstDates.at(i-1) = votedate;
					if(votedate>movieLastDates.at(i-1)) movieLastDates.at(i-1) = votedate;
				}
			}
			float average = sum / count;
			globalsum += sum;
			movieAverages.push_back(average);
			if(level>2){
				float votedateaverage = (float) votedatesum / count;
				movieAverageDates[i-1] = votedateaverage;
			}
		}
		sqrtmoviecountaverage = sqrtmoviesum / db->totalMovies();
		globalAverage = globalsum / db->totalVotes();
		for(int i=1; i<=db->totalUsers(); i++){
			int count = users.numVotes(i);
			float sum = 0;
			int votedatesum = 0;
			for(int j=0; j<count; j++){
				float rating = users.rating(i, j);
				sum += rating;
				if(level>2){
					int votedate = users.votedate(i, j);
					votedatesum += votedate;
					if(votedate<userFirstDates.at(i-1)) userFirstDates.at(i-1) = votedate;
					if(votedate>userLastDates.at(i-1)) userLastDates.at(i-1) = votedate;
				}
			}
			float average = sum / count;
			userAverages.push_back(average);
			if(level>2){
				float votedateaverage = (float) votedatesum / count;
				userAverageDates[i-1] = votedateaverage;
			}
		}
		//	Break out of the function if the level is not 3 or higher
		if(level<3){
			fprintf(stderr, "Done setting global averages.\n");
			script_timer("setAverages", true);
			return true;
		}
		//	Iterate over again, now the min dates are set so we can calculate average time differences
		float sqrtMovieTimeMovieSum = 0;
		float sqrtMovieTimeUserSum = 0;
		for(int i=1; i<=db->totalMovies(); i++){
			int count = movies.numVotes(i);
			for(int j=0; j<count; j++){
				int votedate = movies.votedate(i, j);
				int userindex = movies.userindex(i, j);
				sqrtMovieTimeMovieSum += sqrt( votedate - movieFirstDates.at(i-1) );
				sqrtMovieTimeUserSum += sqrt( votedate - userFirstDates.at(userindex) );
			}
		}
		sqrtMovieTimeMovieAverage = sqrtMovieTimeMovieSum / db->totalVotes();
		sqrtMovieTimeUserAverage = sqrtMovieTimeUserSum / db->totalVotes();
		float sqrtUserTimeUserSum = 0;
		float sqrtUserTimeMovieSum = 0;
		for(int i=1; i<=db->totalUsers(); i++){
			int count = users.numVotes(i);
			for(int j=0; j<count; j++){
				int votedate = users.votedate(i, j);
				int movie = users.movie(i, j);
				sqrtUserTimeUserSum += sqrt( votedate - userFirstDates.at(i-1) );
				sqrtUserTimeMovieSum += sqrt(votedate - movieFirstDates.at(movie-1) );
			}
		}
		sqrtUserTimeUserAverage = sqrtUserTimeUserSum / db->totalVotes();
		sqrtUserTimeMovieAverage = sqrtUserTimeMovieSum / db->totalVotes();
		fprintf(stderr, "Done setting global averages.\n");
		fprintf(stderr, "sqrtMovieTimeMovieAverage: %f\nsqrtMovieTimeUserAverage: %f\nsqrtUserTimeUserAverage: %f\nsqrtUserTimeMovieAverage: %f\n", sqrtMovieTimeMovieAverage, sqrtMovieTimeUserAverage, sqrtUserTimeUserAverage, sqrtUserTimeMovieAverage);
		script_timer("setAverages", true);
		return true;
	}
	
	void setVariances(){
		script_timer("setVariances", false);
		movieVariances.clear();
		userVariances.clear();
		for(int i=1; i<=db->totalMovies(); i++){
			int count = movies.numVotes(i);
			float varraw = 0;
			for(int j=0; j<count; j++){
				float rating = movies.rating(i, j);
				varraw += pow(rating - movieAverages.at(i-1), 2);
			}
			float variance = varraw / (count-1);
			movieVariances.push_back(variance);
		}
		for(int i=1; i<=db->totalUsers(); i++){
			int count = users.numVotes(i);
			float varraw = 0;
			for(int j=0; j<count; j++){
				float rating = users.rating(i, j);
				varraw += pow(rating - userAverages.at(i-1), 2);
			}
			float variance = varraw / (count-1);
			userVariances.push_back(variance);
		}
		script_timer("setVariances", true);
	}

	//	Cache first and last date values to binary files
	void cacheDates(string fileprefix="data/dates"){
		string userFirstCache = fileprefix+".user.first";
		string userLastCache = fileprefix+".user.last";
		string movieFirstCache = fileprefix+".movie.first";
		string movieLastCache = fileprefix+".movie.last";
		fprintf(stderr, "Caching to %s (%d), %s (%d) ...\n", userFirstCache.c_str(), userFirstDates.size(), userLastCache.c_str(), userLastDates.size());
		ofstream userFirstOut(userFirstCache.c_str(), ios::binary);
		ofstream userLastOut(userLastCache.c_str(), ios::binary);
		for(int i=0; i<db->totalUsers(); i++){
			uint val;
			val = userFirstDates.at(i);
			userFirstOut.write((char*)&val, sizeof(uint));
			val = userLastDates.at(i);
			userLastOut.write((char*)&val, sizeof(uint));
		}
		userFirstOut.close();
		userLastOut.close();
		fprintf(stderr, "Caching to %s (%d), %s (%d) ...\n", movieFirstCache.c_str(), movieFirstDates.size(), movieLastCache.c_str(), movieLastDates.size());
		ofstream movieFirstOut(movieFirstCache.c_str(), ios::binary);
		ofstream movieLastOut(movieLastCache.c_str(), ios::binary);
		for(int i=0; i<db->totalMovies(); i++){
			uint val;
			val = movieFirstDates.at(i);
			movieFirstOut.write((char*)&val, sizeof(uint));
			val = movieLastDates.at(i);
			movieLastOut.write((char*)&val, sizeof(uint));
		}
		movieFirstOut.close();
		movieLastOut.close();
	}

	//	Load cached first and last date values from binary files
	void loadDates(string fileprefix="data/dates"){
		string userFirstCache = fileprefix+".user.first";
		string userLastCache = fileprefix+".user.last";
		string movieFirstCache = fileprefix+".movie.first";
		string movieLastCache = fileprefix+".movie.last";
		user_first_dates = mmap_file<uint>((char*)userFirstCache.c_str(), FileSize(userFirstCache), false);
		user_last_dates = mmap_file<uint>((char*)userLastCache.c_str(), FileSize(userLastCache), false);
		movie_first_dates = mmap_file<uint>((char*)movieFirstCache.c_str(), FileSize(movieFirstCache), false);
		movie_last_dates = mmap_file<uint>((char*)movieLastCache.c_str(), FileSize(movieLastCache), false);
	}
	
	bool setThetas(){
		script_timer("setThetas", false);
		movieThetas.clear();
		userThetas.clear();
		userMovieAverageThetas.clear();
		userMovieSupportThetas.clear();
		float xysum = 0;
		float xxsum = 0;
		//	Movie effect
		for(int i=1; i<=db->totalMovies(); i++){
			xysum = 0;
			xxsum = 0;
			int count = movies.numVotes(i);
			for(int j=0; j<count; j++){
				float rating = movies.rating(i, j);
				float residual = rating - globalAverage;
				xysum += residual*1;
				xxsum += 1*1;
			}
			float theta = 0;
			if(xxsum!=0) theta = xysum/xxsum;
			theta = count*theta/(count+25);
			//theta = log(count)*theta/(log(count+200));
			movieThetas.push_back(theta);
		}
		if(level<=1) return true;
		//	User effect
		for(int i=1; i<=db->totalUsers(); i++){
			xysum = 0;
			xxsum = 0;
			int count = users.numVotes(i);
			for(int j=0; j<count; j++){
				float rating = users.rating(i, j);
				int movieid = users.movie(i, j);
				float residual = rating - globalAverage - movieThetas.at(movieid-1);
				xysum += residual*1;
				xxsum += 1*1;
			}
			float theta = 0;
			if(xxsum!=0) theta = xysum/xxsum;
			theta = count*theta/(count+7);
			//theta = log(count)*theta/(log(count+43));
			userThetas.push_back(theta);
		}
		if(level<=2) return true;
		//	User*Time(user)
		for(int i=1; i<=db->totalUsers(); i++){
			xysum = 0;
			xxsum = 0;
			int count = users.numVotes(i);
			for(int j=0; j<count; j++){
				float rating = users.rating(i, j);
				int movieid = users.movie(i, j);
				int votedate = users.votedate(i, j);
				float residual = rating - globalAverage - movieThetas.at(movieid-1) - userThetas.at(i-1);
				float x = sqrt(votedate - userFirstDates.at(i-1)) - sqrtUserTimeUserAverage;
				xysum += residual*x;
				xxsum += x*x;
			}
			float theta = 0;
			if(xxsum!=0) theta = xysum/xxsum;
			theta = count*theta / (count+550);
			userTimeUserThetas.push_back(theta);
		}
		if(level<=3) return true;
		//	User*Time(movie)
		for(int i=1; i<=db->totalUsers(); i++){
			xysum = 0;
			xxsum = 0;
			int count = users.numVotes(i);
			for(int j=0; j<count; j++){
				float rating = users.rating(i, j);
				int movieid = users.movie(i, j);
				int votedate = users.votedate(i, j);
				float residual = rating - globalAverage - movieThetas.at(movieid-1) - userThetas.at(i-1) - userTimeUserThetas.at(i-1)*( sqrt(votedate-userFirstDates.at(i-1))-sqrtUserTimeUserAverage );
//				float residual = rating - globalAverage - movieThetas.at(movieid-1) - userThetas.at(i-1);
				float x = sqrt(votedate - movieFirstDates.at(movieid-1)) - sqrtUserTimeMovieAverage;
				xysum += residual*x;
				xxsum += x*x;
			}
			float theta = 0;
			if(xxsum!=0) theta = xysum/xxsum;
			theta = 0;//TEMP - this disables User*Time(movie). Comment out to enable.
			theta = count*theta / (count+150);
			userTimeMovieThetas.push_back(theta);
		}
		if(level<=4) return true;
		//	Movie*Time(movie)
		for(int i=1; i<=db->totalMovies(); i++){
			xysum = 0;
			xxsum = 0;
			int count = movies.numVotes(i);
			for(int j=0; j<count; j++){
				float rating = movies.rating(i, j);
				int userindex = movies.userindex(i, j);
				int votedate = movies.votedate(i, j);
				float residual = rating - globalAverage - movieThetas.at(i-1) - userThetas.at(userindex) - userTimeUserThetas.at(userindex)*( sqrt(votedate-userFirstDates.at(userindex))-sqrtUserTimeUserAverage ) - userTimeMovieThetas.at(userindex)*( sqrt(votedate-movieFirstDates.at(i-1))-sqrtUserTimeMovieAverage );
//				float residual = rating - globalAverage - movieThetas.at(i-1) - userThetas.at(userindex) ;
				float x = sqrt(votedate - movieFirstDates.at(i-1)) - sqrtMovieTimeMovieAverage;
				xysum += residual*x;
				xxsum += x*x;
			}
			float theta = 0;
			if(xxsum!=0) theta = xysum/xxsum;
			theta = count*theta / (count+4000);
			movieTimeMovieThetas.push_back(theta);
		}
		if(level<=5) return true;
		//	Movie*Time(user)
		for(int i=1; i<=db->totalMovies(); i++){
			xysum = 0;
			xxsum = 0;
			int count = movies.numVotes(i);
			for(int j=0; j<count; j++){
				float rating = movies.rating(i, j);
				int userindex = movies.userindex(i, j);
				int votedate = movies.votedate(i, j);
				float residual = rating - globalAverage - movieThetas.at(i-1) - userThetas.at(userindex) - userTimeUserThetas.at(userindex)*( sqrt(votedate-userFirstDates.at(userindex))-sqrtUserTimeUserAverage ) - userTimeMovieThetas.at(userindex)*( sqrt(votedate-movieFirstDates.at(i-1))-sqrtUserTimeMovieAverage ) - movieTimeMovieThetas.at(i-1)*( sqrt(votedate-movieFirstDates.at(i-1)) - sqrtMovieTimeMovieAverage );
//				float residual = rating - globalAverage - movieThetas.at(i-1) - userThetas.at(userindex);
				float x = sqrt(votedate - userFirstDates.at(userindex)) - sqrtMovieTimeUserAverage;
				xysum += residual*x;
				xxsum += x*x;
			}
			float theta = 0;
			if(xxsum!=0) theta = xysum/xxsum;
			theta = count*theta / (count+500);
			movieTimeUserThetas.push_back(theta);
		}
		if(level<=6) return true;
		//	User*Movie Average
		for(int i=1; i<=db->totalUsers(); i++){
			xysum = 0;
			xxsum = 0;
			int count = users.numVotes(i);
			for(int j=0; j<count; j++){
				float rating = users.rating(i, j);
				int movieid = users.movie(i, j);
				int votedate = users.votedate(i, j);
				float residual = rating - globalAverage - movieThetas.at(movieid-1) - userThetas.at(i-1) - userTimeUserThetas.at(i-1)*( sqrt(votedate-userFirstDates.at(i-1))-sqrtUserTimeUserAverage ) - userTimeMovieThetas.at(i-1)*( sqrt(votedate-movieFirstDates.at(movieid-1))-sqrtUserTimeMovieAverage ) - movieTimeMovieThetas.at(movieid-1)*( sqrt(votedate-movieFirstDates.at(movieid-1)) - sqrtMovieTimeMovieAverage ) - movieTimeUserThetas.at(movieid-1)*sqrt( votedate - sqrtMovieTimeUserAverage );
//				float residual = rating - globalAverage - movieThetas.at(movieid-1) - userThetas.at(i-1);
				float x = movieAverages.at(movieid-1) - globalAverage;
				xysum += residual*x;
				xxsum += x*x;
			}
			float theta = 0;
			if(xxsum!=0) theta = xysum/xxsum;
			theta = count*theta/(count+90);
			userMovieAverageThetas.push_back(theta);
		}
		if(level<=7) return true;
		//	User*Movie Support
		for(int i=1; i<=db->totalUsers(); i++){
			xysum = 0;
			xxsum = 0;
			int count = users.numVotes(i);
			for(int j=0; j<count; j++){
				float rating = users.rating(i, j);
				int movieid = users.movie(i, j);
				int votedate = users.votedate(i, j);
				float residual = rating - globalAverage - movieThetas.at(movieid-1) - userThetas.at(i-1) - userTimeUserThetas.at(i-1)*( sqrt(votedate-userFirstDates.at(i-1))-sqrtUserTimeUserAverage ) - userTimeMovieThetas.at(i-1)*( sqrt(votedate-movieFirstDates.at(movieid-1))-sqrtUserTimeMovieAverage ) - movieTimeMovieThetas.at(movieid-1)*( sqrt(votedate-movieFirstDates.at(movieid-1)) - sqrtMovieTimeMovieAverage ) - movieTimeUserThetas.at(movieid-1)*sqrt( votedate - sqrtMovieTimeUserAverage ) - userMovieAverageThetas.at(i-1)*(movieAverages.at(movieid-1)-globalAverage);
//				float residual = rating - globalAverage - movieThetas.at(movieid-1) - userThetas.at(i-1) - userMovieAverageThetas.at(i-1)*(movieAverages.at(movieid-1)-globalAverage);
				float x = sqrt(movies.numVotes(movieid)) - sqrtmoviecountaverage;
				xysum += residual*x;
				xxsum += x*x;
			}
			float theta = 0;
			if(xxsum!=0) theta = xysum/xxsum;
			theta = count*theta/(count+90);
			userMovieSupportThetas.push_back(theta);
		}
		if(level<=8) return true;
		//	Movie*User(Average)
		for(int i=1; i<=db->totalMovies(); i++){
			xysum = 0;
			xxsum = 0;
			int count = movies.numVotes(i);
			for(int j=0; j<count; j++){
				float rating = movies.rating(i, j);
				int votedate = movies.votedate(i, j);
				int userindex = movies.userindex(i, j);
				float residual = rating - globalAverage - movieThetas.at(i-1) - userThetas.at(userindex) - userTimeUserThetas.at(userindex)*( sqrt(votedate-userFirstDates.at(userindex))-sqrtUserTimeUserAverage ) - userTimeMovieThetas.at(userindex)*( sqrt(votedate-movieFirstDates.at(i-1))-sqrtUserTimeMovieAverage ) - movieTimeMovieThetas.at(i-1)*( sqrt(votedate-movieFirstDates.at(i-1)) - sqrtMovieTimeMovieAverage ) - movieTimeUserThetas.at(i-1)*( sqrt(votedate-userFirstDates.at(userindex)) - sqrtMovieTimeUserAverage ) - userMovieAverageThetas.at(userindex)*(movieAverages.at(i-1)-globalAverage) - userMovieSupportThetas.at(userindex)*(sqrt(count)-sqrtmoviecountaverage);
//				float residual = rating - globalAverage - movieThetas.at(i-1) - userThetas.at(userindex) - userMovieAverageThetas.at(userindex)*(movieAverages.at(i-1)-globalAverage) - userMovieSupportThetas.at(userindex)*(sqrt(count)-sqrtmoviecountaverage);
				float x = userAverages.at(userindex) - movieAverages.at(i-1);
				xysum += residual*x;
				xxsum += x*x;
			}
			float theta = 0;
			if(xxsum!=0) theta = xysum/xxsum;
			theta = count*theta/(count+50);
			movieUserAverageThetas.push_back(theta);
		}
		if(level<=9) return true;
		//	Movie*User(support)
		for(int i=1; i<=db->totalMovies(); i++){
			xysum = 0;
			xxsum = 0;
			int count = movies.numVotes(i);
			for(int j=0; j<count; j++){
				float rating = movies.rating(i, j);
				int votedate = movies.votedate(i, j);
				int userindex = movies.userindex(i, j);
				float residual = rating - globalAverage - movieThetas.at(i-1) - userThetas.at(userindex) - userTimeUserThetas.at(userindex)*( sqrt(votedate-userFirstDates.at(userindex))-sqrtUserTimeUserAverage ) - userTimeMovieThetas.at(userindex)*( sqrt(votedate-movieFirstDates.at(i-1))-sqrtUserTimeMovieAverage ) - movieTimeMovieThetas.at(i-1)*( sqrt(votedate-movieFirstDates.at(i-1)) - sqrtMovieTimeMovieAverage ) - movieTimeUserThetas.at(i-1)*( sqrt(votedate-userFirstDates.at(userindex)) - sqrtMovieTimeUserAverage ) - userMovieAverageThetas.at(userindex)*(movieAverages.at(i-1)-globalAverage) - userMovieSupportThetas.at(userindex)*(sqrt(count)-sqrtmoviecountaverage) - movieUserAverageThetas.at(i-1)*(userAverages.at(userindex)-movieAverages.at(i-1));
//				float residual = rating - globalAverage - movieThetas.at(i-1) - userThetas.at(userindex) - userMovieAverageThetas.at(userindex)*(movieAverages.at(i-1)-globalAverage) - userMovieSupportThetas.at(userindex)*(sqrt(count)-sqrtmoviecountaverage) - movieUserAverageThetas.at(i-1)*(userAverages.at(userindex)-movieAverages.at(i-1));
				float x = sqrt(users.numVotes(userindex+1)) - sqrtmoviecountaverage;
				xysum += residual*x;
				xxsum += x*x;
			}
			float theta = 0;
			if(xxsum!=0) theta = xysum/xxsum;
			theta = count*theta/(count+50);
			movieUserSupportThetas.push_back(theta);
		}
		script_timer("setThetas", true);
	}
	
	float predict(int movieid, int userid, int votedate){
		float pred;
		int u = db->users[userid];
		int m = movieid - 1;
		pred = globalAverage;
		if(level>=1) pred += movieThetas.at(m);
		if(level>=2) pred += userThetas.at(u);
		//	The probe date can potentially be before the first date, and can't take the square root of a negative
		if(level>=3) pred += userTimeUserThetas.at(u) * ( sqrt(max(votedate-userFirstDates.at(u),0))-sqrtUserTimeUserAverage );
		if(level>=4) pred += userTimeMovieThetas.at(u) * ( sqrt(max(votedate-movieFirstDates.at(m),0))-sqrtUserTimeMovieAverage );
		if(level>=5) pred += movieTimeMovieThetas.at(m) * ( sqrt(max(votedate-movieFirstDates.at(m),0)) - sqrtMovieTimeMovieAverage );
		if(level>=6) pred += movieTimeUserThetas.at(m) * ( sqrt(max(votedate-userFirstDates.at(u),0)) - sqrtMovieTimeUserAverage );

		if(level>=7) pred += userMovieAverageThetas.at(u) * (movieAverages.at(m) - globalAverage);
		if(level>=8) pred += userMovieSupportThetas.at(u) * (sqrt(movies.numVotes(movieid)) - sqrtmoviecountaverage);
		if(level>=9) pred += movieUserAverageThetas.at(m) * (userAverages.at(u) - movieAverages.at(m));
		if(level>=10) pred += movieUserSupportThetas.at(m) * (sqrt(users.numVotes(u+1)) - sqrtmoviecountaverage);
		return pred;
	}
};

#endif
