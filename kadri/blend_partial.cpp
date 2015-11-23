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
#ifndef BLEND_PARTIAL_CPP
#define BLEND_PARTIAL_CPP

#include <iostream>
#include <math.h>
#include "util.cpp"
#include "algorithm.cpp"

#include <stdio.h>
#include <stdarg.h>

/*
	This class blends predictions using a part of the probe set, using the *.pred.resid binary data files
		The part of the probe set being blended is determined by config.h's probe_partial_percent and blend_probe_seed
	This class CANNOT run the algorithm::runProbe() function properly, because it never fills the whole *.pred.resid file at once, only by halves for blending and predicting
		Use the algorithm::runProbe_partial() function instead
	It is also not set up to run runTraining() because the predict() function is incomplete
	When calling setUp(), the first argument should be the number of models being blended. e.g.:
		Blend_Partial blendpartial(&db);
		blendpartial.setUp(2, "data/knn", "data/doublecentering");
	Example:
		Blend_Partial blendpartial(&db);
		blendpartial.setUp(3, "data/avg", "data/knn", "data/mf");
		blendpartial.runProbe_partial();
*/

using namespace std;
using namespace util;

extern "C" void sgels_ ( ... );

class Blend_Partial : public Algorithm{
public:
	vector<float> * weightArrayVectors;	//	This will store the regression weights for each model
	Globals globals;
	int matrix_index;	//	Since probe predictions should be in order, we use that as our index.
	int m;	//	The count of probe residual elements
	int n;	//	The number of files to blend
	int movies_count;	//	The count of movies residual elements
	int probe_count;	//	The number of probe prediction and actual elements
	int qual_count;	//	The number of qualifying prediction elements
	int bool_algo_true_count;	//	The number of "true" values in the algorithm::runProbe_partial() bool_array
	char trans;
	int nrhs;
	float* matrixA;	//	Matrix with predictions to blend
	float* matrixA_predict;	//	Matrix with values being predicted by algorithm::runProbe_partial()
	float* vectorB;	//	Vector with actual ratings for predictions being blended
	float* vectorB_predict;	//	Vector with actual ratings for predictions being predicted by algorithm::runProbe_partial()
	float* matrixQ;
	Blend_Partial(DataBase *db, string n="Blend_Partial") : Algorithm(db) , globals(db) {
		name = n;
		globals.setAverages();
		matrix_index = 0;
		bool_algo_true_count = 0;
		trans = 'N';
		nrhs = 1;
	}
	void setMovie(int movieid){
	}
	double determine(int userid){
		return 0;
	}
	
	template<class A> void fillFloatArray(float * array, string filename, int size=0){
		if(size==0) size = FileSize(filename.c_str());
		else size = min(size, FileSize(filename.c_str()));
		ifstream in(filename.c_str(), ios::binary);
		int count = size / sizeof(float);
		for(int i=0; i<count; i++){
			A v;
			in.read((char*)&v, sizeof(A));
			array[i] = (float) v;
		}
		in.close();
	}
	
	template<class A> inline void fillFloatArray_partial(float * array, string filename, bool * bool_array){
		int size = FileSize(filename.c_str());
		ifstream in(filename.c_str(), ios::binary);
		int count = size / sizeof(float);
		int array_index = 0;
		for(int i=0; i<count; i++){
			A v;
			in.read((char*)&v, sizeof(A));
			if(bool_array[i]==true){
				array[array_index] = (float) v;
				array_index++;
			}
		}
		in.close();		
	}
	
