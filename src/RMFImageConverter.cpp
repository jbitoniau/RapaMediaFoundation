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
#include "RMFImageConverter.h"

#include <assert.h>

namespace RMF
{

ImageConverter::ImageConverter( const ImageFormat& outputImageFormat )
	: mImage(NULL)	
{
	mImage = new Image( outputImageFormat );
}

ImageConverter::~ImageConverter()
{
	delete mImage;
	mImage = NULL;
}

bool ImageConverter::update( const Image& sourceImage )
{
	if ( sourceImage.getFormat()==mImage->getFormat() )
		return mImage->getBuffer().copyFrom( sourceImage.getBuffer() );
	return convertImage( sourceImage, *mImage );
}
	
bool ImageConverter::swapFirstAndThirdBytesEveryThreeBytes( MemoryBuffer& buffer )
{
	if ( buffer.getSizeInBytes() % 3 !=0 )
		return false;
	unsigned int count = buffer.getSizeInBytes();
	unsigned char* bytes = buffer.getBytes();
	unsigned char tempByte = 0;
	for ( unsigned int i=0; i<count; i+=3 )
	{
		tempByte = bytes[i];
		bytes[i] = bytes[i+2];
		bytes[i+2] = tempByte;
	}
	return true;
}

bool ImageConverter::convertBGR24ImageToRGB24Image( const Image& bgr24Image, Image& rgb24Image )
{
	// Pre-checks
	if ( bgr24Image.getFormat().getEncoding()!=ImageFormat::BGR24 )
		return false;
	if ( rgb24Image.getFormat().getEncoding()!=ImageFormat::RGB24 )
		return false;

	unsigned int width = bgr24Image.getFormat().getWidth();
	unsigned int height = bgr24Image.getFormat().getHeight();
	if ( rgb24Image.getFormat().getWidth()!=width || rgb24Image.getFormat().getHeight()!=height )
		 return false;

	// The following two-step copy-transformation could be written as a single step one
	rgb24Image.getBuffer().copyFrom( bgr24Image.getBuffer() );
	swapFirstAndThirdBytesEveryThreeBytes( rgb24Image.getBuffer() );

	return true;
}

bool ImageConverter::convertRGB24ImageToBGR24Image( const Image& rgb24Image, Image& bgr24Image )
{
	// Pre-checks
	if ( rgb24Image.getFormat().getEncoding()!=ImageFormat::RGB24 )
		return false;
	if ( bgr24Image.getFormat().getEncoding()!=ImageFormat::BGR24 )
		return false;

	unsigned int width = rgb24Image.getFormat().getWidth();
	unsigned int height = rgb24Image.getFormat().getHeight();
	if ( bgr24Image.getFormat().getWidth()!=width || bgr24Image.getFormat().getHeight()!=height )
		 return false;

	// Same remark here
	bgr24Image.getBuffer().copyFrom( rgb24Image.getBuffer() );
	swapFirstAndThirdBytesEveryThreeBytes( bgr24Image.getBuffer() );

	return true;
}	

#define CLIP_INT_TO_UCHAR(value) ( (value)<0 ? 0 : ( (value)>255 ? 255 : static_cast<unsigned char>(value) ) ) 

bool ImageConverter::convertYUYVImageToRGB24Image( const Image& yuyvImage, Image& rgb24Image )
{
	// Pre-checks
	if ( yuyvImage.getFormat().getEncoding()!=ImageFormat::YUYV )
		return false;
	if ( rgb24Image.getFormat().getEncoding()!=ImageFormat::RGB24 )
		return false;

	unsigned int width = yuyvImage.getFormat().getWidth();
	unsigned int height = yuyvImage.getFormat().getHeight();
	if ( rgb24Image.getFormat().getWidth()!=width || rgb24Image.getFormat().getHeight()!=height )
		 return false;

	// General information about YUV color space can be found here:
	// http://en.wikipedia.org/wiki/YUV 
	// or here:
	// http://www.fourcc.org/yuv.php

	// The following conversion code comes from here:
	// http://stackoverflow.com/questions/4491649/how-to-convert-yuy2-to-a-bitmap-in-c
	// http://msdn.microsoft.com/en-us/library/aa904813(VS.80).aspx#yuvformats_2
	const unsigned char* sourceBytes = yuyvImage.getBuffer().getBytes();
	unsigned char* destBytes = rgb24Image.getBuffer().getBytes();
	for ( unsigned int y=0; y<height; ++y )
	{
		for ( unsigned int i=0; i<width/2; ++i )
		{
			int y0 = sourceBytes[0];
			int u0 = sourceBytes[1];
			int y1 = sourceBytes[2];
			int v0 = sourceBytes[3];
			sourceBytes += 4;	
			
			int c = y0 - 16;
			int d = u0 - 128;
			int e = v0 - 128;
			destBytes[0] = CLIP_INT_TO_UCHAR(( 298 * c           + 409 * e + 128) >> 8);		// Red
			destBytes[1] = CLIP_INT_TO_UCHAR(( 298 * c - 100 * d - 208 * e + 128) >> 8);		// Green
			destBytes[2] = CLIP_INT_TO_UCHAR(( 298 * c + 516 * d           + 128) >> 8);		// Blue
			
			c = y1 - 16;
			destBytes[3] = CLIP_INT_TO_UCHAR(( 298 * c           + 409 * e + 128) >> 8);		// Red
			destBytes[4] = CLIP_INT_TO_UCHAR(( 298 * c - 100 * d - 208 * e + 128) >> 8);		// Green
			destBytes[5] = CLIP_INT_TO_UCHAR(( 298 * c + 516 * d           + 128) >> 8);		// Blue
			destBytes += 6;
		}
	}
	return true;	
}

bool ImageConverter::convertYUYVImageToBGR24Image( const Image& yuyvImage, Image& bgr24Image )
{
	// Pre-checks
	if ( yuyvImage.getFormat().getEncoding()!=ImageFormat::YUYV )
		return false;
	if ( bgr24Image.getFormat().getEncoding()!=ImageFormat::BGR24 )
		return false;

	unsigned int width = yuyvImage.getFormat().getWidth();
	unsigned int height = yuyvImage.getFormat().getHeight();
	if ( bgr24Image.getFormat().getWidth()!=width || bgr24Image.getFormat().getHeight()!=height )
		 return false;

	// General information about YUV color space can be found here:
	// http://en.wikipedia.org/wiki/YUV 
	// or here:
	// http://www.fourcc.org/yuv.php

	// The following conversion code comes from here:
	// http://stackoverflow.com/questions/4491649/how-to-convert-yuy2-to-a-bitmap-in-c
	// http://msdn.microsoft.com/en-us/library/aa904813(VS.80).aspx#yuvformats_2
	const unsigned char* sourceBytes = yuyvImage.getBuffer().getBytes();
	unsigned char* destBytes = bgr24Image.getBuffer().getBytes();
	for ( unsigned int y=0; y<height; ++y )
	{
		for ( unsigned int i=0; i<width/2; ++i )
		{
			int y0 = sourceBytes[0];
			int u0 = sourceBytes[1];
			int y1 = sourceBytes[2];
			int v0 = sourceBytes[3];
			sourceBytes += 4;	
			
			int c = y0 - 16;
			int d = u0 - 128;
			int e = v0 - 128;
			destBytes[0] = CLIP_INT_TO_UCHAR(( 298 * c + 516 * d           + 128) >> 8);		// Blue
			destBytes[1] = CLIP_INT_TO_UCHAR(( 298 * c - 100 * d - 208 * e + 128) >> 8);		// Green
			destBytes[2] = CLIP_INT_TO_UCHAR(( 298 * c           + 409 * e + 128) >> 8);		// Red
			
			c = y1 - 16;
			destBytes[3] = CLIP_INT_TO_UCHAR(( 298 * c + 516 * d           + 128) >> 8);		// Blue
			destBytes[4] = CLIP_INT_TO_UCHAR(( 298 * c - 100 * d - 208 * e + 128) >> 8);		// Green
			destBytes[5] = CLIP_INT_TO_UCHAR(( 298 * c           + 409 * e + 128) >> 8);		// Red
			destBytes += 6;
		}
	}
	return true;
}

bool ImageConverter::convertImage( const Image& sourceImage, Image& destinationImage )
{
	if ( sourceImage.getFormat()==destinationImage.getFormat() )
		return false;

	ImageFormat::Encoding sourceEncoding = sourceImage.getFormat().getEncoding();
	ImageFormat::Encoding destinationEncoding = destinationImage.getFormat().getEncoding();

	if ( sourceEncoding==ImageFormat::BGR24 && destinationEncoding==ImageFormat::RGB24 )
		return convertBGR24ImageToRGB24Image( sourceImage, destinationImage );
	else if ( sourceEncoding==ImageFormat::RGB24 && destinationEncoding==ImageFormat::BGR24 )
		return convertRGB24ImageToBGR24Image( sourceImage, destinationImage );
	else if ( sourceEncoding==ImageFormat::YUYV && destinationEncoding==ImageFormat::RGB24 )
		return convertYUYVImageToRGB24Image( sourceImage, destinationImage );
	else if ( sourceEncoding==ImageFormat::YUYV && destinationEncoding==ImageFormat::BGR24 )
		return convertYUYVImageToBGR24Image( sourceImage, destinationImage );
	return false;
}

}

