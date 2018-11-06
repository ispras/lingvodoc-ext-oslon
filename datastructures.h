#pragma once

#define UNICODE

//#include "default.h"
//#include "windows.h"//почему-то именно тут memcpy, malloc и т.п.

//////////////////////////////////////////////////////////////////////////////////////
template <class T>
class LinkedList
{
public:
	T*	first;
	T*	last;
	LinkedList()
	{
		first = last = NULL;
	}
	T* Add(T* el)
	{
		if (!first)
			first = el;
		else
			last->next = el;

		el->prev = last;
		el->next = NULL;
		last = el;

		return el;
	}
	void Delete(T* el, bool doDestroy)
	{
		if (el->prev)
			el->prev->next = el->next;
		else
			first = el->next;

		if (el->next)
			el->next->prev = el->prev;
		else
			last = el->prev;

		if (doDestroy)
			delete el;
	}

	void Add(T* el, T* prev)
	{
		if (prev)
		{//есть предыдущий
			el->next = prev->next;
			el->prev = prev;

			if (prev->next)
				prev->next->prev = el;
			prev->next = el;
		}
		else
		{//вставить первым
			el->next = first;
			el->prev = NULL;

			if (first)
				first->prev = el;
			first = el;
		}
		if (!el->next)
			last = el;
	}
	/*
		void Add(LinkedList& llToAdd)
		{
			if (!llToAdd.last)
				return;

			if (!first)
				first = llToAdd.first;

			llToAdd.first->prev=last;
			if (last)
				last->next = llToAdd.first;

			last = llToAdd.last;
		}
	*/
	void CutOffAfter(T* el)
	{
		if (el)
			el->next = NULL;
		else
			first = NULL;
		last = el;
	}

	void DestroyAll()
	{
		for (T* el = first; el; el = el->next)
		{
			if (el->prev)
				delete el->prev;
		}
		if (last)
			delete last;
	}
};

template <class T>
class LinkedElement
{
public:
	T*	next;
	T*	prev;
	// 	T()
	// 	{
	// 		next = NULL;
	// 	}
};

