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

#ifndef MMAP_CPP
#define MMAP_CPP

#include <iostream>
#ifdef IS_OS_WIN
#include "winmmap.cpp"
#else
#include <sys/mman.h>
#endif
#include <fcntl.h>
#include "util.cpp"

//jmgonc
#include <string.h>
#include <stdio.h>

//	Example (stores 5 ints):	int * map = mmap_file<int>("file.txt", 5*sizeof(int));
//	Return type of function is based on the template class
template<class A> A * mmap_file(char* filename, size_t filesize, bool writemap=true){
	int fd = open(filename, O_RDWR | O_CREAT, (mode_t)0644);
	if(fd == -1) {
		perror("Error opening (or creating) file.");
		return 0;
	}
	//	If the file is not large enough (which will be the case if it was just created), stretch it (if writemap is true)
	if(util::FileSize(filename)<filesize && writemap==true){
		//	lseek moves the read/write offset to filesize minus one
		int result = lseek(fd, filesize-1, SEEK_SET);
		if(result == -1) {
			perror("Error calling lseek() to 'stretch' the file");
			return 0;
		}
		//	Write empty string to the end of the file so that the file has size filesize
		result = write(fd, "", 1);
		if(result != 1) {
			perror("Error writing last byte of the file");
			return 0;
		}
	}
	A *map;
	//	Need to cast the mmap
	if(writemap) map = (A*) mmap(0, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	else map = (A*) mmap(0, filesize, PROT_READ, MAP_SHARED, fd, 0);
	if(map==MAP_FAILED) {
		perror(strcat("Error mmapping the file ", filename));
		return 0;
	}
	return map;
}

#endif
