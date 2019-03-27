// Bagged Trees / Tree.cpp: implementation of class CTree
//
// (c) Daria Sorokina

#include "Tree.h"
#include "functions.h"

#include <math.h>
#include <stack>
#include <fstream>
#include <algorithm>

INDdata* CTree::pData;
#ifndef _WIN32
TThreadPool* CTree::pPool;

//job class, splits a node, used for multithreading in unix
class CNodeSplitJob : public TThreadPool::TJob
{
public:
    
    CNodeSplitJob() : TThreadPool::TJob() { }
    
    void Run(void* ptr)
    {
		JobData* pJD = (JobData*) ptr;
		nodeip curNH = pJD->curNH;
		TCondition& nodesCond = *pJD->pNodesCond;
		doublev* pAttrCounts = pJD->pAttrCounts;

		double nodeV = 0;
		double entropy = 0;
		double* pEntropy = NULL;
		if(pAttrCounts)
		{
			nodeV = curNH.first->getNodeV();
			pEntropy = &entropy;
		}
		double h = curNH.second;
		double curAlpha = (pJD->H == 0) ? 1 : pow(2, - ( pJD->b +  pJD->H) * h /  pJD->H +  pJD->b);
		bool notLeaf = curNH.first->split(curAlpha, pEntropy);
		
		nodesCond.Lock();
		if(notLeaf)
		{
			pJD->pNodes->push(nodeip(curNH.first->left, curNH.second + 1));
			pJD->pNodes->push(nodeip(curNH.first->right, curNH.second + 1));
			*pJD->pToDoN += 1;

			if(pAttrCounts)
			{
				entropy = max(entropy, 1.0);
				(*pAttrCounts)[curNH.first->getDivAttr()] += nodeV / entropy;
			}
		}
		else
			*pJD->pToDoN -= 1;
		
		nodesCond.Unlock();
		nodesCond.Signal();

		delete pJD;
    }
};
#endif

CTree::CTree(double alphaIn): alpha(alphaIn), root()
{
}

//Generates a tree and increases attribute counts
void CTree::grow(bool doFS, doublev& attrCounts)
{
	double b = - log((double) pData->getBagDataN()) / log(2.0); 
//thought about replacing getTrainN with getBagV, decided against it. The tree should not change if we multiple all weights by 2.
	double H = - log((double) alpha) / log(2.0);

	if(b >= -H)
		b = -H;

	doublev curAttrCounts(attrCounts.size(), 0); //feature scores for the current tree

	//place root into the stack of nodes for splitting
	nodehstack nodes;	//stack
	nodes.push(nodeip(&root, 0));
	
#ifdef _WIN32		
	//grow the tree: take nodes from the stack, try to split them, 
	//if the result is positive, place child nodes into the same stack
	while(!nodes.empty())
	{
		nodeip curNH = nodes.top();
		nodes.pop();
		
		double nodeV = 0;
		double entropy = 0;
		double* pEntropy = NULL;
		if(doFS)
		{
			nodeV = curNH.first->getNodeV();
			pEntropy = &entropy;
		}	
		double h = curNH.second;
		double curAlpha = (H == 0) ? 1 : pow(2, - (b + H) * h / H + b);
		bool notLeaf = curNH.first->split(curAlpha, pEntropy);
	
		if(notLeaf)
		{//process child nodes of this node
			if (doFS)
			{
				entropy = max(entropy, 1.0);
				curAttrCounts[curNH.first->getDivAttr()] += nodeV / entropy;
			}
			nodes.push(nodeip(curNH.first->left, curNH.second + 1));
			nodes.push(nodeip(curNH.first->right, curNH.second + 1));
		}
	}
#else
	//multithreaded version of the same process
	int toDoN = 1; //how many nodes exists but have not been finished processing at the moment
	while(toDoN > 0)
	{
		nodesCond.Lock();
		while(nodes.empty() && (toDoN > 0))
		    nodesCond.Wait();
		if(toDoN > 0)
		{
			nodeip curNH = nodes.top();
			nodes.pop();
			nodesCond.Unlock();

			doublev* pCurAttrCounts = NULL;
			if(doFS)
				pCurAttrCounts = &curAttrCounts;
			JobData* pJD = new JobData(curNH, &nodes, &nodesCond, &toDoN, pCurAttrCounts, b, H);
			pPool->Run(new CNodeSplitJob(), pJD, true);
		}
		else
			nodesCond.Unlock();
	}
#endif
	if(doFS)
		for(size_t attrNo = 0; attrNo < attrCounts.size(); attrNo++)
			attrCounts[attrNo] += curAttrCounts[attrNo] / pData->getBagV();
}

