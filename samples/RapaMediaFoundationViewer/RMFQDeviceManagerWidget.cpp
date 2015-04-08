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
#include "RMFQDeviceManagerWidget.h"

#include <assert.h>
#include "RMFQDeviceWidget.h"

namespace RMF
{

QDeviceManagerWidget::QDeviceManagerWidget( QWidget* parent, RMF::DeviceManager* deviceManager )
	:	QFrame(parent), 
		mDeviceManager(deviceManager),
		mDeviceListWidget(NULL),
		mStartStopButton(NULL)
{
	setWindowTitle( "Device Manager" );

	QVBoxLayout* mainLayout = new QVBoxLayout();
	setLayout( mainLayout);

	mDeviceListWidget = new QListWidget( this );
	mainLayout->addWidget( mDeviceListWidget );
	bool ret = false;
	ret = connect( mDeviceListWidget, SIGNAL( itemDoubleClicked(QListWidgetItem*) ), SLOT( onListItemDoubleCliked(QListWidgetItem*) ) );

	mTimer = new QTimer(this);
	ret = connect( mTimer, SIGNAL( timeout() ), SLOT( onTimerTimeOut() ) );
	assert( ret );
	mTimer->setInterval(0);
	mTimer->start();

	mDeviceManager->addListener(this);
}

QDeviceManagerWidget::~QDeviceManagerWidget()
{
	mDeviceManager->removeListener(this);
}

void QDeviceManagerWidget::onTimerTimeOut()
{
	mDeviceManager->update();
}

Device* QDeviceManagerWidget::getDeviceFromItem( QListWidgetItem* item ) const
{
	int devicePointer = item->data( 0x1234 ).toInt();
	assert(devicePointer);
	Device* device = reinterpret_cast<Device*>(devicePointer);
	return device;
}

QListWidgetItem* QDeviceManagerWidget::getItemFromDevice( Device* device ) const
{
	for ( int i=0; i<mDeviceListWidget->count(); ++i )
	{
		QListWidgetItem* item = mDeviceListWidget->item(i);
		int devicePointer = item->data( 0x1234 ).toInt();
		assert(devicePointer);
		Device* d = reinterpret_cast<Device*>(devicePointer);
		if ( d==device )
			return item;
	}
	return NULL;
}

QDeviceWidget* QDeviceManagerWidget::getDeviceWidgetFromItem( QListWidgetItem* item ) const
{
	Device* device = getDeviceFromItem(item);
	int devicePointer = reinterpret_cast<int>(device);
	QString objectName = QString("Device#%1").arg(devicePointer);
	QDeviceWidget* deviceWidget = findChild<QDeviceWidget*>(objectName);
	return deviceWidget;
}	

void QDeviceManagerWidget::onListItemDoubleCliked( QListWidgetItem* item )
{
	QDeviceWidget* deviceWidget = getDeviceWidgetFromItem(item);
	if ( deviceWidget )
		return;

	Device* device = getDeviceFromItem(item);
	deviceWidget = new QDeviceWidget(this, device, Qt::Window);
	deviceWidget->setAttribute(Qt::WA_DeleteOnClose);
	int devicePointer = reinterpret_cast<int>(device);
	QString objectName = QString("Device#%1").arg(devicePointer);
	deviceWidget->setObjectName(objectName);
	QString title = QString("Device '%1'").arg( QString::fromUtf8(device->getName().c_str()) );
	deviceWidget->setWindowTitle( title );
	deviceWidget->resize( 640, 480 );	
	deviceWidget->show();
}

void QDeviceManagerWidget::onDeviceAdded( DeviceManager* /*deviceManager*/, Device* /*device*/ )
{
	updateDeviceListWidget();
}

void QDeviceManagerWidget::onDeviceRemoving( DeviceManager* /*deviceManager*/, Device* device )
{
	QListWidgetItem* item = getItemFromDevice(device);
	QDeviceWidget* deviceWidget = getDeviceWidgetFromItem(item);
	if ( deviceWidget )
		delete deviceWidget;
}

void QDeviceManagerWidget::onDeviceRemoved( DeviceManager* /*deviceManager*/, Device* /*device*/ )
{
	updateDeviceListWidget();
}

void QDeviceManagerWidget::updateDeviceListWidget()
{
	mDeviceListWidget->clear();

	RMF::Devices devices = mDeviceManager->getDevices();
	for ( std::size_t i=0; i<devices.size(); ++i )
	{
		QString name = QString::fromUtf8( devices[i]->getName().c_str() );
		QListWidgetItem* item = new QListWidgetItem( name );
		mDeviceListWidget->addItem( item );
		Device* device = devices[i];
		int devicePointer = reinterpret_cast<int>(device);
		item->setData( 0x1234, devicePointer );
	}
}

}

