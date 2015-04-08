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
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QApplication>
#include <QKeyEvent>
#ifdef _MSC_VER
	#pragma warning( pop )
#endif

#include "RMFQImageWidget.h"
#include "RMFDevice.h"

namespace RMF
{

class QDeviceWidget :	public QFrame,
						public RMF::Device::Listener
{ 
	Q_OBJECT

public:
	QDeviceWidget( QWidget* parent, RMF::Device* device, Qt::WindowFlags flags=0 );
	virtual ~QDeviceWidget();

	RMF::Device*			getDevice() const { return mDevice; }

protected:
	virtual void			onDeviceCapturedImage( Device* /*device*/ );
	
private slots:
	void					startStopButtonPressed();
	virtual void			keyPressEvent( QKeyEvent* event );

private:
	static bool				writeRGB24ImageAsPPM( const Image& image, const char* filename );
	static bool				writeImageAsPPM( const Image& image, const char* filename );

	RMF::Device*			mDevice;
	RMF::QImageWidget*	mImageWidget;
	QComboBox*				mCaptureSettingsCombo;
	QPushButton*			mStartStopButton;
	QLabel*					mImageNumberLabel;
};

}