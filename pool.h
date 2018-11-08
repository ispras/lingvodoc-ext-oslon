#pragma once

#define UNICODE

//#include "default.h"
//#include "windows.h"//почему-то именно тут memcpy, malloc и т.п.


enum
{
	NPOOLSPERALLOC = 1000,
};

template <class T> class Pool
{
private:
	struct Index
	{
		T*	pool;
		int size;
	}*index;

	int		imaxPool;
	int		ilastmaxPool;
	int		nInEachPool;
	int		szDefault;
	int		nElements;
	/*
		enum
		{
			NPOOLSPERALLOC = 100,
		};
	*/
	inline void AllocNewPool()
	{
		//if (!index)
		//	index = (Index*)malloc(sizeof(Index) * NPOOLSPERALLOC);
		//else 

		if (imaxPool - ilastmaxPool >= NPOOLSPERALLOC)
		{
			index = (Index*)realloc(index, sizeof(Index) * (imaxPool + 1 + NPOOLSPERALLOC));
			ilastmaxPool = imaxPool;
		}
		++imaxPool;

		index[imaxPool].pool = (T*)malloc(nInEachPool * sizeof(T));
		index[imaxPool].size = 0;
	}
public:
	Pool(int _nInEachPool = 1000)
	{
		nInEachPool = _nInEachPool;
		imaxPool = -1;
		ilastmaxPool = -(NPOOLSPERALLOC + 1);
		nElements = 0;
		index = NULL;
	}
	~Pool()
	{
		//Цикл вроде бы устраняется компилятором, если нет деструкторов
//		for (int i = 0; i < nElements; ++i)
//			(*this)[i].~T();
		for (; imaxPool >= 0; imaxPool--)
			free(index[imaxPool].pool);
		if (index)
			free(index);
	}
	inline T* Last()
	{
		return index[imaxPool].pool + index[imaxPool].size;
	}
	T* New(T* from, int n)
	{
		T* last = New(n);
		memcpy(last, from, n * sizeof(T));
		return last;
	}
	inline T* New(int n = 1)
	{
		//надо так, но не сделана обр-ка условий
		//if (!index || index[imaxPool].size > nInEachPool - n)
		//	AllocNewPool();
		if (!index)
			AllocNewPool();
		else if (index[imaxPool].size > nInEachPool - n)
			AllocNewPool();

		T* last = Last();
		index[imaxPool].size += n;
		nElements += n;

		return last;
	}
	inline int Count()
	{
		return nElements;
	}
	inline void RemoveLast(int n = 1)
	{
		SetSize(nElements - n);
	}
	inline void SetSize(int n)
	{
		if (nElements != n)
		{
			int i = n - 1;
			int iPool = i / nInEachPool;

			for (; imaxPool > iPool; imaxPool--)
				free(index[imaxPool].pool);

			imaxPool = iPool;
			index[imaxPool].size = n - iPool * nInEachPool;

			nElements = n;
		}
	}
	inline T& operator [] (int i)
	{//не будет работать, если New делается на более, чем один элемент
		int iPool = i / nInEachPool;
		return index[iPool].pool[i - iPool * nInEachPool];
	}
	//inline operator void*()
	//{
	//	return index[0].pool;
	//}
/*
	int Dump(char* dest)
	{
		for (int i = 0; i <= imaxPool; i++)
		{
			int sz = index[0].size * sizeof(T);
			memcpy(dest, index[0].pool, sz);
			dest += sz;
		}
		return nElements * sizeof(T);
	}
*/
};