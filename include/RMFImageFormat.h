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

#include <string>

namespace RMF
{

/*
	ImageFormat
	
	The ImageFormat defines the fundamental characteristics of an Image:  
	- its size in pixels 
	- its encoding

	The encoding defines how the pixels of the image are laid out in the memory buffer owned by 
	the Image object. More precisely, it defines which color model.
	is used and how the components of the tuples representing the color are stored at the byte level.

	The Image/ImageFormat system only support
	- uncompressed data (no variable-length spatial/temporal compression like JPG, H264, etc...)
	- non-paletized image
	- pixel-oriented or "interleaved" data storage (as opposed to planar-oriented data) 
	
	The concept of stride/padding is not supported.

	Some references:
	http://en.wikipedia.org/wiki/Color_model
	http://software.intel.com/sites/products/documentation/hpc/ipp/ippi/ippi_ch6/ch6_pixel_and_planar_image_formats.html
*/
class ImageFormat
{
public:
	enum Encoding
	{
		RGB24,	// 3 bytes per pixel. The byte sequence is: red, green, blue.
				// This encoding doesn't seem to be returned by Windows APIs (Media Foundation or DirectX),
				// so it's probably not a native encoding at the hardware level.
				// However it is a pretty common representation in some software libraries.

		BGR24,	// 3 bytes per pixel. The byte sequence is: blue, green, red.
				// Same as RGB24 but different order. This is very popular in Media Foundation 

		YUYV,	// 4 bytes per 2-pixel macroblock. The byte sequence: is Y0, U0, Y1, V0. 
				// Where Y0 and Y1 represent the luma component of each pixel 
				// and U0 and V0 represent the chroma component of both pixels.
				// Again quite popular in Media Foundation

		EncodingCount	
	};

	ImageFormat();
	ImageFormat( unsigned int width, unsigned int height, Encoding encoding );

	unsigned int			getWidth() const			{ return mWidth; }
	unsigned int			getHeight() const			{ return mHeight; }
	Encoding				getEncoding() const			{ return mEncoding; }
	const char*				getEncodingName() const		{ return getEncodingName( getEncoding() ); }
	static const char*		getEncodingName( Encoding encoding );
	
	unsigned int			getNumBitsPerPixel() const		{ return getNumBitsPerPixel( getEncoding() ); }
	static unsigned int		getNumBitsPerPixel( Encoding encoding );
	unsigned int			getNumBytesPerLine() const		{ return getNumBitsPerPixel()*getWidth()/8; }	// Note: rounded to the upper byte?
	unsigned int			getDataSizeInBytes() const;

	bool					operator==( const ImageFormat& other ) const;
	bool					operator!=( const ImageFormat& other ) const;

	std::string				toString() const;

private:
	
	static const char*		mEncodingNames[EncodingCount];
	static unsigned int		mEncodingBitsPerPixel[EncodingCount];
	
	unsigned int			mWidth;
	unsigned int			mHeight;
	Encoding				mEncoding;
};

}
