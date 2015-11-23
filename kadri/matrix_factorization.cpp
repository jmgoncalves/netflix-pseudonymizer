/**
 * Copyright (C) 2009 Saqib Kadri (kadence[at]trahald.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Source code must be provided to the author.
 */

#pragma once

#ifndef MATRIX_FACTORIZATION_CPP
#define MATRIX_FACTORIZATION_CPP

#define MAX_FEATURES    25             // Number of features to use

//	Control whether features are trained simultaneously, or incrementally
#ifndef TRAIN_SIMU
	#define TRAIN_SIMU true
#endif

/*
	Set TRAIN_SIMU to control the training method, incremental or simultaneous.
	Example:
		Matrix_Factorization *mf = new Matrix_Factorization(&db);
		mf->training();
		mf->cache("data/mf");
		mf->runProbe();
*/

using namespace std;
using namespace util;

class Matrix_Factorization : public Algorithm{
public:
    float** user_features;
    int num_users;
    float** movie_features;
    int num_movies;
	double* trained_cache;
	int min_epochs;
	int max_epochs;
	float default_val;	//	Controls the default value that features should take
	float min_improvement;
	float LRATE;
	float LAMBDA;
	int startdampen;
	float dampen;
	Globals globals;
	int numVotes; // jmgoncalves

    Matrix_Factorization(DataBase *db, string n="Matrix_Factorization") : Algorithm(db), globals(db) {
    	initFeatures(db);

		stringstream ss;
		ss << MAX_FEATURES;
		name = n + + "_" + ss.str();
		trained_cache = (double*) malloc(db->totalVotes()*sizeof(double));
		min_epochs = 0;
		max_epochs = 100;
		min_improvement = .000005;
		LRATE = .001;
		LAMBDA = .02;
		startdampen = 10;
		dampen = 1.0;	//	Applied after epoch # startdampen

		numVotes = db->totalVotes();
	}

    // jmgoncalves
    void initFeatures(DataBase *db) {
    	int rows = MAX_FEATURES;
    	int columns = db->totalUsers();
    	if(TRAIN_SIMU) {
    		rows = db->totalUsers();
    		columns = MAX_FEATURES;
    	}

    	num_users = db->totalUsers();
    	user_features = (float**) malloc(rows*sizeof(float*));

    	int i;
    	for ( i = 0; i < rows; i++ )
    		user_features[i] = (float*) malloc(sizeof(float)*columns);

    	// movies
    	num_movies = db->totalMovies();
		rows = MAX_FEATURES;
		columns = num_movies;
		if(TRAIN_SIMU) {
			rows = num_movies;
			columns = MAX_FEATURES;
		}
		movie_features = (float**) malloc(rows*sizeof(float*));

		for ( i = 0; i < rows; i++ )
			movie_features[i] = (float*) malloc(sizeof(float)*columns);
    }


	void setMovie(int movieid){
	}
	double determine(int userid){
		return 0;
	}

	void training(){
		if(TRAIN_SIMU) calculate_simu();
		else calculate_incr();
	}

	//	Cache the features to a binary file
	void cache(string fileprefix="none"){
		if(fileprefix=="none"){
			fileprefix = "data/"+name;
		}
		string usersCache = fileprefix+".users.cache";
		string moviesCache = fileprefix+".movies.cache";
		fprintf(stderr, "Caching to %s and %s...\n", usersCache.c_str(), moviesCache.c_str());
		ofstream userFeaturesOut(usersCache.c_str(), ios::binary);
		ofstream movieFeaturesOut(moviesCache.c_str(), ios::binary);
		for(int f=0; f<MAX_FEATURES; f++){
			for(int i=1; i<=num_users; i++){
				float val;
				if(TRAIN_SIMU) val = user_features[i-1][f];
				else val = user_features[f][i-1];
				userFeaturesOut.write((char*)&val, sizeof(float));
			}
			for(int i=1; i<=num_movies; i++){
				float val;
				if(TRAIN_SIMU) val = movie_features[i-1][f];
				else val = movie_features[f][i-1];
				movieFeaturesOut.write((char*)&val, sizeof(float));
			}
		}
		userFeaturesOut.close();
		movieFeaturesOut.close();
		fprintf(stderr, "Done caching.\n");
	}
	
	// required by cache_to_text
	std::string float_to_string(float f)
	{
		std::ostringstream s;
		s << f;
		return s.str();
	}
	std::string int_to_string(int f)
	{
		std::ostringstream s;
		s << f;
		return s.str();
	}

