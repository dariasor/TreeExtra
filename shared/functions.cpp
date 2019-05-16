// functions.cpp: definitions of global functions 
// 
// (c) Daria Sorokina

#include "functions.h"
#include "definitions.h"
#include "ErrLogStream.h"
#include "gtest-internal.h"

#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

//Deletes spaces from the beginning and from the end of the string
//By "spaces" I mean spaces only, not white spaces
string trimSpace(string& str)
{
	int n = (int)str.size();
	
	int b; 
	for(b = 0; (b < n) && (str[b] == ' '); b++);
	
	if(b == n)
	{
		str.clear();
		return string();
	}
	
	int e;
	for(e = n - 1; str[e] == ' '; e--);
	
	return str.substr(b, e - b + 1);
}

//Calculates Root Mean Squared Error (RMSE)
double rmse(doublev& predicts, doublev& realvals, doublev& weights)
{
	int n = (int)predicts.size();

	double mse = 0;
	double wSum = 0;
	for(int i = 0; i < n; i++)
	{
		double err = diff10d(predicts[i], realvals[i]);
		mse += pow(err, 2) * weights[i];
		wSum += weights[i];
	}

	mse /= wSum;
	return sqrt(mse);
}

//Removes an element from a vector
//It is assumed that exactly one copy of the element is present in the vector
int erasev(intv* pVec, int value)
{
	int elemNo = 0;
	for(; (elemNo < (int)pVec->size()) && ((*pVec)[elemNo] != value); elemNo++);
	pVec->erase(pVec->begin() + elemNo);
	return elemNo;
}

//erases first occurence of item from vector, returns its number in no
intv::iterator erasev(intv* pVec, int item, int& no)
{
	for(no = 0; (*pVec)[no] != item; no++);
	return pVec->erase(pVec->begin() + no);
}

//returns a place of the first significant digit after zero
//works only on numbers between 0 and 1
int sDigit(double number)
{
	if((number <= 0) || (number >= 1))
		return -1;

	int ret = 1;
	while(number < 0.1)
	{
		ret++;
		number *= 10;
	}

	return ret;
}

//rounds a positive integer to the order of two important digits
int roundInt(int number)
{
	if(number > 100)
	{
		int upper = 1;
		while(number > upper)
			upper *= 10;

		upper /= 100;
		return (int)round((double)number / upper) * upper;
	}
	return number;
}

//Rounds alpha to the closest appropriate value
double adjustAlpha(double alpha, double trainV)
{
	//A leaf cannot contain less than 1 point, alpha too small is indistinguishable from zero
	//well, now we have weights and the above statement is not true any more, but I don't want to modify this part unless I really need to
	if(leDouble(alpha, 1.0 / trainV))
		return 0;

	//Check if the original alpha is good. This part does not seem to be necessary, but 
	//otherwise annoying round-off errors happen and mess up the further comparisons
	double copyAlpha = alpha;
	while(ltDouble(copyAlpha, 1.0))
		copyAlpha *= 10;
	if(eqDouble(copyAlpha, 1.0) || eqDouble(copyAlpha, 2.0) || eqDouble(copyAlpha, 5.0))
		return alpha;

	// Round alpha up to one of valid values of the form 0.0...0n, where n = 1, 2 or 5.
	// Rounding borders are coarse, because they do not really matter too much.
	double newAlpha = 1;
	while(true)
	{
		if(alpha > 3)
			newAlpha *= 5;
		else if(alpha > 1.5)
			newAlpha *= 2;
		else if(alpha > 0.7)	//entering next level of granularity
			newAlpha *= 1;
		else
			newAlpha *= 0.1;

		//if the digit was non-zero, done
		if(alpha > 0.7)
			break;		
		
		alpha *= 10;
	}
	return newAlpha;
}

//Converts valid values of alpha to string
//Because of rounding errors it has to take care of almost valid values of alpha as well
string alphaToStr(double alpha)
{

	if(alpha == 0)
		return "0.0";

	if(alpha > 0.8)
		return "1.0";

	string ret = string("0.");
	while(true)
	{
		alpha *= 10;
		//round find next digit 
		if(alpha > 4.5)
			ret += '5';
		else if(alpha > 1.5)
			ret += '2';
		else if(alpha > 0.8)
			ret += '1';
		else
			ret += '0';

		//if the digit was non-zero, done
		if(alpha > 0.8)
			break;
	}
	return ret;
}

