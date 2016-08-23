// LogStream.h: declaration of LogStream class and implementation of LogStream << operator
// Redirects output to both console and file log.txt
// (c) Daria Sorokina

#pragma once

#include <fstream>
#include <iostream>

#include "definitions.h"

class LogStream
{
public:
	//Clears or creates log.txt file. Should be called once in the whole program.
	static void init(bool doOut_in);
	static bool doOut; //turns on/off console output
};	

template <class T>
LogStream& operator << (LogStream& logcout, T data)
{
	if(LogStream::doOut)
	{
		cout << data;
		cout.flush();
	}

	fstream fout;
	fout.open("log.txt", ios_base::out | ios_base::app);

	fout << data;
	fout.close();
	return logcout;
}