	//	cache the features to a text file - jmgoncalves
	void cache_to_text(string fileprefix="none"){
		if(fileprefix=="none"){
			fileprefix = "mf"+name;
		}
		string usersCache = fileprefix+".users.cache";
		string moviesCache = fileprefix+".movies.cache";
		fprintf(stderr, "Caching in text to %s and %s...\n", usersCache.c_str(), moviesCache.c_str());
		fprintf(stderr, "Writing %s features for %s users and %s movies...\n", int_to_string(MAX_FEATURES).c_str(), int_to_string(num_users).c_str(), int_to_string(num_movies).c_str());
		ofstream userFeaturesOut(usersCache.c_str(), ios::out);
		ofstream movieFeaturesOut(moviesCache.c_str(), ios::out);
		for(int f=0; f<MAX_FEATURES; f++){
			string fstr = int_to_string(f);
			userFeaturesOut << fstr << ": ";
			movieFeaturesOut << fstr << ": ";
			for(int i=1; i<=num_users; i++){
				float val;
				if(TRAIN_SIMU) val = user_features[i-1][f];
				else val = user_features[f][i-1];
				string valStr = float_to_string(val);
				userFeaturesOut << valStr << " ";
			}
			for(int i=1; i<=num_movies; i++){
				float val;
				if(TRAIN_SIMU) val = movie_features[i-1][f];
				else val = movie_features[f][i-1];
				string valStr = float_to_string(val);
				movieFeaturesOut << valStr << " ";
			}
		}
		userFeaturesOut.close();
		movieFeaturesOut.close();
		fprintf(stderr, "Done caching in text.\n");
	}

	//	Load cached feature values from a binary file
	void load_cache(string fileprefix="none", bool bool_simu=false){
		if(TRAIN_SIMU){
			name = name + "_Simu";
		}
		else{
			name = name + "_Incr";
		}
		if(fileprefix=="none"){
			fileprefix = "data/"+name;
		}
		setup();
		string usersCache = fileprefix+".users.cache";
		string moviesCache = fileprefix+".movies.cache";
		fprintf(stderr, "Loading cache from %s and %s...\n", usersCache.c_str(), moviesCache.c_str());
		ifstream userFeaturesIn(usersCache.c_str(), ios::binary);
		ifstream movieFeaturesIn(moviesCache.c_str(), ios::binary);
		for(int f=0; f<MAX_FEATURES; f++){
			for(int i=1; i<=num_users; i++){
				float val;
				userFeaturesIn.read((char*)&val, sizeof(float));
				if(TRAIN_SIMU) user_features[i-1][f] = val;
				else user_features[f][i-1] = val;
			}
			for(int i=1; i<=num_movies; i++){
				float val;
				movieFeaturesIn.read((char*)&val, sizeof(float));
				if(TRAIN_SIMU) movie_features[i-1][f] = val;
				else movie_features[f][i-1] = val;
			}
		}
		userFeaturesIn.close();
		movieFeaturesIn.close();
		fprintf(stderr, "Done loading cache.\n");
	}

	void setup(){
		float globalAverage = globals.getGlobalAverage();
		if(TRAIN_SIMU){
			//	If pre-processing, the global average may be negative.
			if(globalAverage>0) default_val = sqrt(globalAverage/MAX_FEATURES);
			else default_val = -1 * sqrt(abs(globalAverage)/MAX_FEATURES);
		}
		else if(db->use_preprocessor) default_val = 0.01;
		else default_val = 0.1;		
		for (int f=0; f<MAX_FEATURES; ++f) {
			for (int i = 0; i < num_movies; ++i){
				//	If training features simultaneouly, add or subtract a small random quantity
				if(TRAIN_SIMU) movie_features[i][f] = default_val + ((float) rand()/RAND_MAX - 0.5 )/500;
				else movie_features[f][i] = default_val;
			}
			for (int i=0; i<num_users; i++){
				if(TRAIN_SIMU) user_features[i][f] = default_val + ((float) rand()/RAND_MAX - 0.5 )/500;
				else user_features[f][i] = default_val;
			}
		}
		for(int i=0; i<db->totalVotes(); i++){
			trained_cache[i] = 0.0;
		}
	}

	void calculate_incr(){
		name = name + "_Incr";
		setup();
		int iteration = 0;
		double err = 0;
		double rmse = 100;
		full_output = false;
		double rmse_last = rmse;
		bool writeStats_old;
		writeStats_old = writeStats;
		writeStats = false;
		float probe_rmse = runProbe();
		float probe_rmse_last = 100;
		for(int f=0; f<MAX_FEATURES; f++){
			fprintf(stderr, "%s - Starting feature %d.\n", name.c_str(), f);
			int movieId;
			for(int e=0; e<max_epochs && probe_rmse<=probe_rmse_last-min_improvement; e++){
				double squaredError = 0;
				rmse_last = rmse;
				int cache_index = 0;
				users.setId(6);
				for(int i=0; i<num_users; i++){
					int count = users.votes();
					for(int v=0; v<count; v++){
						int movieid = users.movie(v);
						float pred = multiply_incr(movieid, i, f, trained_cache[cache_index], true);
						float rating = users.score(v);
						err = rating - pred;
						squaredError += err * err;
//						printf("%d %f %f %f %f\n", f, rating, pred, err, squaredError);
						float uf_old = user_features[f][i];
						float mf_old = movie_features[f][movieid-1];
						user_features[f][i] += LRATE*(err*mf_old - LAMBDA*uf_old);
						movie_features[f][movieid-1] += LRATE*(err*uf_old - LAMBDA*mf_old);
						cache_index++;
					}	//	for vote v
					users.next();
				}	//	for user i
				rmse = sqrt(squaredError / numVotes);
				iteration++;
				fprintf(stderr, "\titerations: '%d' epoch: '%d' training rmse: %f %0.2f seconds\n", iteration, e, rmse, micro_time()-time_start);
				if(e>=min_epochs && e%1==0){	//	Check every few iterations past min_epochs to see if the improvement falls below the threshold
					probe_rmse_last = probe_rmse;
					probe_rmse = runProbe();
					fprintf(stderr, "Probe RMSE: %f\n", probe_rmse);
				}
				if(e>=startdampen){
					LRATE *= dampen;
					fprintf(stderr, "New LRATE: %f\n", LRATE);
				}
			}	//	for e
			int cache_index = 0;
			users.setId(6);
			for (int i=0; i<num_users; i++){
				for (int v=0; v<users.votes(); ++v){
					int movieid = users.movie(v);
					trained_cache[cache_index] = multiply_incr(movieid, i, f, trained_cache[cache_index], false);
					cache_index++;
				}
				users.next();
			}
		}	//	for f<MAX_FEATURES
		full_output = true;
		writeStats = writeStats_old;
	}	//	calculate_incr()

