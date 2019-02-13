#pragma once

#define UNICODE


//class BNode;
//если этого не предобъявить, наш к-лер делает болванку этого класса 
//внутри BTree, а нормальный не предвидит, где будет объявление
//т.е. надо по-разному делать в сл-е class BNode; и class BNode* ndToInsert;


class OwnNew
{
public:
	void* operator new (size_t size, void* memAlloc)
	{
		return memAlloc;
	}
};

class BNode : public OwnNew
{
	//friend	BTree;
public:
	BNode*	son[2];
	//private:должно быть так, но с работающим //friend	BTree;
	byte	depth[2];
public:
	//	Word	wName;
	/*
		BNode()
		{
			son[0] = son[1] = NULL;
			depth[0] = depth[1] = 0;
		}
	*/
	//	LPTSTR	Data;

};


class BTree
{
private:
	int				olddir;
	bool			doReplace;
	BNode*			ndToInsert;
	BNode*			ndOrphan;
	BNode*			ndOrphan2;
	BNode*			ndFound;
public://временно, для RecurListScopes
	BNode*			ndRoot;
	BNode*			ndToFind;
	//Word			wSearch;

	virtual	int		CompareNodes(BNode*, BNode*, void*) = 0;
	/*
		int CompareNodes(BNode* n1, BNode* n2, void*_)
		{
			return 0xbadf00d;
		}
	*/
	class Walker
	{
		BTree* 		tree;
		//		BNode**		stack;
		BNode*		stack[100];
		int			maxStack;
		BNode*		curr;

	public:
		Walker(BTree* _tree)
		{
			tree = _tree;
			//stack = new BNode*[tree->ndRoot->depth[0]*2+2];
			maxStack = -1;
			curr = NULL;
		}
		BNode* Current()
		{
			return curr;
		}
		BNode* Next()
		{
			if (!curr)
				curr = tree->ndRoot;
			else
				curr = curr->son[1];

			while (curr || maxStack >= 0)
			{
				while (curr)
				{
					maxStack++;
					stack[maxStack] = curr;
					curr = curr->son[0];
				}

				curr = stack[maxStack];
				maxStack--;

				return curr;
			}
			return NULL;
		}
		~Walker()
		{
			//delete stack[];
		}
	};

	byte insert(BNode* node)
	{
		int dir1 = CompareNodes(ndToInsert, node, NULL);

		byte depth;

		if (dir1 < 0)
			dir1 = 0;
		else if (dir1 > 0)
			dir1 = 1;
		else
		{
			ndFound = node;

			if (doReplace)
			{
				ndToInsert->depth[0] = node->depth[0];
				ndToInsert->depth[1] = node->depth[1];
				ndToInsert->son[0] = node->son[0];
				ndToInsert->son[1] = node->son[1];

				ndOrphan = ndToInsert;
				if (node == ndRoot)
					ndRoot = ndToInsert;
			}
			return 0;
		}

		if (BNode* son = node->son[dir1])
		{
			if (!(depth = insert(son)))
			{
				if (ndOrphan)
				{
					node->son[dir1] = ndOrphan;
					ndOrphan = NULL;
				}
				return 0;
			}
		}
		else
		{
			node->son[dir1] = ndToInsert;
			ndFound = NULL;
			depth = 1;
		}

		int dir2 = 1 - dir1;

		node->depth[dir1] = depth;

		if (node->depth[dir2] == depth - 1)
		{
			olddir = dir1;
			return depth + 1;
		}
		else if (node->depth[dir2] == depth)
			return 0;

		if (dir1 == olddir)
		{
			BNode* son1 = node->son[dir1],
				*son2 = son1->son[dir2];

			node->son[dir1] = son2;
			node->depth[dir1] = son1->depth[dir2];
			son1->son[dir2] = node;
			son1->depth[0] = son1->depth[1] = depth - 1;

			if (node == ndRoot)
				ndRoot = son1;
			else
				ndOrphan = son1;
		}
		else
		{
			BNode* son = node->son[dir1],
				*gson = son->son[dir2],
				*son1 = gson->son[dir1],
				*son2 = gson->son[dir2];

			gson->son[dir2] = node;
			gson->son[dir1] = son;

			node->son[dir1] = son2;
			node->depth[dir1] = gson->depth[dir2];
			son->son[dir2] = son1;
			son->depth[dir2] = gson->depth[dir1];
			gson->depth[0] = gson->depth[1] = depth - 1;

			if (node == ndRoot)
				ndRoot = gson;
			else
				ndOrphan = gson;
		}
		return 0;
	}

