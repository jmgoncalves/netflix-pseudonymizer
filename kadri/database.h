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
#ifndef DATABASE_H
#define DATABASE_H

#include "config.h"
#include "util.cpp"
#include "mmap.cpp"

using namespace std;
using namespace util;

/*
	Array			Maps
	storedvotes		movies.data (uint)		3 bits for rating, 29 for user ID; sorted by movie ID, then user ID
	storedmovies	movies.index (uint)		Index of the first vote of each movie, sorted by movie ID
	storedUsers		users.data (uint)		3 bits for rating, 29 for user ID; sorted by user ID, then movie ID
	storedUsersIndex	users.index (uint)	The user ID for each user, sorted by user ID; followed by the index of the first vote for each user, sorted by user ID; followed by the total number of votes
	probemap		probe.data (uint)
	qualmap		qualifying.data (uint)
	moviesprefloatmap	<input>.movies.data		Movie residuals
	usersprefloatmap	<input>.users.data		User residuals
	probeprefloatmap	<input>.probe.data		Probe residuals
	datesmap		dates.data (uint)		16 bits for unix days sorted by movie ID, 16 bits for unix days sorted by user ID
	
*/

class DataBase{
	friend class Movie;
	friend class User;
public:
	uint *storedmovies;
	uint *storedvotes;
	uint m_totalVotes;	//	m_totalVotes needs to be defined after storedvotes and before storedUsers
	uint *storedUsers;
	uint *storedUsersIndex;
	uint m_totalUsers;
	map<int, int> users;
	string moviesFileName;
	string userFileName;
	string votesFileName;
	string userIndexFileName;
	string m_rootPath;
	uint is_loaded;
	uint m_totalMovies; // added by jmgoncalves
	
	vector<string> movieTitles;	//	Optional - the movie titles, set using setTitles()
	vector<int> movieYears;	//	Optional - the movie release years, set using setTitles()
	bool use_preprocessor;	//	Whether or not a preprocessor is being used
	uint *probemap;
	uint *qualmap;
	int probeSize;	//	The size of probeDataFile, in number of uints
	int probeMapSize;	//	The size of the probe_map
	int qualSize;
	int qualMapSize;	//	The size of qual_dates
	string probeDataFile;
	string qualDataFile;
	float *moviesprefloatmap;
	float *usersprefloatmap;
	float *probeprefloatmap;
	string datesmapfile;
	string probeDatesFileName;
	string qualDatesFileName;
	uint *datesmap;	//	Map of the vote dates, 16 bits sorted by movie, 16 bits sorted by userid
	uint *probe_ratings;	//	Contains the probe ratings.
	uint *probe_movies;	//	Contains the probe movie IDs
	uint *probe_users;	//	Contains the probe user IDs
	uint *probe_dates;	//	Map of the probeDatesFileName
	uint *qual_movies;	//	Contains qualifying movie IDs
	uint *qual_users;	//	Contains qualifying user IDs
	uint *qual_dates;	//	Map of the qualDatesFileName

	map<short, short> *userDatesTable;	//	Convert vote dates to date indexes
	map<short, short> *movieDatesTable;	//	Convert vote dates to date indexes
	bool dates_indexed;	//	Whether the datesTable has been created
	bool date_indexes_mapped;	//	Whether the date index files are already mapped
	short *movie_date_indexes;	//	Map of movie sorted date indexes
	short *user_date_indexes;	//	Map of user sorted date indexes
	short *probe_movie_date_indexes;	//	Map of probe date indexes
	short *probe_user_date_indexes;	//	Map of probe date indexes
	short *qual_movie_date_indexes;	//	Map of qual date indexes
	short *qual_user_date_indexes;	//	Map of qual date indexes

	DataBase(const string &rootPath = "..");
	~DataBase();
	void indexDates(bool userDates=true, bool indexMovies=true);	//	Create maps of dates. Optionally index user dates and movie dates.
	void saveIndexes();
	void mapIndexes();
	// internal
	void generateMovieDatabase();
	void generateUserDatabase();
	void generateProbeDates();
	void generateQualDates();
	bool load();
	string rootPath() const;
//	static void saveDatabase(const QVector<uint> &movies, const QString &fileName);

	inline bool isLoaded() const{
		return is_loaded;
	}

	inline int totalUsers() const{
		return m_totalUsers;
	}
	inline int lastUser() const{
		return storedUsersIndex[totalUsers() - 1];
	}
	inline int totalVotes() const{
		return m_totalVotes;
	}
	inline int totalMovies() const{
		return (isLoaded()) ? m_totalMovies : 0;
	}

	// functions to pull out the vote and user
	static inline uint gscore(uint x){
		return (x >> 29);
	}
	//	guser also pulls out movie ID's as well as user indexes
	static inline uint guser(uint x){
		return ((x << 3) >> 3);
	}

	//	Since we are using a map, this function cannot be const
	inline int mapUser(int user) {
		return users[user];
	}

	inline int lastUserSize() const{
		return FileSize(userFileName) / sizeof(uint) - storedUsersIndex[totalUsers()];
	}
	
