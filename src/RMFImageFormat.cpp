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
#include "RMFImageFormat.h"

#include <assert.h>
#include <sstream>

namespace RMF
{

unsigned int ImageFormat::mEncodingBitsPerPixel[EncodingCount] = 
{
	24,
	24,
	16
};

const char* ImageFormat::mEncodingNames[EncodingCount] = 
{
	"RGB24",
	"BGR24",
	"YUYV"	
};
	
ImageFormat::ImageFormat()
	: mWidth(0), 
	  mHeight(0), 
	  mEncoding(RGB24)
{
}

ImageFormat::ImageFormat( unsigned int width, unsigned int height, Encoding encoding )
	: mWidth(width), 
	  mHeight(height), 
	  mEncoding(encoding)
{
}

unsigned int ImageFormat::getNumBitsPerPixel( Encoding encoding )
{
	if ( encoding>=EncodingCount )
		return 0;
	return mEncodingBitsPerPixel[encoding];
}

const char* ImageFormat::getEncodingName( Encoding encoding )
{
	if ( encoding>=EncodingCount )
		return "Unknown";
	return mEncodingNames[encoding];
}

unsigned int ImageFormat::getDataSizeInBytes() const
{
	unsigned int size = getHeight() * getNumBytesPerLine();
	return size;
}

bool ImageFormat::operator==( const ImageFormat& other ) const
{
	return	mWidth == other.mWidth && 
			mHeight == other.mHeight &&
			mEncoding == other.mEncoding;
}

bool ImageFormat::operator!=( const ImageFormat& other ) const
{
	return	!( *this==other );
}

std::string ImageFormat::toString() const
{
	std::stringstream stream;
	stream << getWidth() << "x" << getHeight() << " pixels, " << getEncodingName() << " encoding";
	return stream.str();
}

}
