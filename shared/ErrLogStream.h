// ErrLogStream.h: implementation of ErrLogStream class and << operator
// Redirects output to both cerr (console error output) and file log.txt
// (c) Daria Sorokina

#pragma once

#include <fstream>
#include <iostream>

class ErrLogStream
{
};	

template <class T>
ErrLogStream& operator << (ErrLogStream& errlogout, T& data)
{
	cerr << data;
	cerr.flush();

	fstream fout;
	fout.open("log.txt", ios_base::out | ios_base::app);
	fout << data;
	fout.close();
	return errlogout;
}
