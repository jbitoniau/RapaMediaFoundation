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
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX 
#include <windows.h>

#include "RMFDeviceManager.h"
#include "RMFImageConverter.h"
#include <stdio.h>
#include <assert.h>
#include <sstream>
#include <fstream>

bool writeImageAsRaw( const RMF::Image& image, const char* filename )
{
	std::ofstream stream( filename, std::ios::binary|std::ios::out );
	if ( stream.is_open() )
	{
		stream.write( reinterpret_cast<const char*>( image.getBuffer().getBytes() ), image.getBuffer().getSizeInBytes() );
		if ( stream.fail() )
			return false;
	}
	return true;
}

bool writeRGB24ImageAsPPM( const RMF::Image& image, const char* filename )
{
	if ( image.getFormat().getEncoding()!=RMF::ImageFormat::RGB24 )
		return false;
	
	const RMF::ImageFormat& imageFormat = image.getFormat();
	const RMF::MemoryBuffer& imageBuffer = image.getBuffer();

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

bool writeImageAsPPM( const RMF::Image& image, const char* filename )
{
	RMF::ImageConverter converter( RMF::ImageFormat( image.getFormat().getWidth(), image.getFormat().getHeight(), RMF::ImageFormat::RGB24 ) );
	if ( !converter.update( image ) )
		return false;
	return writeRGB24ImageAsPPM( converter.getImage(), filename );
}

void testDeviceCaptureSettings( RMF::Device* device, std::size_t index )
{
	const RMF::CaptureSettingsList& settingsList = device->getSupportedCaptureSettingsList();
	const RMF::CaptureSettings& settings = settingsList[index];
	printf("\t%s", settings.toString().c_str() );
	
	if ( !device->startCapture( index ) )
	{
		printf(" Failed\n");
		return;
	}

	std::stringstream stream;
	RMF::ImageFormat format = settings.getImageFormat();
	stream << device->getName() << "_" << index << "_" << format.getWidth() << "x" << format.getHeight() << "." << format.getEncodingName();
	std::string filename = stream.str();
	stream << ".PPM";
	std::string filenamePPM = stream.str();

	const RMF::CapturedImage* capturedImage = NULL;
	do
	{
		device->update();
		capturedImage = device->getCapturedImage();
		Sleep(100);
		printf( "." );
	} 
	while ( !capturedImage || capturedImage->getSequenceNumber()< 5 );

	if ( capturedImage )
	{
		const RMF::Image& image = capturedImage->getImage();
		writeImageAsRaw( image, filename.c_str() );
		writeImageAsPPM( image, filenamePPM.c_str() );
	}
	
	printf(" OK\n");
	device->stopCapture();
}

void testDevice( const RMF::DeviceManager* deviceManager, std::size_t index )
{
	const RMF::Devices& devices = deviceManager->getDevices();
	RMF::Device* device = devices[index];
	
	printf("Device %d - %s\n", index, device->getName().c_str() );
	const RMF::CaptureSettingsList& supportedSettings = device->getSupportedCaptureSettingsList();
	for ( std::size_t i=0; i<supportedSettings.size(); ++i )
	{
		testDeviceCaptureSettings( device, i );
		Sleep(100);
	}
}

int main()
{
	RMF::DeviceManager* deviceManager = new RMF::DeviceManager();
	deviceManager->update();

	const RMF::Devices& devices = deviceManager->getDevices();
	for ( std::size_t i=0; i<devices.size(); ++i )
		testDevice( deviceManager, i );
	
	delete deviceManager;
	deviceManager = NULL;

	return 0;
}