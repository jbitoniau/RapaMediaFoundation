#
# A function to search for MediaFoundation header and lib in a given SDK root path (Windows Platform SDK or DirectX SDK).
# This function looks for the x64 lib if compiling in 64 bits, x86 otherwise.
# The function sets the following variables:
#  MEDIAFOUNDATION_FOUND 
#  MediaFoundation_INCLUDE_DIR 
#  MediaFoundation_LIBRARIES
# 
FUNCTION( FINDMEDIAFOUNDATION SDK_ROOT_PATH )

	# Determine the header path
	MESSAGE("Searching for MediaFoundation in ${SDK_ROOT_PATH}")
	FIND_PATH(	MediaFoundation_INCLUDE_DIR NAMES mfapi.h 
				PATHS "${SDK_ROOT_PATH}/Include/um" "${SDK_ROOT_PATH}/Include" 
				NO_DEFAULT_PATH  )
		
	# Determine the library path depending on the architecture (x86 or x64)
	# The structure of the SDKs (DirectX and Platform) varies significantly from version to version.
	# In the DirectX SDK (at least for the June 2010 one):
	#	/?????
	#		/<Root>
	#			/Lib              (for x86) 
	#				/x64          (for x64)
	#
	# In old Platform SDKs:
	#	/Microsoft SDKs
	#		/<Root>               (v7.0A, v7.1A, v8.0, etc)
	#			/Lib     
	#		 		/x86          (for x86)
	#		 		/x64          (for x64)
	# 
	# In more recent Platform SDKs called "Windows Kits" (for eg: Microsoft Windows SDK for Windows 7 and .NET Framework 4)
	#	/Windows Kits
	#		/<Root>               (8.0)
	#			/Lib
	#				/win8
	#					/um
	#						/x86  (for x86)
	#						/x64  (for x64)
	#
	# Or in Microsoft Windows SDK for Windows 8.1 and .NET Framework 4.5.1:
	#	/Windows Kits
	#		/<Root>               (8.1)
	#			/Lib
	#				/winv6.3
	#					/um
	#						/x86  (for x86)
	#						/x64  (for x64)
	IF ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		SET( _LIBS_STANDARD_SEARCH_PATH 
			 "${SDK_ROOT_PATH}/Lib/winv6.3/um/x64" 
			 "${SDK_ROOT_PATH}/Lib/win8/um/x64" 
			 "${SDK_ROOT_PATH}/Lib/x64" )
	ELSE()
		SET( _LIBS_STANDARD_SEARCH_PATH 
			 "${SDK_ROOT_PATH}/Lib/winv6.3/um/x86" 
			 "${SDK_ROOT_PATH}/Lib/win8/um/x86" 
			 "${SDK_ROOT_PATH}/Lib/x86" 
			 "${SDK_ROOT_PATH}/Lib" )
	ENDIF()
	
	# Try to find the MediaFoundation libs
	# http://msdn.microsoft.com/en-us/library/windows/desktop/ee663600(v=vs.85).aspx
	FIND_LIBRARY( MediaFoundation_MF_LIBRARY NAMES mf PATHS ${_LIBS_STANDARD_SEARCH_PATH} NO_DEFAULT_PATH )
	FIND_LIBRARY( MediaFoundation_MFPLAT_LIBRARY NAMES mfplat PATHS ${_LIBS_STANDARD_SEARCH_PATH} NO_DEFAULT_PATH )
	FIND_LIBRARY( MediaFoundation_MFUUID_LIBRARY NAMES mfuuid PATHS ${_LIBS_STANDARD_SEARCH_PATH} NO_DEFAULT_PATH )
	FIND_LIBRARY( MediaFoundation_SHLWAPI_LIBRARY NAMES shlwapi PATHS ${_LIBS_STANDARD_SEARCH_PATH} NO_DEFAULT_PATH )
	FIND_LIBRARY( MediaFoundation_MFREADWRITE_LIBRARY NAMES mfreadwrite PATHS ${_LIBS_STANDARD_SEARCH_PATH} NO_DEFAULT_PATH )
	
	# Check that the header path and the libs have been found 
	INCLUDE( FindPackageHandleStandardArgs )
	FIND_PACKAGE_HANDLE_STANDARD_ARGS( MediaFoundation DEFAULT_MSG 
						MediaFoundation_MF_LIBRARY MediaFoundation_MFPLAT_LIBRARY MediaFoundation_MFUUID_LIBRARY
						MediaFoundation_SHLWAPI_LIBRARY MediaFoundation_MFREADWRITE_LIBRARY
						MediaFoundation_INCLUDE_DIR )
	
	# Promote the MEDIAFOUNDATION_FOUND variable to parent scope (otherwise it disappears at the function return).
	# Not needed for MediaFoundation_INCLUDE_DIR and MediaFoundation_LIBRARY as they are put in the cache which makes them global
	SET( MEDIAFOUNDATION_FOUND ${MEDIAFOUNDATION_FOUND} PARENT_SCOPE )	
	
	SET( MediaFoundation_LIBRARIES 
		 ${MediaFoundation_MF_LIBRARY} ${MediaFoundation_MFPLAT_LIBRARY} ${MediaFoundation_MFUUID_LIBRARY} 
		 ${MediaFoundation_SHLWAPI_LIBRARY} ${MediaFoundation_MFREADWRITE_LIBRARY}
		 PARENT_SCOPE )
ENDFUNCTION()

#
# We go through a list of SDK root-paths and try to find MediaFoundation there.
#
IF( CMAKE_SYSTEM_NAME MATCHES Windows )
	
	# The default paths where to look for MediaFoundation. This can be user modifiable
	SET(	MediaFoundation_SDK_SEARCH_PATHS 
			"C:/Program Files (x86)/Windows Kits/8.1"
			"C:/Program Files (x86)/Windows Kits/8.0"
			"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)"
			"C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A"
			"C:/Program Files (x86)/Microsoft SDKs/Windows/v7.0A"
			 CACHE STRING 
			 "The list of SDK root-paths where to search for MediaFoundation" )
	
	# Goes through each path and try it
	FOREACH( SDK_ROOT_PATH ${MediaFoundation_SDK_SEARCH_PATHS} )
		IF( NOT MEDIAFOUNDATION_FOUND )
			FINDMEDIAFOUNDATION( ${SDK_ROOT_PATH} )
		ENDIF()
	ENDFOREACH()

ENDIF()


