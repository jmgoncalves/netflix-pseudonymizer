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
#ifndef ALGORITH_CPP
#define ALGORITH_CPP

#include "config.h"
#include "mmap.cpp"
#include "rmse.h"

#define MAGICID 344

using namespace std;
using namespace util;

class Algorithm{
public:
	DataBase *db;
	Movie movies;
	User users;
	string name;
	//Whether to output the prediction
	bool printSubmit;
	//	Store the start time and end time for the prediction phase when doing a prediction run on a file
	double time_start;
	double time_end;
	//	Store the total predictions, number of backups (optional), and number of errors
	int totpreds;
	int backups;
	int errors;
	int fixed;
	bool running_probe;		//	Whether running runProbe() or not
	bool running_qualifying;		//	Whether running runQualifying() or not
	bool running_training;		//	Whether running runTraining() or not
	bool use_cache;			//	Whether to use the cache or not
	int mapcountindex;	//	The index we are presently at
	float* residcachemap;	//	Optional: Cached residuals in movie order
	float* squaredErrors;	//	Sum of squared errors by movie for the probe data, set by runProbe() and runProbe() partial
	float* squaredErrorCounts;
	uint* implicitCounts;	//	Contains the count of ratings, including probe and qualifying, for each user
	vector<uint>* implicitMovieIDs;	//	Contains the IDs of movies for each user from the probe and qualifying set
	bool debugging;
	Algorithm(DataBase *db) : db(db), movies(db), users(db) {
		time_start = micro_time();
		printSubmit = false;
		running_probe = false;
		running_qualifying = false;
		use_cache = false;
		totpreds = 0;
		backups = 0;
		errors = 0;
		fixed = 0;
		squaredErrors = new float[db->totalMovies()];
		squaredErrorCounts = new float[db->totalMovies()];
		for(int i=0; i<db->totalMovies(); i++){
			squaredErrors[i] = 0;
			squaredErrorCounts[i] = 0;
		}
		mapcountindex = 0;
		debugging = false;
	}
	void loadCache(string fileprefix){
		use_cache = true;
		string filename = fileprefix+".movies.resid";
		residcachemap = mmap_file<float>((char*)filename.c_str(), FileSize(filename), false);
	}
	virtual ~Algorithm()
	{}
	virtual void setMovie(int) = 0;
	virtual double determine(int) = 0;

	virtual float predict(int movieid, int useractualid, int votedate) = 0;
//	virtual float predict(int movieid, int useractualid) = 0;

	//	If outfile equals "stdout", then the output is printed to the screen.
	//	If a filename (not "none" or "stdout") is specified, then the residuals are written to the filename.
	float runProbe(string outfile="none"){
		script_timer("RunProbe", false);
		running_probe = true;
		//	Run probe
		RMSE rmse;
		ofstream out;
		if(outfile!="none" && outfile!="stdout"){
			out.open(outfile.c_str(), ios::binary);
		}
		//	The total count of probe predictions is at db->probemap[1]
		int total = db->probemap[1];
		int percentDone = 0;
		mapcountindex = 0;
		int currentMovie = -1;
		vector<float> vguesses;
		vguesses.reserve(db->probeSize);
		for(int i=2; i<db->probeSize; i++){
			//	If the current value is 0, that means that the next value contains the movie ID
			if(db->probemap[i]==0){
				currentMovie = -1;
				continue;
			}
			if(currentMovie==-1){
				currentMovie = db->probemap[i];
				if(outfile=="stdout") printf("%d:\n", currentMovie);
				continue;
			}
			//	The user is at i, the real value is at the next i
			int user = db->probemap[i];
			i++;
			int realValue = db->probemap[i];
			int votedate = db->probe_dates[mapcountindex];
//			fprintf(stderr, "DEBUG: Calling predict with values currentMovie:%d, user:%d, votedate:%d\n", currentMovie, user, votedate);
			float guess = predict(currentMovie, user, votedate);
//			fprintf(stderr, "DEBUG: Got prediction: %f\n", guess);
			totpreds++;
			//	Adjust guess is pre-processed
			if(db->use_preprocessor){
				//	Actual - residual = prediction, so to combine predictions do guess+realValue-residual=newguess+oldguess
				guess = guess + realValue - db->probeprefloatmap[mapcountindex];
			}
			if(guess<1 || guess>5){
//				fprintf(stderr, "Fixed guess of %f.\n", guess);
				fixed++;
			}
			//	Boundary check will not catch NAN's, so use _isnan()
			if(isnan(guess)){
				fprintf(stderr, "MovieID: %d UserID: %d - Error - invalid guess of %f.\n", currentMovie, user, guess);
				guess = 3.6;	//	Set to global average
				fixed++;
			}
			if(debug){
				fprintf(stderr, "totpreds: %d Guess before bounds checking: %f\n", totpreds, guess);
			}
			if(guess<1) guess = 1;
			if(guess>5) guess = 5;
			rmse.addPoint(realValue, guess);
			vguesses.push_back(guess);
			squaredErrors[currentMovie-1] += pow(realValue-guess, 2);
			squaredErrorCounts[currentMovie-1]++;
			//	If the percentDone value changed, then output an update
			int t = rmse.count() / (total / 100);
			if(t!=percentDone){
				percentDone = t;
				if(full_output) fprintf(stderr, "%d%% done after %.0f seconds RMSE: %f\n", percentDone, micro_time()-time_start, rmse.result());
			}
			//	Print the prediction to the screen
			if(outfile=="stdout"){
				printf("%.3f\n", guess);
			}
			//	Write the residual to outfile
			else if(outfile!="none"){
				float f = realValue - guess;
				out.write((char*)&f, sizeof(float));
			}
			//	Increase the index for probe_dates and probeprefloatmap
			mapcountindex++;
		}
		if(full_output){
			int size = vguesses.size();
			valarray<float> valguesses(size);
			copy(vguesses.begin(), vguesses.end(), &valguesses[0]);
			float meanguess = valguesses.sum()/size;
			float stdevguess = sqrt( pow((valguesses - meanguess), 2).sum()/ (size-1) );
			fprintf(stderr, "\nAverage prediction: %f\nSt. Dev. of predictions: %f\n", meanguess, stdevguess);
		}
		finish(rmse.result());
		if(outfile!="none"  && outfile!="stdout"){
			out.close();
			fprintf(stderr, "Wrote residuals to file %s.\n", outfile.c_str());
		}
		running_probe = false;
		script_timer("RunProbe", true);
		return rmse.result();
	}
	
