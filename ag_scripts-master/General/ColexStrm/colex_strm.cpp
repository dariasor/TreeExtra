//colex_strm: main function of executable colex_strm
//Simpler version of |Stat colex that allows for larger input lines
//Takes data from the input stream
//
//(c) Daria Sorokina

#pragma warning(disable : 4996)

#include <fstream>
#include <iostream>
#include <sstream>
#include <errno.h>

#include <vector>
#include <string>
#include <string.h>
#include <stdlib.h>

using namespace std;

typedef vector<int> intv;
typedef vector<string> stringv;

void split(const string &s, char delim, stringv &elems) {
    elems.clear();
	stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

const int lineLen = 150000; //max size that I so far needed

//colex_strm _lineNo1_ _lineNo2_ ...
int main(int argc, char* argv[])
{
	try{

	if(argc < 2)
		throw string("Usage: colex_strm _colNo1_ _colNo2_ ...\n1 refers to first column.");

	intv columns; //numbers of columns to print
	char *end;
	for(int argNo = 1; argNo < argc; argNo++)
	{
		long value = strtol(argv[argNo], &end, 10);
		if(!value)
			throw string("Usage: colex_strm _colNo1_ _colNo2_ ...\n1 refers to first column.");
		columns.push_back((int)value);
	}
	//read data line by line, from every line output values of requested columns
	char buf[lineLen];
	cin.getline(buf, lineLen);
	int colN = -1;	//number of columns
	while(!cin.fail())
	{
		//convert current line to a set of column values (strings)
		stringv colVals;
		string line(buf);
		split(line, '\t', colVals);
		
		if (colN == -1)
		{
			colN = (int)colVals.size();
			for (int outColNo = 0; outColNo < (int)columns.size(); outColNo++)
			if (columns[outColNo] > colN)
				throw string("Error: the data has fewer columns than max requested column number");
		}
		else
			if(colVals.size() != colN)
				throw string("Error: number of columns is inconsistent across lines in the input file");

		//output values of requested columns from current line
		for (int outColNo = 0; outColNo < (int)columns.size(); outColNo++)
		{
			if (outColNo > 0)
				cout << "\t";
			cout << colVals[columns[outColNo] - 1];
		}
		cout << endl;

		cin.getline(buf, lineLen);
		colVals.clear();
	}

	}catch(string err){
		cerr << err << "\n" << endl;
		return 1;
	}catch(...){
		string errstr = strerror(errno);
		cerr << "Error: " << errstr << endl;
		return 1;
	}
	return 0;
}




