cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)

project(MBPM CXX)

set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "" FORCE
)
macro(MBPM_UpdateLibraries TargetToUpdate LinkType BuildType)
	set(MBPM_TARGET_EXTPACKET_LIBRARIES "")
	foreach(PACKET IN MBPM_DEPENDENCIES)
		set(NewLibraryToAdd "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/${PACKET}/${PACKET}")
		if(${LinkType} STREQUAL "STATIC")
			set(NewLibraryToAdd "${NewLibraryToAdd}_S")
		elseif(${LinkType} STREQUAL "DYNAMIC")
			set(NewLibraryToAdd "${NewLibraryToAdd}_D")
		endif()
		if(${BuildType} STREQUAL "DEBUG")
			set(NewLibraryToAdd "${NewLibraryToAdd}D")
		elseif(${BuildType} STREQUAL "RELEASE")
			set(NewLibraryToAdd "${NewLibraryToAdd}R")
		endif()
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
	set_target_properties(${TargetToUpdate} PROPERTIES PREFIX "")
	set_target_properties(${TargetToUpdate} PROPERTIES SUFFIX "")
	set_target_properties(${TargetToUpdate} PROPERTIES OUTPUT_NAME ${TargetToUpdate})
endmacro()

macro(MBPM_SetTargetCompileOptions TargetToUpdate LinkType BuildType)

endmacro()

macro(MBPM_ApplyTargetConfiguration TargetToUpdate LinkType BuildType)
	MBPM_UpdateTargetVariables({TargetToUpdate} ${LinkType} ${BuildType})
	MBPM_UpdateOutput({TargetToUpdate})
	MBPM_SetTargetCompileOptions(${TargetToUpdate} ${LinkType} ${BuildType})
endmacro()
#Global variables
set(EXTLIBS_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../MBExternalDependencies/"
)

set (EXTPACKETS_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../"
)

set(MBPM_EXTPACKETS_HEADERS
    "${EXTPACKETS_DIR}/BasicChatCmake/MrBoboSockets/MrBoboSockets.h"
)
set(MBPM_EXTPACKETS_SOURCES
    "${EXTPACKETS_DIR}/BasicChatCmake/MrBoboSockets/MrBoboSockets.cpp"
)


set(PROJECT_NAME MBPM)


#globala linker settings


################################################################################
# Source groups
################################################################################

set(EXTLIBS_HEADER_FILES

)
set(EXTLIBS_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../BasicChatCmake/MrBoboSockets/MrBoboSockets.cpp;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../BasicChatCmake/MrBoboSockets/TLSHandler.cpp;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../BasicChatCmake/MBCrypto/MBCrypto.cpp;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../BasicChatCmake/MBErrorHandling.cpp;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../BasicChatCmake/MBParsing/MBParsing.cpp;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../BasicChatCmake/MBUnicode/MBUnicode.cpp;"
)

set(Header_Files
    "${CMAKE_CURRENT_SOURCE_DIR}/MBPacketManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/MB_PacketProtocol.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "${CMAKE_CURRENT_SOURCE_DIR}/MBPacketManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MB_PacketProtocol.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MBPMCLIMain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MBPM_CLI.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MBCLI/MBCLI.cpp"
)
message(${Source_Files})
source_group("Source Files" FILES ${Source_Files})



set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
    ${EXTLIBS_SOURCE_FILES}
    ${MBPM_EXTPACKETS_SOURCES}
)



################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

add_library("TestMBPMLibrary" STATIC ${Header_Files} ${Source_Files})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_features("TestMBPMLibrary" PRIVATE cxx_std_17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

################################################################################
# Include directories
################################################################################
#testar att ha olika beroende på om det är windows eller inte
target_include_directories(${PROJECT_NAME} PUBLIC
    #"${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../Program Files %28x86%29/Windows Kits/10/Include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MBExternalDependencies/;"
    "${CMAKE_CURRENT_SOURCE_DIR}/;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../BasicChatCmake/;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MrBoboLibrary/;"
)

target_include_directories("TestMBPMLibrary" PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../MBExternalDependencies/;"
    "${CMAKE_CURRENT_SOURCE_DIR}/;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../BasicChatCmake/;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MrBoboLibrary/;"
)
if(UNIX)
    set(ADDITIONAL_LIBRARY_DEPENDENCIES 
        "stdc++fs"
    )
else()
    set(ADDITIONAL_LIBRARY_DEPENDENCIES 
        "Ws2_32"
        #"ws2_32.lib"
        "Secur32.lib"
        "Bcrypt.lib"
        "Mfplat.lib"
        "Mfuuid.lib"
        "Strmiids.lib"
    )
endif()

#mina link options
if(MSVC)
    message("Build type: ${CMAKE_BUILD_TYPE}")
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        message("Bygger Debug Build")    
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
        target_compile_options(${PROJECT_NAME} PRIVATE /MTd)
        target_link_options(${PROJECT_NAME} PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
        list(APPEND ADDITIONAL_LIBRARY_DEPENDENCIES "${EXTLIBS_DIR}/cryptopp/cryptlib_D.lib")
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
        target_compile_options(${PROJECT_NAME} PRIVATE /MT)
        target_link_options(${PROJECT_NAME} PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
        list(APPEND ADDITIONAL_LIBRARY_DEPENDENCIES "${EXTLIBS_DIR}/cryptopp/cryptlib.lib")
    endif()
    message("Additional librarys: ${ADDITIONAL_LIBRARY_DEPENDENCIES}")
endif()


target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")
target_link_libraries("TestMBPMLibrary" PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")
