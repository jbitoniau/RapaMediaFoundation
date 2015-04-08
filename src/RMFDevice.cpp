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
#include "RMFDevice.h"

#include <assert.h>
#include <algorithm>
#include "RMFDeviceInternals.h"

namespace RMF
{

Device::Device( void* activateSharedPtrAsVoidPtr, const std::string& name, const std::string& symbolicLink )
	: mName(name),
	  mSymbolicLink(symbolicLink),
	  mSupportedCaptureSettingsList(),
	  mMediaTypeIndices(),
	  mInternals(NULL),
	  mStartedCaptureSettingsIndex(0),
	  mCapturedImage(NULL),
	  mTempImage(NULL)
{
	COMObjectSharedPtr<IMFActivate>& activateSharedPtr = *(reinterpret_cast< COMObjectSharedPtr<IMFActivate>* >( activateSharedPtrAsVoidPtr ));
	mInternals = new DeviceInternals( activateSharedPtr, name );

	// Convert the supported VideoMediaTypes of the DeviceInternals into a CaptureSettingsList
	// We only keep the types that our Image class can handle
	CaptureSettingsList settingsList;
	const DeviceInternals::VideoMediaTypes&	mediaTypes = mInternals->getSupportedVideoMediaTypes();
	for ( std::size_t index=0; index<mediaTypes.size(); ++index )
	{
		const DeviceInternals::VideoMediaType mediaType = mediaTypes[index];
		
		bool supported = true;
		ImageFormat::Encoding encoding = ImageFormat::BGR24;
		if ( mediaType.subType==MFVideoFormat_RGB24 )
			encoding = ImageFormat::BGR24;
		else if ( mediaType.subType==MFVideoFormat_YUY2 )
			encoding = ImageFormat::YUYV;
		else 
			supported = false;

		if ( supported )
		{
			ImageFormat imageFormat = ImageFormat( mediaType.width, mediaType.height, encoding );
			CaptureSettings settings( imageFormat, static_cast<float>( mediaType.frameRate ) );
			mSupportedCaptureSettingsList.push_back( settings );
			mMediaTypeIndices.push_back(index);
		}
	}
}

Device::~Device()
{
	if ( isCapturing() )
		stopCapture();
	delete mInternals;
	mInternals = NULL;
}

bool Device::isCapturing() const
{
	return mInternals->isCapturing();
}

bool Device::startCapture( const CaptureSettings& captureSettings )
{
	if ( isCapturing() )
		return false;

	for ( std::size_t index=0; index<mSupportedCaptureSettingsList.size(); ++index )
	{
		if ( captureSettings==mSupportedCaptureSettingsList[index] )
			return startCapture( index );
	}
	return false;
}

bool Device::startCapture( std::size_t captureSettingsIndex )
{
	if ( isCapturing() )
		return false;

	if (captureSettingsIndex<0 || captureSettingsIndex >= mSupportedCaptureSettingsList.size())
		return false;

	// Remember which CaptureSettings we've started
	mStartedCaptureSettingsIndex = static_cast<unsigned int>(captureSettingsIndex);
	
	// Prepare the Image that will receive the data when the update method is called
	const CaptureSettings& captureSettings = mSupportedCaptureSettingsList[mStartedCaptureSettingsIndex];
	mCapturedImage = new CapturedImage( captureSettings.getImageFormat() );
	
	// Find the MediaType corresponding to the index of the CaptureSettings to use
	assert( mStartedCaptureSettingsIndex<mMediaTypeIndices.size() );
	int mediaTypeIndex = static_cast<int>(mMediaTypeIndices[mStartedCaptureSettingsIndex]);
	const DeviceInternals::VideoMediaType& mediaType = mInternals->getSupportedVideoMediaTypes()[mediaTypeIndex];

	// Prepare an image for vertical flip if necessary
	assert( !mTempImage );
	if ( mediaType.stride<0 )
		mTempImage = new Image( captureSettings.getImageFormat() );
	
	// Start the capture
	bool ret = mInternals->startCapture( mediaTypeIndex );
	if ( ret )
	{
		// Notify
		for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
			(*itr)->onDeviceStarted( this );
	}
	else
	{
		delete mTempImage;
		mTempImage = NULL;
	}

	return ret;
}

bool Device::getStartedCaptureSettingsIndex( unsigned int& index ) const
{
	index = 0;
	if ( !isCapturing() )
		return false;
	index = mStartedCaptureSettingsIndex;
	return true;
}

bool Device::getStartedCaptureSettings( CaptureSettings& settings ) const
{
	settings = CaptureSettings();
	if ( !isCapturing() )
		return false;
	settings = mSupportedCaptureSettingsList[mStartedCaptureSettingsIndex];
	return true;
}

void Device::stopCapture()
{
	if ( !isCapturing() )
		return; 

	// Notify
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceStopping( this );

	mInternals->stopCapture();

	// Delete the CaptureImage that receives the data
	delete mCapturedImage;
	mCapturedImage = NULL;
	
	// Also delete the TempImage when that is only created when a vertical flip is needed
	delete mTempImage;
	mTempImage = NULL;

	mStartedCaptureSettingsIndex = 0;
}

bool Device::flipImageVertically( const Image& sourceImage, Image& destinationImage )
{
	if ( sourceImage.getFormat()!=destinationImage.getFormat() )
		return false;
	unsigned int height = sourceImage.getFormat().getHeight();
	unsigned int numBytesPerLine = sourceImage.getFormat().getNumBytesPerLine();
	const unsigned char* sourceBytes = sourceImage.getBuffer().getBytes();
	unsigned char* destinationBytes = destinationImage.getBuffer().getBytes() + ( numBytesPerLine * (height-1) );
	for ( unsigned int y=0; y<height; ++y )
	{
		memcpy( destinationBytes, sourceBytes, numBytesPerLine );
		destinationBytes -= numBytesPerLine;
		sourceBytes += numBytesPerLine;
	}
	
	return true;
}

void Device::update()
{
	if ( !isCapturing() )
		return;

	assert( mCapturedImage );

	unsigned int sequenceNumber = 0;
	LONGLONG timestamp = 0;
		
	if ( mTempImage )
	{
		// If we need to flip the image vertically, we ask the Internals object
		// to copy its image buffer into the temporary Image
		MemoryBuffer& buffer = mTempImage->getBuffer();
		bool ret = mInternals->getCapturedImage( buffer, sequenceNumber, timestamp );
		
		// It's legal for getCapturedImage() to fail even though isCapturing() returns true
		// This happens when the camera has just started but hasn't captured the first image yet
		if ( !ret )
			return;

		// We can then copy+flip the Temp image into the final CaptureImage
		Image& capturedImage = mCapturedImage->getImage();
		assert( mTempImage->getFormat()==capturedImage.getFormat() );
		ret = flipImageVertically( *mTempImage, capturedImage );
		assert( ret );	

		// Note: in theory we could have the Internals object perform this copy+flip
		// in one go and avoid having the Temp image at all, but I wanted the Internals
		// to be dumb and simply fill a blob (a MemoryBuffer) with timestamp and 
		// sequence number. I didn't want it to know anything about our Image class 
		// and how to flip its contents. The Internals object is in theory capable of
		// returning a complex blob like a JPG image (that happens when it's capturing
		// in MJPG on the Apple FaceTime HD Camera for example, though we don't exploit
		// this format yet). So introducing a flip option in the Internals would be a
		// bit weird I think
	}	
	else
	{
		// When there's no flip involved, the CapturedImage is directly filled from
		// the Internals object 
		MemoryBuffer& buffer = mCapturedImage->getImage().getBuffer();
		bool ret = mInternals->getCapturedImage( buffer, sequenceNumber, timestamp );

		// See comment above
		if ( !ret )
			return;
	}
	
	
	// Note: the sign of the stride in the underlying MediaType dictates whether we should
	// flip the the image vertically. We don't do it for now...

	// Set the sequence number
	mCapturedImage->setSequenceNumber( sequenceNumber );
	
	// Set the timestamp
	// The timestamp coming form the Internals object is in 100 nanosecond units
	// http://msdn.microsoft.com/fr-fr/library/windows/desktop/dd374658(v=vs.85).aspx
	float timestampInSec = static_cast<float>(timestamp) /  1e7f;		
	mCapturedImage->setTimestampInSec( timestampInSec );	

	// Notify
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceCapturedImage( this );
}

void Device::addListener( Listener* listener )
{
	assert(listener);
	mListeners.push_back(listener);
}

bool Device::removeListener( Listener* listener )
{
	Listeners::iterator itr = std::find( mListeners.begin(), mListeners.end(), listener );
	if ( itr==mListeners.end() )
		return false;
	mListeners.erase( itr );
	return true;
}

}