	void setUp(int num_args, ...){
		script_timer("Blend_Partial::setUp()", false);
		va_list marker, marker_copy, marker_copy_2;
		va_start(marker, num_args);
		va_copy(marker_copy, marker);
		va_copy(marker_copy_2, marker);
		//	For some reason, a multiplier of 1 doesn't work - 13 and under give corrupted results, 14-16 result in segmentation faults, and only 17+ work
//		weightArrayVectors = (vector<float>*) malloc(1*num_args*sizeof(vector<float>));
		weightArrayVectors = (vector<float>*) malloc(17*num_args*sizeof(vector<float>));
		fprintf(stderr, "Attemping to blend %d items.\n", num_args);
		//	Set this.movies_count and this.m and this.n, and check that all file sizes match
		for(int i=0; i<num_args; i++){
			char* fileprefix = va_arg(marker, char*);
			string probe_filename = (string)fileprefix+".probe.resid";
			string qual_filename = (string)fileprefix+".qual.pred";
			string movies_filename = (string)fileprefix+".movies.resid";
			int current_movies_count = FileSize(movies_filename) / sizeof(float);
			int current_probe_count = FileSize(probe_filename) / sizeof(float);
			int current_qual_count = FileSize(qual_filename) / sizeof(float);
			if(i==0){
				movies_count = FileSize(movies_filename) / sizeof(float);
				probe_count = current_probe_count;
				qual_count = current_qual_count;
				//	algorithm::runProbe_partial() uses total * probe_partial_percent, and total=probe_count, so we use the complement here
				bool_algo_true_count = probe_count * probe_partial_percent;
				m = probe_count - bool_algo_true_count;
				this->n = num_args;
			}
			else if(movies_count!=current_movies_count || probe_count!=current_probe_count || qual_count!=current_qual_count){
				fprintf(stderr, "ERROR - Blend - file size doesn't match for fileprefix %s\n", fileprefix);
				exit(0);
			}
//			cout << fileprefix << endl;
		}
		fill_matrices(num_args, marker_copy);
//		print_blend_matrices();
		setWeights();
		//	Since setWeights frees the matrices, we must fill them again so predict() can use them
		fill_matrices(num_args, marker_copy_2);
		va_end(marker);
		script_timer("Blend_Partial::setUp()", true);
	}
	
	void fill_matrices(int num_args, va_list marker){
		script_timer("Blend_Partial::fill_matrices()", false);
		matrixA = (float*) malloc(m*n*sizeof(float));
		matrixA_predict = (float*) malloc(bool_algo_true_count*n*sizeof(float));
		vectorB = (float*) malloc(m*sizeof(float));
		vectorB_predict = (float*) malloc(bool_algo_true_count*sizeof(float));
		matrixQ = (float*) malloc(qual_count*n*sizeof(float));
	    char* fileprefix;
		bool* bool_array = (bool*) malloc(probe_count*sizeof(bool));
		//	bool_array_complement is the inverse of runProbe_partial()'s bool_array, using bool_algo_true_count
		bool* bool_array_complement = (bool*) malloc(probe_count*sizeof(bool));
		for(int i=0; i<probe_count; i++){
			if(i<bool_algo_true_count) bool_array[i] = true;
			else bool_array[i] = false;
		}
		script_timer("Blend_Partial::fill_matrices()::random_shuffle()", false);
		//	Shuffle bool_array
		srand(blend_probe_seed);
		random_shuffle(bool_array, bool_array+probe_count-1);
		script_timer("Blend_Partial::fill_matrices()::random_shuffle()", true);
		//	Fill bool_array_complement after bool_array has been shuffled
		for(int i=0; i<probe_count; i++){
			if(bool_array[i]==true) bool_array_complement[i] = false;
			else bool_array_complement[i] = true;
		}
		//	Fill matrixA with probe file residuals, and matrixQ with qualifying predictions
	    for(int i=0; i<num_args; i++){
	        fileprefix = va_arg(marker, char*);
			string probe_filename = (string)fileprefix+".probe.resid";
			//	m (which is probe_count-bool_algo_true_count) is the number of elements to fill each matrixA column with
			fillFloatArray_partial<float>(matrixA+m*i, probe_filename, bool_array_complement);
			fprintf(stderr,"matrixA: filled %d elements for %s\n", m, probe_filename.c_str());
			//	Fill the predictions array
			fillFloatArray_partial<float>(matrixA_predict+bool_algo_true_count*i, probe_filename, bool_array);
			fprintf(stderr,"matrixA_predict: filled %d elements for %s\n", m, probe_filename.c_str());
			//	Fill the qualifying array
			string qual_filename = (string)fileprefix+".qual.pred";
			fillFloatArray<float>(matrixQ+qual_count*i, qual_filename);
			fprintf(stderr,"matrixQ: filled %d elements for %s\n", qual_count, qual_filename.c_str());
	    }
		int vectorB_index = 0;
		//	Fill vectorB with actual probe ratings
		for(int i=0; i<probe_count; i++){
			if(bool_array_complement[i]==true){
				vectorB[vectorB_index] = db->probe_ratings[i];
				vectorB_index++;
			}
		}
		int vectorB_predict_index = 0;
		//	Fill vectorB_predict with actual probe ratings
		//	Can't be done in the same loop as vectorB because the indexes don't match
		for(int i=0; i<probe_count; i++){
			if(bool_array[i]==true){
				vectorB_predict[vectorB_predict_index] = db->probe_ratings[i];
				vectorB_predict_index++;
			}
		}
		//	matrixA is filled with residuals. Fill it with actual predictions.
		for(int i=0; i<num_args; i++){
			for(int j=0; j<m; j++){
				 matrixA[i*m+j] = vectorB[j]-matrixA[i*m+j];
			}
		}
		//	matrixA_predict is filled with residuals. Fill it with actual predictions.
		for(int i=0; i<num_args; i++){
			for(int j=0; j<bool_algo_true_count; j++){
				 matrixA_predict[i*bool_algo_true_count+j] = vectorB_predict[j]-matrixA_predict[i*bool_algo_true_count+j];
			}
		}
		script_timer("Blend_Partial::fill_matrices()", true);
	}
	
