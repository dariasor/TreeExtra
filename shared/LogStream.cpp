// LogStream.cpp: implementation of LogStream class and << operator
// Redirects output to both console and file log.txt
// (c) Daria Sorokina

#include "LogStream.h"
#include "definitions.h"

//static variable showing if log messages are sent into the standard output in addition to log files
bool LogStream::doOut = true;

//static initialization, needs to be called once in the whole program
void LogStream::init(bool doOut_in)
{ 
	doOut = doOut_in;

	//if log.txt exists, append its content to log.archive.txt
	fstream foldlog;
	foldlog.open("log.txt", ios_base::in);
	if(foldlog)
	{
		fstream farchive;
		farchive.open("log.archive.txt", ios_base::out | ios_base::app);
		farchive << foldlog.rdbuf();
	}
	
	//open clean log.txt
	fstream fout; 
	fout.open("log.txt", ios_base::out);
	if(!fout)
		cout << "\nWARNING: failed to open log file log.txt\n";
	fout.close(); 
}


