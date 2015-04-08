/*
   The MIT License (MIT) (http://opensource.org/licenses/MIT)
   
   Copyright (c) 2015 Jacques Menuet
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
#pragma once

#include <Unknwn.h>

namespace RMF
{

/*
	COMObjectSharedPtr

	A RAII-style object (Resource Acquisition Is Initialization) that facilitates
	the add-ref'ing and release of COM IUnknown-based objects. 
*/
template<class T>
class COMObjectSharedPtr
{
public:
	COMObjectSharedPtr()
		: mUnknown( NULL )
	{
	}
	
	COMObjectSharedPtr( T* unknown )
		: mUnknown( unknown )
	{
	}

	~COMObjectSharedPtr()
	{
		reset();
	}

	void reset()
	{
		if ( mUnknown )
			mUnknown->Release();
		mUnknown = NULL;
	}

	T* get()
	{
		return mUnknown;
	}

	const T* get() const
	{
		return mUnknown;
	}

	T* operator->()
	{
		return mUnknown;
	}

	T* operator->() const
	{
		return mUnknown;
	}

	// Copy-constructor
	COMObjectSharedPtr( const COMObjectSharedPtr& other ) 
	{	
		// Increment the other's IUnknown object reference counter
		if ( other.mUnknown )
			other.mUnknown->AddRef();
			
		// Assign the other into this one
		mUnknown = other.mUnknown;
	}
	
	// Assignment operator
	COMObjectSharedPtr& operator=( const COMObjectSharedPtr& other ) 
	{
		if ( &other==this )
			return *this;

		// Increment the other's IUnknown object reference counter
		if ( other.mUnknown )
			other.mUnknown->AddRef();
			
		// Decrement this IUnknown object reference counter
		if ( mUnknown )
			mUnknown->Release();
					
		// Assign the other into this one
		mUnknown = other.mUnknown;

		return *this;
	}
	
private:
	T* mUnknown;
};

}