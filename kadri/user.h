/**
 * Copyright (C) 2006-2007 Benjamin C. Meyer (ben at meyerhome dot net)
 * Modifications Copyright (C) 2009 Saqib Kadri (kadence[at]trahald.com)
 * Also see license and terms of packaged code, Copyright Benjamin Meyer
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

#pragma once
#ifndef USER_H
#define USER_H

class User{
public:
	DataBase *db;
	int m_id;
	uint offset;
	uint indexOffset;
	uint constantOffset;
	uint m_size;

	User(DataBase *db, int id = 0);
	User(const User &otherUser);
	/*
		The first valid id is 6
	*/
	void setId(int number);
	/*
		Sets this user to the next user
		*warning* no validation is done to make sure there is a next user.
	*/
	void next();
	int seenMovie(int id) const;

	inline int id() const{
		return m_id;
	}
	inline int votes() const{
		return m_size;
	}

	inline int movie(int x) const{
		return DataBase::guser(db->storedUsers[offset + x]);
	}

/*	inline int score(int x) const{
		return DataBase::gscore(db->storedUsers[offset + x]);
	}*/
	
	inline float score(int x) const{
		if(db->use_preprocessor==true) return db->usersprefloatmap[offset + x];
		else{
			int retval = 0;	//	Must be an int
			retval = DataBase::gscore(db->storedUsers[offset + x]);
			return retval;
		}
	}

	inline DataBase *dataBase() const{
		return db;
	}
	
	inline int numVotes(int uindex) const{
		int u = uindex-userStartIndex;
		//	The number of votes is the next starting index minus this users's starting index
		return db->storedUsersIndex[constantOffset+u] - db->storedUsersIndex[constantOffset+u-1];
	}
	
	//	movie ranges from 1 to 17770
	inline int movie(int uindex, int vindex) const{
		int u = uindex-userStartIndex;
		int v = vindex-voteStartIndex;
		return DataBase::guser(db->storedUsers[db->storedUsersIndex[constantOffset+u-1] + v]);
	}
	
	inline float rating(int uindex, int vindex) const{
		int u = uindex-userStartIndex;
		int v = vindex-voteStartIndex;
		if(db->use_preprocessor==true) return db->usersprefloatmap[db->storedUsersIndex[constantOffset+u-1]+v];
		else{
			int retval = 0;	//	Must be an int
			retval = DataBase::gscore(db->storedUsers[db->storedUsersIndex[constantOffset+u-1]+v]);
			return retval;
		}
	}
	
	//	Returns the real user ID. realid(1) returns 6
	inline uint realid(int uindex){
		int u = uindex-userStartIndex;
		return db->storedUsersIndex[u];
	}

	//	Returns the user index given a real ID. id_to_index(6) returns 1
	inline uint id_to_index(uint realid){
		return db->users[realid]+1;
	}

	//	Returns the vote date
	inline int votedate(int v){
		uint unix_days = db->datesmap[offset + v];
		return unix_days << 16 >> 16;
	}

	//	Returns the vote date
	inline int votedate(int uindex, int v){
		int u = uindex-userStartIndex;
		uint unix_days = db->datesmap[db->storedUsersIndex[constantOffset+u-1] + v];
		return unix_days << 16 >> 16;
	}

	//	Returns the vote date converted to an index (starting at 0)
	inline short votedateIndex(int v){
		if(!db->date_indexes_mapped) return -1;	//	Indexes aren't mapped, so we can't give a value
		short index = db->user_date_indexes[offset + v];
		return index;
	}

	//	Returns the vote date converted to an index (starting at 0)
	inline short votedateIndex(int uindex, int v){
		if(!db->date_indexes_mapped) return -1;	//	Indexes aren't mapped, so we can't give a value
		int u = uindex-userStartIndex;
		short index = db->user_date_indexes[db->storedUsersIndex[constantOffset+u-1] + v];
		return index;
	}

};

#endif
