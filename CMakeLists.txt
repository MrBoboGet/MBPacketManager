cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)

project(MBPM CXX)

set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "" FORCE
)

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
    "${EXTPACKETS_DIR}/BasicChatCmake/MrBoboSockets/MrBoboSockets.h"
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


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)


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
