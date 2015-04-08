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
#include "RMFDeviceInternals.h"

#include <assert.h>
#include <sstream>

#include "RMFCriticalSectionEnterer.h"

namespace RMF
{

/*	
	Media Foundation overview
	http://msdn.microsoft.com/en-us/library/windows/desktop/ms696219(v=vs.85).aspx

	SourceReader
	http://msdn.microsoft.com/en-us/library/windows/desktop/dd940436(v=vs.85).aspx (overview)
	http://msdn.microsoft.com/en-us/library/windows/desktop/dd374655(v=vs.85).aspx (methods)
	http://msdn.microsoft.com/en-us/library/windows/desktop/gg583871(v=vs.85).aspx (asynchronous use)
	
	Notes:
	- Frame	rate should probably be expressed as a float, or as a couple numerator-denominator 
	  instead of an UINT32

	- Information about image in memory and stride:
		http://msdn.microsoft.com/en-us/library/windows/desktop/aa473780(v=vs.85).aspx
	- Contiguous representation means no stride 
		http://msdn.microsoft.com/en-us/library/windows/desktop/ms698965(v=vs.85).aspx

*/

/*
	DeviceInternals::VideoMediaType
	The VideoMediaType is a helper object that stores useful attributes concerning 
	an IMFMediaType which "major" type is MFMediaType_Video.
	http://msdn.microsoft.com/en-us/library/windows/desktop/bb530109(v=vs.85).aspx
*/
DeviceInternals::VideoMediaType::VideoMediaType()
	: width(0),
	  height(0),
	  frameRate(0),
	  subType()
{
}

bool DeviceInternals::VideoMediaType::operator==( const DeviceInternals::VideoMediaType& other ) const
{	
	return	width==other.width &&
			height==other.height &&
			frameRate==other.frameRate &&
			subType==other.subType;
}

bool DeviceInternals::VideoMediaType::operator!=( const DeviceInternals::VideoMediaType& other ) const
{
	return	!( (*this)==other );
}

std::string DeviceInternals::VideoMediaType::getSubTypeName( const GUID& mediaType )
{
	// Information about the subtypes can be found here:
	// http://msdn.microsoft.com/fr-fr/library/windows/desktop/aa370819(v=vs.85).aspx

	// The base Media Foundation GUIDs are defined in mfapi.h
	// Additional types seem to exist as seen here:  
	// http://www.koders.com/cpp/fid1791D327A443F07957F42F94831CD7B6FA6A712B.aspx
	// or here: 
	// http://mediaxw.sourceforge.net/files/tmp/uuids.h
	// These GUIDs are in fact defined in some very old DirectX header: uuids.h which can normally be found
	// in C:\Program Files\Microsoft SDKs\Windows\v6.0A\Include\uuids.h

	// On the iMac, the FaceTime HD camera supports YUYV/YUY2 formats as well as MJPG
	// The old Logitech QuickCam Pro 3000 / Logicool supports Y420 formats which aren't listed in Media Foundation either
	// See http://en.wikipedia.org/wiki/YUV#Y.27UV420p_.28and_Y.27V12_or_YV12.29
	
	// Special case for the raw / DirectX-based image formats
	if ( mediaType==MFVideoFormat_RGB32 )	
		return "RGB32";
	if ( mediaType==MFVideoFormat_ARGB32 )	
		return "ARGB32";
	if ( mediaType==MFVideoFormat_RGB24 )	
		return "BGR24";
	if ( mediaType==MFVideoFormat_RGB555 )	
		return "RGB555";
	if ( mediaType==MFVideoFormat_RGB565 )	
		return "RGB565";
	if ( mediaType==MFVideoFormat_RGB8 )	
		return "RGB8";

	// For the other GUIDs, we extract the FourCC 
	DWORD fcc = mediaType.Data1;
	char* fccAsChars = reinterpret_cast<char*>( &fcc );
	std::string ret( fccAsChars, sizeof(DWORD) );

	return ret;
}

std::string DeviceInternals::VideoMediaType::toString() const
{
	std::stringstream stream;
	stream << "width:" << width << " height:" << height << " stride:" << stride << " frameRate:" << frameRate << " subType:" << getSubTypeName();
	return stream.str();
}

/*
	DeviceInternals
*/
DeviceInternals::DeviceInternals( const COMObjectSharedPtr<IMFActivate>& activate, const std::string& name )
	: mActivate( activate ),
	  mName(name),				// Just used for debug purpose, so when inspecting this object we can check out its name
	  mCriticalSection(),
	  mReferenceCounter(1),
	  mSourceReaderRes(),
	  mIsCapturing(false),
	  mCapturedImageNumber(0),
	  mCapturedImageTimestamp(0),
	  mCapturedImageBuffer(NULL)
{
	InitializeCriticalSection( &mCriticalSection );

	// Create a MediaSource temporarily just to get the list of the MediaTypes it supports
	createMediaSourceReader();
	if ( mSourceReaderRes.get() )
	{
		mSupportedVideoMediaTypes = getVideoMediaTypes( mSourceReaderRes.get() );
		
		// Uncomment this to print all the MediaTypes supported by this Device
		//for ( std::size_t i=0; i<mSupportedVideoMediaTypes.size(); ++i )
		//	printf("%s - %d - %s\n", getName().c_str(), i, mSupportedVideoMediaTypes[i].toString().c_str() );
		deleteMediaSourceReader();
	}
}

DeviceInternals::~DeviceInternals()
{
	if ( isCapturing() )
		stopCapture();

	mSupportedVideoMediaTypes.clear();
	DeleteCriticalSection(&mCriticalSection);
}
/*
bool DeviceInternals::startCapture( const VideoMediaType& videoMediaType )
{
	// Note: it's OK for the CS to be entered here and again in the call to startCapture below
	CriticalSectionEnterer criticalSectionRAII( mCriticalSection );				

	if ( isCapturing() )
		return false;

	const VideoMediaTypes& mediaTypes = getSupportedVideoMediaTypes();
	for ( std::size_t index=0; index<mediaTypes.size(); ++index )
	{
		if ( mediaTypes[index]==videoMediaType )
		{
			bool ret = startCapture( index );
			return ret;
		}	
	}
	return false;
}*/

bool DeviceInternals::startCapture( DWORD mediaTypeIndex )
{
	CriticalSectionEnterer criticalSectionRAII( mCriticalSection );

	if ( isCapturing() )
		return false;

	// Create the SourceReader
	if ( !createMediaSourceReader() )
		return false;

	// Fetch the appropriate MediaType
	IMFMediaType* mediaType = NULL;
	HRESULT hr = mSourceReaderRes->GetNativeMediaType( 0, mediaTypeIndex, &mediaType );
	COMObjectSharedPtr<IMFMediaType> mediaTypeRes( mediaType );

	// Set the SourceReader to this type
	hr = mSourceReaderRes->SetCurrentMediaType( 0, NULL, mediaTypeRes.get() );
	if ( FAILED(hr) )
		return false;

	// Update members
	mIsCapturing = true;
	mCapturedImageNumber = 0;
	mCapturedImageTimestamp = 0;
	assert( !mCapturedImageBuffer );
	mCapturedImageBuffer = NULL;

	// Request the first video frame
	hr = mSourceReaderRes->ReadSample( (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL ); 
	if ( FAILED(hr) ) 
		return false; 

	return true;
}

void DeviceInternals::stopCapture()
{
	CriticalSectionEnterer criticalSectionRAII( mCriticalSection );

	if ( !isCapturing() )	
		return;

	// Delete the SourceReader
	deleteMediaSourceReader();

	// Update members
	mIsCapturing = false;
	mCapturedImageNumber = 0;
	mCapturedImageTimestamp = 0;
	delete mCapturedImageBuffer;
	mCapturedImageBuffer = NULL;
}

/*unsigned int DeviceInternals::getCapturedImageSequenceNumber() const
{
	CriticalSectionEnterer criticalSectionRAII( mCriticalSection );
	unsigned int number = mCapturedImageNumber;
	return number;
}*/

bool DeviceInternals::getCapturedImage( MemoryBuffer& buffer, unsigned int& sequenceNumber, LONGLONG& timestamp ) const
{
	CriticalSectionEnterer criticalSectionRAII( mCriticalSection );
	
	// Check that we're currently capturing and that a first image was already grabbed
	if ( !mCapturedImageBuffer )
		return false;

	// Initialize output parameters
	sequenceNumber = 0;
	timestamp = 0;

	// Check that the destination buffer matches the size
	if ( buffer.getSizeInBytes()!=mCapturedImageBuffer->getSizeInBytes() )
		return false;
	
	// Fill the output parameters
	bool ret = buffer.copyFrom( *mCapturedImageBuffer );
	assert( ret );
	sequenceNumber = mCapturedImageNumber;
	timestamp = mCapturedImageTimestamp;
	
	return true;
}

bool DeviceInternals::getVideoMediaType( IMFSourceReader* sourceReader, DWORD index, VideoMediaType& mediaTypeInfo )
{
	// The list of MediaType attributes can be found here:
	// http://msdn.microsoft.com/fr-fr/library/windows/desktop/aa376629(v=vs.85).aspx
	HRESULT hr = S_OK;
	IMFMediaType* mediaTypeRaw = NULL;
	hr = sourceReader->GetNativeMediaType( 0, index, &mediaTypeRaw );
	COMObjectSharedPtr<IMFMediaType> mediaTypeRes( mediaTypeRaw );
	if ( FAILED(hr) )
		return false;

	UINT32 width = 0;
	UINT32 height = 0;
	hr = MFGetAttributeSize( mediaTypeRes.get(), MF_MT_FRAME_SIZE, &width, &height );
	if ( FAILED(hr) )
		return false;

	UINT32 numerator = 0;
	UINT32 denominator = 0;
	hr = MFGetAttributeRatio( mediaTypeRes.get(), MF_MT_FRAME_RATE, &numerator, &denominator);
	if ( FAILED(hr) )
		return false;
	if ( denominator==0 )
		return false;
	UINT32 frameRate = (UINT32)( (float)numerator / (float)denominator + 0.5f );
	
	// If getting the stride fails, we'll probably have to implement the following things:
	// See the GetDefaultStride() code sample:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/aa473821(v=vs.85).aspx
	INT32 stride = 0;
	hr = mediaTypeRes->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&stride);
	
	// Note: the "Apple FaceTime HD Camera (Built-in)" triggers the following assert when run 
	// under Parallels Desktop (might also occur under Windows?)
	// assert( SUCCEEDED(hr) );		// Note: just to spot broken devices or drivers...
	if ( FAILED(hr) )    
		return false;

	GUID subType = { 0 };
	hr = mediaTypeRes.get()->GetGUID( MF_MT_SUBTYPE, &subType );
			
	mediaTypeInfo.width = width;
	mediaTypeInfo.height = height;
	mediaTypeInfo.stride = stride;
	mediaTypeInfo.frameRate = frameRate;
	mediaTypeInfo.subType = subType;
	
	return true;
}

DeviceInternals::VideoMediaTypes DeviceInternals::getVideoMediaTypes( IMFSourceReader* sourceReader )
{
	VideoMediaTypes mediaTypeInfos;
	DWORD index = 0;
	VideoMediaType mediaTypeInfo;
	while ( getVideoMediaType( sourceReader, index, mediaTypeInfo ) )
	{
		mediaTypeInfos.push_back( mediaTypeInfo );
		index++;
	}
	return mediaTypeInfos;
}

// Use the Activate object given at constructor time to create a MediaSource
bool DeviceInternals::createMediaSourceReader()
{
	CriticalSectionEnterer criticalSectionRAII( mCriticalSection );

	HRESULT hr = S_OK;
	
	// Use the Activate object to get a grip on the actual MediaSource
	IMFMediaSource* mediaSourceRaw= NULL;
	hr = mActivate->ActivateObject(	__uuidof(IMFMediaSource), (void**)&mediaSourceRaw );
	COMObjectSharedPtr<IMFMediaSource> mediaSourceRes( mediaSourceRaw );
	if ( FAILED(hr) )
		return false;

	// Set up a few attributes on the MediaSource
	// The full list of attributes can be found here:
	// http://msdn.microsoft.com/fr-fr/library/windows/desktop/dd389286(v=vs.85).aspx
	IMFAttributes* attributesRaw = NULL;
	hr = MFCreateAttributes( &attributesRaw, 2 );
	COMObjectSharedPtr<IMFAttributes> attributesRes( attributesRaw );
	if ( FAILED(hr) )
		return false;
	
	// Register this object as the receiver of the SourceReader callback
	hr = attributesRes->SetUnknown( MF_SOURCE_READER_ASYNC_CALLBACK, this );
	if ( FAILED(hr) )
		return false;

	// Disable converters 
	hr = attributesRes->SetUINT32( MF_READWRITE_DISABLE_CONVERTERS, TRUE );		
	if ( FAILED(hr) )
		return false;

	// Create a SourceReader from the MediaSource
	IMFSourceReader* sourceReaderRaw = NULL;
	hr = MFCreateSourceReaderFromMediaSource( mediaSourceRes.get(), attributesRes.get(), &sourceReaderRaw );
	COMObjectSharedPtr<IMFSourceReader> sourceReaderRes( sourceReaderRaw );
	if ( FAILED(hr) )
		return false;
	
	// Store SourceReader as member
	mSourceReaderRes = sourceReaderRes;

	return true;	
}

void DeviceInternals::deleteMediaSourceReader()
{
	CriticalSectionEnterer criticalSectionRAII( mCriticalSection );

	if ( !mSourceReaderRes.get() )
		return;

	// Release the SourceReader
	mSourceReaderRes = COMObjectSharedPtr<IMFSourceReader>();
	
	// Detach the Activate object
	HRESULT hr = mActivate->DetachObject();
	assert( SUCCEEDED(hr) );
}

STDMETHODIMP DeviceInternals::QueryInterface( REFIID riid, void** ppv )
{
	static const QITAB qit[] = { QITABENT(DeviceInternals, IMFSourceReaderCallback), { 0 }, };
	return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) DeviceInternals::AddRef()
{
	return InterlockedIncrement( &mReferenceCounter );
}

STDMETHODIMP_(ULONG) DeviceInternals::Release()
{
	ULONG counter = InterlockedDecrement(&mReferenceCounter);
	if ( counter==0 )
		delete this;
	return counter;
}

STDMETHODIMP DeviceInternals::OnReadSample( HRESULT hrStatus, DWORD /*dwStreamIndex*/, DWORD /*dwStreamFlags*/, LONGLONG llTimestamp, IMFSample *pSample )
{
	// See "Implementing the Callback Interface" here:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/gg583871(v=vs.85).aspx
	
	CriticalSectionEnterer criticalSectionRAII( mCriticalSection );
	
	// Pre-checks
	assert( isCapturing() );
	if ( !isCapturing() )
		return S_FALSE;		// http://msdn.microsoft.com/fr-fr/library/windows/desktop/dd374658(v=vs.85).aspx

	if ( FAILED(hrStatus) )
		return S_FALSE;
	
	HRESULT hr = S_OK;
	if ( pSample )
	{
		// Get the MediaBuffer from the Sample
		hr = S_OK;
		IMFMediaBuffer* mediaBufferRaw = NULL;		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms696261(v=vs.85).aspx
		hr = pSample->ConvertToContiguousBuffer( &mediaBufferRaw );
		COMObjectSharedPtr<IMFMediaBuffer> mediaBufferRes( mediaBufferRaw );
		if ( FAILED(hr) )
			return S_FALSE;
	
		// Instead of directly working with the IMFMediaBuffer, we should probably try to query its MF2DBuffer and use 
		// it if it exists (which it didn't on most webcams when I tried). We might even check if the buffer is using 
		// a Direct3D surface. This is explained here:
		// http://msdn.microsoft.com/en-us/library/windows/desktop/aa473821(v=vs.85).aspx

		// Lock	the MediaBuffer 
		BYTE* bufferData = NULL;
		DWORD maxLength = 0;
		DWORD currentLength = 0;
		hr = mediaBufferRes->Lock( &bufferData, &maxLength, &currentLength );
		if ( FAILED(hr) )
			return S_FALSE;

		// If it's the first time since start that the callback is called, the ImageBuffer
		// doesn't exist. We allocate one to receive this sample and the next one.
		// By doing this, we assume that the size of the sample buffer doesn't change which
		// seems fair enough
		if ( !mCapturedImageBuffer )
			mCapturedImageBuffer = new MemoryBuffer( currentLength );

		unsigned int imageBufferSize = mCapturedImageBuffer->getSizeInBytes();
		if ( imageBufferSize!=currentLength )
		{
			hr = mediaBufferRes->Unlock();		// No RAII locker object here :(
			return S_FALSE;
		}
		
		// Copy the data from the sample buffer into our image buffer
		unsigned char* imageBufferData = mCapturedImageBuffer->getBytes();
		memcpy( imageBufferData, bufferData, imageBufferSize ); 

		// Update sequence number and timestamp
		mCapturedImageNumber++;
		mCapturedImageTimestamp = llTimestamp;
	
		// Unlock the MediaBuffer
		hr = mediaBufferRes->Unlock();
		if ( FAILED(hr) )
			return S_FALSE;
	}

	// Request to read more samples
	hr = mSourceReaderRes->ReadSample( (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL );
	if ( FAILED(hr) )
		return S_FALSE;

	return S_OK;
}

STDMETHODIMP DeviceInternals::OnEvent( DWORD, IMFMediaEvent* )
{
	return S_OK;
}

STDMETHODIMP DeviceInternals::OnFlush( DWORD )
{
	return S_OK;
}

}