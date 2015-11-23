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

#ifndef MATRIX_FACTORIZATION_PP_CPP
#define MATRIX_FACTORIZATION_PP_CPP

#define MAX_FEATURES_PP    10             // Number of features to use

//	Set to true for Matrix_Factorization_PP. Tells caching how matrices are set up.
#ifndef TRAIN_SIMU
	#define TRAIN_SIMU true
#endif

/*
	NOTE:	This class is currently faulty. It breaks with initial values not set to zero, and trains very slowly.
	Example:
		Matrix_Factorization_PP * mfpp = new Matrix_Factorization_PP(&db);
		mfpp->training();
		mfpp->cache("data/mfpp");
		mfpp->runProbe();
*/

using namespace std;
using namespace util;

class Matrix_Factorization_PP : public Algorithm{
public:
	float** user_features;
	int num_users;
    float** movie_features;
    int num_movies;
	float* feedback_y;
	float* user_biases;
	float* movie_biases;
	double* sum_y;
	float* gradient_sum_y;
	int min_epochs;
	int max_epochs;
	float default_val;	//	Controls the default value that features should take
	float min_improvement;
	float LRATE1, LRATE2;
	float LAMBDA1, LAMBDA2;
	Globals globals;
	bool is_training;	//	Whether we are in the training phase. Affects how predictions are calculated.
	int trainingUser;
	int numVotes; // jmgoncalves

    Matrix_Factorization_PP(DataBase *db, string n="Matrix_Factorization_PP") : Algorithm(db), globals(db) {
    	initFeatures(db);

    	stringstream ss;
		ss << MAX_FEATURES_PP;
		name = n + + "_" + ss.str();
		sum_y = (double*) malloc(db->totalVotes()*sizeof(double));
		min_epochs = 0;
		max_epochs = 30;
		min_improvement = .000005;
		LRATE1 = .007;
		LRATE2 = .007;
		LAMBDA1 = .005;
		LAMBDA2 = .015;

		numVotes = db->totalVotes();
	}

    // jmgoncalves
	void initFeatures(DataBase *db) {
		num_users = db->totalUsers();
		user_features = (float**) malloc(db->totalUsers()*sizeof(float*));

		int i;
		for ( i = 0; i < db->totalUsers(); i++ )
			user_features[i] = (float*) malloc(sizeof(float)*MAX_FEATURES_PP);

		num_movies = db->totalMovies();
		movie_features = (float**) malloc(num_movies*sizeof(float*));

		for ( i = 0; i < num_movies; i++ )
			movie_features[i] = (float*) malloc(sizeof(float)*MAX_FEATURES_PP);
	}

	void setMovie(int movieid){
	}
	double determine(int userid){
		return 0;
	}

	void training(){
		calculate();
	}
	void setup(){
		globals.setAverages(0);
		sum_y = new double[MAX_FEATURES_PP];
		user_biases = new float[num_users];
		movie_biases = new float[num_movies];
		feedback_y = new float[num_movies];
		gradient_sum_y = new float[num_movies];
		float divisor = 50;	//	For MAX_FEATURES_PP=1, the initial values will be between -.01,.01 with divisor 50
		divisor = divisor*MAX_FEATURES_PP*MAX_FEATURES_PP;	//	The more the features, the smaller the initial values
		for(int f=0; f<MAX_FEATURES_PP; f++){
			sum_y[f] = 0;
		}
		for(int u=0; u<num_users; u++){
			//user_biases[u] = globals.getUserAverage(u+1);	//	User offset
			user_biases[u] = 0;
			for(int f=0; f<MAX_FEATURES_PP; f++){
				//user_features[u][f] = ((float)rand()/RAND_MAX - .5) / divisor;
				user_features[u][f] = (rand()%14000 + 2000) * 0.000001235f;
				//user_features[u][f] = 0;
			}
		}
		for(int m=0; m<num_movies; m++){
			//movie_biases[m] = globals.getMovieAverages(m+1);	//	Movie offset
			movie_biases[m] = 0;
			//feedback_y[m] = ((float)rand()/RAND_MAX - .5) / divisor;
			feedback_y[m] = 0;
			gradient_sum_y[m] = 0;
			for(int f=0; f<MAX_FEATURES_PP; f++){
				//movie_features[m][f] = ((float)rand()/RAND_MAX - .5) / divisor;
				movie_features[m][f] = (rand()%14000 + 2000) * -0.000001235f;
				//movie_features[m][f] = 0;
			}
		}
	}