//analyzes rms values on bagging iterations we got so far and checks 
//if more bagging will benefit the performance
//
//the answer is No if 
//	relative improvement over last 20 iterations is < 1/1000 
//	and 
//	bagging has converged: relative difference between max and min over last 20 iterations is < 1/500
bool moreBag(doublev bagPerf)
{
	int bagN = (int)bagPerf.size();
	if(bagN <= 20)
		return true;
	
	//find the best and worst performance during last 20 iterations
	double bestPerf = bagPerf[bagN - 1];
	double worstPerf = bagPerf[bagN - 20];
	for(int bagNo = bagN - 20; bagNo < bagN; bagNo++)
	{
		if(bagPerf[bagNo] < bestPerf)
			bestPerf = bagPerf[bagNo];
		if(bagPerf[bagNo] > worstPerf)
			worstPerf = bagPerf[bagNo];
	}

	//calculate reverse relative difference between max and min
	double relMaxMin;
	if(worstPerf > bestPerf)
		relMaxMin = worstPerf / (worstPerf - bestPerf);
	else
		relMaxMin = QNAN;

	//calculate reverse relative improvement over last 20 iterations
	double relImprove;
	if(bagPerf[bagN - 21] > bagPerf[bagN - 1])
		relImprove = bagPerf[bagN - 21] / (bagPerf[bagN - 21] - bagPerf[bagN - 1]);
	else
		relImprove = QNAN;
	
	//check stopping criterion
	if((isnan(relMaxMin) || (relMaxMin > 500)) && (isnan(relImprove) || (relImprove > 1000)))
		return false;
	return true;
}

//extends fstream::getline with check on exceeding the buffer size
std::streamsize getLineExt(fstream& fin, char* buf)
{
	fin.getline(buf, LINE_LEN);
	std::streamsize bufLen = fin.gcount();
	if(bufLen == LINE_LEN - 1)
		throw LONG_LINE_ERR;
	return bufLen;
}

//outputs error messages for shared TreeExtra errors
void te_errMsg(TE_ERROR err)
{
	ErrLogStream errlog;
	errlog << "\n";
	switch(err)
	{
		case OPEN_ATTR_ERR:
			errlog << "Error: failed to open attribute file.\n";
			break;
		case OPEN_TRAIN_ERR:
			errlog << "Error: failed to open train set file.\n";
			break;
		case OPEN_VALID_ERR:
			errlog << "Error: failed to open validation set file.\n";
			break;
		case OPEN_TEST_ERR:
			errlog << "Error: failed to open test set file.\n";
			break;
		case OPEN_OUT_ERR:
			errlog << "Error: failed to create an output file.\n";
			break;
		case MULT_CLASS_ERR:
			errlog << "Error: multiple attributes marked as class (response) attributes.\n";
			break;
		case NO_CLASS_ERR:
			errlog << "Error: response attribute should be marked with \"(class)\".\n";
			break;
		case LONG_LINE_ERR:
		{
			int limit = LINE_LEN;
			errlog << "Error: lines in the data file exceed current limit of "  << limit
				<< " symbols. The limit can be changed by modifing LINE_LEN constant in "
				<< "\"shared/definition.h\" source file.\n";
			break;
		}
		case ATTR_ID_ERR:
			errlog << "Error: model and data do not match.\n";
			break;
		case ROC_ERR:
			errlog << "Error: can't calculate ROC with response values out of [0,1] range.\n";
			break;
		case ATTR_DATA_MISMATCH_L_ERR:
			errlog << "Error: data has fewer attributes than listed in the attribute file.\n";
			break;
		case ATTR_DATA_MISMATCH_G_ERR:
			errlog << "Error: data has more attributes than listed in the attribute file.\n";
			break;
		case MODEL_ATTR_MISMATCH_ERR:
			errlog << "Error: model and attribute file do not match.\n";
			break;
		case ATTR_NAME_ERR:
			errlog << "Error: attribute name misspelled or the attribute is not active.\n";
			break;
		case TREE_LOAD_ERR:
			errlog << "Error: model (or temporary) file is missing or corrupted.\n";
			break;
		case NO_EFFECT_ERR:
			errlog << "Error: One of the features does not have any effect on the response: it takes on a single value on the whole validation set.\n";
			break;
		case MODEL_ERR:
			errlog << "Error: model file is missing or corrupted.\n";
			break;
		case ATTR_TYPE_ERR:
			errlog << "Error: wrong attribute type. Type can be continuous (\"cont\"), nominal (\"nom\") or boolean (\"0,1\").\n";
			break;
		case ATTR_NOT_BOOL_ERR:
			errlog << "Error: boolean attribute's value is different from \"0\" or \"1\".\n";
			break;
		case TREE_WRITE_ERR:
			errlog << "Error: could not write the model to the file. Disk space problems?\n";
			break;
		case TRAIN_EMPTY_ERR:
			errlog << "Error: train set is empty.\n";
			break;
		case VALID_EMPTY_ERR:
			errlog << "Error: validation set is empty.\n";
			break;
		case ATTR_NAME_DEF_ERR:
			errlog << "Error: attribute names can not include any of \\/*?\":|<> symbols.\n";
			break;
		case NOM_ACTIVE_ERR:
			errlog << "Error: a nominal attribute can not be active.\n";
			break;
		case NUMERIC_ARG_ERR:
			errlog << "Error: non-numeric value for a numeric argument.\n";
			break;
		case ROC_FLAT_ERR:
			errlog << "Error: cannot calculate ROC - all labels have the same value.\n";
			break;
		case MV_CLASS_TRAIN_ERR:
			errlog << "Error: missing values in the train set class(response) column.\n";
			break;
		case MV_CLASS_VALID_ERR:
			errlog << "Error: missing values in the validation set class(response) column.\n";
			break;		
		case NON_NUMERIC_VALUE_ERR:
			errlog << "Error: non-numeric value for an active attribute in the data.\n";
			break;
		case CORR_MV_ERR:
			errlog << "Error: the data has missing values and cannot be used for correlation analysis.\n";
			break;
		default:
			throw err;
	}
}

