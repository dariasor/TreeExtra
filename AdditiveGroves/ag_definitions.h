// Additive Groves / ag_definitions.h: constants, enumerators, typedefs and macros
//
// (c) Daria Sorokina

#pragma once
#pragma warning(disable : 4996)

enum AG_ERROR
{
	INPUT_ERR = 101,
	WIN_ERR = 102,
	ALPHA_ERR = 103,
	TIGN_ERR = 104,
	BAGN_ERR = 105,
	TEMP_ERR = 106, 
	OPEN_NWAY_ERR = 107,
	DIR_ERR = 108,
	MERGE_MISMATCH_ERR = 109,
	SAME_SEED_ERR = 110,
	TRAIN_EQ_VALID_ERR = 111
};

