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
#ifndef KNN_CPP
#define KNN_CPP

/*
	To run this class, set the options below (MAX_NEIGHBORS, PEARSON_FILE_KNN, etc.), call setup() and runProbe() or runQualifying() as normal.
	Example:
		KNN knn(&db);
		knn.setup();
		knn.runProbe();
		knn.runQualifying("none", true);
*/

#include <iostream>
#include "config.h"
#include "util.cpp"
#include "algorithm.cpp"
#include "user.h"

#include <vector>
#include <valarray>
#include <math.h>
#include <map>
#include <fstream>
#include <ctime>

#ifdef IS_OS_WIN
#include "winmmap.h"
#else
#include <sys/mman.h>
#endif

using namespace std;

#define PEARSON_FILE_KNN "data/pearson.data"
#define COUNT_FILE_KNN "data/count.data"
bool MMAP_PEARSON = true;
bool MMAP_COUNT = true;
int MAX_NEIGHBORS = 30;
int MIN_COMMON_VIEWERS = 24;
int MIN_SEEN_NEIGHBORS = 2;
bool POSITIVE_ONLY = false;

struct simStats{
	vector<float> simVotesI;
	vector<float> simVotesJ;
};

struct nStruct{
	int id;
	int count;
	float rating;
	int votedate;
	float pearson;
	float similarity;
	nStruct(){}
	nStruct(int ID, float s, float r=0, float p=0, int c=0,  int date=0){
		this->id = ID;
		this->count = c;
		this->rating = r;
		this->votedate = date;
		this->pearson = p;
		this->similarity = s;
	}
	//	Sort descending order by similarity
	bool operator<(const nStruct& rhs) const {
		float fixed = similarity;
		float rhsfixed = rhs.similarity;
		if(isnan(fixed) | isinf(fixed)) fixed = 0;
		if(isnan(rhsfixed) | isinf(rhsfixed)) rhsfixed = 0;
		return fixed > rhsfixed;
	}
};

float pearson(vector<float> vi, vector<float> vj){
	//	Input vectors must be the same size
	if(vi.size()!=vj.size()) return false;
	int size = vi.size();
	float xsum = 0;
	float ysum = 0;
	float xysum = 0;
	float xxsum = 0;
	float yysum = 0;
	for(int f=0; f<size; f++){
		float x = vi.at(f);
		float y = vj.at(f);
		xsum += x;
		xxsum += x*x;
		xysum += x*y;
		ysum += y;
		yysum += y*y;
	}
	float numerator = xysum/size - (xsum/size)*(ysum/size);
	float denominator = sqrt(xxsum/size-pow(xsum/size, 2)) * sqrt(yysum/size-pow(ysum/size, 2));
	float corr = numerator / denominator;
	return corr;
}

float fisher(float r, int n, float zscore, bool isLower=true){
	float z = .5 * log( (1+r) / (1-r));
	float diff = zscore / sqrt(n-3);
	if(isLower) diff = diff * -1;
	return z + diff;
}

float fisher_inv(float z){
	return (exp(2*z) - 1) / (exp(2*z) + 1);
}

float calcSim(float pearson, int count){
	//	A pearson 1 1 will give an error in fisher()
	if(pearson>=.999) pearson=.999;
	float fisher_lower;
	if(pearson>0) fisher_lower = fisher_inv(fisher(pearson, count, 1.96));
	else fisher_lower = fisher_inv(fisher(pearson, count, -1.96));
	//	Check to see if the sign of pearson was changed; if so, set to 0
	if(pearson>=0 && fisher_lower<0) fisher_lower = 0;
	if(pearson<=0 && fisher_lower>0) fisher_lower = 0;
	return fisher_lower * fisher_lower / (1 - fisher_lower * fisher_lower) * log(count);
}

float get_float(ifstream &in, int num){
	&in.seekg(num*sizeof(int), ios_base::beg);
	float value;
	&in.read((char*)&value, sizeof(float));
	return value;
}

int get_int(ifstream &in, int num){
	&in.seekg(num*sizeof(int), ios_base::beg);
	int value;
	&in.read((char*)&value, sizeof(int));
	return value;
}

