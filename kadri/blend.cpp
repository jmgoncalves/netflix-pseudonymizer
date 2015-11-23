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
#ifndef BLEND_CPP
#define BLEND_CPP

#include <iostream>
#include <math.h>
#include "util.cpp"
#include "algorithm.cpp"

#include <stdio.h>
#include <stdarg.h>

/*
	Example:
		Blend blend(&db);
		blend.setUp(3, "data/avg", "data/knn", "data/mf");
		blend.runProbe();
		blend.runQualifying("blendedpreds.txt");
*/

using namespace std;
using namespace util;

extern "C" void sgels_ ( ... );

class Blend : public Algorithm{
public:
	vector<float> weightsVector;	//	This will store the regression weights for each model
	Globals globals;
	int movies_count;	//	The count of movies residual elements
	int probe_index;	//	Since probe predictions should be in order, we use that as our index.
	int m;	//	The count of probe residual elements
	int n;	//	The number of files to blend
	int qual_count;	//	The number of qualifying prediction elements
	char trans;
	int nrhs;
	float* matrixA;
	float* vectorB;
	float* matrixQ;
	Blend(DataBase *db, string n="Blend") : Algorithm(db) , globals(db) {
		name = n;
		globals.setAverages(2);
		probe_index = 0;
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
	
	void setUp(int num_args, ...){
		script_timer("Blend::setUp()", false);
		va_list marker, marker_copy, marker_copy_2;
		va_start(marker, num_args);
		va_copy(marker_copy, marker);
		va_copy(marker_copy_2, marker);
		fprintf(stderr, "Attemping to blend %d items.\n", num_args);
		//	Set this.movies_count and this.m and this.n, and check that all file sizes match
		for(int i=0; i<num_args; i++){
			char* fileprefix = va_arg(marker, char*);
			string probe_filename = (string)fileprefix+".probe.resid";
			int probe_count = FileSize(probe_filename) / sizeof(float);
			string qual_filename = (string)fileprefix+".qual.pred";
			string movies_filename = (string)fileprefix+".movies.resid";
			if(i==0){
				movies_count = FileSize(movies_filename) / sizeof(float);
				m = probe_count;
				this->n = num_args;
				qual_count = FileSize(qual_filename) / sizeof(float);
			}
			else if(probe_count!=m){
				fprintf(stderr, "ERROR - Blend - file size doesn't match, file %s has %d items, not %d.\n", probe_filename.c_str(), probe_count, m);
				exit(0);
			}
//			cout << fileprefix << endl;
		}
		fill_matrices(num_args, marker_copy);
//		print_matrices();
		setWeights();
		//	Since setWeights frees the matrices, we must fill them again so predict() can use them
		fill_matrices(num_args, marker_copy_2);
		va_end(marker);
		script_timer("Blend::setUp()", true);
	}
	
	void fill_matrices(int num_args, va_list marker){
		script_timer("Blend::fill_matrices()", false);
		matrixA = (float*) malloc(m*n*sizeof(float));
		vectorB = (float*) malloc(m*sizeof(float));
		matrixQ = (float*) malloc(qual_count*n*sizeof(float));
	    char* fileprefix;
		//	Fill matrixA with probe file residuals, and matrixQ with qualifying predictions
	    for(int i=0; i<num_args; i++){
	        fileprefix = va_arg(marker, char*);
			string probe_filename = (string)fileprefix+".probe.resid";
			fillFloatArray<float>(matrixA+m*i, probe_filename);
			fprintf(stderr,"matrixA: filled %d elements for %s\n",m, probe_filename.c_str());
			string qual_filename = (string)fileprefix+".qual.pred";
			fillFloatArray<float>(matrixQ+qual_count*i, qual_filename);
			fprintf(stderr,"matrixQ: filled %d elements for %s\n",m, qual_filename.c_str());
	    }
		//	Fill vectorB with actual probe ratings
		for(int i=0; i<m; i++){
			vectorB[i] = db->probe_ratings[i];
		}
		//	matrixA is filled with residuals. Fill it with actual predictions.
		for(int i=0; i<num_args; i++){
			for(int j=0; j<m; j++){
				matrixA[i*m+j] = vectorB[j]-matrixA[i*m+j];
			}
		}
		script_timer("Blend::fill_matrices()", true);
	}
	
	void setWeights(){
		float *mWork;
		int info = 0;
		int lwork = -1;
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
			weightsVector.push_back(vectorB[k]);
			fprintf(stderr, "weightsVector.at(%d): %.10f\n", k, weightsVector.at(k));
		}
		free(matrixA);
		free(vectorB);
		free(matrixQ);
	}
	
	//	Debugging - Output the contents of vectorB and matrixA
	void print_matrices(){
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
//		fprintf(stderr, "DEBUG: user_offset:%d\n", user_offset);
//		if(user_offset!=-1){
//			//...
//		}
//		else{
			actual = db->probe_ratings[probe_index];
			for(int k=0; k<n; k++){
				//	m and this->m differ!
				if(running_qualifying) preds_vector.push_back(matrixQ[k*this->m+probe_index]);
				else preds_vector.push_back(matrixA[k*this->m+probe_index]);
			}
			probe_index++;
//		}
//		fprintf(stderr, "DEBUG: weightsVector.size():%d, preds_vector.size():%d, n:%d\n", weightsVector.size(), preds_vector.size(), n);
		for(int k=0; k<n; k++){
			pred += weightsVector.at(k) * preds_vector.at(k);
		}
		return pred;
	}
};

#endif
