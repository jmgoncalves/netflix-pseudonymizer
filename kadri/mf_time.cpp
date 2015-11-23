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

#ifndef MF_TIME_CPP
#define MF_TIME_CPP

#define MAX_FEATURES_MFTIME	10		//	Number of features to use
#define NUM_MOVIE_BINS 30			//	Number of bins for movie dates

//	Control whether features are trained simultaneously, or incrementally
#define TRAIN_SIMU_MFTIME true

/*
	This class is a modified implemention of SVD++(3), but without the |N(u)|^(-1/2)*Yi term.
	The alpha_u term is also currently commented out in predict() and multiply_simu(). It can be re-enabled by uncommenting in those two places.
	NOTE:	This class requires the running of DataBase::saveIndexes() so that the files that translate dates into indexes may be generated.
			This class does not have incremental feature training, only simultaneous.
	Example:
		db.saveIndexes();
		MF_Time *mft = new MF_Time(&db);
		mft->training();
		mft->runProbe();
		mft->runQualifying("none", true);
*/

using namespace std;
using namespace util;

class MF_Time : public Algorithm{
public:
	float** user_features;
	float** alpha_u_k;
	int num_users;
	float** movie_features;
	int num_movies;
	float* b_u;
	float** b_u_t;
	float* b_i;
	float** b_i_t;
	float** b_i_t_full;
	float* alpha_u;
	float* dev_u_scalar;
	float* dev_i_scalar;
	double* trained_cache;
	int min_epochs;
	int max_epochs;
	float min_improvement;
	float LRATE1, LRATE2, LRATE3;
	float LAMBDA1, LAMBDA2, LAMBDA3;
	int startdampen;
	float dampen;
	float* feedback_y;
	double* sum_y;
	double* temp_sum_y;
	float* gradient_sum_y;
	Globals globals;
	int numVotes; // jmgoncalves

    MF_Time(DataBase *db, string n="MF_Time") : Algorithm(db), globals(db) {
    	initFeatures(db);

		stringstream ss1;
		ss1 << MAX_FEATURES_MFTIME;
		name = n + + "_" + ss1.str();
		trained_cache = (double*) malloc(db->totalVotes()*sizeof(double));
		min_epochs = 0;
		max_epochs = 200;
		min_improvement = .000005;
		LRATE1 = .007;
		LRATE2 = .007;
		LRATE3 = .0005;
		LAMBDA1 = .005;
		LAMBDA2 = .015;
		LAMBDA3 = .03;
		startdampen = 10;
		dampen = .95;	//	Applied after epoch # startdampen

		numVotes = db->totalVotes();
	}

    // jmgoncalves
	void initFeatures(DataBase *db) {
		int rows = MAX_FEATURES_MFTIME;
		int columns = db->totalUsers();
		if(TRAIN_SIMU_MFTIME) {
			rows = db->totalUsers();
			columns = MAX_FEATURES_MFTIME;
		}

		num_users = db->totalUsers();
		b_u = (float*) malloc(db->totalUsers()*sizeof(float));
		alpha_u = (float*) malloc(db->totalUsers()*sizeof(float));
		dev_u_scalar = (float*) malloc(db->totalUsers()*sizeof(float));
		b_u_t  = (float**) malloc(db->totalUsers()*sizeof(float*));
		user_features = (float**) malloc(rows*sizeof(float*));
		alpha_u_k = (float**) malloc(rows*sizeof(float*));

		int i;
		for ( i = 0; i < rows; i++ ) {
			user_features[i] = (float*) malloc(sizeof(float)*columns);
			alpha_u_k[i] = (float*) malloc(sizeof(float)*columns);
		}

		// movies
		num_movies = db->totalMovies();
		rows = MAX_FEATURES_MFTIME;
		columns = num_movies;
		if(TRAIN_SIMU_MFB) {
			rows = num_movies;
			columns = MAX_FEATURES_MFTIME;
		}
		b_i = (float*) malloc(num_movies*sizeof(float));
		b_i_t = (float**) malloc(num_movies*sizeof(float));
		b_i_t_full = (float**) malloc(num_movies*sizeof(float));
		dev_i_scalar = (float*) malloc(num_movies*sizeof(float));
		movie_features = (float**) malloc(rows*sizeof(float*));

		for ( i = 0; i < rows; i++ ) {
			movie_features[i] = (float*) malloc(sizeof(float)*columns);
			b_i_t[i] = (float*) malloc(sizeof(float)*NUM_MOVIE_BINS);
		}
	}

