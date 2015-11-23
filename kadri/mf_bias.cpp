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

#ifndef MF_BIAS_CPP
#define MF_BIAS_CPP

#define MAX_FEATURES_MFB	100				//	Number of features to use

//	Control whether features are trained simultaneously, or incrementally
#define TRAIN_SIMU_MFB true

/*
	This class is is an implementation of matrix factorization with biases.
	Example:
		MF_Bias *mfb = new MF_Bias(&db);
		mfb->training();
		mfb->runProbe();
		mfb->runQualifying("none", true);
*/

using namespace std;
using namespace util;

class MF_Bias : public Algorithm{
public:
    float** user_features;
    int num_users;
    float** movie_features;
    int num_movies;
    float* b_u;
	float* b_i;
	double* trained_cache;
	int min_epochs;
	int max_epochs;
	float min_improvement;
	float LRATE1, LRATE2;
	float LAMBDA1, LAMBDA2;
	int startdampen;
	float dampen;
	Globals globals;
	int numVotes; // jmgoncalves

    MF_Bias(DataBase *db, string n="MF_Bias") : Algorithm(db), globals(db) {
    	initFeatures(db);

		stringstream ss1;
		ss1 << MAX_FEATURES_MFB;
		name = n + + "_" + ss1.str();
		trained_cache = (double*) malloc(db->totalVotes()*sizeof(double));
		min_epochs = 0;
		max_epochs = 200;
		min_improvement = .000005;
		LRATE1 = .007;
		LRATE2 = .007;
		LAMBDA1 = .005;
		LAMBDA2 = .015;
		startdampen = 10;
		dampen = .95;	//	Applied after epoch # startdampen

		numVotes = db->totalVotes();
	}

    // jmgoncalves
	void initFeatures(DataBase *db) {
		int rows = MAX_FEATURES_MFB;
		int columns = db->totalUsers();
		if(TRAIN_SIMU_MFB) {
			rows = db->totalUsers();
			columns = MAX_FEATURES_MFB;
		}

		num_users = db->totalUsers();
		b_u = (float*) malloc(db->totalUsers()*sizeof(float));
		user_features = (float**) malloc(rows*sizeof(float*));

		int i;
		for ( i = 0; i < rows; i++ )
			user_features[i] = (float*) malloc(sizeof(float)*columns);

		// movies
		num_movies = db->totalMovies();
		rows = MAX_FEATURES_MFB;
		columns = num_movies;
		if(TRAIN_SIMU_MFB) {
			rows = num_movies;
			columns = MAX_FEATURES_MFB;
		}
		b_i = (float*) malloc(num_movies*sizeof(float));
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
		if(TRAIN_SIMU_MFB) calculate_simu();
		else calculate_incr();
	}

	//	Cache the features to a binary file
	void cache(string fileprefix="none"){
		if(fileprefix=="none"){
			fileprefix = "data/"+name;
		}
		string usersCache = fileprefix+".users.cache";
		string moviesCache = fileprefix+".movies.cache";
		string userBiasesCache = fileprefix+".userbiases.cache";
		string movieBiasesCache = fileprefix+".moviebiases.cache";
		fprintf(stderr, "Caching to:\n%s\n%s\n%s\n%s\n\n", usersCache.c_str(), moviesCache.c_str(), userBiasesCache.c_str(), movieBiasesCache.c_str());
		ofstream userFeaturesOut(usersCache.c_str(), ios::binary);
		ofstream movieFeaturesOut(moviesCache.c_str(), ios::binary);
		ofstream userBiasesOut(userBiasesCache.c_str(), ios::binary);
		ofstream movieBiasesOut(movieBiasesCache.c_str(), ios::binary);
		for(int f=0; f<MAX_FEATURES; f++){
			for(int i=1; i<=num_users; i++){
				float val;
				if(TRAIN_SIMU_MFB) val = user_features[i-1][f];
				else val = user_features[f][i-1];
				userFeaturesOut.write((char*)&val, sizeof(float));
			}
			for(int i=1; i<=num_movies; i++){
				float val;
				if(TRAIN_SIMU_MFB) val = movie_features[i-1][f];
				else val = movie_features[f][i-1];
				movieFeaturesOut.write((char*)&val, sizeof(float));
			}
		}
		for(int i=0; i<num_users; i++){
			float val = b_u[i];
			userBiasesOut.write((char*)&val, sizeof(float));
		}
		for(int i=0; i<num_movies; i++){
			float val = b_i[i];
			movieBiasesOut.write((char*)&val, sizeof(float));
		}
		userFeaturesOut.close();
		movieFeaturesOut.close();
		userBiasesOut.close();
		movieBiasesOut.close();
		fprintf(stderr, "Done caching.\n");
	}
	