//Expands error messages for std::exception 
void exception_errMsg(string& errstr)
{
	if(errstr.compare("St9bad_alloc") == 0)
		errstr += ". Out of memory.";
}


// this code for ROC is taken (and adjusted a bit) from PERF software:
		//R. Caruana, The PERF Performance Evaluation Code,
		// http://www.cs.cornell.edu/~caruana/perf
double roc(doublev& preds, doublev& tars, doublev& weights)
{
	int itemN = (int)preds.size();
	double volume = 0; //sum of all weights

	dddtriplev data(itemN);
	bool onlyOnes = true;
	bool onlyZeros = true;
	for(int i = 0; i < itemN; i++)
	{
		data[i].first = preds[i];
		data[i].second.first = tars[i];
		data[i].second.second = weights[i];
		volume += weights[i];
		if((tars[i] < 0) || (tars[i] > 1))
			throw ROC_ERR;
		if(tars[i] != 0)
			onlyZeros = false;
		if(tars[i] != 1)
			onlyOnes = false;
	}
	if(onlyZeros || onlyOnes)
		throw ROC_FLAT_ERR;
    
	sort(data.begin(), data.end());
	
	/* get the fractional weights. If there are ties we count the number
    of cases tied and how many positives there are, and we assign  
    each case to be #pos/#cases positive */

	doublev fraction(itemN);
	int item = 0;
	while(item < itemN)
	{
		int begin = item;
		double posV = 0;
		double tieV = 0;
		for(;(item < itemN) && (data[item].first == data[begin].first); item++)
		{
			posV += data[item].second.first * data[item].second.second; 
			tieV += data[item].second.second;
		}

		double curFrac = posV / tieV;
		for(int i = begin; i < item; i++)
			fraction[i] = curFrac;
    }

	double pos = 0.0; //number of positives in the data
	double neg = 0.0; //number of negatives in the data
	for(int i = 0; i < itemN; i++)
	{
		pos += data[i].second.first * data[i].second.second;
		neg += (1 - data[i].second.first) * data[i].second.second;
	}
   
	double tpos = 0.0; //number of true positives
	double fpos = 0.0; //number of false positives

	double roc_area = 0.0;
	double tpr_prev = 0.0; //tpr = true positives rate
	double fpr_prev = 0.0; //fpr = false positives rate

	//calculate auc incrementally
	for(item = itemN - 1; item > -1; item--)
    {
		tpos += fraction[item] * data[item].second.second;
		fpos += (1 - fraction[item]) * data[item].second.second;
		double tpr  = tpos / pos;
		double fpr  = fpos / neg;
		roc_area += 0.5 * (tpr + tpr_prev) * (fpr - fpr_prev);
		tpr_prev = tpr;
		fpr_prev = fpr;
    }

	return roc_area;
}