	//	This function runs the probe on a random half of the probe set
	float runProbe_partial(string outfile="none"){
		script_timer("RunProbe_Partial", false);
		running_probe = true;
		RMSE rmse;
		ofstream out;
		if(outfile!="none" && outfile!="stdout"){
			out.open(outfile.c_str(), ios::binary);
		}
		//	The total count of probe predictions is at db->probemap[1]
		int total = db->probemap[1];
		int bool_algo_true_count = total * probe_partial_percent;
		bool* bool_array = (bool*) malloc(total*sizeof(bool));
		//	Fill bool_array with bool_algo_true_count "true" elements
		for(int i=0; i<total; i++){
			if(i<bool_algo_true_count) bool_array[i] = true;
			else bool_array[i] = false;
		}
		//	Shuffle bool_array
		srand(blend_probe_seed);
		random_shuffle(bool_array, bool_array+total-1);
		//	Because we increment bool_index before checking bool_array, bool_index needs to start off as -1
		int bool_index = -1;
		int percentDone = 0;
		mapcountindex = 0;
		int currentMovie = -1;
		vector<float> vguesses;
		for(int i=2; i<db->probeSize; i++){
			//	If the current value is 0, that means that the next value contains the movie ID
			if(db->probemap[i]==0){
				currentMovie = -1;
				continue;
			}
			if(currentMovie==-1){
				currentMovie = db->probemap[i];
				if(outfile=="stdout") printf("%d:\n", currentMovie);
				continue;
			}
			//	The user is at i, the real value is at the next i
			int user = db->probemap[i];
			i++;
			int realValue = db->probemap[i];
			bool_index++;
			//	After incrementing "i" and bool_index, if bool_array[bool_index] is false, then continue to the next value
			//	Note that bool_index must start off as -1, as we have already incremented it
			if(bool_array[bool_index]!=true){
				mapcountindex++;	//	Increment before continuing
				continue;
			}
			int votedate = db->probe_dates[i-2];
			float guess = predict(currentMovie, user, votedate);
			totpreds++;
			//	Adjust guess is pre-processed
			if(db->use_preprocessor){
				//	Actual - residual = prediction, so to combine predictions do guess+realValue-residual=newguess+oldguess
				guess = guess + realValue - db->probeprefloatmap[mapcountindex];
			}
			if(guess<1 | guess>5){
//				fprintf(stderr, "Error - invalid guess of %f.\n", guess);
				fixed++;
			}
			if(debug){
				fprintf(stderr, "totpreds: %d Guess before bounds checking: %f\n", totpreds, guess);
			}
			if(guess<1) guess = 1;
			if(guess>5) guess = 5;
			rmse.addPoint(realValue, guess);
			vguesses.push_back(guess);
			squaredErrors[currentMovie-1] += pow(realValue-guess, 2);
			squaredErrorCounts[currentMovie-1]++;
			//	If the percentDone value changed, then output an update
			int t = rmse.count() / (total / 100);
			if(t!=percentDone){
				percentDone = t;
				if(full_output) fprintf(stderr, "%d%% done after %.0f seconds RMSE: %f\n", percentDone, micro_time()-time_start, rmse.result());
			}
			//	Print the prediction to the screen
			if(outfile=="stdout"){
				printf("%.3f\n", guess);
			}
			//	Write the residual to outfile
			else if(outfile!="none"){
				float f = realValue - guess;
				out.write((char*)&f, sizeof(float));
			}
			//	Increase the index at which this residual will be in the probeprefloatmap
			mapcountindex++;
		}
		if(full_output){
			int size = vguesses.size();
			valarray<float> valguesses(size);
			copy(vguesses.begin(), vguesses.end(), &valguesses[0]);
			float meanguess = valguesses.sum()/size;
			float stdevguess = sqrt( pow((valguesses - meanguess), 2).sum()/ (size-1) );
			fprintf(stderr, "\nAverage prediction: %f\nSt. Dev. of predictions: %f\n", meanguess, stdevguess);
		}
		finish(rmse.result());
		if(outfile!="none"  && outfile!="stdout"){
			out.close();
			fprintf(stderr, "Wrote residuals to file %s.\n", outfile.c_str());
		}
		running_probe = false;
		script_timer("RunProbe_Partial", true);
		return rmse.result();
	}