	void fill_probe_arrays(){
		int currentMovie = -1;
		int index = 0;
		probe_ratings = (uint*) malloc(probeSize*sizeof(uint));
		probe_movies = (uint*) malloc(probeSize*sizeof(uint));
		probe_users = (uint*) malloc(probeSize*sizeof(uint));
		for(int i=2; i<probeSize; i++){
			if(probemap[i]==0)
			//	If the current value is 0, that means that the next value contains the movie ID
			if(probemap[i]==0){
				currentMovie = -1;
				continue;
			}
			if(currentMovie==-1){
				currentMovie = probemap[i];
				continue;
			}
			//	The user is at i, the real value is at the next i
			int user = probemap[i];
			i++;
			int realValue = probemap[i];
			probe_ratings[index] = realValue;
			probe_movies[index] = currentMovie;
			probe_users[index] = user;
			index++;
		}
		probeMapSize = index;
	}

	void fill_qual_arrays(){
		//	The total count of qualifying predictions is at db->qualmap[1]
		int total = qualmap[1];
		int currentMovie = -1;
		int index = 0;
		qual_movies = (uint*) malloc(qualSize*sizeof(uint));
		qual_users = (uint*) malloc(qualSize*sizeof(uint));
		for(int i=2; i<qualSize; i++){
			//	If two consecutive qualifying.data values are 0, that means that the next value contains the movie ID
			//	qualmap[3] contains the first movie id
			if( (qualmap[i]==0 && qualmap[i-1]==0) || i==2){
				currentMovie = -1;
				continue;
			}
			if(currentMovie==-1){
				currentMovie = qualmap[i];
				continue;
			}
			//	Else this is where the actual rating would be in the probe.data file
			if(qualmap[i]==0){
				continue;
			}
			//	The user is at i, the real value would be the next i
			int user = qualmap[i];
			qual_users[index] = user;
			qual_movies[index] = currentMovie;
			i++;
			index++;
		}
	}

	void setTitles(){
		string titlesfile = configPath+"/"+"movie_titles.txt";
		ifstream in(titlesfile.c_str(), ios::in);
		char buffer[1024];
		int linenum = 1;
		while(!in.getline(buffer, 1024).eof()){
			string line(buffer);
			int pos = line.find(',', 0);
			int id = atoi(line.substr(0, pos).c_str());
			//	The id should be irrelevant, since the movies should be sorted by ID
			if(linenum!=id){
				fprintf(stderr, "ERROR: db.setTitles(): Order of movies in %s not by movie ID.\n", titlesfile.c_str());
				exit(0);
			}
			int pos2 = line.find(',', pos+1);
			int year = atoi(line.substr(pos+1, pos2-1).c_str());
			string title = line.substr(pos2+1);
			movieYears.push_back(year);
			movieTitles.push_back(title);
			linenum++;
		}
		in.close();
	}
	
	//	Check the database for corruption by comparing the sums of user and movie ratings
	//	Returns true if everything is OK, otherwise false
	bool checkDB(){
		float msum = 0;
		for(int i=0; i<totalVotes(); i++){
			int rating = gscore(storedvotes[i]);
			msum += rating;
		}
		float usum = 0;
		for(int i=0; i<totalVotes(); i++){
			int rating = gscore(storedUsers[i]);
			usum += rating;
		}
		if(msum==usum) return true;
		else fprintf(stderr, "Sum of votes from users and from movies does not match! msum=%f usum=%f\n", msum, usum);
		return false;
	}
	
	void loadPreProcessor(string fileprefix){
		use_preprocessor = true;
		fprintf(stderr, "Loading pre-processor '%s'...\n", fileprefix.c_str());
		string moviesprefile = fileprefix+".movies.resid";
		string usersprefile = fileprefix+".users.resid";
		string probeprefile = fileprefix+".probe.resid";
		moviesprefloatmap = mmap_file<float>((char*)moviesprefile.c_str(), FileSize(moviesprefile), false);
		usersprefloatmap = mmap_file<float>((char*)usersprefile.c_str(), FileSize(usersprefile), false);
		probeprefloatmap = mmap_file<float>((char*)probeprefile.c_str(), FileSize(probeprefile), false);
	}

	inline int findUserAtMovie(int uindex, int mindex){
		int m = mindex - 1;
		int u = uindex - 1;
		int userid = storedUsersIndex[u];
		int start = storedmovies[m];
		int numvotes = 0;
		if(mindex<totalMovies()) numvotes = storedmovies[m+1] - storedmovies[m];
		else numvotes = totalVotes() - storedmovies[m];
		int end = start + numvotes - 1;
		int i = userBinarySearch(storedvotes, userid, start, end);
		if(guser(storedvotes[i])==userid){
			return i - storedmovies[m];
		}
		return -1;
	}
	
	int userBinarySearch(uint *array, uint value, int start, int end){
		int i = (start + end + 1) / 2;
		while(end-start>0){
			if(guser(array[i])>value) end = i - 1;
			else start = i;
			i = (start + end + 1) / 2;
		}
		return i;
	}
	
};

struct DateStruct{
	int unix_days;
	int userid;
	bool operator<(const DateStruct& rhs) const {
		return userid < rhs.userid;
	}
	bool operator==(const DateStruct& rhs) const {
		return userid == rhs.userid;
	}
	bool operator>(const DateStruct& rhs) const {
		return userid > rhs.userid;
	}
};

#endif

struct ProbeStruct{
	int index;
	int votedate;
	ProbeStruct(int index, int votedate) {
		this->index = index;
		this->votedate = votedate;
	}
	bool operator<(const ProbeStruct& rhs) const {
		return index<rhs.index;
	}
};
