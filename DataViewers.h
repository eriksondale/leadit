/*
 * DataViewers.h
 *
 *  Created on: Oct 18, 2011
 *      Author: dkoes
 *
 *      Some interfaces for looking at tree data the generalize leaf (single tree)
 *      and node (double tree) data.
 */

#ifndef DATAVIEWERS_H_
#define DATAVIEWERS_H_

#include <vector>
#include "MappableOctTree.h"
#include "GSSTypes.h"
#include "GSSTreeStructures.h"

using namespace std;

//a slice of a dataviewer, the trees are always allocated (not mapped to data)
struct Cluster
{
	vector<unsigned> indices; //indexing into a dataview
	const MappableOctTree *MIV;
	const MappableOctTree *MSV;

	Cluster(): MIV(NULL), MSV(NULL) {}

	Cluster(const Cluster& rhs): MIV(NULL), MSV(NULL)
	{
		indices = rhs.indices;
		if(rhs.MIV) MIV = rhs.MIV->clone();
		if(rhs.MSV) MSV = rhs.MSV->clone();
	}

	friend void swap(Cluster& first, Cluster& second)
	{
		// enable ADL (not necessary in our case, but good practice)
		using std::swap;
		swap(first.indices, second.indices);
		swap(first.MIV, second.MIV);
		swap(first.MSV, second.MSV);
	}

	void setToSingleton(unsigned i, const MappableOctTree* miv, const MappableOctTree* msv)
	{
		clear();
		indices.push_back(i);
		MIV = miv->clone();
		MSV = msv->clone();
	}

	bool isValid() const { return MIV != NULL && MSV != NULL; }
	unsigned size() const { return indices.size(); }
	unsigned operator[](unsigned i) const { return indices[i]; }

	//invalidates a and b
	void mergeInto(Cluster& a, Cluster& b)
	{
		indices.reserve(a.indices.size() + b.indices.size());
		copy(a.indices.begin(), a.indices.end(), inserter(indices, indices.end()));
		copy(b.indices.begin(), b.indices.end(), inserter(indices, indices.end()));

		const MappableOctTree *itrees[2] = {a.MIV, b.MIV};
		MIV = MappableOctTree::createFromIntersection(2, itrees);

		const MappableOctTree *utrees[2] = {a.MSV, b.MSV};
		MSV = MappableOctTree::createFromUnion(2, utrees);

		a.clear();
		b.clear();
	}

	void addInto(Cluster& a)
	{
		copy(a.indices.begin(), a.indices.end(), inserter(indices, indices.end()));
		const MappableOctTree *itrees[2] = {MIV, a.MIV};
		MIV = MappableOctTree::createFromIntersection(2, itrees);

		free((void*)itrees[0]);

		const MappableOctTree *utrees[2] = {MSV, a.MSV};
		MSV = MappableOctTree::createFromUnion(2, utrees);


		free((void*)utrees[0]);

		a.clear();
	}

	//invalidates a
	void moveInto(Cluster& a)
	{
		swap(indices,a.indices);
		MIV = a.MIV;
		MSV = a.MSV;
		a.MIV = NULL;
		a.MSV = NULL;

		a.clear();
	}
	~Cluster()
	{
		if(MIV) free((void*)MIV);
		if(MSV) free((void*)MSV);
	}

	void clear()
	{
		if(MIV) free((void*)MIV);
		if(MSV) free((void*)MSV);
		MIV = NULL;
		MSV = NULL;
		indices.clear();
	}
};


//a wrapper that can view single tree leaves the same as internal nodes
class DataViewer
{
protected:
	const char *treeptr;
	vector<file_index> pointtos; //what these trees point to (either objects or nodes)
	vector<file_index> treeindices;
public:
	DataViewer(void *data, vector<file_index>& treei, vector<file_index>& pt): treeptr((const char*)data) {
		swap(pt,pointtos);
		swap(treei, treeindices);
		assert(pointtos.size() == treeindices.size());
		pt.reserve(pointtos.size()/2);
		treei.reserve(treeindices.size()/2);
	}
	virtual ~DataViewer() {}
	//these are file ind
	virtual const MappableOctTree* getMSV(unsigned i) const = 0;
	virtual const MappableOctTree* getMIV(unsigned i) const = 0;
	file_index getIndex(unsigned i) const { return pointtos[i]; }
	unsigned size() const { return treeindices.size(); }
	virtual bool isLeaf() const  = 0;

};

//this class has pointers to single trees and the actual object data
class LeafViewer: public DataViewer
{
public:
	LeafViewer(void *data, vector<file_index>& treei, vector<file_index>& pt): DataViewer(data, treei, pt)
	{

	}

	virtual const MappableOctTree* getMSV(unsigned i) const
	{
		return (const MappableOctTree*)&treeptr[treeindices[i]];
	}

	virtual const MappableOctTree* getMIV(unsigned i) const
	{
		return (const MappableOctTree*)&treeptr[treeindices[i]];
	}

	virtual bool isLeaf() const
	{
		return true;
	}
};

//this class has pointers to double trees and nodes
class NodeViewer: public DataViewer
{
public:
	NodeViewer(void *data, vector<file_index>& treei, vector<file_index>& pt): DataViewer(data, treei, pt)
	{

	}

	virtual const MappableOctTree* getMSV(unsigned i) const
	{
		const GSSDoubleTree *dbl = (const GSSDoubleTree*)&treeptr[treeindices[i]];
		return dbl->getMSV();
	}

	virtual const MappableOctTree* getMIV(unsigned i) const
	{
		const GSSDoubleTree *dbl = (const GSSDoubleTree*)&treeptr[treeindices[i]];
		return dbl->getMIV();	}

	virtual bool isLeaf() const
	{
		return false;
	}
};

#endif /* DATAVIEWERS_H_ */