	float runTraining(string outfile="none"){
		script_timer("RunTraining", false);
		running_training = true;
		RMSE rmse;
		ofstream out;
		if(outfile!="none"){
			out.open(outfile.c_str(), ios::binary);
		}
		int total = db->totalVotes();
		int percentDone = 0;
		mapcountindex = 0;
		vector<float> vguesses;
		for(int i=1; i<=db->totalMovies(); i++){
			int count = movies.numVotes(i);
			for(int j=0; j<count; j++){
				int userid = movies.userid(i, j);
				int votedate = movies.votedate(i, j);
				//float pred = predict(i, userid, votedate);	//	<-Is this necessary?
				int index = db->storedmovies[i-1] + j;
				int realValue = movies.rating(i, j);
				float guess = predict(i, userid, votedate);
				totpreds++;
				//	Adjust guess is pre-processed
				if(db->use_preprocessor){
					//	Actual - residual = prediction, so to combine predictions do guess+realValue-residual=newguess+oldguess
					guess = guess + realValue - db->moviesprefloatmap[mapcountindex];
				}
				if(guess<1 | guess>5){
					fixed++;
				}
				if(debug){
					fprintf(stderr, "totpreds: %d Guess before bounds checking: %f\n", totpreds, guess);
				}
				if(guess<1) guess = 1;
				if(guess>5) guess = 5;
				rmse.addPoint(realValue, guess);
				vguesses.push_back(guess);
				//	If the percentDone value changed, then output an update
				int t = rmse.count() / (total / 100);
				if(t!=percentDone){
					percentDone = t;
					if(full_output) fprintf(stderr, "%d%% done after %.0f seconds RMSE: %f\n", percentDone, micro_time()-time_start, rmse.result());
				}
				//	Print the prediction to the screen
				if(outfile=="stdout"){
					printf("%.3f\n", guess);
				}
				//	Write the prediction to outfile
				else if(outfile!="none"){
					float f = guess;
					out.write((char*)&f, sizeof(float));
				}
				//	Increase the index at which this prediction will be in the moviesprefloatmap
				mapcountindex++;
			}
		}
		if(full_output){
			int size = vguesses.size();
			valarray<float> valguesses(size);
			copy(vguesses.begin(), vguesses.end(), &valguesses[0]);
			float meanguess = valguesses.sum()/size;
			float stdevguess = sqrt( pow((valguesses - meanguess), 2).sum()/ (size-1) );
			fprintf(stderr, "\nAverage prediction: %f\nSt. Dev. of predictions: %f\n", meanguess, stdevguess);
		}
		finish(rmse.result());
		if(outfile!="none"  && outfile!="stdout"){
			out.close();
			fprintf(stderr, "Wrote predictions to file %s.\n", outfile.c_str());
		}
		running_training = false;
		script_timer("RunTraining", true);
		return rmse.result();
	}

