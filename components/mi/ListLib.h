/***************************************************
Copyright (c) 2017 Luis Llamas
(www.luisllamas.es)

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License
 ****************************************************/

#ifndef _ListLib_h
#define _ListLib_h

//#if defined(ARDUINO) && ARDUINO >= 100
//#include <Arduino.h>
//#else
//	#include "WProgram.h"
//#endif
#include <stddef.h>

template <typename T>
class List
{
public:
	List();
	List(size_t capacity);
	~List();

	size_t Capacity() const;
	size_t Count() const;

	T& operator[](const size_t index);

	bool Contains(T item);
	size_t IndexOf(T item);

	T& First();
	T& Last();

	void Add(T item);
	void AddRange(T* items, size_t numItems);

	void Insert(T item);
	void Insert(size_t index, T item);
	void InsertRange(T* items, size_t numItems);
	void InsertRange(size_t index, T* items, size_t numItems);

	void RemoveFirst();
	void Remove(size_t index);
	void RemoveLast();
	void RemoveRange(size_t index, size_t numItems);

	void Replace(size_t index, T item);
	void ReplaceRange(size_t index, T* items, size_t numItems);

	void Reverse();
	void Clear();
	bool IsEmpty();
	bool IsFull();
	void Trim();
	void Trim(size_t reserve);
	
	T* ToArray();
	T* ToArray(size_t index, size_t numItems);
	void CopyTo(T* items);
	void CopyTo(T* items, size_t index, size_t numItems);
	void FromArray(T* items, size_t numItems);


private:
	T* _items;

	size_t _count = 0;
	size_t _capacity = 4;

	void shift(size_t index, size_t numItems);
	void unshift(size_t index, size_t numItems);
	void reserve(size_t size);
	void resize(size_t size);
};


template <typename T>
List<T>::List()
{
	_items = new T[_capacity];
}

template <typename T>
List<T>::List(size_t capacity)
{
	_capacity = capacity;
	_items = new T[_capacity];
}

template <typename T>
List<T>::~List()
{
	delete[] _items;
}

template <typename T>
T& List<T>::operator[](const size_t index)
{
	return _items[index];
}

template <typename T>
size_t List<T>::Capacity() const
{
	return _capacity;
}

template <typename T>
size_t List<T>::Count() const
{
	return _count;
}

template <typename T>
T& List<T>::First()
{
	return _items[0];
}

template <typename T>
T& List<T>::Last()
{
	return _items[_count - 1];
}

template <typename T>
void List<T>::Add(T item)
{
	++_count;
	reserve(_count);
	_items[_count - 1] = item;
}

template <typename T>
void List<T>::AddRange(T* items, size_t numItems)
{
	reserve(_count + numItems);
	memmove(_items + _count , items, numItems * sizeof(T));
	_count += numItems;
}

template <typename T>
void List<T>::Insert(T item)
{
	++_count;
	reserve(_count);
	shift(0, 1);
	_items[0] = item;
}

template <typename T>
void List<T>::InsertRange(T* items, size_t numItems)
{
	_count += numItems;
	reserve(_count);
	shift(0, numItems);
	memmove(_items, items, numItems * sizeof(T));
}

template <typename T>
void List<T>::Insert(size_t index, T item)
{
	if (index > _count - 1) return;

	++_count;
	reserve(_count);
	shift(index, 1);
	_items[index] = item;
}

template <typename T>
void List<T>::InsertRange(size_t index, T* items, size_t numItems)
{
	if (index > _count - 1) return;

	_count += numItems;
	reserve(_count);
	shift(index, numItems);
	memmove(_items + index, items, numItems * sizeof(T));
}

template <typename T>
void List<T>::RemoveFirst()
{
	if (_count == 0) return;

	unshift(0, 1);
	--_count;
}

template <typename T>
void List<T>::Remove(size_t index)
{
	if (index > _count - 1) return;

	unshift(index, 1);
	--_count;
}

template <typename T>
void List<T>::RemoveLast()
{
	if (_count == 0) return;

	--_count;
}

template <typename T>
void List<T>::RemoveRange(size_t index, size_t numItems)
{
	unshift(index, numItems);
	_count -= numItems;
}

template <typename T>
void List<T>::Replace(size_t index, T item)
{
	if (index > _count - 1) return;

	_items[index] = item;
}

template <typename T>
void List<T>::ReplaceRange(size_t index, T* items, size_t numItems)
{
	memmove(_items + index, items, numItems * sizeof(T));
}

template <typename T>
void List<T>::Reverse()
{
	T item;
	for (int index = 0; index < _count / 2; index++)
	{
		item = _items[index];
		_items[index] = _items[_count - 1 - index];
		_items[_count -1 - index] = item;
	}
}

template <typename T>
void List<T>::Clear()
{
	_count = 0;
}

template <typename T>
bool List<T>::IsEmpty()
{
	return (_count == 0);
}

template <typename T>
bool List<T>::IsFull()
{
	return (_count >= _capacity);
}

template <typename T>
void List<T>::Trim()
{
	resize(_count);
}

template <typename T>
void List<T>::Trim(size_t reserve)
{
	resize(_count + reserve);
}

template <typename T>
T* List<T>::ToArray()
{
	T* items = new T[_count];
	memmove(items, _items, _count * sizeof(T));
	return items;
}

template <typename T>
T* List<T>::ToArray(size_t index, size_t numItems)
{
	T* items = new T[numItems];
	memmove(items, _items + index, numItems * sizeof(T));
	return items;
}

template <typename T>
void List<T>::CopyTo(T* items)
{
	memmove(items, _items, _count * sizeof(T));
}

template <typename T>
void List<T>::CopyTo(T* items, size_t index, size_t numItems)
{
	memmove(items, _items + index , numItems * sizeof(T));
}

template <typename T>
void List<T>::FromArray(T* items, size_t numItems)
{
	reserve(numItems);
	_count = numItems;
	memmove(_items, items, numItems * sizeof(T));
}

template <typename T>
bool List<T>::Contains(T item)
{
	for (int index = 0; index < _count; index++)
		if (_items[index] == item)
			return true;
	return false;
}

template <typename T>
size_t List<T>::IndexOf(T item)
{
	for (int index = 0; index < _count; index++)
		if (_items[index] == item)
			return index;
	return -1;
}

template <typename T>
void List<T>::shift(size_t index, size_t numItems)
{
	memmove(_items + index + numItems, _items + index, (_count - index - 1) * sizeof(T));
}

template <typename T>
void List<T>::unshift(size_t index, size_t numItems)
{
	memmove(_items + index , _items + index + numItems, (_count - index - numItems + 1) * sizeof(T));
}

template <typename T>
void List<T>::reserve(size_t size)
{
	if (_count > _capacity)
	{
		size_t newSize = _capacity * 2 > size ? _capacity * 2 : size;
		resize(newSize);
	}
}

template <typename T>
void List<T>::resize(size_t size)
{
	if (_count > size) return;
	if (_capacity == size) return;
	
	T* newItems = new T[size];
	memmove(newItems, _items, _count  * sizeof(T));
	delete[] _items;
	_capacity = size;
	_items = newItems;
}

#endif