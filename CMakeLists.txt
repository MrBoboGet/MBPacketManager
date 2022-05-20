project(MBPacketManager)
set(PROJECT_NAME "MBPacketManager")
##BEGIN MBPM_VARIABLES
set(MBPM_DEPENDENCIES 
	"MBCLI"
	"MBUnicode"
	"MBUtility"
	"MBCrypto"
	"cryptopp"
	"MrBigInt"
	"MBParsing"
	"MrBoboSockets"
	"MBMime"
)
set(MBPM_SystemLibraries
Ws2_32.lib
Secur32.lib
Bcrypt.lib
Mfplat.lib
opengl32
Mfuuid.lib
	Strmiids.lib
)
set(MBPM_DependanciesIncludeDirectories
)
set(MBPM_TARGET_EXTPACKET_LIBRARIES )
set(MBPM_TARGET_COMPILE_OPTIONS )
set(MBPM_TARGET_LINK_OPTIONS )
#MBPM_Functions#variables MBPM_DEPENDENCIES
#MBPM_TARGET_EXTPACKET_LIBRARIES
#MBPM_TARGET_COMPILE_OPTIONS
#MBPM_TARGET_LINK_OPTIONS
#Environment variables MBPM_PACKETS_INSTALL_DIRECTORY
set(MBPM_CXX_FLAGS ${CMAKE_CXX_FLAGS})
set(MBPM_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
set(MBPM_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
macro(MBPM_UpdateLibraries TargetToUpdate LinkType BuildType)
	set(MBPM_TARGET_EXTPACKET_LIBRARIES "")
	foreach(PACKET IN LISTS MBPM_DEPENDENCIES)
		set(NewLibraryName "")
		set(PacketName ${PACKET})
		set(LibraryName ${PACKET})
		if(${PACKET} MATCHES "#")
			string(REPLACE "#" ";" PACKET_PART_LIST ${PACKET})
			list(GET PACKET_PART_LIST 0 PacketName)
			list(GET PACKET_PART_LIST 1 LibraryName)
		endif()
		set(NewLibraryToAdd "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/${PacketName}/MBPM_Builds/")
		if(${LinkType} STREQUAL "STATIC")
			set(NewLibraryName "${LibraryName}_S")
		elseif(${LinkType} STREQUAL "DYNAMIC")
			set(NewLibraryName "${LibraryName}_D")
		endif()
		if(${BuildType} STREQUAL "DEBUG")
			set(NewLibraryName "${NewLibraryName}D")
		elseif(${BuildType} STREQUAL "RELEASE")
			set(NewLibraryName "${NewLibraryName}R")
		endif()
		if(UNIX)
			set(NewLibraryName "lib${NewLibraryName}.")
			if(${LinkType} STREQUAL "STATIC")
				set(NewLibraryName "${NewLibraryName}a")
			else()
				set(NewLibraryName "${NewLibraryName}so")
			endif()
		else()
			if(${LinkType} STREQUAL "STATIC")
				set(NewLibraryName "${NewLibraryName}.lib")
			else()
				set(NewLibraryName "${NewLibraryName}.dll")				
			endif()
		endif()
		set(NewLibraryToAdd "${NewLibraryToAdd}${NewLibraryName}")
		list(APPEND MBPM_TARGET_EXTPACKET_LIBRARIES ${NewLibraryToAdd})
	endforeach()
endmacro()

macro(MBPM_UpdateTargetVariables TargetToUpdate LinkType BuildType)
	MBPM_UpdateLibraries(${TargetToUpdate} ${LinkType} ${BuildType})
endmacro()

macro(MBPM_UpdateOutput TargetToUpdate)
	set_target_properties(${TargetToUpdate} 
	PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/"
	)
	#set_target_properties(${TargetToUpdate} PROPERTIES PREFIX "")
	#set_target_properties(${TargetToUpdate} PROPERTIES SUFFIX "")
	set_target_properties(${TargetToUpdate} PROPERTIES OUTPUT_NAME ${TargetToUpdate})
endmacro()

macro(MBPM_SetTargetCompileOptions TargetToUpdate LinkType BuildType)
	set(CMAKE_CXX_FLAGS " ")
	set(CMAKE_CXX_FLAGS_DEBUG " ")
	set(CMAKE_CXX_FLAGS_RELEASE " ")
	set(CMAKE_C_FLAGS " ")
	set(CMAKE_C_FLAGS_DEBUG " ")
	set(CMAKE_C_FLAGS_RELEASE " ")

	if(${BuildType} STREQUAL "DEBUG" AND ${LinkType} STREQUAL "STATIC")
		set(NEW_TARGET_COMPILE_OPTIONS "${MBPM_CXX_FLAGS} ${MBPM_CXX_FLAGS_DEBUG}")
	else()
		set(NEW_TARGET_COMPILE_OPTIONS "${MBPM_CXX_FLAGS} ${MBPM_CXX_FLAGS_RELEASE}")
	endif()
	string(REPLACE " " ";" NEW_TARGET_COMPILE_OPTIONS_LIST ${NEW_TARGET_COMPILE_OPTIONS})
	target_compile_options(${TargetToUpdate} PRIVATE ${NEW_TARGET_COMPILE_OPTIONS_LIST})
	message(${TargetToUpdate} " Compiled with follwing flags: ""${NEW_TARGET_COMPILE_OPTIONS_LIST}")
endmacro()

macro(MBPM_LinkTargetPacketLibraries Target)
	target_link_libraries(${Target} ${MBPM_TARGET_EXTPACKET_LIBRARIES})
	target_link_libraries(${Target} ${MBPM_SystemLibraries})
endmacro()

macro(MBPM_SetTargetIncludeDirectories Target)
	target_include_directories(${Target} PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/"
	"$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/"
	)
	target_include_directories(${Target} PUBLIC ${MBPM_DependanciesIncludeDirectories})
endmacro()

macro(MBPM_ApplyTargetConfiguration TargetToUpdate LinkType BuildType)
	MBPM_UpdateTargetVariables(${TargetToUpdate} ${LinkType} ${BuildType})
	MBPM_UpdateOutput(${TargetToUpdate})
	MBPM_SetTargetCompileOptions(${TargetToUpdate} ${LinkType} ${BuildType})
	MBPM_SetTargetIncludeDirectories(${TargetToUpdate})
endmacro()
##END MBPM_VARIABLES
set(PROJECT_SOURCES 

	"${CMAKE_CURRENT_SOURCE_DIR}//MBPacketManager.cpp"
	#"${CMAKE_CURRENT_SOURCE_DIR}/MBCLI/MBCLI.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}//MBPM_CLI.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}//MB_PacketProtocol.cpp"
	#"${CMAKE_CURRENT_SOURCE_DIR}/MBCLI/MBCLI.cpp"
)
set(PROJECT_HEADERS 
	"${CMAKE_CURRENT_SOURCE_DIR}//MBPacketManager.h"
	"${CMAKE_CURRENT_SOURCE_DIR}//MBPM_CLI.h"
	"${CMAKE_CURRENT_SOURCE_DIR}//MB_PacketProtocol.h"
)
set(COMMON_FILES ${PROJECT_SOURCES} ${PROJECT_HEADERS})
set(COMMON_DYNAMIC_LIBRARIES 
)
set(COMMON_STATIC_LIBRARIES
)
set(EXECUTABLE_FILES "${CMAKE_CURRENT_SOURCE_DIR}//MBPMCLIMain.cpp")
if(WIN32)
	list(APPEND "COMMON_STATIC_LIBRARIES" "Ws2_32")
else()
	list(APPEND "COMMON_STATIC_LIBRARIES" "stdc++fs")
endif()
set(COMMON_LIBRARIES ${COMMON_STATIC_LIBRARIES} ${COMMON_DYNAMIC_LIBRARIES})

add_library(MBPacketManager_SD STATIC ${COMMON_FILES})
MBPM_ApplyTargetConfiguration("MBPacketManager_SD" "STATIC" "DEBUG")
target_compile_features("MBPacketManager_SD" PRIVATE cxx_std_17)
target_link_libraries("MBPacketManager_SD" ${COMMON_LIBRARIES})


add_library(MBPacketManager_SR STATIC ${COMMON_FILES})
MBPM_ApplyTargetConfiguration("MBPacketManager_SR" "STATIC" "RELEASE")
target_compile_features("MBPacketManager_SR" PRIVATE cxx_std_17)
target_link_libraries("MBPacketManager_SR" ${COMMON_LIBRARIES})


add_executable("mbpm" ${COMMON_FILES} ${EXECUTABLE_FILES})
MBPM_ApplyTargetConfiguration("mbpm" "STATIC" "DEBUG")
target_compile_features("mbpm" PRIVATE cxx_std_17)
MBPM_LinkTargetPacketLibraries("mbpm")
target_link_libraries("mbpm" ${COMMON_LIBRARIES})
message(${CMAKE_CXX_FLAGS})

add_executable("mbpm_test" ${COMMON_FILES} "MBPM_TestMain.cpp")
MBPM_ApplyTargetConfiguration("mbpm_test" "STATIC" "DEBUG")
target_compile_features("mbpm_test" PRIVATE cxx_std_17)
MBPM_LinkTargetPacketLibraries("mbpm_test")
target_link_libraries("mbpm_test" ${COMMON_LIBRARIES})
#message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
#message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
#message("CMAKE_CXX_FLAGS_RELWITHDEBINFO is ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
#message("CMAKE_CXX_FLAGS_MINSIZEREL is ${CMAKE_CXX_FLAGS_MINSIZEREL}")