	void runQualifying(string outfile="none", bool submit_format=true){
		script_timer("RunQualifying", false);
		running_qualifying = true;
		ofstream out;
		if(outfile!="none"){
			out.open(outfile.c_str(), ios::binary);
		}
		//	The total count of qualifying predictions is at db->qualmap[1]
		int total = db->qualmap[1];
		int currentMovie = -1;
		mapcountindex = 0;
		vector<float> vguesses;
		for(int i=2; i<db->qualSize; i++){
			//	If two consecutive qualifying.data values are 0, that means that the next value contains the movie ID
			//	qualmap[3] contains the first movie id
			if( (db->qualmap[i]==0 && db->qualmap[i-1]==0) || i==2){
				currentMovie = -1;
				continue;
			}
			if(currentMovie==-1){
				currentMovie = db->qualmap[i];
				//	Output the current movie if using the submission format
				if(submit_format) printf("%d:\n", currentMovie);
				continue;
			}
			//	Else this is where the actual rating would be in the probe.data file
			if(db->qualmap[i]==0){
				continue;
			}
			//	The user is at i, the real value is at the next i
			int user = db->qualmap[i];
			i++;
			//	For qualifying.data, realValue will simply be 0, as there is none
			int realValue = db->qualmap[i];
			int votedate = db->qual_dates[mapcountindex];
			float guess = predict(currentMovie, user, votedate);
			totpreds++;
			//	CAN'T PREPROCESS QUALIFYING
			if(guess<1 | guess>5){
//				fprintf(stderr, "Error - invalid guess of %f.\n", guess);
				fixed++;
			}
			if(debug){
				fprintf(stderr, "totpreds: %d Guess before bounds checking: %f\n", totpreds, guess);
			}
			if(guess<1) guess = 1;
			if(guess>5) guess = 5;
			vguesses.push_back(guess);
			//	Write the prediction to outfile
			if(outfile!="none"){
				float f = guess;
				out.write((char*)&f, sizeof(float));
			}
			//	Output the prediction if using the submission format
			if(submit_format) printf("%.3f\n", guess);
			//	Increase the index at which the date will be at in qual_dates
			mapcountindex++;
		}
		if(full_output){
			int size = vguesses.size();
			valarray<float> valguesses(size);
			copy(vguesses.begin(), vguesses.end(), &valguesses[0]);
			float meanguess = valguesses.sum()/size;
			float stdevguess = sqrt( pow((valguesses - meanguess), 2).sum()/ (size-1) );
			fprintf(stderr, "\nAverage prediction: %f\nSt. Dev. of predictions: %f\n", meanguess, stdevguess);
		}
		finish(100.0);
		if(outfile!="none"){
			out.close();
			fprintf(stderr, "Wrote predictions to file %s.\n", outfile.c_str());
		}
		running_qualifying = false;
		script_timer("RunQualifying", true);
	}
	
	void finish(float rmse){
		time_end = micro_time();
		fprintf(stderr, "\nRMSE of %s: %0.7f\n\n", name.c_str(), rmse);
		if(full_output) fprintf(stderr, "Total predictions: %d\nBackups: %d\nErrors: %d\nFixed: %d\n", totpreds, backups, errors, fixed);
		//	Append to statsFile
		if(writeStats==true){
			ofstream out(statsFile.c_str(), ios::out | ios::app);
			char buffer[1024];
			sprintf(buffer, "%s\t%f\t%.1f\t%d\t%d\t%d\t%d\t%d\n", name.c_str(), rmse, time_end-time_start, db->totalVotes(), totpreds, backups, errors, fixed);
			out.write(buffer, strlen(buffer));
			fprintf(stderr, "Wrote to %s as '%s'.\n", statsFile.c_str(), name.c_str());
			out.close();
		}
	}
	