	void setMovie(int movieid){
	}
	double determine(int userid){
		return 0;
	}

	void training(){
		calculate_simu();
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
				if(TRAIN_SIMU_MFTIME) val = user_features[i-1][f];
				else val = user_features[f][i-1];
				userFeaturesOut.write((char*)&val, sizeof(float));
			}
			for(int i=1; i<=num_movies; i++){
				float val;
				if(TRAIN_SIMU_MFTIME) val = movie_features[i-1][f];
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
		if(TRAIN_SIMU_MFTIME){
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
				if(TRAIN_SIMU_MFTIME) user_features[i-1][f] = val;
				else user_features[f][i-1] = val;
			}
			for(int i=1; i<=num_movies; i++){
				float val;
				movieFeaturesIn.read((char*)&val, sizeof(float));
				if(TRAIN_SIMU_MFTIME) movie_features[i-1][f] = val;
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
		globals.setAverages(2.5);
		if(!db->date_indexes_mapped){
			db->mapIndexes();	//	Make sure vote date to index files are mapped
		}
		if(!db->dates_indexed){
	//		db->indexDates(false, true);	//	Make sure the datesTables have been created
		}
		for(int f=0; f<MAX_FEATURES_MFTIME; ++f) {
			for(int i = 0; i < num_movies; ++i){
				if(TRAIN_SIMU_MFTIME) movie_features[i][f] = (rand()%14000 + 2000) * 0.000001235f;
				else movie_features[f][i] = (rand()%14000 + 2000) * 0.000001235f;
			}
			for(int i=0; i<num_users; i++){
				if(TRAIN_SIMU_MFTIME){
					user_features[i][f] = (rand()%14000 + 2000) * 0.000001235f;
				}
				else{
					user_features[f][i] = (rand()%14000 + 2000) * 0.000001235f;
				}
			}
		}
		users.setId(6);
		for(int u=0; u<num_users; u++){
			b_u[u] = 0;
			alpha_u[u] = 0;
			for(int f=0; f<MAX_FEATURES_MFTIME; ++f) {
				alpha_u_k[u][f] = 0;
			}
			b_u_t[u] = new float[users.votes()];	//	Number of items per row is the same as the number of votes per user
			for(int v=0; v<users.votes(); v++){
				b_u_t[u][v] = 0;
			}
			users.next();
		}
		for(int i = 0; i < num_movies; ++i){
			b_i[i] = 0;
			int mindex = i+1;
			b_i_t_full[i] = new float[movies.numVotes(mindex)];	//	Number of items per row is the same as the number of votes per movie
			for(int v=0; v<movies.numVotes(mindex); v++){
				b_i_t_full[i][v] = 0;
			}
		}
		for(int i=0; i<num_movies; i++){
			for(int t=0; t<NUM_MOVIE_BINS; t++){
				b_i_t[i][t] = 0;
			}
		}
		for(int i=0; i<db->totalVotes(); i++){
			trained_cache[i] = 0.0;
		}
		feedback_y = new float[num_movies];
		sum_y = new double[MAX_FEATURES_MFTIME];
		temp_sum_y = new double[MAX_FEATURES_MFTIME];
		gradient_sum_y = new float[num_movies];
		for(int f=0; f<MAX_FEATURES_MFTIME; f++){
			sum_y[f] = 0;
			temp_sum_y[f] = 0;
		}
		for(int m=0; m<num_movies; m++){
			feedback_y[m] = 0;
			gradient_sum_y[m] = 0;
		}
		buildImplicit();
		fprintf(stderr, "Setting dev values...\n");
		setDevs();
		fprintf(stderr, "Done initializing.\n");
	}

	int movieBin(int votedate){
		int newdate = votedate - MIN_DATE;
		float x = NUM_DAYS;
		float y = NUM_MOVIE_BINS;
		int binSize = ceil(x/y);
		return floor((float) newdate/binSize);
	}

	float dev(int votedate, float avg){
		return sqrtreal(votedate-avg);
	}

	void setDevs(){
		users.setId(6);
		for(int i=0; i<num_users; i++){
			int count = users.votes();
			float maxdev = -9999;
			float mindev = 9999;
			float avg = globals.userAverageDates[i];
			for(int v=0; v<count; v++){
				int votedate = users.votedate(i, v);
				float votedev = dev(votedate, avg);
				if(maxdev<votedev) maxdev=votedev;
				if(mindev>votedev) mindev=votedev;
			}
			dev_u_scalar[i] = max(abs(maxdev), abs(mindev));
			users.next();
		}
		for(int i=0; i<num_movies; i++){
			int mindex = i+1;
			int count = movies.numVotes(mindex);
			float maxdev = -9999;
			float mindev = 9999;
			float avg = globals.movieAverageDates[i];
			for(int v=0; v<count; v++){
				int votedate = movies.votedate(mindex, v);
				float votedev = dev(votedate, avg);
				if(maxdev<votedev) maxdev=votedev;
				if(mindev>votedev) mindev=votedev;
			}
			dev_i_scalar[i] = max(abs(maxdev), abs(mindev));
		}
	}

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
				float N_u = pow(implicitCounts[i], -0.5);
				float avgdate = globals.userAverageDates[i];
				for(int f=0; f<MAX_FEATURES_MFTIME; f++){
					sum_y[f] = 0;
					gradient_sum_y[f] = 0;
					//	Iterative over implicit vote movie IDs
					for(vector<uint>::iterator iter=implicitMovieIDs[i].begin(); iter!=implicitMovieIDs[i].end(); iter++){
						int movieid = *iter;
						sum_y[f] += N_u * feedback_y[movieid-1];
					}
					temp_sum_y[f] = sum_y[f];
				}
				for(int v=0; v<count; v++){
					int movieid = users.movie(v);
					float rating = users.score(v);
					int votedate = users.votedate(v);
					int userDateIndex = users.votedateIndex(v);
					//int movieDateIndex = db->movieDatesTable[movieid-1][votedate];
					int movieDateIndex = 0;
					float dev_u_hat = 0;
					if(dev_u_scalar[i]>0) dev_u_hat = dev(votedate, avgdate) / dev_u_scalar[i];	//	dev_u_scalar[i] may be zero
					int movie_bin = movieBin(votedate);
					float pred = multiply_simu(movieid, i, votedate, dev_u_hat, userDateIndex, movieDateIndex);
					err = rating - pred;
					squaredError += err*err;
					nv++;
					float bu_old = b_u[i];
					float bi_old = b_i[movieid-1];
					b_u[i] += LRATE1*(err - LAMBDA1*bu_old);
					b_i[movieid-1] += LRATE1*(err - LAMBDA1*bi_old);
					float bit_old = b_i_t[movieid-1][movie_bin];
					b_i_t[movieid-1][movie_bin] += LRATE1*(err - LAMBDA1*bit_old);
					float alpha_u_old = alpha_u[i];
					alpha_u[i] += LRATE1*(err*dev_u_hat - LAMBDA1*alpha_u_old);
					float but_old = b_u_t[i][userDateIndex];
					b_u_t[i][userDateIndex] += LRATE1*(err - LAMBDA1*but_old);
					//float bit_full_old = b_i_t_full[movieid-1][movieDateIndex];
					//b_i_t_full[movieid-1][movieDateIndex] += LRATE1*(err - LAMBDA1*bit_full_old);
					for(int f=0; f<MAX_FEATURES_MFTIME; f++){
						float uf_old = user_features[i][f];
						float mf_old = movie_features[movieid-1][f];
						float alpha_uk_old = alpha_u_k[i][f];
						float temp_sum_y_old = temp_sum_y[f];
						user_features[i][f] += LRATE2 * (err*mf_old - LAMBDA2*uf_old);
						//movie_features[movieid-1][f] += LRATE2 * (err*(uf_old+alpha_uk_old*dev_u_hat) - LAMBDA2*mf_old);
						movie_features[movieid-1][f] += LRATE2 * (err*(uf_old+alpha_uk_old*dev_u_hat+temp_sum_y_old) - LAMBDA2*mf_old);
						//alpha_u_k[i][f] += LRATE2 * (err*dev_u_hat*mf_old - LAMBDA2*alpha_uk_old);
						alpha_u_k[i][f] = LRATE2 * (err*dev_u_hat*mf_old - LAMBDA2*alpha_uk_old);
						/*if(v==count-1) alpha_u_k[i][f] = LRATE2 * (err*dev_u_hat*mf_old - LAMBDA2*alpha_uk_old);
						else alpha_u_k[i][f] = 0;*/
						temp_sum_y[f] += LRATE2 * (err*mf_old - LAMBDA2*temp_sum_y_old);
					}	//	for f<MAX_FEATURES_MFTIME
					//	Add the error for the last feature being trained
				}	//	for vote v
				//	Iterate over votes again to perform gradient step
				for(int v=0; v<count; v++){
					int movieid = users.movie(v);
					for(int f=0; f<MAX_FEATURES_MFTIME; f++){
						float feedback_y_old = feedback_y[movieid-1];
						feedback_y[movieid-1] += N_u * (temp_sum_y[f] - sum_y[f]);
					}
				}	//	for vote v
				//	Iterative over implicit votes
				for(vector<uint>::iterator iter=implicitMovieIDs[i].begin(); iter!=implicitMovieIDs[i].end(); iter++){
					int movieid = *iter;
					for(int f=0; f<MAX_FEATURES_MFTIME; f++){
						float feedback_y_old = feedback_y[movieid-1];
						feedback_y[movieid-1] += N_u * (temp_sum_y[f] - sum_y[f]);
					}
				}
				if(i>0 && i%100000==0) fprintf(stderr, "...user %d (calculate_simu) rmse: %f %.2f seconds.\n", i, sqrt(squaredError/nv), micro_time()-time_start);
				users.next();
			}	//	for user i
			rmse = sqrt(squaredError/numVotes);
			fprintf(stderr, "\tepoch: '%d' rmse: %f %0.2f seconds\n", e, rmse, micro_time()-time_start);
			if(e>=min_epochs && e%1==0){	//	Check every 2 iterations past min_epochs to see if the improvement falls below the threshold
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

	inline float multiply_simu(int movieid, int u, int votedate, float dev_u_hat, int userDateIndex, int movieDateIndex){
		int m = movieid-1;
		float sum = 0;
		sum += globals.globalAverage;
		sum += b_u[u];
		sum += b_i[m];
		int movie_bin = movieBin(votedate);
		sum += b_i_t[m][movie_bin];
	//	sum += alpha_u[u]*dev_u_hat;
		sum += b_u_t[u][userDateIndex];
	//	sum += b_i_t_full[m][movieDateIndex];
		//	Sum over trained features
		for(int f=0; f<MAX_FEATURES_MFTIME; f++){
			//sum += movie_features[movieid-1][f] * (user_features[u][f] + alpha_u_k[u][f]*dev_u_hat);
			//sum += movie_features[movieid-1][f] * (user_features[u][f] + alpha_u_k[u][f]*dev_u_hat + temp_sum_y[f]);
			sum += movie_features[movieid-1][f] * (user_features[u][f] + alpha_u_k[u][f] + temp_sum_y[f]);
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
		short userDateIndex, movieDateIndex;
		if(running_probe){
			userDateIndex = db->probe_user_date_indexes[mapcountindex];
			movieDateIndex = db->probe_movie_date_indexes[mapcountindex];
		}
		else if(running_qualifying){
			userDateIndex = db->qual_user_date_indexes[mapcountindex];
			movieDateIndex = db->qual_movie_date_indexes[mapcountindex];
		}
		else{
			try{
				userDateIndex = db->userDatesTable[u][votedate];
				movieDateIndex = db->movieDatesTable[m][votedate];
			}
			catch(...){
				fprintf(stderr, "Exception. The userDatesTable and/or movieDatesTable has not been set using DataBase::indexDates(). Exiting.\n");
				exit(1);
			}
		}
		int movie_bin = movieBin(votedate);
		float dev_u_hat = 0;
		if(dev_u_scalar[u]>0) dev_u_hat = dev(votedate, globals.userAverageDates[u]) / dev_u_scalar[u];	//	dev_u_scalar[u] may be zero
		if(TRAIN_SIMU_MFTIME) pred = multiply_simu(movieid, u, votedate, dev_u_hat, userDateIndex, movieDateIndex);
		else{
			pred += globals.globalAverage;
			pred += b_u[u];
			pred += b_i[m];
			pred += b_i_t[m][movie_bin];
	//		pred += alpha_u[u]*dev_u_hat;
			pred += b_u_t[u][userDateIndex];
	//		pred += b_i_t_full[m][movieDateIndex];
			for(int f=0; f<MAX_FEATURES_MFTIME; f++){
				//pred += movie_features[f][m] * (user_features[f][u] + alpha_u_k[f][u]*dev_u_hat);
				pred += movie_features[f][m] * (user_features[f][u] + alpha_u_k[f][u]*dev_u_hat + temp_sum_y[f]);
			}
		}
		return pred;
	}
};

#endif
