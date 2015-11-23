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


/*
	Instructions:
		First run a matrix_factorization model that generate a file with cached user_feature values
		Then load cache user features from that file
	Example:
		Matrix_Factorization *mf = new Matrix_Factorization(&db);
		mf->training();
		mf->cache("data/mf");					//	Generates 'mf.movies.cache' and 'mf.users.cache'
		User_KNN * uknn = new User_KNN(&db);
		uknn->setup();
		uknn->loadUserFeatures("data/mf.users.cache");
		uknn->runProbe();
*/

#pragma once
#ifndef USER_KNN_CPP
#define USER_KNN_CPP

#define MAX_FEATURES_UKNN 10
#define MAX_USER_NEIGHBORS 100

#include "knn.cpp"

using namespace std;
using namespace util;

class User_KNN : public Algorithm
{
public:
	float** user_features;
	int num_users;
	Globals globals;
	int min_user_seen_neighbors;
	User_KNN(DataBase *db, string n="User_KNN") : Algorithm(db), globals(db)
	{
		initUserFeatures(db);

		name = n;
		stringstream ss;
		ss << MAX_NEIGHBORS;
		name = n + + "_" + ss.str();
		min_user_seen_neighbors = 2;
	}

	 // jmgoncalves
	void initUserFeatures(DataBase *db) {
		num_users = db->totalUsers();
		user_features = (float**) malloc(db->totalUsers()*sizeof(float*));

		int i;
		for ( i = 0; i < db->totalUsers(); i++ )
			user_features[i] = (float*) malloc(sizeof(float)*MAX_FEATURES_UKNN);
	}

	void setMovie(int id){
		movies.setId(id);
	}
	
	double determine(int userid){
	    int movieId = movies.id();
		return 0;
	}

	float userCorr(int u1, int u2){
		float xsum = 0;
		float ysum = 0;
		float xysum = 0;
		float xxsum = 0;
		float yysum = 0;
		for(int f=0; f<MAX_FEATURES_UKNN; f++){
			float x = user_features[u1][f];
			float y = user_features[u2][f];
			xsum += x;
			xxsum += x*x;
			xysum += x*y;
			ysum += y;
			yysum += y*y;
		}
		float numerator = xysum/MAX_FEATURES_UKNN - (xsum/MAX_FEATURES_UKNN)*(ysum/MAX_FEATURES_UKNN);
		float denominator = sqrt(xxsum/MAX_FEATURES_UKNN-pow(xsum/MAX_FEATURES_UKNN, 2)) * sqrt(yysum/MAX_FEATURES_UKNN-pow(ysum/MAX_FEATURES_UKNN, 2));
		float corr = numerator / denominator;
		return corr;
	}

	void setup(){
		globals.setAverages(2);
		globals.setThetas();
	}

	float baseline(int movieid, int userid, int votedate){
		return globals.predict(movieid, userid, votedate);
	}

	float predict(int movieid, int userid, int votedate){
		int u = db->users[userid];
		int m = movieid - 1;
		float pred = 0;
		vector<nStruct> neighbors;
		for(int v=0; v<movies.numVotes(movieid); v++){
			int neighborid = movies.userid(movieid, v);
			if(neighborid!=userid){
				float rating = movies.rating(movieid, v);
				nStruct n;
				n.id = neighborid;
				n.count = 0;	//	We are not setting counts
				int u2 = db->users[neighborid];
				n.pearson = userCorr(u, u2);
				n.rating = rating;
				n.votedate = movies.votedate(movieid, v);
				n.similarity = n.pearson;	//	Since we do not know counts, use pearson as the similar measure
				neighbors.push_back(n);
			}
		}
		int size = (int) neighbors.size();
		//	Backup algorithm triggered by min_user_seen_neighbors
		if(size<min_user_seen_neighbors){
			backups++;
			pred = baseline(movieid, userid, votedate);
			return pred;
		}
		partial_sort(neighbors.begin(), neighbors.begin()+min(size, MAX_USER_NEIGHBORS), neighbors.end());
		float weightSum = 0;
		float combinationSum = 0;
		//	Iterate over MAX_USER_NEIGHBORS
		for(int i=0; i<min(size, MAX_USER_NEIGHBORS); i++){
			nStruct n = neighbors.at(i);
			int neighborid = n.id;
			float pearson = n.pearson;
			float similarity = n.similarity;
			weightSum += similarity;
			float delta = n.rating - baseline(movieid, neighborid, n.votedate);
			float score;
			if(pearson>0) score = baseline(movieid, userid, votedate) + delta;
			else score = baseline(movieid, userid, votedate) - delta;
			combinationSum += similarity*score;
		}
		pred = combinationSum / weightSum;
		return pred;
	}

	void loadUserFeatures(string filename){
		printf("Loading cached user features from %s for MAX_FEATURES_UKNN %d...\n", filename.c_str(), MAX_FEATURES_UKNN);
		ifstream userFeaturesIn(filename.c_str(), ios::binary);
		for(int f=0; f<MAX_FEATURES_UKNN; f++){
			for(int i=0; i<num_users; i++){
				float val;
				userFeaturesIn.read((char*)&val, sizeof(float));
				user_features[i][f] = val;
			}
		}
		userFeaturesIn.close();
	}
	
	~User_KNN(){}
};

#endif