	void setWeights(){
		float *mWork;
		int info = 0;
		int lwork = -1;
		int index = 0;
		weightArrayVectors[index].clear();
		mWork = (float*) malloc(1*sizeof(float));
		sgels_(&trans, &m, &n, &nrhs, matrixA, &m, vectorB, &m, mWork, &lwork, &info);
		lwork = (int) mWork[0];
		fprintf(stderr, "Optimal lwork: %d\n", lwork);
		free(mWork);
		mWork = (float*) malloc(lwork*sizeof(float));
		sgels_(&trans, &m, &n, &nrhs, matrixA, &m, vectorB, &m, mWork, &lwork, &info);
		free(mWork);
		if(info) fprintf(stderr, "Error - sgels returned: %d\n", info);
		for(int k=0; k<n; k++){
			weightArrayVectors[index].push_back(vectorB[k]);
			fprintf(stderr, "weightArrayVectors[%d].at(%d): %.10f\n", index, k, weightArrayVectors[index].at(k));
		}
		free(matrixA);
		free(vectorB);
		free(matrixQ);
	}
	
	//	Debugging - Output the contents of vectorB and matrixA
	void print_blend_matrices(){
		printf("probeNum\tvectorB");
		for(int j=0; j<n; j++){
			printf("\tmatrixA(%d)", j);
		}
		printf("\n");
		for(int i=0; i<m; i++){
			printf("%d\t%f", i, vectorB[i]);
			for(int j=0; j<n; j++){
				printf("\t%f", matrixA[j*m+i]);
			}
			printf("\n");
		}
	}
	
	//	Debugging - Output the contents of matrixQ
	void print_qual_matrices(){
		printf("qualNum");
		for(int j=0; j<n; j++){
			printf("\tmatrixQ(%d)", j);
		}
		printf("\n");
		for(int i=0; i<qual_count; i++){
			printf("%d", i);
			for(int j=0; j<n; j++){
				printf("\t%f", matrixQ[j*qual_count+i]);
			}
			printf("\n");
		}
	}

	float predict(int movieid, int userid, int votedate){
		float pred = 0;
		int u = db->users[userid];
		int m = movieid - 1;
		int i = 0;
		int movie_offset = db->storedmovies[m];
		int user_offset = db->findUserAtMovie(u+1, movieid);
		float actual = 0;
		vector<float> preds_vector;
		//	If the user_offset is -1, then this is a probe prediction. Otherwise it is a training set prediction.
		if(user_offset!=-1){
			//...
			//	This would have to be based on vector<ifstream*> movie_streams
		}
		else{
			actual = db->probe_ratings[matrix_index];
			for(int k=0; k<n; k++){
				//	m and this->m differ
				if(running_qualifying) preds_vector.push_back(matrixQ[k*this->qual_count+matrix_index]);
				else preds_vector.push_back(matrixA_predict[k*this->bool_algo_true_count+matrix_index]);
			}
			matrix_index++;
		}
		for(int k=0; k<n; k++){
			pred += weightArrayVectors[0].at(k) * preds_vector.at(k);
		}
//		printf("Pred: %f\tActual: %f\n", pred, actual);
		return pred;
	}
};

#endif
