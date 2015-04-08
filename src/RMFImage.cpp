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
#include "RMFImage.h"

#include <stdio.h>
#include <cstring>
#include <assert.h>

namespace RMF
{

// Construct an empty image with zero size. The image instance obtained can still  
// be filled with data later using the assignment operator (which can modify its format).
Image::Image()
	: mFormat(),
	  mBuffer()
{
}

// Construct a blank image of a specific format. The internal image data is allocated and zero-filled
Image::Image( const ImageFormat& imageFormat )
	: mFormat( imageFormat), 
	  mBuffer( imageFormat.getDataSizeInBytes() )
{
}

// Construct an image from another one. The source image data is copied during the process
Image::Image( const Image& other )
	: mFormat( other.getFormat() ), 
	  mBuffer( other.getBuffer() )
{
}

}
