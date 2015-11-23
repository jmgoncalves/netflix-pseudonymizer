/**
 * Copyright (C) 2009 Saqib Kadri (kadence[at]trahald.com)
 * Also see license and terms of packaged code, Copyright Benjamin Meyer
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

//#define NUM_VOTES 100480507	//	The full training set
//#define NUM_VOTES	99072112	//	Training set with probe data scrubbed
// NUM_VOTES is now dynamically obtained from db->totalVotes()
//#define NUM_USERS	480189		//  Original number of users
// NUM_USERS is now dynamically obtained from db->totalUsers()
//#define NUM_MOVIES	17770
// NUM_MOVIES is now dynamically obtained from db->totalMovies()
#define MIN_DATE	10906		//	The earliest votedate, in unix days
#define MAX_DATE	13148		//	The latest votedate, in unix days
#define NUM_DAYS	MAX_DATE-MIN_DATE+1	//	Number of total days

#include <iostream>
#include <vector>
#include <math.h>
#include <valarray>
#include <map>
#include <fstream>
#include <ctime>

struct Matrix{
public:
	float *data;
	int rows;
	int cols;
	inline float *operator[](int row){
		return &data[row*cols];
	}
};

#include "util.cpp"
#include "config.h"
#include "ymddate.cpp"
#include "binarysearch.h"

#include "mmap.cpp"

#include "database.h"
#include "movie.h"
#include "user.h"

#include "database.cpp"
#include "movie.cpp"
#include "user.cpp"

#include "globals.cpp"
#include "average.cpp"
#include "knn.cpp"
#include "user_knn.cpp"
#include "matrix_factorization.cpp"
#include "blend.cpp"
#include "blend_partial.cpp"
#include "mf_bias.cpp"
#include "mf_time.cpp"

using namespace std;
using namespace util;

int main(int argc, char *argv[]){
	script_timer("Total", false);
	if(argv[1] && strstr(argv[1],"deb")!=NULL) debug = true;	//	Set debug to true if argv[1] contains "deb"
	if(debug) fprintf(stderr, "Debug mode on.\n");
	if(argv[1] && strstr(argv[1],"off")!=NULL) full_output = false;	//	Disable full RMSE output if argv[1] contains "off"

	DataBase db;
	db.load();
	if(db.checkDB()) fprintf(stderr, "checkDB OK\n");
	else fprintf(stderr, "DB Corrupt.\n");
	db.setTitles();
	Movie movies(&db);
	User users(&db);
	movies.setId(1);
	users.setId(6);

	fprintf(stderr, "db.totalUsers()=%d\n", db.totalUsers());
	fprintf(stderr, "db.totalMovies()=%d\n", db.totalMovies());
	fprintf(stderr, "db.totalVotes()=%d\n", db.totalVotes());

/*
	db.loadPreProcessor("data/somemodel");	//	Load a preprocessor built using Algorithm::buildPreProcessor("data/somemodel")
*/

	Average avg(&db);
	//avg.runProbe();
	//avg.runQualifying("none", true);
	avg.buildPreProcessor("data_average");

/*
	Globals globals(&db);
	globals.setAverages(10);
	globals.setVariances();
	globals.setThetas();
	globals.runProbe();
	//globals.runQualifying("none", true);
*/

	#define TRAIN_SIMU true
	Matrix_Factorization *mf = new Matrix_Factorization(&db);
	mf->training();
//	mf->cache("data_mf_simu");
	//mf->runProbe();
	//mf->runQualifying("none", true);
	mf->buildPreProcessor("data_mf");


//	User_KNN * uknn = new User_KNN(&db);
//	uknn->setup();
//	uknn->loadUserFeatures("data_mf_simu.users.cache");
	//uknn->runProbe();
	//uknn->runQualifying("none", true);
//	uknn->buildPreProcessor("data_uknn");


	Blend blend(&db);
//	blend.setUp(3, "data_average", "data_mf_simu", "data_uknn");
	blend.setUp(2, "data_average", "data_mf");
	blend.runProbe();
	//blend.runQualifying("none", true);

/*
	Blend_Partial blendpartial(&db);
	blendpartial.setUp(3, "data/average", "data/mf_simu", "data/uknn");
	blendpartial.runProbe_partial();
	blendpartial.runQualifying("none", true);
*/
/*
	KNN knn(&db);
	knn.setup();
	knn.runProbe();
E
E
E
	knn.runQualifying("none", true);
*/
	script_timer("Total", true);
	fprintf(stderr, "\n");
	print_timer_summary_map();
}
