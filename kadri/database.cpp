/**
 * Copyright (C) 2006-2007 Benjamin C. Meyer (ben at meyerhome dot net)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Benjamin Meyer nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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


#include "ymddate.cpp"
#include <vector>

#ifdef IS_OS_WIN
#include "winmmap.h"
#else
#include <sys/mman.h>
#endif

#ifndef USERFILENAME
	#define USERFILENAME  "users"
#endif
#ifndef MOVIESFILENAME
	#define MOVIESFILENAME "movies"
#endif
#ifndef DATESFILENAME
	#define DATESFILENAME "dates"
#endif
#define QUALIFYINGFILENAME "qualifying"
#define PROBEFILENAME "probe"

#define TRAININGSETFOLDER "training_set"

DataBase::DataBase(const string &rootPath) :
        storedmovies(0),
        storedvotes(0),
        m_totalVotes(0),
        storedUsers(0),
        storedUsersIndex(0),
        m_totalUsers(0),
        m_rootPath(rootPath),
		is_loaded(0)
{
	use_preprocessor = false;
	probeDataFile = configPath + "/" + PROBEFILENAME + ".data";
	qualDataFile = configPath + "/" + QUALIFYINGFILENAME + ".data";
	probeDatesFileName = configPath + "/" + PROBEFILENAME + ".dates.data";
	qualDatesFileName = configPath + "/" + QUALIFYINGFILENAME + ".dates.data";
	if(FileSize(probeDataFile)>0) probemap = mmap_file<uint>((char*)probeDataFile.c_str(), FileSize(probeDataFile), false);
	if(FileSize(qualDataFile)>0) qualmap = mmap_file<uint>((char*)qualDataFile.c_str(), FileSize(qualDataFile), false);
	probeSize = FileSize(probeDataFile) / sizeof(uint);
	qualSize = FileSize(qualDataFile) / sizeof(uint);
	datesmapfile = configPath + "/" + DATESFILENAME + ".data";
	dates_indexed = false;
	date_indexes_mapped = false;
}

DataBase::~DataBase()
{
	munmap(storedmovies, FileSize(moviesFileName));
	munmap(storedvotes, FileSize(votesFileName));
	munmap(storedUsers, FileSize(userFileName));
	munmap(storedUsersIndex, FileSize(userIndexFileName));
}

string DataBase::rootPath() const
{
    return m_rootPath;
}

bool userlessthan(const int &s1, const int &s2)
{
    return DataBase::guser(s1) < DataBase::guser(s2);
}

bool load(string filename, uint **pointer)
{
	bool error = false;
	int fd = open(filename.c_str(), O_RDWR | O_CREAT, (mode_t)0644);
	if(fd == -1) {
		perror("Error opening (or creating) file.");
		return 0;
	}
	if (FileSize(filename)>0
		&& FileExists(filename) ) {
			*pointer = (uint*)
				mmap(0, FileSize(filename), PROT_READ, MAP_SHARED, fd, (off_t)0);
		if (*pointer == (uint*) - 1) {
			cerr << "mmap failed" << filename << endl;;
			error = true;
		}
	}
	else {
		cerr << "unable to load database" << filename << endl;;
		error = true;
	}
	return error;
}

bool DataBase::load()
{
	string movieFileName = rootPath() + "/" + MOVIESFILENAME + ".index";
	if (!FileExists(movieFileName)){
		printf("Run setup to generate movie data file (%s). Exiting.\n", movieFileName.c_str());
		exit(0);
	}
	is_loaded = true;

	bool moviesFileError = ::load(movieFileName, &storedmovies);
	// Basic sanity check
	//if (!moviesFileError && (storedmovies[0] != 0 || (storedmovies[1] != 547 && storedmovies[1] != 524))) {
	if (!moviesFileError && (storedmovies[0] != 0)) {
		cerr << "Movie database error, possibly corrupt.  Expected [0] to be 0, but it is:"
		<< storedmovies[0] << endl;;
		munmap(storedmovies, FileSize(moviesFileName));
		moviesFileError = true;
	}
	if (moviesFileError) {
		is_loaded = false;
		return false;
	}

	// jmgoncalves
	m_totalMovies  = FileSize(movieFileName) / 4;
	fprintf(stderr, "Set totalMovies to %d which is a 4th of %d the filesize of %s\n", m_totalMovies, FileSize(movieFileName), movieFileName.c_str());

	votesFileName = rootPath() + "/" + MOVIESFILENAME + ".data";
	bool votesFileError = ::load(votesFileName, &storedvotes);
	m_totalVotes  = FileSize(votesFileName) / 4;
	// Basic sanity check
//	Movie m(this, 1);
//	if (!votesFileError && (m.votes() != 547 && m.votes() != 524)) {
//		cerr << "votes database error, needs updating or possibly corrupt.  Expect movie " << m.id() << " to have 547 or 524 votes, but it only has: " << m.votes() << endl;;
//		munmap(storedvotes, FileSize(votesFileName));
//		votesFileError = true;
//	}
//	if (!votesFileError && m.findScore(1488844) != 3) {
//		cerr << "votes database error, needs updating or possibly corrupt.  Expect movie"  << m.id() << " to have rank of 3: " << m.findScore(1488844) << " for user 1488844." << endl;;
//		munmap(storedvotes, FileSize(votesFileName));
//		votesFileError = true;
//	}

	if (votesFileError) {
		is_loaded = false;
		return false;
	}

	userFileName = rootPath() + "/" + USERFILENAME + ".data";
	if ( !FileSize(userFileName)>0 ){
		printf("Run setup to generate user data files. Exiting.\n");
		exit(0);
	}
		
	bool userFileError = ::load(userFileName, &storedUsers);
	userIndexFileName = rootPath() + "/" + USERFILENAME + ".index";
	bool userIndexFileError = ::load(userIndexFileName, &storedUsersIndex);
	if (userFileError || userIndexFileError) {
		is_loaded = false;
		return false;
	}
	m_totalUsers = FileSize(userIndexFileName) / 8;
	for (int i = 0; i < totalUsers(); ++i) {
		users.insert(make_pair(storedUsersIndex[i], i));
	}
	// Basic sanity check that the database is ok and as we expect it to be
	User user(this, 6);
//	if ((user.votes() != 626 && user.votes() != 623) || user.movie(0) != 30) {
//		cerr << "Expected " << m.id() << " movie(0) to be user 30, but it is actually:" << user.movie(0) << endl;;
//		cerr << "OR user database error, possibly corrupt.  Expected " << user.id() << " to have 626 votes, but it only has:" << user.votes() << endl;;
//		cerr << "It is suggested you delete users.index and users.data and re-run setup to regenerate data files." << endl;;
//		munmap( storedUsers, FileSize(userFileName) );
//		munmap( storedUsersIndex, FileSize(userIndexFileName) );
//		users.clear();
//		is_loaded = false;
//		return false;
//	}

	// Debug jmgoncalves
	// see what ratings are different!
	int voteCount = 0;
	int mVoteSum = 0;
	int uVoteSum = 0;
	User u(this, 6);
	for (int i=0; i<totalUsers(); i++) {
		for (int j=0; j<u.votes(); j++) {
			// get ids
			int mid =  u.movie(j);
			int uid = u.id();

			// get user score using different methods
			int uscore = u.score(j);
//			if (uscore != u.rating(i,j))
//				fprintf(stderr, "ERROR FOUND! u.score(%d) returns %d and u.rating(%d,%d) returns %f\n", j, uscore, i, j, u.rating(i,j));
			if (uscore != gscore(storedUsers[voteCount]))
				fprintf(stderr, "ERROR FOUND! u.score(%d) returns %d and gscore(storedUsers[%d]) returns %d\n", j, uscore, voteCount, gscore(storedUsers[voteCount]));

			// get movie score using different methods
			Movie m(this, mid);
			int mo = m.dataBaseOffset();
			int mv = m.findVote(uid);
			int mscore = m.findScore(uid);
			if (mscore != m.score(mv))
				fprintf(stderr, "ERROR FOUND! m.findScore(%d) returns %d and m.score(%d) returns %d\n", uid, mscore, mv, m.score(mv));
//			if (mscore != m.rating(mid-1,mv))
//				fprintf(stderr, "ERROR FOUND! m.findScore(%d) returns %d and m.rating(%d,%d) returns %f\n", uid, mscore, mid-1, mv, m.rating(mid-1,mv));
			if (uscore != gscore(storedvotes[mo+mv]))
				fprintf(stderr, "ERROR FOUND! u.findScore(%d) returns %d and gscore(storedvotes[%d]) returns %d\n", uid, mscore, mo+mv, gscore(storedvotes[mo+mv]));

			// verify match and validity
			if (mscore!=uscore || mscore>5 || mscore<1 || uscore>5 || uscore<1)
				fprintf(stderr, "ERROR FOUND! m.findScore(%d) for movie %d returns %d and the user score is %d\n", u.id(), mid, mscore, uscore);
			voteCount++;
			mVoteSum += mscore;
			uVoteSum += uscore;
		}
		u.next();
	}
	fprintf(stderr, "Finished comparing %d ratings with movie ratings sum of %d and user ratings sum of %d!\n", voteCount, mVoteSum, uVoteSum);

	string datesFileName = rootPath() + "/" + DATESFILENAME + ".data";
	if( !FileSize(datesFileName)>0 ){
		printf("Run setup to generate dates data file. Exiting.\n");
		exit(0);
	}
	datesmap = mmap_file<uint>((char*)datesmapfile.c_str(), FileSize(datesmapfile.c_str()), false);
	fill_probe_arrays();
	fill_qual_arrays();

	if( !FileSize(probeDatesFileName)>0 ){
		printf("Run setup to generate probe dates data file. Exiting.\n");
		exit(0);
	}
	probe_dates = mmap_file<uint>((char*)probeDatesFileName.c_str(), FileSize(probeDatesFileName), false);

	if( !FileSize(qualDatesFileName)>0 ){
		printf("Run setup to generate qualifying dates data file.\n");
		//exit(0);
	}
	else{
		qual_dates = mmap_file<uint>((char*)qualDatesFileName.c_str(), FileSize(qualDatesFileName), false);
		qualMapSize = FileSize(qualDatesFileName) / 4;
	}

	is_loaded = true;
	return true;
}

void DataBase::indexDates(bool userDates, bool movieDates){
	script_timer("indexDates()", false);
	fprintf(stderr, "Converting dates to indexes...\n");
	User currentUser(this);
	currentUser.setId(6);
	Movie movies(this);
	userDatesTable = new map<short, short>[totalUsers()];
	movieDatesTable = new map<short, short>[totalMovies()];
	try{
		userDatesTable[0][currentUser.votedate(1, 0)] = 1;
		movieDatesTable[0][movies.votedate(1, 0)] = 1;
	}
	catch(...){
		fprintf(stderr, "Exception - probably out of RAM (DataBase::indexDates()). Exiting.\n");
		exit(1);
	}
	//	Insert the dates into the maps (including probe and qualifying dates)
	if(userDates){
		fprintf(stderr, "Indexing users dates...\n");
		for(int i=0; i<totalUsers(); i++){
			for(int v=0; v<currentUser.votes(); v++){
				try{
					int votedate = currentUser.votedate(v);
					userDatesTable[i][votedate] = 1;
				}
				catch(...){
					fprintf(stderr, "Exception - probably out of RAM (DataBase::indexDates() userDatesTable[%d][%d]). Exiting.\n", i, v);
					exit(1);
				}
			}
			currentUser.next();
		}
	}
	if(movieDates){
		fprintf(stderr, "Indexing movie dates...\n");
		for(int i=0; i<totalMovies(); i++){
			int mindex = i+1;
			for(int v=0; v<movies.numVotes(mindex); v++){
				try{
					int votedate = movies.votedate(mindex, v);
					movieDatesTable[i][votedate] = 1;
				}
				catch(...){
					fprintf(stderr, "Exception - probably out of RAM (DataBase::indexDates() movieDatesTable[%d][%d]). Exiting.\n", i, v);
					exit(1);
				}
			}
		}
	}
	for(int i=0; i<probeMapSize; i++){
		int votedate = probe_dates[i];
		int u = users[probe_users[i]];
		int m = probe_movies[i] - 1;
		if(userDates) userDatesTable[u][votedate] = 1;
		if(movieDates) movieDatesTable[m][votedate] = 1;
	}
	for(int i=0; i<qualMapSize; i++){
		int votedate = qual_dates[i];
		int u = users[qual_users[i]];
		int m = qual_movies[i] - 1;
		if(userDates) userDatesTable[u][votedate] = 1;
		if(movieDates) movieDatesTable[m][votedate] = 1;
	}
	//	Set the indexes for each date
	if(userDates) for(int i=0; i<totalUsers(); i++){
		int index = 0;
		for(map<short, short>::iterator iter=userDatesTable[i].begin(); iter!=userDatesTable[i].end(); iter++){
			iter->second = index;
			index++;
		}
	}
	if(movieDates) for(int i=0; i<totalMovies(); i++){
		int index = 0;
		for(map<short, short>::iterator iter=movieDatesTable[i].begin(); iter!=movieDatesTable[i].end(); iter++){
			iter->second = index;
			index++;
		}
	}
	dates_indexed = true;
	fprintf(stderr, "Done indexing dates.\n");
	script_timer("indexDates()", true);
}

void DataBase::saveIndexes(){
	Movie movies(this);
	User currentUser(this);
	if(!dates_indexed) indexDates();
	fprintf(stderr, "Saving movie date indexes to file...\n");
	vector<short> movieIndexes;
	movieIndexes.reserve(totalVotes());
	for(int i=0; i<totalMovies(); i++){
		for(int v=0; v<movies.numVotes(i); v++){
			int votedate = movies.votedate(i, v);
			short votedateIndex = movieDatesTable[i][votedate];
			movieIndexes.push_back(votedateIndex);
		}
	}
	util::vectorToFile<short>(movieIndexes, rootPath()+"/dateindexes."+MOVIESFILENAME+".data");
	movieIndexes.clear();
	movieIndexes.reserve(0);
	fprintf(stderr, "Saving user date indexes to file...\n");
	vector<short> userIndexes;
	userIndexes.reserve(totalVotes());
	currentUser.setId(6);
	for(int i=0; i<totalUsers(); i++){
		for(int v=0; v<currentUser.votes(); v++){
			int votedate = currentUser.votedate(v);
			short votedateIndex = userDatesTable[i][votedate];
			userIndexes.push_back(votedateIndex);
		}
		currentUser.next();
	}
	util::vectorToFile<short>(userIndexes, rootPath()+"/dateindexes."+USERFILENAME+".data");
	userIndexes.clear();
	userIndexes.reserve(0);
	fprintf(stderr, "Saving probe date indexes to file...\n");
	vector<short> probeUserIndexes;
	vector<short> probeMovieIndexes;
	for(int i=0; i<probeMapSize; i++){
		int votedate = probe_dates[i];
		int u = users[probe_users[i]];
		int m = probe_movies[i] - 1;
		short probeUserIndex = userDatesTable[u][votedate];
		short probeMovieIndex = movieDatesTable[m][votedate];
		probeUserIndexes.push_back(probeUserIndex);
		probeMovieIndexes.push_back(probeMovieIndex);
	}
	util::vectorToFile<short>(probeUserIndexes, rootPath()+"/dateindexes."+PROBEFILENAME+"user.data");
	util::vectorToFile<short>(probeMovieIndexes, rootPath()+"/dateindexes."+PROBEFILENAME+"movie.data");
	probeUserIndexes.clear();
	probeMovieIndexes.clear();
	fprintf(stderr, "Saving qualifying date indexes to file...\n");
	vector<short> qualUserIndexes;
	vector<short> qualMovieIndexes;
	for(int i=0; i<qualMapSize; i++){
		int votedate = qual_dates[i];
		int u = users[qual_users[i]];
		int m = qual_movies[i] - 1;
		short qualUserIndex = userDatesTable[u][votedate];
		short qualMovieIndex = movieDatesTable[m][votedate];
		qualUserIndexes.push_back(qualUserIndex);
		qualMovieIndexes.push_back(qualMovieIndex);
	}
	util::vectorToFile<short>(qualUserIndexes, rootPath()+"/dateindexes."+QUALIFYINGFILENAME+"user.data");
	util::vectorToFile<short>(qualMovieIndexes, rootPath()+"/dateindexes."+QUALIFYINGFILENAME+"movie.data");
	qualUserIndexes.clear();
	qualMovieIndexes.clear();
	fprintf(stderr, "Done saving date indexes to files.\n");
}

void DataBase::mapIndexes(){
	string filename = rootPath()+"/dateindexes."+MOVIESFILENAME+".data";
	if(!(FileSize(filename)>0)){
		fprintf(stderr, "Date index file not present. Running DataBase::indexDates() and Database::saveIndexes() first...\n");
		indexDates();
		saveIndexes();
	}
	movie_date_indexes = mmap_file<short>((char*)filename.c_str(), FileSize(filename), false);
	filename = rootPath()+"/dateindexes."+USERFILENAME+".data";
	user_date_indexes = mmap_file<short>((char*)filename.c_str(), FileSize(filename), false);
	filename = rootPath()+"/dateindexes."+PROBEFILENAME+"user.data";
	probe_user_date_indexes = mmap_file<short>((char*)filename.c_str(), FileSize(filename), false);
	filename = rootPath()+"/dateindexes."+PROBEFILENAME+"user.data";
	probe_movie_date_indexes = mmap_file<short>((char*)filename.c_str(), FileSize(filename), false);
	filename = rootPath()+"/dateindexes."+QUALIFYINGFILENAME+"user.data";
	qual_user_date_indexes = mmap_file<short>((char*)filename.c_str(), FileSize(filename), false);
	filename = rootPath()+"/dateindexes."+QUALIFYINGFILENAME+"movie.data";
	qual_movie_date_indexes = mmap_file<short>((char*)filename.c_str(), FileSize(filename), false);
	date_indexes_mapped = true;
}