	//	Load cached feature values from a binary file
	void load_cache(string fileprefix="none", bool bool_simu=false){
		if(TRAIN_SIMU_MFB){
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
		string userBiasesCache = fileprefix+".userbiases.cache";
		string movieBiasesCache = fileprefix+".moviebiases.cache";
		fprintf(stderr, "Loading cache from:\n%s\n%s\n%s\n%s\n\n", usersCache.c_str(), moviesCache.c_str(), userBiasesCache.c_str(), movieBiasesCache.c_str());
		ifstream userFeaturesIn(usersCache.c_str(), ios::binary);
		ifstream movieFeaturesIn(moviesCache.c_str(), ios::binary);
		ifstream userBiasesIn(userBiasesCache.c_str(), ios::binary);
		ifstream movieBiasesIn(movieBiasesCache.c_str(), ios::binary);
		for(int f=0; f<MAX_FEATURES; f++){
			for(int i=1; i<=num_users; i++){
				float val;
				userFeaturesIn.read((char*)&val, sizeof(float));
				if(TRAIN_SIMU_MFB) user_features[i-1][f] = val;
				else user_features[f][i-1] = val;
			}
			for(int i=1; i<=num_movies; i++){
				float val;
				movieFeaturesIn.read((char*)&val, sizeof(float));
				if(TRAIN_SIMU_MFB) movie_features[i-1][f] = val;
				else movie_features[f][i-1] = val;
			}
		}
		for(int i=0; i<num_users; i++){
			float val;
			userBiasesIn.read((char*)&val, sizeof(float));
			b_u[i] = val;
		}
		for(int i=0; i<num_movies; i++){
			float val;
			movieBiasesIn.read((char*)&val, sizeof(float));
			b_i[i] = val;
		}
		userFeaturesIn.close();
		movieFeaturesIn.close();
		userBiasesIn.close();
		movieBiasesIn.close();
		fprintf(stderr, "Done loading cache.\n");
	}

	void setup(){
		globals.setAverages(0);
		for (int f=0; f<MAX_FEATURES_MFB; ++f) {
			for (int i = 0; i < num_movies; ++i){
				if(TRAIN_SIMU_MFB) movie_features[i][f] = (rand()%14000 + 2000) * 0.000001235f;
				else movie_features[f][i] = (rand()%14000 + 2000) * 0.000001235f;
			}
			for (int i=0; i<num_users; i++){
				if(TRAIN_SIMU_MFB){
					user_features[i][f] = (rand()%14000 + 2000) * 0.000001235f;
				}
				else{
					user_features[f][i] = (rand()%14000 + 2000) * 0.000001235f;
				}
			}
		}
		for(int u=0; u<num_users; u++){
			b_u[u] = 0;
		}
		for (int i = 0; i < num_movies; ++i){
			b_i[i] = 0;
		}
		for(int i=0; i<db->totalVotes(); i++){
			trained_cache[i] = 0.0;
		}
		fprintf(stderr, "Done initializing.\n");
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
		for(int f=0; f<MAX_FEATURES_MFB; f++){
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
						int votedate = 0;	//	Not used
						float pred = predict_incr(movieid, i, votedate, f, trained_cache[cache_index]);
						float rating = users.score(v);
						err = rating - pred;
						squaredError += err * err;
						float bu_old = b_u[i];
						float bi_old = b_i[movieid-1];
						float uf_old = user_features[f][i];
						float mf_old = movie_features[f][movieid-1];
						b_u[i] += LRATE1*(err - LAMBDA1*bu_old);
						b_i[movieid-1] += LRATE1*(err - LAMBDA1*bi_old);
						user_features[f][i] += LRATE2*(err*mf_old - LAMBDA2*uf_old);
						movie_features[f][movieid-1] += LRATE2*(err*uf_old - LAMBDA2*mf_old);
						cache_index++;
					}	//	for vote v
					users.next();
					if(i>0 && i%100000==0) fprintf(stderr, "%d %f %0.2f seconds\n", i, sqrt(squaredError/cache_index), micro_time()-time_start);
				}	//	for user i
				rmse = sqrt(squaredError / numVotes);
				iteration++;
				fprintf(stderr, "\titerations: '%d' epoch: '%d' training rmse: %f %0.2f seconds\n", iteration, e, rmse, micro_time()-time_start);
				if(e>min_epochs && e%1==0){	//	Check every few iterations past min_epochs to see if the improvement falls below the threshold
					probe_rmse_last = probe_rmse;
					probe_rmse = runProbe();
					fprintf(stderr, "Probe RMSE: %f\n", probe_rmse);
				}
				if(e>=startdampen){
					LRATE1 *= dampen;
					LRATE2 *= dampen;
					fprintf(stderr, "New LRATE1: %f\nNew LRATE2: %f\n", LRATE1, LRATE2);
				}
			}	//	for e
			int cache_index = 0;
			users.setId(6);
			for (int i=0; i<num_users; i++){
				for (int v=0; v<users.votes(); ++v){
					int movieid = users.movie(v);
					int votedate = users.votedate(v);
					trained_cache[cache_index] = predict_incr(movieid, i, votedate, f, trained_cache[cache_index]);
					cache_index++;
				}
				users.next();
			}
		}	//	for f<MAX_FEATURES_MFB
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
					int votedate = users.votedate(v);
					float pred = multiply_simu(movieid, i, votedate);
					err = rating - pred;
					squaredError += err*err;
					nv++;
					float bu_old = b_u[i];
					float bi_old = b_i[movieid-1];
					b_u[i] += LRATE1*(err - LAMBDA1*bu_old);
					b_i[movieid-1] += LRATE1*(err - LAMBDA1*bi_old);
					for(int f=0; f<MAX_FEATURES_MFB; f++){
						float uf_old = user_features[i][f];
						float mf_old = movie_features[movieid-1][f];
						user_features[i][f] += LRATE2 * (err*mf_old - LAMBDA2*uf_old);
						movie_features[movieid-1][f] += LRATE2 * (err*uf_old - LAMBDA2*mf_old);
					}	//	for f<MAX_FEATURES_MFB
					//	Add the error for the last feature being trained
				}	//	for vote v
				if(i>0 && i%100000==0) fprintf(stderr, "...user %d (calculate_simu) rmse: %f %.2f seconds.\n", i, sqrt(squaredError/nv), micro_time()-time_start);
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
				LRATE1 *= dampen;
				LRATE2 *= dampen;
				fprintf(stderr, "New LRATE1: %f\nNew LRATE2: %f\n", LRATE1, LRATE2);
			}
		}	//	for epoch e
		full_output = true;
		writeStats = writeStats_old;
	}	//	calculate_simu()

    inline float predict_incr(short movieid, int u, int votedate, int f, double cache_val=0){
		int m = movieid - 1;
		float sum = 0;
		if(cache_val>0) sum = cache_val;	//	Replace with cached value if set
		else{
			sum += globals.globalAverage;
			sum += b_u[u];
			sum += b_i[m];
		}
		//	Sum over current feature only
		sum += movie_features[f][m] * user_features[f][u];
		//	Check bounds, since they are not checked automatically
		if(!db->use_preprocessor){
			if(sum<1) sum = 1;
			else if(sum>5) sum = 5;
		}
		return sum;
	}

	inline float multiply_simu(int movieid, int u, int votedate){
		int m = movieid-1;
		float sum = 0;
		sum += globals.globalAverage;
		sum += b_u[u];
		sum += b_i[m];
		//	Sum over trained features
		for(int f=0; f<MAX_FEATURES_MFB; f++){
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
		if(TRAIN_SIMU_MFB) pred = multiply_simu(movieid, u, votedate);
		else{
			pred += globals.globalAverage;
			pred += b_u[u];
			pred += b_i[m];
			for(int f=0; f<MAX_FEATURES_MFB; f++){
				pred += movie_features[f][m] * user_features[f][u];
			}
		}
		return pred;
	}
};

#endif
