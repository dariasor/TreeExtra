// LogStream.cpp: implementation of LogStream class and << operator
// Redirects output to both console and file log.txt
// (c) Daria Sorokina

#include "LogStream.h"
#include "definitions.h"

//static variable
bool LogStream::doOut = true;

//static initialization, needs to be called once in the whole program
void LogStream::init(bool doOut_in)
{ 
	doOut = doOut_in;
	fstream fout; 
	fout.open("log.txt", ios_base::out); 
	fout.close(); 
}


