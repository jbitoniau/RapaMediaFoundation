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
#include "RMFQDeviceWidget.h"

#include <assert.h>
#include <sstream>
#include <fstream>

namespace RMF
{

QDeviceWidget::QDeviceWidget( QWidget* parent, RMF::Device* device, Qt::WindowFlags flags )
	: QFrame(parent, flags),
	  mDevice(device),
	  mImageWidget(NULL),
	  mCaptureSettingsCombo(NULL),
	  mStartStopButton(NULL),
	  mImageNumberLabel(NULL)	 
{
	QVBoxLayout* mainLayout = new QVBoxLayout();
	setLayout( mainLayout);

	mImageWidget = new RMF::QImageWidget( this );
	mImageWidget->setFrameShape( QFrame::Box );
	mainLayout->addWidget( mImageWidget );

	QHBoxLayout* bottomBarLayout = new QHBoxLayout();
	mainLayout->addLayout( bottomBarLayout );

	mCaptureSettingsCombo = new QComboBox( this );
	bottomBarLayout->addWidget( mCaptureSettingsCombo );
	
	const RMF::CaptureSettingsList& settingsList = mDevice->getSupportedCaptureSettingsList();
	for ( std::size_t i=0; i<settingsList.size(); ++i )
	{
		QString text = QString("#%1, %2").arg(i).arg( QString(settingsList[i].toString().c_str()) );
		mCaptureSettingsCombo->addItem( text );
	}

	mStartStopButton = new QPushButton( "Start", this );
	mStartStopButton->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, mStartStopButton->sizePolicy().verticalPolicy() ) );
	bottomBarLayout->addWidget( mStartStopButton );
	connect( mStartStopButton, SIGNAL( pressed() ), this, SLOT( startStopButtonPressed() ) );

	mImageNumberLabel = new QLabel( "0", this );
	bottomBarLayout->addWidget( mImageNumberLabel );
	mImageNumberLabel->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
	mImageNumberLabel->setMinimumWidth( 35 );
	
	mDevice->addListener( this );
}

QDeviceWidget::~QDeviceWidget()
{
	mDevice->removeListener( this );
}

void QDeviceWidget::startStopButtonPressed()
{
	if ( mDevice->isCapturing() )
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		QApplication::processEvents();
		mDevice->stopCapture();
		QApplication::restoreOverrideCursor();
		mImageNumberLabel->setText("0");
		mStartStopButton->setText( "Start" );
		mCaptureSettingsCombo->setEnabled(true);
		mImageWidget->setImage( Image() );
	}
	else
	{
		int index = mCaptureSettingsCombo->currentIndex();
		const RMF::CaptureSettingsList& settingsList = mDevice->getSupportedCaptureSettingsList();
		if ( index>=0 && index<static_cast<int>(settingsList.size()) )
		{
			mStartStopButton->setText( "Stop" );
			QApplication::setOverrideCursor(Qt::WaitCursor);
			QApplication::processEvents();
			if ( mDevice->startCapture( index ) )
				mCaptureSettingsCombo->setEnabled(false);
			else
				mStartStopButton->setText( "Start" );
			QApplication::restoreOverrideCursor();
		}
	}	
}

void QDeviceWidget::onDeviceCapturedImage( Device* device )
{
	assert( device==mDevice );
	const RMF::CapturedImage* capturedImage = device->getCapturedImage();
	if ( capturedImage )
	{
		mImageWidget->setImage( capturedImage->getImage() );
		mImageNumberLabel->setText( QString::number( capturedImage->getSequenceNumber() ) );
	}
}

bool QDeviceWidget::writeRGB24ImageAsPPM( const Image& image, const char* filename )
{
	if ( image.getFormat().getEncoding()!=ImageFormat::RGB24 )
		return false;

	const ImageFormat& imageFormat = image.getFormat();
	const MemoryBuffer& imageBuffer = image.getBuffer();

	// Open file and write
	std::ofstream stream( filename, std::ios::binary|std::ios::out );
	if ( stream.is_open() )
	{
		stream << "P6 " << imageFormat.getWidth() << " " << imageFormat.getHeight() << " 255\n";
		if ( stream.fail() )
			return false;
		stream.write( reinterpret_cast<const char*>(imageBuffer.getBytes()), imageBuffer.getSizeInBytes() );
		if ( stream.fail() )
			return false;
	}               
	return true;
}

bool QDeviceWidget::writeImageAsPPM( const Image& image, const char* filename )
{
	ImageConverter converter( ImageFormat( image.getFormat().getWidth(), image.getFormat().getHeight(), ImageFormat::RGB24 ) );
	if ( !converter.update( image ) )
		return false;
	return writeRGB24ImageAsPPM( converter.getImage(), filename );
}

void QDeviceWidget::keyPressEvent( QKeyEvent* event )
{
	if( event->key() == Qt::Key_F1 )
    {
     	if ( mDevice->isCapturing() )
		{
			const RMF::CapturedImage* capturedImage = mDevice->getCapturedImage();
			if ( capturedImage )
			{
				std::stringstream stream;
				stream << mDevice->getName().c_str() << "_" << capturedImage->getSequenceNumber() << ".ppm";
				writeImageAsPPM( capturedImage->getImage(), stream.str().c_str() );
			}
		}
    }
}

}
