project(MBPacketManager)
set(PROJECT_NAME "MBPacketManager")
##BEGIN MBPM_VARIABLES
set(MBPM_DEPENDENCIES 
	"MrBoboSockets"
	"MBMime"
	"MBCrypto"
	"MBCLI"
	"MrBigInt"
	"MBUnicode"
	"MBSystem"
	"MBParsing"
	"cryptopp"
	"MBUtility"
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
#MBPM_Functions
set(MB_BUILD_TYPE "Debug") #Abstraction around CMAKE_BUILD_TYPE potentially being empty
set(MBPM_EXTPACKETS_BUILD_DIRECTORIES )
set(MBPM_EXTPACKETS_LIBRARIES)
macro(MBPM_UpdateLibraries)
	set(MBPM_EXTPACKETS_LIBRARIES "")
	set(MBPM_EXTPACKETS_BUILD_DIRECTORIES "")
	foreach(PACKET IN LISTS MBPM_DEPENDENCIES)
		set(NewLibraryName "")
		set(PacketName ${PACKET})
		set(LibraryName ${PACKET})
		if(${PACKET} MATCHES "#")
			string(REPLACE "#" ";" PACKET_PART_LIST ${PACKET})
			list(GET PACKET_PART_LIST 0 PacketName)
			list(GET PACKET_PART_LIST 1 LibraryName)
		endif()
		set(NewLibraryDirectory "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/${PacketName}/MBPM_Builds/${MB_BUILD_TYPE}/")
		list(APPEND MBPM_EXTPACKETS_LIBRARIES ${LibraryName})
		list(APPEND MBPM_EXTPACKETS_BUILD_DIRECTORIES ${NewLibraryDirectory})
	endforeach()
endmacro()
set(MBPM_VariablesUpdated "FALSE")
macro(MBPM_UpdateVariables)
	if(${MBPM_VariablesUpdated} STREQUAL "FALSE")
		if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "" AND DEFINED CMAKE_BUILD_TYPE)
			set(MB_BUILD_TYPE ${CMAKE_BUILD_TYPE})
		endif()
		MBPM_UpdateLibraries()
	endif()	
	set(MBPM_VariablesUpdated "TRUE")
endmacro()

macro(MBPM_UpdateOutput TargetToUpdate)
	set(BuildDirectorySuffix ${MB_BUILD_TYPE})
	set_target_properties(${TargetToUpdate} 
	PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${BuildDirectorySuffix}/>
	LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${BuildDirectorySuffix}/>
	RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${BuildDirectorySuffix}/>
	)
endmacro()

macro(MBPM_LinkTargetPacketLibraries Target)
	target_link_directories(${Target} PRIVATE ${MBPM_EXTPACKETS_BUILD_DIRECTORIES})
	target_link_libraries(${Target} ${MBPM_EXTPACKETS_LIBRARIES})
	target_link_libraries(${Target} ${MBPM_SystemLibraries})
endmacro()

macro(MBPM_SetTargetIncludeDirectories Target)
	target_include_directories(${Target} PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/"
	"$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/"
	)
	target_include_directories(${Target} PUBLIC ${MBPM_DependanciesIncludeDirectories})
endmacro()

macro(MBPM_ApplyConfiguration TargetToUpdate)
	MBPM_UpdateVariables()
	MBPM_UpdateOutput(${TargetToUpdate})
	MBPM_SetTargetIncludeDirectories(${TargetToUpdate})
	MBPM_LinkTargetPacketLibraries(${TargetToUpdate})
endmacro()
##END MBPM_VARIABLES
set(PROJECT_SOURCES 

	"${CMAKE_CURRENT_SOURCE_DIR}/MBPacketManager.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/MBPM_CLI.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/MB_PacketProtocol.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MBBuild/MBBuild.cpp"
)
set(PROJECT_HEADERS 
	"${CMAKE_CURRENT_SOURCE_DIR}/MBPacketManager.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/MBPM_CLI.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/MB_PacketProtocol.h"

)
set(COMMON_FILES ${PROJECT_SOURCES} ${PROJECT_HEADERS})
set(COMMON_DYNAMIC_LIBRARIES 
)
set(COMMON_STATIC_LIBRARIES
)
set(EXECUTABLE_FILES "${CMAKE_CURRENT_SOURCE_DIR}//MBPMCLIMain.cpp")
set(MBBUILD_EXECUTABLE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/MBBuild/MBBuildMain.cpp")
if(WIN32)
	list(APPEND "COMMON_STATIC_LIBRARIES" "Ws2_32")
else()
	list(APPEND "COMMON_STATIC_LIBRARIES" "stdc++fs")
endif()
set(COMMON_LIBRARIES ${COMMON_STATIC_LIBRARIES} ${COMMON_DYNAMIC_LIBRARIES})

add_library(MBPacketManager STATIC ${COMMON_FILES})
MBPM_ApplyConfiguration("MBPacketManager")
target_compile_features("MBPacketManager" PRIVATE cxx_std_17)

add_executable("mbpm" ${COMMON_FILES}  ${EXECUTABLE_FILES})
MBPM_ApplyConfiguration("mbpm")
target_compile_features("mbpm" PRIVATE cxx_std_17)

add_executable("mbbuild" ${COMMON_FILES} ${MBBUILD_EXECUTABLE_FILES})
MBPM_ApplyConfiguration("mbbuild")
target_compile_features("mbbuild" PRIVATE cxx_std_17)

add_executable("mbpm_test" ${COMMON_FILES} "MBPM_TestMain.cpp")
MBPM_ApplyConfiguration("mbpm_test")
target_compile_features("mbpm_test" PRIVATE cxx_std_17)
