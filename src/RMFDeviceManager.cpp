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
#include "RMFDeviceManager.h"

#include <mfapi.h>
#include <mfidl.h>
#include <ks.h>
#include <Dbt.h>

#include <assert.h>
#include <algorithm>

#include "RMFCOMObjectSharedPtr.h"

namespace RMF
{

/*
	DeviceManager::Internals
*/
class DeviceManager::Internals
{
public:
	Internals( DeviceManager* parentDeviceManager );
	~Internals();

	void			update();
	const Devices&	getDevices() const	{ return mDevices; }

	void			addListener( Listener* listener );
	bool			removeListener( Listener* listener );

protected:
	static void		wideCharStringToMultiByteString( const wchar_t* wideCharString, std::string& multiByteString );

	static void		getName( const IMFActivate* activate, std::string& name );
	static void		getSymbolicLink( const IMFActivate* activate, std::string& symbolicLink );
	
	typedef std::vector< COMObjectSharedPtr<IMFActivate> > IMFActivates;
	static void		enumerateDevices( IMFActivates& activates );
	void			updateDeviceList();

	void			createDevice( COMObjectSharedPtr<IMFActivate>& activate );
	void			deleteDevice( Device* device );

	static LRESULT CALLBACK wndProcHook( int nCode, WPARAM wParam, LPARAM lParam );

private:
	DeviceManager*  mParentDeviceManager;
	bool			mUpdateDeviceListAtNextUpdate;
	Devices			mDevices;

	typedef	std::vector<DeviceManager::Listener*> Listeners; 
	Listeners		mListeners;