class KNN : public Algorithm
{
public:
	KNN(DataBase *db, string n="KNN") : Algorithm(db), globals(db), currentMovie(db), currentUser(db), doPearson(false), doCount(false), totalMovies(db->totalMovies()), totalUsers(db->totalUsers())
	{
		name = n;
	}

	void setMovie(int id){
		currentMovie.setId(id);
	}
	
	double determine(int userid){
	    int movieId = currentMovie.id();
		return 0;
	}

	void setup(){
		time_t start_time = time(NULL);
		globals.setAverages();
		globals.setVariances();
		globals.setThetas();
		ifstream pearsonIn(PEARSON_FILE_KNN, ios::binary);
		ifstream countIn(COUNT_FILE_KNN, ios::binary);
		if(pearsonIn.fail()){
			fprintf(stderr, "Generating pearson file...\n");
			doPearson = true;
		}
		if(countIn.fail()){
			fprintf(stderr, "Generating count file...\n");
			doCount = true;
		}
		if(doPearson || doCount){
			ofstream pearsonOut(PEARSON_FILE_KNN, ios::binary);
			ofstream countOut(COUNT_FILE_KNN, ios::binary);
			generatePearson(pearsonOut, countOut);
			pearsonOut.close();
			countOut.close();
			fprintf(stderr, "\n...Done Writing pearson file.\n");
			pearsonIn.open(PEARSON_FILE_KNN, ios::binary);
			countIn.open(COUNT_FILE_KNN, ios::binary);
			int dur = time(NULL) - start_time;
			fprintf(stderr, "Creation of .data files took: %d seconds\n", dur);
		}
		pearsonIn.close();
		countIn.close();
		if(MMAP_PEARSON){
			fprintf(stderr, "Attempting to mmap %s...", PEARSON_FILE_KNN);
			int pearsonIn = open(PEARSON_FILE_KNN, O_RDWR, (mode_t)0644);
			if(pearsonIn){
				pearsonMap = (float*) mmap(0, FileSize(PEARSON_FILE_KNN), PROT_READ, MAP_SHARED, pearsonIn, (off_t)0);
				if(pearsonMap==(float*) - 1){
					MMAP_PEARSON = false;
				}
			}
			//	Else there was an error
			else MMAP_PEARSON = false;
			if(MMAP_PEARSON) fprintf(stderr, "Success.\n");
			else fprintf(stderr, "Failure.\n");
		}
		if(MMAP_COUNT){
			fprintf(stderr, "Attempting to mmap %s...", COUNT_FILE_KNN);
			int countIn = open(COUNT_FILE_KNN, O_RDWR, (mode_t)0644);
			if(countIn){
				countMap = (int*) mmap(0, FileSize(COUNT_FILE_KNN), PROT_READ, MAP_SHARED, countIn, (off_t)0);
				if(countMap==(int*) - 1){
					MMAP_COUNT = false;
				}
			}
			//	Else there was an error
			else MMAP_COUNT = false;
			if(MMAP_COUNT) fprintf(stderr, "Success.\n");
			else fprintf(stderr, "Failure.\n");
		}
		setMovieAverages();
		setUserAverages();
	}

