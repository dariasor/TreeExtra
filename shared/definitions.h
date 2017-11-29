// definitions.h: constants, enumerators, typedefs and macros
//
// (c) Daria Sorokina

#pragma once
#pragma warning(disable : 4996)


#include <string>
#include <vector>
#include <set>
#include <map>
#include <limits>
#include <stack>

#include <string.h>
#include <stdlib.h>
#include <float.h>

using namespace std;

typedef set<int> intset;
typedef map<int, double> ifmap;
typedef map<double, double> ddmap;

typedef vector<int> intv;
typedef vector<intv> intvv;
typedef vector<intvv> intvvv;
typedef vector<double> doublev;
typedef vector<doublev> doublevv;
typedef vector<doublevv> doublevvv;
typedef vector<float> floatv;
typedef vector<floatv> floatvv;
typedef vector<string> stringv;
typedef vector<bool> boolv;
typedef vector<boolv> boolvv;

typedef pair<double, int> dipair;
typedef pair<float, int> fipair;
typedef pair<int, double> idpair;
typedef pair<double, double> ddpair;
typedef pair<int, int> iipair;
typedef pair<bool, bool> bbpair;

typedef vector<iipair> iipairv;
typedef vector<dipair> dipairv;
typedef vector<fipair> fipairv;
typedef vector<idpair> idpairv;
typedef vector<ddpair> ddpairv;
typedef vector<dipairv> dipairvv;
typedef vector<fipairv> fipairvv;
typedef vector<bbpair> bbpairv; 

typedef numeric_limits<float> flim;

enum DATA_SET
{
	TRAIN = 1,
	VALID = 2,
	TEST = 3
};

enum TE_ERROR
{
	TREE_LOAD_ERR = 1,
	OPEN_ATTR_ERR = 2,
	MULT_CLASS_ERR = 3,
	NO_CLASS_ERR = 4,
	OPEN_TRAIN_ERR = 5,
	OPEN_VALID_ERR = 6,
	OPEN_TEST_ERR = 7,
	ATTR_ID_ERR = 8,
	LONG_LINE_ERR = 9,
	VALID_EMPTY_ERR = 10,
	ROC_ERR = 11,
	ATTR_DATA_MISMATCH_L_ERR = 12,
	MODEL_ATTR_MISMATCH_ERR = 13,
	ATTR_NAME_ERR = 14,
	NO_EFFECT_ERR = 15,
	MODEL_ERR = 16,
	EMPTY_MODEL_NAME_ERR = 17,
	ATTR_TYPE_ERR = 18,
	ATTR_NOT_BOOL_ERR = 19,
	TREE_WRITE_ERR = 20,
	TRAIN_EMPTY_ERR = 21,
	ATTR_NAME_DEF_ERR = 22,
	NOM_ACTIVE_ERR = 23,
	ATTR_DATA_MISMATCH_G_ERR = 24,
	NUMERIC_ARG_ERR = 25,
	ROC_FLAT_ERR = 26,
	OPEN_OUT_ERR = 27,
	MV_CLASS_TRAIN_ERR = 28,
	MV_CLASS_VALID_ERR = 29,
	NON_NUMERIC_VALUE_ERR = 30
};

//this enum has to be in the general definition file, because it is a part of a model file, and all model 
//files should be compatible
enum AG_TRAIN_MODE
{
	FAST = 201,
	SLOW = 202,
	LAYERED = 203
};


#define VERSION "2.5.3" //release version
#define LINE_LEN 20000	//maximum length of line in the input file
#define QNAN flim::quiet_NaN()

#if defined(_MSC_VER) && (_MSC_VER <= 1600)
    #define isnan(a) _isnan(a)	//for old versions of Visual Studio that did not have isnan
#endif