	//	Note that one cannot build a preprocessor for the qualifying file, as there are no actuals
	void buildPreProcessor(string fileprefix){
		//	Build probe.resid file
		fprintf(stderr, "Building probe residuals %s file...\n", (fileprefix+".probe.resid").c_str());
		runProbe(fileprefix+".probe.resid");
		//	Build qual.pred file
		fprintf(stderr, "Building qualifying predictions %s file...\n", (fileprefix+".qual.pred").c_str());
		runQualifying(fileprefix+".qual.pred", false);
		//	Array that stores residuals, so we don't have to predict twice for movies and users
		float * residarray = (float*) malloc(db->totalVotes()*sizeof(float));
		fprintf(stderr, "Building array of all predictions...\n");
		int total = db->totalVotes();
		int done = 0;
		int percentDone = 0;
		double movies_start = micro_time();
		for(int i=1; i<=db->totalMovies(); i++){
			int count = movies.numVotes(i);
			for(int j=0; j<count; j++){
				int userid = movies.userid(i, j);
				int votedate = movies.votedate(i, j);
				float pred = predict(i, userid, votedate);
				int index = db->storedmovies[i-1] + j;
				residarray[index] = movies.rating(i, j) - pred;
				done++;
				int t = done / (total/100);
				if(t!=percentDone){
					fprintf(stderr, "%d%% done after %.0f seconds, %.0f total seconds.\n", percentDone, micro_time()-movies_start, micro_time()-time_start);
					percentDone = t;
				}
			}
		}
		//	Build movie sorted residual movies.resid file
		fprintf(stderr, "Building movie residuals %s file...\n", (fileprefix+".movies.resid").c_str());
		ofstream moviesOut((fileprefix+".movies.resid").c_str(), ios::binary);
		for(int i=0; i<db->totalVotes(); i++){
			float f = residarray[i];
			moviesOut.write((char*)&f, sizeof(float));
		}
		moviesOut.close();
		//	Build user sorted residual users.resid file
		fprintf(stderr, "Building user residuals %s file...\n", (fileprefix+".users.resid").c_str());
		double users_start = micro_time();
		ofstream usersOut((fileprefix+".users.resid").c_str(), ios::binary);
		for(int i=1; i<=db->totalUsers(); i++){
			int count = users.numVotes(i);
			for(int j=0; j<count; j++){
				int movie = users.movie(i, j);
				movies.setId(movie);
				int index = db->storedmovies[movie-1] + db->findUserAtMovie(i, movie);
				float f = residarray[index];
				usersOut.write((char*)&f, sizeof(float));
			}
		}
		fprintf(stderr, "Done building user residuals in %.0f seconds, %.0f seconds total.\n", micro_time()-users_start, micro_time()-time_start);
		usersOut.close();
		free(residarray);
	}

	//	Fille implicitCounts and implicitMovieIDs
	void buildImplicit(){
		implicitCounts = new uint[db->totalUsers()];
		implicitMovieIDs = new vector<uint>[db->totalUsers()];
		for(int i=0; i<db->totalUsers(); i++){
			implicitCounts[i] = users.numVotes(i+1);
			implicitMovieIDs[i].clear();
		}
		for(int i=0; i<db->probeMapSize; i++){
			int user = db->probe_users[i];
			int u = db->users[user];
			implicitCounts[u]++;
			implicitMovieIDs[u].push_back(db->probe_movies[i]);
		}
		//	The total count of qualifying predictions is at db->qualmap[1]
		for(int i=0; i<db->qualMapSize; i++){
			int user = db->qual_users[i];
			int u = db->users[user];
			implicitCounts[u]++;
			implicitMovieIDs[u].push_back(db->qual_movies[i]);
		}
		/*
		int total = db->qualmap[1];
		int currentMovie = -1;
		for(int i=2; i<db->qualSize; i++){
			//	If two consecutive qualifying.data values are 0, that means that the next value contains the movie ID
			//	qualmap[3] contains the first movie id
			if( (db->qualmap[i]==0 && db->qualmap[i-1]==0) || i==2){
				currentMovie = -1;
				continue;
			}
			if(currentMovie==-1){
				currentMovie = db->qualmap[i];
				continue;
			}
			//	Else this is where the actual rating would be in the probe.data file
			if(db->qualmap[i]==0){
				continue;
			}
			//	The user is at i, the real value is at the next i
			int user = db->qualmap[i];
			int u = db->users[user];
			implicitCounts[u]++;
			i++;
		}
		*/
	}

	//	Output movies sorted by either rmse or squared error
	void printErrors(bool rmse=false){
		vector<Element> errors;
		for(int i=0; i<db->totalMovies(); i++){
			int movieid = i+1;
			float val = squaredErrors[i];
			if(rmse){
				//	Not all movies will have probe votes, so set their RMSE to 0
				if(squaredErrorCounts[i]>0) val = sqrt(val / squaredErrorCounts[i]);
				else val = 0;
			}
			errors.push_back(Element(movieid, val));
		}
		sort(errors.begin(), errors.end());
		reverse(errors.begin(), errors.end());
		fprintf(stderr, "\nMovies sorted by probe error\n");
		if(rmse) fprintf(stderr, "MovieID\tTitle\tRMSE\n");
		else fprintf(stderr, "MovieID\tTitle\tSquaredError\n");
		for(int i=0; i<db->totalMovies(); i++){
			int movieid = errors.at(i).key;
			fprintf(stderr, "%d\t%s\t%f\n", movieid, movies.title(movieid).c_str(), errors.at(i).value);
		}
	}
};

#endif
