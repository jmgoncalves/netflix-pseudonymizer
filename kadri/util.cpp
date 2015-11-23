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

#ifndef UTIL_CPP
#define UTIL_CPP

#include <iostream>
#include <vector>
#include <valarray>
#include <map>
#include <fstream>
#include <sys/time.h>
#include <fcntl.h>
#include <sstream>

//	Check to see if this is a Windows OS
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
	#define IS_OS_WIN true
#endif

#ifdef IS_OS_WIN
	//#include "gettimeofday.c"
	#include <sys/stat.h>
	//	Define the uint, ushort, and ulong types for Windows
	typedef unsigned int uint;
	typedef unsigned short ushort;
	typedef unsigned long int ulong;
#endif

//jmgonc
#include <sys/stat.h>

using namespace std;

//	General functions
namespace util{

	//	Convert a type, especially floats and doubles, to a string
	//	Note that to use with printf(), you have to use c_str(), e.g.:
	//		printf("%s\n", to_string(3.14).c_str());
	template <class A> string to_string(A a){
		stringstream s;
		s << a;
		return s.str();
	}
	
	template<class A> A array_max(A arr[], int slicestart=0, int sliceend=0){
		A max = arr[slicestart];
		if(sliceend==0) sliceend = sizeof(arr);
		else sliceend++;	//	Check the end index
		for(int i=slicestart+1; i<sliceend; i++) if(arr[i]>max) max=arr[i];
		return max;
	}
	
	template<class A> A array_min(A arr[], int slicestart=0, int sliceend=0){
		A min = arr[slicestart];
		if(sliceend==0) sliceend = sizeof(arr);
		else sliceend++;	//	Check the end index
		for(int i=slicestart+1; i<sliceend; i++) if(arr[i]<min) min=arr[i];
		return min;
	}

	int min(int a, size_t b){
		if(a<b) return a;
		else return (int) b;
	}

	template<class A> int BinarySearch(A *arr, int len, A value){
		int lower = 0;
		int upper = len-1;
		int i = (upper+lower+1)/2;
		while(upper-lower>0){
			if(arr[i]>value) upper = i - 1;
			else lower = i;
			i = (upper+lower+1)/2;
		}
		return i;
	}
}

//	File functions
namespace util{
	
	bool FileExists(const char* filename){
		struct stat stFileInfo;
		int iStat = stat(((string) filename).c_str(), &stFileInfo);
		//	if iStat is 0, then file attributes were present, thus the file exists
		if(iStat==0) return true;
		//	else the file doesn't exist, or we don't have permissions for the folder
		//	check return value of stat for more details
		else return false;
	};
	
	//	Alias for FileExists() using string
	bool FileExists(string filename){
		return FileExists(filename.c_str());
	}
	
	size_t FileSize(const char * szFileName){
		struct stat fileStat;
		int err = stat(szFileName, &fileStat);
		if(err!=0) return 0;	//	err will be 0 is everything is fine
		return fileStat.st_size;
	}
	
	//	Alias for FileSize using string
	size_t FileSize(string szFileName){
		return FileSize(szFileName.c_str());
	}
	
	//	Retrieve a specific value from a binary file's ifstream
	template<class A> A get_binary_value(ifstream &in, int num){
		&in.seekg(num*sizeof(A), ios_base::beg);
		A value;
		&in.read((char*)&value, sizeof(A));
		return value;
	}

	//	Save a vector in a file
	template<class A> void vectorToFile(const vector<A> &v, const string &filename){
		ofstream out(filename.c_str(), ios::binary);
		for(int i=0; i<v.size(); i++){
			A val = v.at(i);
			out.write((char*)&val, sizeof(A));
		}
		out.close();
	}

}

//	Time functions
namespace util{

	double micro_time(){
		struct timeval t;
		gettimeofday(&t, NULL);
		double d = t.tv_sec + (double) t.tv_usec/1000000;
		return d;
	};

	vector<string> script_names;
	map<string, int> script_calls;
	map<string, map<string, double> > timer_map;
	map<string, double> timer_summary_map;
	//	Set the timer_map. "Start" contains the time of the first call, "Mid" the start of the last call, and "End" the end of the last call.
	//	"Elapsed" contains the overall time used among all calls.
	void script_timer(string script_name, bool is_end=false){
		//check if present using binary search on v
		//if not present, insert, then sort(v.begin(), v.end())
		bool previously_called = binary_search(script_names.begin(), script_names.end(), script_name);
		if(is_end==false){
			//	If called before, don't change the Start time. Instead set a Mid time.
			if(previously_called) timer_map[script_name]["Mid"] = micro_time();
			else{
				//	If first call, insert script name, and then sort so that binary searches can be run
				script_names.push_back(script_name);
				sort(script_names.begin(), script_names.end());
				timer_map[script_name]["Start"] = micro_time();
				//	And set calls to 0
				script_calls[script_name] = 0;
			}
		}
		else{
			timer_map[script_name]["End"] = micro_time();
			//	If script_calls>0 set, add End-Mid to Elapsed
			//	Else set Elapsed to equal End-Start
			if(previously_called && script_calls[script_name] > 0) timer_map[script_name]["Elapsed"] += timer_map[script_name]["End"] - timer_map[script_name]["Mid"];
			else timer_map[script_name]["Elapsed"] = timer_map[script_name]["End"] - timer_map[script_name]["Start"];
			timer_summary_map[script_name] = timer_map[script_name]["Elapsed"];
			script_calls[script_name]++;
		}
	};
	
	//	Print the timer_summary_map, or optionally return it
	string print_timer_summary_map(bool returnstring=false){
		string s;
		for(map<string, double>::iterator iter=timer_summary_map.begin(); iter!=timer_summary_map.end(); iter++){
			char buffer[128];
			sprintf(buffer, "%f", iter->second);
			s = s + iter->first + "\t" + buffer + "\n";
		}
		if(returnstring) return s;
		fprintf(stderr, "%s\n", s.c_str());
		string blank;
		return blank;
	}

}

//	Mathematical functions
namespace util{

	//	Calculate the unbiased linear Beta estimator for the model yi=Beta*xi+error
	template<class A, class B> double calcbeta(vector<A> y, vector<B> x){
		if(y.size()!=x.size()) return 0;
		int size = y.size();
		valarray<double> Y(size);
		valarray<double> X(size);
		copy(y.begin(), y.end(), &Y[0]);
		copy(x.begin(), x.end(), &X[0]);
		double Beta = (Y*X).sum() / (X*X).sum();
		return Beta;
	}

	//	Calculate the square root of a number that may be negative, preserving its sign
	float sqrtreal(float x){
		if(x<0) return -1*sqrt(abs(x));
		else return sqrt(x);
	}

	//	Raise a number that may be negative to a power, preserving its sign
	float powreal(float x, float exponent){
		if(x<0) return -1*pow(abs(x), exponent);
		else return pow(x, exponent);
	}
	
}

//	Structs
namespace util{
	struct Element{
		int key;
		float value;
		Element(int k, float v){
			key = k;
			value = v;
		}
		bool operator<(const Element& rhs) const {
			return value < rhs.value;
		}
		bool operator>(const Element& rhs) const {
			return value > rhs.value;
		}
		bool operator==(const Element& rhs) const {
			return value == rhs.value;
		}
		bool operator!=(const Element& rhs) const {
			return value != rhs.value;
		}
	};
}

#endif
