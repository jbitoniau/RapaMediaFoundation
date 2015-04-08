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
#include "RMFQImageWidget.h"

namespace RMF
{

/*
	QImageWidget
*/
QImageWidget::QImageWidget( QWidget* parent )
	: QFrame(parent),
	  mQImageMaker(NULL)
{
}

QImageWidget::~QImageWidget()
{
	delete mQImageMaker;
	mQImageMaker = NULL;
}

void QImageWidget::paintEvent( QPaintEvent* /*paintEvent*/ )
{
	QPainter painter(this);
	
	QColor backgroundColor( 180, 180, 180 );
	painter.setBrush(backgroundColor);
	painter.drawRect( 0, 0, width()-1, height()-1 );

	if ( mQImageMaker )
		painter.drawImage( QPointF(0,0), mQImageMaker->getQImage() );
}
	
void QImageWidget::setImage( const RMF::Image& image )
{
	unsigned int width = image.getFormat().getWidth();
	unsigned int height = image.getFormat().getHeight();
	int qwidth = 0;
	int qheight = 0;
	if ( mQImageMaker )
	{
		qwidth = mQImageMaker->getQImage().width();
		qheight = mQImageMaker->getQImage().height();
	}

	if ( !mQImageMaker || qwidth!=static_cast<int>(width) || qheight!=static_cast<int>(height) )
	{
		delete mQImageMaker;
		mQImageMaker = new QRGB888ImageMaker( width, height );
	}
	mQImageMaker->update( image );

	update();
}

/*
	QRGB888ImageMaker
*/
QRGB888ImageMaker::QRGB888ImageMaker( int width, int height )
	: mQImage(NULL),
	  mImageConverter(NULL)
{
	ImageFormat rgbFormat( width, height, ImageFormat::RGB24 );
	mImageConverter = new ImageConverter( rgbFormat );
	
	// Get a grip onto the data of the image that serves as output of the ImageConverter
	uchar* data = reinterpret_cast<uchar*>( mImageConverter->getImage().getBuffer().getBytes() );

	// Create a QImage pointing *directly* onto this data
	mQImage = new QImage( data, width, height, QImage::Format_RGB888 );
}

QRGB888ImageMaker::~QRGB888ImageMaker()
{
	delete mQImage;
	mQImage = NULL;

	delete mImageConverter;
	mImageConverter = NULL;
}
	
bool QRGB888ImageMaker::update( const Image& image )
{
	// Here we just have to update the ImageConverter
	// It internally updates the Image it contains.
	// Without having to do anything, this updates the QImage because
	// it shares its data with the the Image we just updated
	return mImageConverter->update( image );
}

}