	//	Note: movieAverages and userAverages must be set before running this
	float predict(int movieId, int user, int votedate){
		//	Check for caching
		if(use_cache==true && running_probe==false){
			int u = db->users[user];
			int user_index = db->findUserAtMovie(u+1, movieId);
			int index = db->storedmovies[movieId-1] + user_index;
			return currentMovie.rating(movieId, user_index) - residcachemap[index];
		}
		User currentUser(currentMovie.dataBase());
		currentUser.setId(user);
		int totalUserVotes = currentUser.votes();
//		fprintf(stderr, "MovieId: %d\tUserId: %d\n", movieId, user);
		ifstream pearsonIn(PEARSON_FILE_KNN, ios::binary);
		ifstream countIn(COUNT_FILE_KNN, ios::binary);
		vector<nStruct> v;
		//	For each movie seen, add the neighbor
		for(int j=0; j<totalUserVotes; j++){
			int neighborid = currentUser.movie(j);
			if(movieId!=neighborid){
				float score = currentUser.score(j);
				int itemNum;
				//	If movieId>neighborid, use the mirror matrix position. Adjust itemNum according to Gaussian sequence.
				if(movieId<=neighborid) itemNum = (movieId-1)*totalMovies + (neighborid-1) - (movieId-1)*movieId/2;
				else itemNum = (neighborid-1)*totalMovies + (movieId - 1) - (neighborid-1)*neighborid/2;
				nStruct n;
				n.id = neighborid;
				if(MMAP_COUNT) n.count = countMap[itemNum];
				else n.count = get_int(countIn, itemNum);
				//	Don't proceed with this neighbor if MIN_COMMON_VIEWERS not met
				if(n.count>MIN_COMMON_VIEWERS){
					if(MMAP_PEARSON) n.pearson = pearsonMap[itemNum];
					else n.pearson = get_float(pearsonIn, itemNum);
					//	Don't proceed if POSITIVE_ONLY==true and the pearson is negative
					if(n.pearson>0 | POSITIVE_ONLY!=true){
						n.rating = score;
						n.similarity = calcSim(n.pearson, n.count);
						v.push_back(n);
					}
				}
			}
		}
		pearsonIn.close();
		countIn.close();
		int size = (int) v.size();
		//	Backup algorithm triggered by MIN_SEEN_NEIGHBORS
		if(size<MIN_SEEN_NEIGHBORS){
			backups++;
			float prediction = globals.predict(movieId, user, votedate);
			return prediction;
		}
		//	Sort the neighbors in descending order
		partial_sort(v.begin(), v.begin()+min(size, MAX_NEIGHBORS), v.end());
		float weightSum = 0;
		float combinationSum = 0;
		//	Iterate over MAX_NEIGHBORS
		for(int i=0; i<min(size, MAX_NEIGHBORS); i++){
			nStruct n = v.at(i);
			int neighborid = n.id;
			float pearson = n.pearson;
			int count = n.count;
			float similarity = n.similarity;
			weightSum += similarity;
			float delta = n.rating - movieAverages[neighborid];
			float score;
			//	Whether to add or subtract the delta is based on the sign of the correlation
			if(pearson>0) score = movieAverages[movieId] + delta;
			else score = movieAverages[movieId] - delta;
			combinationSum += similarity*score;
//			fprintf(stderr, "Pred: %d\tMovieID: %d\tUserID: %d\tNeighborID: %d.\n", totpreds, movieId, user, neighborid);
//			fprintf(stderr, "i: %d NeighborID: %d Pearson: %f Count: %d Similarity: %f Delta: %f Score: %f CombinationSum: %f WeightSum: %f\n", i, neighborid, pearson, count, similarity, delta, score, combinationSum, weightSum);
		}
		float prediction = combinationSum / weightSum;
		//	Backup algorithm triggered by nan or inf values
		//	This can happen if all movies have similarity 0, which can happen if the similar count is low thus the Pearson and inverse(Fisher Lower) signs don't match
		if(isnan(prediction) | isinf(prediction)){
			backups++;
			return .5*movieAverages[movieId] + .5*userAverages[user];
		}
		return prediction;
	}
	
	void setMovieAverages(){
	script_timer("knn.MovieAverages", false);
		for(int i=1; i<=totalMovies; i++){
			double average = 0;
			currentMovie.setId(i);
			uint numVotes = currentMovie.votes();
			for(uint j=0; j<numVotes; j++){
				average += currentMovie.score(j);
			}
			average = average / (double) numVotes;
			movieAverages[i] = average;
		}
	script_timer("knn.MovieAverages", true);
	}
	
	void setUserAverages(){
		script_timer("knn.setUserAverages", false);
		int totalUsers = currentMovie.dataBase()->totalUsers();
		currentUser.setId(6);
		for (int i = 0; i < totalUsers; ++i){
			double average = 0;
			uint numVotes = currentUser.votes();
			for(uint j=0; j<numVotes; j++){
				average += currentUser.score(j);
			}
			average = average / (double) numVotes;
			userAverages[currentUser.id()] = average;
			currentUser.next();
		}
		script_timer("knn.setUserAverages", true);
	}
	