	byte remove(BNode* node)
	{
		int	dir1,
			dir2;


		char _depth,
			ret;
		BNode* _son;
		BNode*	son1,
			*son2,
			*gson;

		if (ndFound)
			dir1 = 1;
		else {
			dir1 = CompareNodes(ndToFind, node, NULL);

			/*

					int	sz=node->wName.size;

					if (sz>wSearch.size)
						sz=wSearch.size;

					dir1=memcmp(wSearch.pos,node->wName.pos,sz);
			*/
			if (dir1 < 0)
				dir1 = 0;
			else if (dir1 > 0)
				dir1 = 1;
			/*
					else if (node->wName.size>wSearch.size)
						dir1=0;
					else if (node->wName.size<wSearch.size)
						dir1=1;
			*/
			else {
				ndFound = node;

				if (node->depth[0] && node->depth[1]) {
					_depth = remove(node->son[0]);

					if (ndOrphan) {
						ndOrphan2->son[0] = ndOrphan;
						ndOrphan = NULL;
					}

					if (_depth != -4)
						ndOrphan2->depth[0] = (byte)_depth;

					if (node == ndRoot)
						ndRoot = ndOrphan2;

					node = ndOrphan2;
					dir1 = 0;
					_depth = node->depth[0];

					ndOrphan = node;

					goto Check;
				}

				if (node == ndRoot) {
					if (node->depth[0])
						ndRoot = node->son[0];
					else
						ndRoot = node->son[1];
					ret = -1;
				}
				else if (node->depth[0]) {
					ndOrphan = node->son[0];
					ret = node->depth[0];
				}
				else if (node->depth[1]) {
					ndOrphan = node->son[1];
					ret = node->depth[1];
				}
				else {
					ret = -4;
				}
				goto Done;
			}
		}

		_son = node->son[dir1];

		if (_son) {
			_depth = remove(_son);
			switch (_depth) {
			case -1:
				ret = -1;
				goto Done;
			case -4:
				if (ndOrphan) {
					node->son[dir1] = ndOrphan;
					ndOrphan = NULL;
					_depth = 1;
				}
				else {
					node->son[dir1] = NULL;
					_depth = 0;
				}
			}
		}
		else if (ndFound) {
			son1 = ndFound->son[0];
			son2 = ndFound->son[1];

			if (node != son1) {
				_son = node->son[0];

				node->son[0] = son1;

				if (son1->depth[0] >= son1->depth[1])
					node->depth[0] = son1->depth[0] + 1;
				else
					node->depth[0] = son1->depth[1] + 1;
				ndOrphan = _son;
			}

			node->son[1] = son2;

			if (son2->depth[0] >= son2->depth[1])
				node->depth[1] = son2->depth[0] + 1;
			else
				node->depth[1] = son2->depth[1] + 1;

			ndOrphan2 = node;

			ret = -4;
			goto Done;
		}
		else {
			ret = -1;
			goto Done;
		}

		if (ndOrphan) {
			node->son[dir1] = ndOrphan;
			ndOrphan = NULL;
		}
		node->depth[dir1] = (byte)_depth;

	Check:
		dir2 = 1 - dir1;

		if (node->depth[dir2] <= _depth + 1) {
			olddir = dir2;

			if (node->depth[0] >= node->depth[1]) {
				ret = node->depth[0] + 1;
				goto Done;
			}
			else {
				ret = node->depth[1] + 1;
				goto Done;
			}
		}

		///////////////////////////	
		dir2 = dir1;
		dir1 = 1 - dir1;
		///////////////////////////	

		son1 = node->son[dir1];

		if (son1->depth[dir2] > son1->depth[dir1])
			olddir = dir2;
		else
			olddir = dir1;
		_depth = node->depth[dir1];

		if (dir1 == olddir) {
			son1 = node->son[dir1];
			son2 = son1->son[dir2];

			node->son[dir1] = son2;

			node->depth[dir1] = son1->depth[dir2];
			son1->son[dir2] = node;

			if (node->depth[0] >= node->depth[1])
				son1->depth[dir2] = node->depth[0] + 1;
			else
				son1->depth[dir2] = node->depth[1] + 1;

			if (node == ndRoot)
				ndRoot = son1;
			else {
				ndOrphan = son1;
				if (son1->depth[0] >= son1->depth[1]) {
					ret = son1->depth[0] + 1;
					goto Done;
				}
				else {
					ret = son1->depth[1] + 1;
					goto Done;
				}
			}
		}
		else {
			_son = node->son[dir1];
			gson = _son->son[dir2];

			son1 = gson->son[dir1];
			son2 = gson->son[dir2];

			gson->son[dir2] = node;
			gson->son[dir1] = _son;

			node->son[dir1] = son2;

			node->depth[dir1] = gson->depth[dir2];
			_son->son[dir2] = son1;

			_son->depth[dir2] = gson->depth[dir1];

			gson->depth[dir1] = _depth - 1;
			if (node->depth[0] >= node->depth[1])
				gson->depth[dir2] = node->depth[0] + 1;
			else
				gson->depth[dir2] = node->depth[1] + 1;

			if (node == ndRoot)
				ndRoot = gson;
			else {
				ndOrphan = gson;
				if (gson->depth[0] >= gson->depth[1]) {
					ret = gson->depth[0] + 1;
					goto Done;
				}
				else {
					ret = gson->depth[1] + 1;
					goto Done;
				}
			}
		}
		ret = -10;
	Done:
		return ret;
	}
	BNode* find(BNode* node)
	{
		int dir1 = CompareNodes(ndToFind, node, NULL);
		/*
		int	sz = node->wName.size;

		if (sz>wSearch.size)
			sz = wSearch.size;
		//не шустрей ли, если свою сделать?
		dir1 = memcmp(wSearch.pos, node->wName.pos, sz);
		*/
		if (dir1 < 0)
			dir1 = 0;
		else if (dir1 > 0)
			dir1 = 1;
		/*else if (node->wName.size>wSearch.size)
			dir1 = 0;
		else if (node->wName.size<wSearch.size)
			dir1 = 1;*/
		else
			return node;

		BNode*	son = node->son[dir1];
		BNode*	BNode;

		if (son) {
			if (BNode = find(son))
				return BNode;
		}
		return NULL;
	}
public:
	BTree()
	{
		ndRoot = NULL;
	}
	void Empty()
	{
		ndRoot = NULL;
	}
	bool IsEmpty()
	{
		return ndRoot != NULL;
	}
	BNode* Add(BNode* node, bool replace = false)
	{
		node->son[0] = node->son[1] = NULL;
		node->depth[0] = node->depth[1] = 0;
		if (ndRoot) {
			doReplace = replace;
			ndToInsert = node;
			ndOrphan = NULL;

			insert(ndRoot);
			return ndFound;
		}
		else {
			ndRoot = node;
			return NULL;
		}
	}
	bool Remove(BNode* _ndToFind)
	{
		if (ndRoot)
		{
			ndFound = NULL;
			ndToFind = _ndToFind;
			remove(this->ndRoot);
			return !!ndFound;
		}
		else
			return false;
	}


	BNode* Find(BNode* _ndToFind)
	{
		ndToFind = _ndToFind;
		if (this->ndRoot)
			return find(this->ndRoot);
		else
			return NULL;
	}

	BNode* Replace(BNode* node)
	{
		return Add(node, true);
	}
};

