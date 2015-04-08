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

#include <vector>
#include <string>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include "RMFCOMObjectSharedPtr.h"
#include "RMFMemoryBuffer.h"

namespace RMF
{

class DeviceInternals : public IMFSourceReaderCallback
{
public:
	DeviceInternals( const COMObjectSharedPtr<IMFActivate>& activate, const std::string& name );
	virtual ~DeviceInternals();

	class VideoMediaType
	{
	public:
		VideoMediaType();					// Note: put proper accessors here
		UINT32				width ;
		UINT32				height;
		INT32				stride;
		UINT32				frameRate;
		GUID				subType;
				
		std::string			getSubTypeName() const		{ return getSubTypeName(subType); }
		static std::string	getSubTypeName( const GUID& mediaType );

		std::string			toString() const;

		bool				operator==( const DeviceInternals::VideoMediaType& other ) const;
		bool				operator!=( const DeviceInternals::VideoMediaType& other ) const;
	};
	typedef std::vector<VideoMediaType> VideoMediaTypes;
	const VideoMediaTypes&		getSupportedVideoMediaTypes() const { return mSupportedVideoMediaTypes; }

	//bool						startCapture( const VideoMediaType& videoMediaType );
	bool						startCapture( DWORD videoMediaTypeIndex );
	void						stopCapture();
	bool						isCapturing() const	{ return mIsCapturing; }
	//unsigned int				getCapturedImageSequenceNumber() const;
	bool						getCapturedImage( MemoryBuffer& buffer, unsigned int& sequenceNumber, LONGLONG& timestamp ) const;

protected:
	static bool					getVideoMediaType( IMFSourceReader* sourceReader, DWORD index, VideoMediaType& mediaTypeInfo );
	static VideoMediaTypes		getVideoMediaTypes( IMFSourceReader* sourceReader );
	bool						createMediaSourceReader();
	void						deleteMediaSourceReader();

	STDMETHODIMP				QueryInterface( REFIID riid, void** ppv );
	STDMETHODIMP_(ULONG)		AddRef();
	STDMETHODIMP_(ULONG)		Release();
	STDMETHODIMP				OnReadSample( HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample);
	STDMETHODIMP				OnEvent( DWORD, IMFMediaEvent* );
	STDMETHODIMP				OnFlush( DWORD );
	
private:
	COMObjectSharedPtr<IMFActivate>	mActivate;			
	std::string					mName;
	mutable CRITICAL_SECTION	mCriticalSection;
	ULONG						mReferenceCounter;
	COMObjectSharedPtr<IMFSourceReader> mSourceReaderRes;
	bool						mIsCapturing;
	VideoMediaTypes				mSupportedVideoMediaTypes;
	unsigned int				mCapturedImageNumber;
	LONGLONG					mCapturedImageTimestamp;
	MemoryBuffer*				mCapturedImageBuffer;
};

}