	void generatePearson(ofstream &pearsonOut, ofstream &countOut){
		User user(currentMovie.dataBase());
		int genstart = time(NULL);
		//	movie ID's are from 1 to number of movies
		for(int i=1; i<=totalMovies; ++i){
			currentMovie.setId(i);
			//	store stats in array, indexes 1 to totalMovies
			simStats statArray[totalMovies+1];
			//	initialize
			for(int sim=0; sim<=totalMovies; ++sim){
				statArray[sim].simVotesI.clear();
				statArray[sim].simVotesJ.clear();
			}
			int numVotes = currentMovie.votes();
			//	votes are from 0 to numVotes-1
			for(int v=0; v<numVotes; ++v){
				float ratingI = currentMovie.score(v);
				int userid = currentMovie.user(v);
				user.setId(userid);
//				printf("i: %d\tMovieID: %d\tv: %d\tUser: %d\n", i, currentMovie.id(), v, userid);
				int numUserVotes = user.votes();
				for(int j=0; j<numUserVotes; ++j){
					int movieId = user.movie(j);
					float ratingJ = user.score(j);
//					printf("MovieI: %d j: %d v: %d User: %d ratingI: %f MovieJ: %d ratingJ: %f\n", i, j, v, userid, ratingI, movieId, ratingJ);
					statArray[movieId].simVotesI.push_back(ratingI);
					statArray[movieId].simVotesJ.push_back(ratingJ);
				}
			}
			//	Only calculate for half the matrix, if i<=j
			for(int j=1; j<=totalMovies; ++j) if(i<=j){
				int count = statArray[j].simVotesI.size();
				//	calculate correlation if 2 or more similar votes
				float correlation = 0;
				if(count>=2) correlation = pearson(statArray[j].simVotesI, statArray[j].simVotesJ);
//				printf("i: %d j:%d correl: %f\n", i, j, correlation);
				&pearsonOut.write((char*)&correlation, sizeof(float));
				&countOut.write((char*)&count, sizeof(int));
			}
			float percent = (float) 100 * i / (totalMovies);
			if(i%25==0) fprintf(stderr, "Pearson/Count: Approximately %f%% done generating after %d seconds.\n", percent, time(NULL)-genstart);
		}
	}
	
	void showNeighbors(int movieId){
		vector<nStruct> v;
		ifstream pearsonIn(PEARSON_FILE_KNN, ios::binary);
		ifstream countIn(COUNT_FILE_KNN, ios::binary);
		for(int neighborid=1; neighborid<=totalMovies; neighborid++) if(neighborid!=movieId){
			int itemNum;
			//	If movieId>neighborid, use the mirror matrix position. Adjust itemNum according to Gaussian sequence.
			if(movieId<=neighborid) itemNum = (movieId-1)*totalMovies + (neighborid - 1) - (movieId-1)*movieId/2;
			else itemNum = (neighborid-1)*totalMovies + (movieId - 1) - (neighborid-1)*neighborid/2;
			nStruct n;
			n.id = neighborid;
			if(MMAP_PEARSON) n.pearson = pearsonMap[itemNum];
			else n.pearson = get_float(pearsonIn, itemNum);
			if(MMAP_COUNT) n.count = countMap[itemNum];
			else n.count = get_int(countIn, itemNum);
			n.similarity = calcSim(n.pearson, n.count);
			v.push_back(n);
		}
		pearsonIn.close();
		countIn.close();
		int size = (int) v.size();
		partial_sort(v.begin(), v.begin()+min(size, MAX_NEIGHBORS), v.end());
		int i = 1;
		for(vector<nStruct>::iterator iter=v.begin(); iter!=v.begin()+min(size, MAX_NEIGHBORS); iter++){
			int neighborid = (*iter).id;
			float pearson = (*iter).pearson;
			float similarity = (*iter).similarity;
			int count = (*iter).count;
			printf("Movie: %d\tCount: %d\tPearson: %f Similarity: %f\n", neighborid, count, pearson, similarity);
			i++;
		}
	}
	
	~KNN(){}

	Movie currentMovie;
	User currentUser;
	bool doPearson, doCount;
	int totalMovies, totalUsers;
	map<uint, double> movieAverages;
	map<uint, double> userAverages;
	Globals globals;
	float *pearsonMap;
	int *countMap;
};

#endif