//inserts suffix into a string (usually file name)
//if dots present, inserts before the last dot
//otherwise inserts after the original file name
string insertSuffix(string fileName, string suffix)
{
	string newFName;
	string::size_type lastDot = fileName.rfind('.');
	if(lastDot == string::npos)
		newFName = fileName + "." + suffix;
	else
		newFName = fileName.substr(0, lastDot) + "." + suffix + fileName.substr(lastDot);

	return newFName;
}

//returns the part of the string (usually a file name) before the last dot
//if there are no dots, returns the original string
string beforeLastDot(string fileName)
{
	string prefix;
	string::size_type lastDot = fileName.rfind('.');
	if(lastDot == string::npos)
		return fileName;
	else
		return fileName.substr(0, lastDot);
}

//checks if the first set is a subset of the second set
bipair isSubset(intset& set1, intset& set2)
{
	for(intset::iterator it1 = set1.begin(); it1 != set1.end(); it1++)
		if(set2.find(*it1) == set2.end())
			return bipair(false, *it1);
	return bipair(true, 0);
}

//converts string to int, throws error if the string is unconvertable
int atoiExt(char* str)
{
	char *end;
	long value = strtol(str, &end, 10);
	if((end == str) || (*end != '\0'))
		throw NUMERIC_ARG_ERR;
	return (int) value;
}

//converts string to double, throws error if the string is unconvertable
double atofExt(char* str)
{
	char *end;
	double value = strtod(str, &end);
	if((end == str) || (*end != '\0'))
		throw NUMERIC_ARG_ERR;
	return value;
}

//returns difference, unless it is due to the rounding error
double diff10d(double d1, double d2)
{
	if((d1 == 0) && (d2 == 0))
		return 0;
	double base = max(fabs(d1), fabs(d2));
	double diff = d1 - d2;
	if(fabs((d1 - d2) / base) < 0.0000000001)
		return 0;
	return diff;
}

//returns random double between 0 and 1
double rand_coef()
{
#if RAND_MAX > 32767
		return (double) rand() / RAND_MAX;
#else
		return (double) ((rand() << int(log(RAND_MAX + 1.0) / log(2.0) + 0.5)) + rand() ) / ((RAND_MAX + 1) * (RAND_MAX + 1) - 1);
#endif
}

//less function with NaN greater than numbers
bool lessNaN(double i, double j) 
{ 
	if(isnan(j) && !isnan(i))
		return true;
	else
		return (i<j); 
}

//equals function taking into account NaN
bool equalsNaN(double i, double j)
{
	return (i == j) || isnan(i) && isnan(j);
}

//less function with NaN greater than numbers for pairs
bool lessNaNP(ddpair p1, ddpair p2)
{
	if(lessNaN(p1.first, p2.first))
		return true;
	if(equalsNaN(p1.first, p2.first))
		return lessNaN(p1.second, p2.second);
	return false;
}

//converts double to string, NaN to question mark
string ftoaExt(double d)
{
	if(isnan(d))
		return "?";

	stringstream s;
	s << d;
	return s.str();
}

//equals function for doubles, takes round-off errors into account
bool eqDouble(double i, double j)
{
	const FloatingPoint<double> lhs(i), rhs(j);
	return lhs.AlmostEquals(rhs);
}

//equals or less function for doubles, takes round-off errors into account
bool leDouble(double i, double j)
{
	return (i < j) || eqDouble(i,j);
}

//less function for doubles, takes round-off errors into account
bool ltDouble(double i, double j)
{
	return (i < j) && !eqDouble(i,j);
}

//greater function comparing by the second item
bool gtSecond(idpair p1, idpair p2)
{
	return p1.second > p2.second;
}

//less function comparing by the second item
bool ltSecond(fipair p1, fipair p2)
{
	return p1.second < p2.second;
}

//greater function comparing by the absolute value of the third item
bool gtAbsThird(ssdtriple t1, ssdtriple t2)
{
	return abs(t1.second) > abs(t2.second);
}