	typedef std::vector< DeviceManager::Internals* > InternalObjects;
	static InternalObjects mInternalObjects;
	HHOOK			mHookHandle;

};

// The list of all the DeviceManager::Internals instances that exist in the application
// This is needed because of the static nature of the WindowProc hook
DeviceManager::Internals::InternalObjects DeviceManager::Internals::mInternalObjects;

DeviceManager::Internals::Internals( DeviceManager* parentDeviceManager )
	: mParentDeviceManager( parentDeviceManager ),
	  mUpdateDeviceListAtNextUpdate(true),
	  mDevices(),
	  mListeners(),
	  mHookHandle(0)
{
	// Initialize the COM library 
	// We dont check the result on purpose here. See documentation: 
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms695279(v=vs.85).aspx
	HRESULT hr = S_OK;
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	
    // Initialize Media Foundation
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms702238(v=vs.85).aspx
    hr = MFStartup( MF_VERSION, MFSTARTUP_NOSOCKET );		// Note: in our case, we don't need the sockets library
	assert( SUCCEEDED(hr) );	

	// Add this instance to the static list
	mInternalObjects.push_back( this );
	
	// Register hook when this is the first instance of DeviceManager::Internals created
	// See this article for extra information
	// http://www.codeproject.com/Articles/14500/Detecting-Hardware-Insertion-and-or-Removal
	if ( mInternalObjects.size()==1 )
	{
		DWORD threadID = GetCurrentThreadId();
		HINSTANCE hInstance = GetModuleHandle(NULL) ;
		mHookHandle = SetWindowsHookEx( WH_CALLWNDPROC, wndProcHook, hInstance, threadID );
		assert( mHookHandle );
	}
}

DeviceManager::Internals::~Internals()
{
	// Delete all devices
	Devices devices = mDevices;		// The copy is on purpose here
	for ( std::size_t i=0; i<devices.size(); ++i )
		deleteDevice( devices[i] );
	assert( mDevices.empty() );

	// Shutdown Media Foundation and COM 
	HRESULT hr = MFShutdown();
	assert( SUCCEEDED(hr) );
	
	CoUninitialize();

	// Remove this instance from the static list
	InternalObjects::iterator itr = std::find( mInternalObjects.begin(), mInternalObjects.end(), this );
	assert( itr!=mInternalObjects.end() );
	mInternalObjects.erase(itr);

	// If there's no more instances, remove the hook
	if ( mInternalObjects.empty() )
	{
		BOOL ret = UnhookWindowsHookEx( mHookHandle );
		assert( ret );
		mHookHandle = 0;
	}
}


LRESULT CALLBACK DeviceManager::Internals::wndProcHook( int nCode, WPARAM wParam, LPARAM lParam )
{
/*	std::wstringstream stream;
	stream << std::hex;
	stream << L"code=" << nCode << L" wparam=" << params.wParam << L" lparam=" << params.lParam << L" message=" << params.message << L" hwnd=" << params.hwnd << L"\n";
	OutputDebugString(stream.str().c_str());
*/
	// Note:
	// We only process the message if it is sent by the current thread.
	// By "current" I guess that they mean the same thread as the one called the hook
	// registration function... but that's a guess :(
	// See http://msdn.microsoft.com/en-us/library/windows/desktop/ms644975(v=vs.85).aspx
	// My goal is to use this information to avoid having to handle concurrency with 
	// a critical section or whatnot.
	// Because if this hook is called by different thread, there's a micro chance that the
	// array of InternalObjects I'm using changes while I'm using it
	if ( wParam!=0 )
	{
		const CWPSTRUCT& params = *reinterpret_cast<CWPSTRUCT*>(lParam);
		if ( params.message==WM_DEVICECHANGE )
		{
			for ( std::size_t i=0; i<mInternalObjects.size(); ++i )
				mInternalObjects[i]->mUpdateDeviceListAtNextUpdate = true;
		}
	}

	// Process event
	return CallNextHookEx( NULL, nCode, wParam, lParam );
}

void DeviceManager::Internals::update()
{
	if ( mUpdateDeviceListAtNextUpdate )
	{
		updateDeviceList();
		mUpdateDeviceListAtNextUpdate = false;
	}

	for ( Devices::iterator itr=mDevices.begin(); itr!=mDevices.end(); ++itr )
	{
		Device* device = *itr;
		device->update();
	}
}

void DeviceManager::Internals::enumerateDevices( DeviceManager::Internals::IMFActivates& activates )
{
	activates.clear();
	HRESULT hr = S_OK;
	
	// Prepare attributes for enumerating the devices
	IMFAttributes* attributesRaw = NULL;	
	hr = MFCreateAttributes( &attributesRaw, 1 );
	COMObjectSharedPtr<IMFAttributes> attributesRes( attributesRaw );
	if ( FAILED(hr) )
		return;
	
	// Specify we're only interested in video devices
    hr = attributesRes->SetGUID( MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID );
	if ( FAILED(hr) )
		return;

    // Enumerate the devices
	IMFActivate** activatesList = NULL;
	UINT32 numActivates = 0;
    hr = MFEnumDeviceSources( attributesRes.get(), &activatesList, &numActivates );
	if ( FAILED(hr ) )
		return;

	// Populate our list of Activates with the result
	activates.resize( numActivates );
	for ( UINT32 i=0; i<numActivates; ++i )
	{
		COMObjectSharedPtr<IMFActivate> activate = activatesList[i];
		activates[i] = activate;
	}

	// Release list of enumerated devices
	CoTaskMemFree( activatesList );
}

void DeviceManager::Internals::updateDeviceList()
{
	// Get up-to-date list of Devices
	DeviceManager::Internals::IMFActivates currentActivates;
	DeviceManager::Internals::enumerateDevices( currentActivates );

	// Determine freshly added Devices
	DeviceManager::Internals::IMFActivates newActivates;
	std::string symbolicLink;
	for ( std::size_t i=0; i<currentActivates.size(); ++i )
	{
		const COMObjectSharedPtr<IMFActivate>& currentActivate = currentActivates[i];
		getSymbolicLink( currentActivate.get(), symbolicLink );
		bool found = false;
		for ( std::size_t j=0; j<mDevices.size(); ++j )
		{
			if ( mDevices[j]->getSymbolicLink()==symbolicLink )
			{
				found = true;
				break;
			}
		}

		if ( !found )
			newActivates.push_back( currentActivates[i] );
	}
	
	// Determine freshly removed Devices
	Devices removedDevices;
	for ( std::size_t i=0; i<mDevices.size(); ++i )
	{
		Device* device = mDevices[i];
		bool found = false;

		for ( std::size_t j=0; j<currentActivates.size(); ++j )
		{	
			const COMObjectSharedPtr<IMFActivate>& currentActivate = currentActivates[j];
			getSymbolicLink( currentActivate.get(), symbolicLink );
		
			if ( device->getSymbolicLink()==symbolicLink )
			{
				found = true;
				break;
			}
		}
		if ( !found )
			removedDevices.push_back( device );
	}

	// Create and add the new Devices
	for ( std::size_t i=0; i<newActivates.size(); ++i )
		createDevice( newActivates[i] );
	
	// Remove detached Devices
	for ( std::size_t i=0; i<removedDevices.size(); ++i )
		deleteDevice( removedDevices[i] );
}

void DeviceManager::Internals::createDevice( COMObjectSharedPtr<IMFActivate>& activate )
{
	// Notify 
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceAdding( mParentDeviceManager );

	std::string name;
	getName( activate.get(), name );

	std::string symbolicLink;
	getSymbolicLink( activate.get(), symbolicLink );

	Device* device = new Device( &activate, name, symbolicLink );
	mDevices.push_back( device );

	// Notify 
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceAdded( mParentDeviceManager, device );
}

void DeviceManager::Internals::deleteDevice( Device* device )
{
	Devices::iterator itr = std::find( mDevices.begin(), mDevices.end(), device );
	if ( itr==mDevices.end() )
		return;

	// Notify
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceRemoving( mParentDeviceManager, device );

	mDevices.erase( itr );
	delete device;

	// Notify
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceRemoved( mParentDeviceManager, device );
}

void DeviceManager::Internals::wideCharStringToMultiByteString( const wchar_t* wideCharString, std::string& multiByteString )
{
	assert( wideCharString );
	size_t size = wcslen(wideCharString)+1;
	char* buffer = new char[size];
	size_t numCharConverted = 0;
	int ret = wcstombs_s( &numCharConverted, buffer, size, wideCharString, size );
	if ( ret==0 )
		multiByteString = buffer;
	delete[] buffer;
}

void DeviceManager::Internals::getName( const IMFActivate* activate, std::string& name )
{
	// See http://msdn.microsoft.com/de-de/library/bb970406(v=vs.85).aspx
	WCHAR* theName = NULL;
	HRESULT hr = const_cast<IMFActivate*>(activate)->GetAllocatedString( MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &theName, NULL );	
	if ( SUCCEEDED(hr) )
	{
		wideCharStringToMultiByteString( theName, name );
		CoTaskMemFree( theName );
	}
	else
	{
		name = "No name";
	}
}

void DeviceManager::Internals::getSymbolicLink( const IMFActivate* activate, std::string& symbolicLink )
{
	WCHAR* theSymbolicLink;
	HRESULT hr = const_cast<IMFActivate*>(activate)->GetAllocatedString( MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &theSymbolicLink, NULL );
	if ( SUCCEEDED(hr) )
	{	
		wideCharStringToMultiByteString( theSymbolicLink, symbolicLink );
		CoTaskMemFree( theSymbolicLink );
	}
	else
	{
		symbolicLink = "No symbolic link";
	}
}

void DeviceManager::Internals::addListener( Listener* listener )
{
	assert(listener);
	mListeners.push_back(listener);
}

bool DeviceManager::Internals::removeListener( Listener* listener )
{
	Listeners::iterator itr = std::find( mListeners.begin(), mListeners.end(), listener );
	if ( itr==mListeners.end() )
		return false;
	mListeners.erase( itr );
	return true;
}

/*
	DeviceManager
*/
DeviceManager::DeviceManager()
	: mInternals(NULL)
{
	mInternals = new Internals(this);
}

DeviceManager::~DeviceManager()
{
	delete mInternals;
	mInternals = NULL;
}

void DeviceManager::update()
{
	mInternals->update();
}

const Devices& DeviceManager::getDevices() const
{
	return mInternals->getDevices();
}

void DeviceManager::addListener( Listener* listener )
{
	mInternals->addListener(listener);
}

bool DeviceManager::removeListener( Listener* listener )
{
	return mInternals->removeListener(listener);
}

}