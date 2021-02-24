// TreeNode.h: interface for the CTreeNode class.
// Represents a node of a regression (decision) tree
// Contains functions used during training and prediction stages
// The performance metric used by the tree is Root Mean Squared Error (RMSE)

// (c) Daria Sorokina

#pragma once

#include "INDdata.h"
#include "SplitInfo.h"

//Node of a regression tree
class CTreeNode  
{
private:
	static INDdata* pData;
public:
	//initialize static data pointer
	static void setData(INDdata& data){pData = &data;}

public:
	//constructor
	CTreeNode();
	
	//copy constructor
	CTreeNode(const CTreeNode& original);
	
	//destructor
	virtual ~CTreeNode();

	//assignment operator
	CTreeNode& operator=(const CTreeNode& rhs);

	//get functions 
	int getDivAttr() {return splitting.divAttr;}
	double getThresh() {return splitting.border;}
	double getResp() {return (*pItemSet)[0].response;} //should be applied to leaves only
	double getNodeV();
	
	double getEntropy(int attrNo); //get entropy of this feature in this node
	
	//initializes fresh root
	void setRoot();

	//changes train set responses to residuals
	void resetRoot(doublev& othpreds);

	//deletes an attribute from the internal structures
	void delAttr(int attrNo);

	//checks if a node is a leaf
	bool isLeaf() {return left == NULL;}

	//sends a test case down the tree (used in generating prediction for the test case)
	void traverse(int itemNo, double coef, double& ltCoef, double& rtCoef, DATA_SET dset);

	//splits the node; grows two offsprings 
	bool split(double alpha, double* pEntropy = NULL);

	//saves the node into a binary file
	void save(fstream& fsave);

	//loads the node from a binary file
	bool load(fstream& fload);

	
private:
	//delete a subtree
	void del();	

	//returns several summaries of the prediction values set in this node
	bool getStats(double& nodeV, double& nodeSum);

	//cleans training data out of a leaf
	void makeLeaf(double nodeMean); 

	//finds and sets a splitting info with the best MSE
	bool setSplit(double nodeV, double nodeSum);

	//finds and sets a splitting info with the best MSE when weights are present in the data
	bool setSplitW(double nodeV, double nodeSum);

	//finds and sets a splitting info with the best MSE when missing values and possibly weights are present in the data
	bool setSplitMV(double nodeV, double nodeSum);

	//evaluates boolean split
	double evalBool(SplitInfo& canSplit, double nodeV, double nodeSum);

	//evaluates boolean split with weights present
	double evalBoolW(SplitInfo& canSplit, double nodeV, double nodeSum);

	//evaluates boolean split when missing values present in the data
	double evalBoolMV(SplitInfo& canSplit, double nodeV, double nodeSum, double missV, double missSum);

public:
	CTreeNode*	left;		//pointer to the left child
	CTreeNode*	right;		//pointer to the right child

private:
	ItemInfov*	pItemSet;	//subset of the training set that belongs to the node during training
	fipairvv*   pSorted;	//current itemset indexes sorted by value of attribute
	intv*		pAttrs;		//set of valid attributes in the node	
	SplitInfo	splitting;	//split (attribute, split point, proportion for missing values)

};

typedef vector<CTreeNode> CTreeNodev;
typedef pair<CTreeNode*, double> nodecoefp;
typedef pair<CTreeNode*, int> nodeip;
typedef stack<nodeip> nodehstack;
