// functions.h: declarations of global functions
//
// (c) Daria Sorokina

#pragma once
#include "definitions.h"

//deletes spaces from the beginning and from the end of the string
string trimSpace(string& str);

//calculates root mean squared error
double rmse(doublev& predicts, doublev& realvals, doublev& weights);

//removes an element from a vector
int erasev(intv* pVec, int value);

//erase first occurence of item from vector, return its number in variable no
intv::iterator erasev(intv* pVec, int item, int& no);

//returns a place of the first significant digit after zero
int sDigit(double number);

//rounds a positive integer to the order of two important digits
int roundInt(int number);

//rounds up alpha to the closest appropriate value
double adjustAlpha(double alpha, double trainV);

//converts valid values of alpha to string
string alphaToStr(double alpha);

//checks if more bagging will benefit the performance
bool moreBag(doublev bagPerf);

//extends fstream::getline with check on exceeding the buffer size
std::streamsize getLineExt(fstream& fin, char* buf);

//outputs error messages for shared TreeExtra errors
void te_errMsg(TE_ERROR err);

//Expands error messages for std::exception 
void exception_errMsg(string& errstr);

//calculates probabilistic roc - response can be any probability between 0 and 1
//when response values are 0/1, behaves like a standard roc
double roc(doublev& preds, doublev& tars, doublev& weights);

//inserts suffix into a string (usually file name)
string insertSuffix(string fileName, string suffix);

//returns the part of the string (usually a file name) before the last dot
string beforeLastDot(string fileName);

//checks if the first set is a subset of the second set
bipair isSubset(intset& set1, intset& set2);

//checks if the element is in the array
bool isIn(stringv& v, string str);

//converts string to int, throws error if the string is unconvertable
int atoiExt(char* str);

//converts string to double, throws error if the string is unconvertable
double atofExt(char* str);

//returns difference, unless it is due to the rounding error
double diff10d(double d1, double d2);

//returns random double between 0 and 1
double rand_coef();

// XW. Make the rand_coef function thread-safe
double rand_coef(unsigned int& state);

//less function with NaN greater than numbers
bool lessNaN(double i, double j); 

//less function with NaN greater than numbers for pairs
bool lessNaNP(ddpair p1, ddpair p2);

//equals function taking into account NaN
bool equalsNaN(double i, double j);

//converts double to string, NaN to question mark
string ftoaExt(double d);

//equals function for doubles, takes round-off errors into account
bool eqDouble(double i, double j);

//equals or less function for doubles, takes round-off errors into account
bool leDouble(double i, double j);

//less function for doubles, takes round-off errors into account
bool ltDouble(double i, double j);

//greater function comparing by the second item
bool gtSecond(idpair p1, idpair p2);

//less function comparing by the second item
bool ltSecond(fipair p1, fipair p2);

//greater function comparing by the absolute value of the third item
bool gtAbsThird(ssdtriple t1, ssdtriple t2);

//converts number to string
std::string itoa(int value, int base);
