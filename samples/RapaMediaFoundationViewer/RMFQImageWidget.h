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

#ifdef _MSC_VER
	#pragma warning( push )
	#pragma warning ( disable : 4127 )
	#pragma warning ( disable : 4231 )
	#pragma warning ( disable : 4251 )
	#pragma warning ( disable : 4800 )	
#endif
#include <QFrame>
#include <QPainter>
#ifdef _MSC_VER
	#pragma warning( pop )
#endif

#include "RMFImage.h"
#include "RMFImageConverter.h"

namespace RMF
{

class QRGB888ImageMaker;

/*
	QImageWidget

	A widget able to display a RMF Image 
*/
class QImageWidget: public QFrame
{ 
	Q_OBJECT

public:
	QImageWidget( QWidget* parent );
	virtual ~QImageWidget();

	void				setImage( const RMF::Image& image );

protected:
	virtual void		paintEvent( QPaintEvent* paintEvent );

private:
	QRGB888ImageMaker*	mQImageMaker;
};

/*
	QRGB888ImageMaker
	
	Produces a QImage with the QT RGB888 format from a RMF Image
	(performs the necessary conversion under the hood)	

	Note: with QT's RGB888 format, if you read the QImage buffer *byte after byte* 
	then the color components appear in this order: red, green, blue.
	This format is strictly equivalent to the RMF::ImageFormat::RGB24.
*/
class QRGB888ImageMaker
{
public:
	QRGB888ImageMaker( int width, int height );
	~QRGB888ImageMaker();
	
	bool			update( const Image& image );
	const QImage&	getQImage() const { return *mQImage; }

private:
	QImage*			mQImage;
	ImageConverter*	mImageConverter;
};


}