	void calculate_simu(){
		name = name + "_Simu";
		setup();
		float err = 0;
		float rmse = 100;
		full_output = false;
		bool writeStats_old;
		writeStats_old = writeStats;
		writeStats = false;
		float rmse_last = rmse;
		float probe_rmse = runProbe();
		float probe_rmse_last = 100;
		for(int e=0; e<max_epochs && probe_rmse<=probe_rmse_last-min_improvement; e++){
			fprintf(stderr, "%s - Starting epoch %d.\n", name.c_str(), e);
			double squaredError = 0;
			rmse_last = rmse;
			int nv = 0;
			users.setId(6);
			for(int i=0; i<num_users; i++){
				int count = users.votes();
				for(int v=0; v<count; v++){
					int movieid = users.movie(v);
					float rating = users.score(v);
					//	Since training simultaneously, multiply up across all features rather than just up to f
					float pred = multiply_simu(movieid, i);
					err = rating - pred;
					squaredError += err*err;
					nv++;
					for(int f=0; f<MAX_FEATURES; f++){
						float uf_old = user_features[i][f];
						float mf_old = movie_features[movieid-1][f];
						user_features[i][f] += LRATE * (err*mf_old - LAMBDA*uf_old);
						movie_features[movieid-1][f] += LRATE * (err*uf_old - LAMBDA*mf_old);
					}	//	for f<MAX_FEATURES
					//	Add the error for the last feature being trained
				}	//	for vote v
				if(i>0 && i%100000==0) printf("...user %d (calculate_simu) rmse: %f %.2f seconds.\n", i, sqrt(squaredError/nv), micro_time()-time_start);
				users.next();
			}	//	for user i
			rmse = sqrt(squaredError/numVotes);
			fprintf(stderr, "\tepoch: '%d' rmse: %f %0.2f seconds\n", e, rmse, micro_time()-time_start);
			if(e>=min_epochs && e%1==0){	//	Check every few iterations past min_epochs to see if the improvement falls below the threshold
				probe_rmse_last = probe_rmse;
				probe_rmse = runProbe();
				fprintf(stderr, "Probe RMSE: %f\n", probe_rmse);
			}
			if(e>=startdampen){
				LRATE *= dampen;
				fprintf(stderr, "New LRATE: %f\n", LRATE);
			}
		}	//	for epoch e
		full_output = true;
		writeStats = writeStats_old;
	}	//	calculate_simu()

    inline float multiply_incr(short movieid, int u, int f, double cache_val=0, bool sum_remaining=true){
		float sum = 0;
		if(cache_val>0) sum = cache_val;	//	Replace with cached value if set
		//	Sum over current feature only
		sum += movie_features[f][movieid-1] * user_features[f][u];
		//	The remaining untrained features still have the default value. Add them here.
		if (sum_remaining) sum += (MAX_FEATURES - f - 1) * (default_val * default_val);
		//	Check bounds, since they are not checked automatically
		if(!db->use_preprocessor){
			if(sum<1) sum = 1;
			else if(sum>5) sum = 5;
		}
		return sum;
	}

	inline float multiply_simu(int movieid, int u){
		float sum = 0;
		//	Sum over trained features
		for(int f=0; f<MAX_FEATURES; f++){
			sum += user_features[u][f] * movie_features[movieid-1][f];
		}
		//	Check bounds, since they are not checked automatically
		if(!db->use_preprocessor){
			if(sum<1) sum = 1;
			else if(sum>5) sum = 5;
		}
		return sum;
	}

	float predict(int movieid, int userid, int votedate){
		float pred = 0;
		int u = db->users[userid];
		int m = movieid - 1;
		if(TRAIN_SIMU) pred = multiply_simu(movieid, u);
		else for(int f=0; f<MAX_FEATURES; f++){
			pred += movie_features[f][m] * user_features[f][u];
		}
		return pred;
	}
};

#endif