//Saves the tree into the binary file. 
//Nodes packed into the file in preorder. 
void CTree::save(const char* fileName)
{
	fstream fsave(fileName, ios_base::binary | ios_base::out | ios_base::app);	//file
	stack<CTreeNode*> nodes;	//stack for keeping roots of subtrees in the packing order
	nodes.push(&root);
	while(!nodes.empty())
	{
		CTreeNode* pNode = nodes.top(); //current node to be packed
		nodes.pop();
		pNode->save(fsave);
		if(!pNode->isLeaf())
		{
			nodes.push(pNode->right);
			nodes.push(pNode->left);
		}
	}

	fsave.close();
}

//Loads the tree from the binary file. 
//Nodes are packed into the file in preorder. 
void CTree::load(fstream& fload)
{
	bool rootLeaf = root.load(fload);
	stack<CTreeNode*> nodes;	//stack for keeping nodes without leaves attached yet
	if(!rootLeaf)
		nodes.push(&root);
	while(!nodes.empty())
	{
		CTreeNode* pNode = nodes.top(); //current node to be packed
		if(pNode->left == NULL) //next node is the left child of pNode
		{
			pNode->left = new CTreeNode();
			bool leaf = pNode->left->load(fload);
			if(!leaf)
				nodes.push(pNode->left);
		}
		else	//next node is the right child of pNode
		{
			nodes.pop();
			pNode->right = new CTreeNode();				
			bool leaf = pNode->right->load(fload);
			if(!leaf)
				nodes.push(pNode->right);
		}
	}	
}

//Calculates prediction for one data point
double CTree::predict(int itemNo, DATA_SET dset)
{
	//Implementation note: because of missing values, the item can end up in several leaves with
	//different coefficients. We trace which nodes it visits and what coefficients it has in each
	//node

	stack<nodecoefp, vector<nodecoefp> > nodes;	//stack of node-coef pairs which this case visits
	vector<nodecoefp> leaves; //list of leaves where this case ends up 
	nodecoefp btnLOut, btnROut, btnIn; //additional variables to work with nodes and coefs
	
	//place the item in the root with coef 1
	nodes.push(nodecoefp(&root, 1));

	//follow the item down the tree
	while(!nodes.empty())
	{
		//pick up the node from the stack
		btnIn = nodes.top();
		nodes.pop();

		if(btnIn.first->isLeaf())
		{
			//reached the leaf, update leaves list
			leaves.push_back(btnIn);
		}
		else
		{//this node is not a leaf. Figure out, which branches the case follows down from here 
		 //and place corresponing child node-coef pairs into the stack  
			btnIn.first->traverse(itemNo, btnIn.second, btnLOut.second, btnROut.second, dset);
			if(btnLOut.second)
			{//key follows left branch
				btnLOut.first = btnIn.first->left;
				nodes.push(btnLOut);
			}
			if(btnROut.second)
			{//key follows right branch
				btnROut.first = btnIn.first->right;
				nodes.push(btnROut);
			}
		}
	}
	
	//calculate aggregated prediction from all leaves
	double ret = 0; 
	int leafN = (int)leaves.size();
	for(int leafNo = 0; leafNo < leafN; leafNo++)
		ret += leaves[leafNo].first->getResp() * leaves[leafNo].second;

	return ret;
}

//input: predictions for train set data points produced by the rest of the model (not by this tree)	
//Changes ground truth to residuals in the root train set
void CTree::resetRoot(doublev& othpreds){
	root.resetRoot(othpreds);
}

//loads data into the root
void CTree::setRoot(){
	root.setRoot();
}