	void calculate(){
		is_training = true;
		setup();
		buildImplicit();
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
				trainingUser = i;
				int count = users.votes();
				float implicit_pow = pow(implicitCounts[i], -0.5);
				for(int f=0; f<MAX_FEATURES_PP; f++){
					sum_y[f] = 0;
					gradient_sum_y[f] = 0;
					for(int v=0; v<count; v++){
						int movieid = users.movie(v);
						sum_y[f] += implicit_pow * feedback_y[movieid-1];
					}
					//	Iterative over implicit vote movie IDs
					for(vector<uint>::iterator iter=implicitMovieIDs[i].begin(); iter!=implicitMovieIDs[i].end(); iter++){
						int movieid = *iter;
						sum_y[f] += implicit_pow * feedback_y[movieid-1];
					}
				}	//	for feature f
				for(int v=0; v<count; v++){
					int movieid = users.movie(v);
					float rating = users.score(v);
					int votedate = 0;	//	Not using dates here
					float pred = predict(movieid, users.id(), votedate);
					float err = rating - pred;
					squaredError += err*err;
					nv++;
					//	Train biases
					float ubias_old = user_biases[i];
					float mbias_old = movie_biases[movieid-1];
					user_biases[i] += LRATE1*(err - LAMBDA1*ubias_old);
					movie_biases[movieid-1] += LRATE1*(err - LAMBDA1*mbias_old);
					//	Train movie and user features
					for(int f=0; f<MAX_FEATURES_PP; f++){
						float uf_old = user_features[i][f];
						float mf_old = movie_features[movieid-1][f];
						user_features[i][f] += LRATE2*(err*mf_old - LAMBDA2*uf_old);
						movie_features[movieid-1][f] += LRATE2*(err*(uf_old+sum_y[f]+gradient_sum_y[f]) - LAMBDA2*mf_old);
						float ts_old = gradient_sum_y[f];
						//	The gradient stores what feedback_y[movieid-1] would be altered by
						//gradient_sum_y[f] += LRATE2*(err*implicit_pow*mf_old - LAMBDA2*ts_old);
						gradient_sum_y[f] += err*implicit_pow*mf_old;
					}
				}	//	for vote v
				
				//	Iterate over votes again to perform gradient step
				for(int v=0; v<count; v++){
					int movieid = users.movie(v);
					for(int f=0; f<MAX_FEATURES_PP; f++){
						float feedback_y_old = feedback_y[movieid-1];
						//feedback_y[movieid-1] += gradient_sum_y[f];
						feedback_y[movieid-1] += LRATE2*(gradient_sum_y[f] - LAMBDA2*feedback_y_old);
					}
				}	//	for vote v
				//	Iterative over implicit votes
				for(vector<uint>::iterator iter=implicitMovieIDs[i].begin(); iter!=implicitMovieIDs[i].end(); iter++){
					int movieid = *iter;
					for(int f=0; f<MAX_FEATURES_PP; f++){
						float feedback_y_old = feedback_y[movieid-1];
						//feedback_y[movieid-1] += gradient_sum_y[f];
						feedback_y[movieid-1] += LRATE2*(gradient_sum_y[f] - LAMBDA2*feedback_y_old);
					}
				}
				
				if(i>0 && i%100000==0) fprintf(stderr, "...user %d training rmse: %f %0.2f seconds\n", i, sqrt(squaredError/nv), micro_time()-time_start);
				users.next();
			}	//	for user i
			rmse = sqrt(squaredError/numVotes);
			fprintf(stderr, "\tepoch: '%d' rmse: %f %0.2f seconds\n", e, rmse, micro_time()-time_start);
			if(e>(min_epochs-1) && e%2==0){	//	Check every 2 iterations past min_epochs to see if the improvement falls below the threshold
				probe_rmse_last = probe_rmse;
				probe_rmse = runProbe();
			}
			LRATE1 = LRATE1 * 0.95f;	//	Dampen the learning rate
			LRATE2 = LRATE2 * 0.95f;
			fprintf(stderr, "New LRATE1: %f\nNew LRATE2: %f\n", LRATE1, LRATE2);
		}	//	for epoch e
		full_output = true;
		writeStats = writeStats_old;
		is_training = false;
	}	//	calculate_simu()

	float multiply(int m, int u){
		float sum = 0;
		if(is_training){
			for(int f=0; f<MAX_FEATURES_PP; f++){
				//sum += movie_features[m][f] * (user_features[u][f] + sum_y[f] + gradient_sum_y[f]);
				sum += movie_features[m][f] * (user_features[u][f] + sum_y[f]);
				if(isnan(sum)) printf("%d %d %f %f %f %f\n", m, u, movie_features[m][f], user_features[u][f], sum_y[f], gradient_sum_y[f]);
			}
		}
		else{
			//	predict() will do users.setId()
			float feedback_sum = 0;
			for(int f=0; f<MAX_FEATURES_PP; f++){
				for(int v=0; v<users.votes(); v++){
					int movieid = users.movie(v);
					feedback_sum += feedback_y[movieid-1];
				}
				for(vector<uint>::iterator iter=implicitMovieIDs[u].begin(); iter!=implicitMovieIDs[u].end(); iter++){
					int movieid = *iter;
					feedback_sum += feedback_y[movieid-1];
				}
				sum += movie_features[m][f] * (user_features[u][f] + pow(implicitCounts[u], -0.5)*feedback_sum);
			}
		}
		return sum;
	}

	float predict(int movieid, int userid, int votedate){
		float pred = globals.globalAverage;
		int u;
		u = db->users[userid];
		int m = movieid - 1;
		pred += user_biases[u];
		pred += movie_biases[m];
		users.setId(userid);	//	Needed by multiply()
		pred += multiply(m, u);
		if(!db->use_preprocessor){
			if(pred<1) pred = 1;
			else if(pred>5) pred = 5;
		}
		return pred;
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
		string feedbackCache = fileprefix+".feedback.cache";
		fprintf(stderr, "Caching to:\n%s\n%s\n%s\n%s\n%s\n\n", usersCache.c_str(), moviesCache.c_str(), userBiasesCache.c_str(), movieBiasesCache.c_str(), feedbackCache.c_str());
		ofstream userFeaturesOut(usersCache.c_str(), ios::binary);
		ofstream movieFeaturesOut(moviesCache.c_str(), ios::binary);
		ofstream userBiasesOut(userBiasesCache.c_str(), ios::binary);
		ofstream movieBiasesOut(movieBiasesCache.c_str(), ios::binary);
		ofstream feedbackOut(feedbackCache.c_str(), ios::binary);
		for(int f=0; f<MAX_FEATURES_PP; f++){
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
		for(int i=0; i<num_users; i++){
			float val = user_biases[i];
			userBiasesOut.write((char*)&val, sizeof(float));
		}
		for(int i=0; i<num_movies; i++){
			float val = movie_biases[i];
			movieBiasesOut.write((char*)&val, sizeof(float));
			val = feedback_y[i];
			feedbackOut.write((char*)&val, sizeof(float));
		}
		userFeaturesOut.close();
		movieFeaturesOut.close();
		userBiasesOut.close();
		movieBiasesOut.close();
		feedbackOut.close();
		fprintf(stderr, "Done caching.\n");
	}
	
	//	Load cached feature values from a binary file
	void load_cache(string fileprefix="none", bool bool_simu=false){
		if(bool_simu==true){
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
		string feedbackCache = fileprefix+".feedback.cache";
		fprintf(stderr, "Loading cache from:\n%s\n%s\n%s\n%s\n%s\n\n", usersCache.c_str(), moviesCache.c_str(), userBiasesCache.c_str(), movieBiasesCache.c_str(), feedbackCache.c_str());
		ifstream userFeaturesIn(usersCache.c_str(), ios::binary);
		ifstream movieFeaturesIn(moviesCache.c_str(), ios::binary);
		ifstream userBiasesIn(userBiasesCache.c_str(), ios::binary);
		ifstream movieBiasesIn(movieBiasesCache.c_str(), ios::binary);
		ifstream feedbackIn(feedbackCache.c_str(), ios::binary);
		for(int f=0; f<MAX_FEATURES_PP; f++){
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
		for(int i=0; i<num_users; i++){
			float val;
			userBiasesIn.read((char*)&val, sizeof(float));
			user_biases[i] = val;
		}
		for(int i=0; i<num_movies; i++){
			float val;
			movieBiasesIn.read((char*)&val, sizeof(float));
			movie_biases[i] = val;
			feedbackIn.read((char*)&val, sizeof(float));
			feedback_y[i] = val;
		}
		userFeaturesIn.close();
		movieFeaturesIn.close();
		userBiasesIn.close();
		movieBiasesIn.close();
		feedbackIn.close();
		fprintf(stderr, "Done loading cache.\n");
	}
};

#endif
