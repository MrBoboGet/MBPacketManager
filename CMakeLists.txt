project(MBPacketManager)
set(TOTAL_MBPM_PACKET_LIBRARIES
    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/MrBoboSockets/MBPM_Builds/Debug/MrBoboSockets.lib
    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/MBCLI/MBPM_Builds/Debug/MBCLI.lib
    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/MBMime/MBPM_Builds/Debug/MBMime.lib
    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/MBCrypto/MBPM_Builds/Debug/MBCrypto.lib
    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/MBBenchmark/MBPM_Builds/Debug/MBBenchmark.lib
    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/MrBigInt/MBPM_Builds/Debug/MrBigInt.lib
    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/MBUnicode/MBPM_Builds/Debug/MBUnicode.lib
    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/MBSystem/MBPM_Builds/Debug/MBSystem.lib
    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/MBParsing/MBPM_Builds/Debug/MBParsing.lib
    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/cryptopp/MBPM_Builds/Debug/cryptopp.lib
    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/MBUtility/MBPM_Builds/Debug/MBUtility.lib
)
set(SYSTEM_LIBRARIES
Ws2_32.lib
Secur32.lib
Bcrypt.lib
Mfplat.lib
opengl32
Mfuuid.lib
	Strmiids.lib
)
set(SOURCES_Lib
    MBPacketManager.cpp
    MBPM_CLI.cpp
    MB_PacketProtocol.cpp
    MBBuild/MBBuild.cpp
)
set(SOURCES_mbbuild
    MBPacketManager.cpp
    MBPM_CLI.cpp
    MB_PacketProtocol.cpp
    MBBuild/MBBuild.cpp
    MBBuild/MBBuildMain.cpp
)
set(SOURCES_mbpm
    MBPacketManager.cpp
    MBPM_CLI.cpp
    MB_PacketProtocol.cpp
    MBBuild/MBBuild.cpp
    MBPMCLIMain.cpp
)
add_library(Lib STATIC ${SOURCES_Lib})
target_compile_features(Lib PRIVATE cxx_std_17)
target_include_directories(Lib PRIVATE $ENV{MBPM_PACKETS_INSTALL_DIRECTORY})
target_link_libraries(Lib PRIVATE ${TOTAL_MBPM_PACKET_LIBRARIES} ${SYSTEM_LIBRARIES})
set_target_properties(Lib PROPERTIES 
ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>
RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>
LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>
)

add_executable(mbbuild ${SOURCES_mbbuild})
target_compile_features(mbbuild PRIVATE cxx_std_17)
target_include_directories(mbbuild PRIVATE $ENV{MBPM_PACKETS_INSTALL_DIRECTORY})
target_link_libraries(mbbuild PRIVATE ${TOTAL_MBPM_PACKET_LIBRARIES} ${SYSTEM_LIBRARIES})
set_target_properties(mbbuild PROPERTIES 
ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>
RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>
LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>
)

add_executable(mbpm ${SOURCES_mbpm})
target_compile_features(mbpm PRIVATE cxx_std_17)
target_include_directories(mbpm PRIVATE $ENV{MBPM_PACKETS_INSTALL_DIRECTORY})
target_link_libraries(mbpm PRIVATE ${TOTAL_MBPM_PACKET_LIBRARIES} ${SYSTEM_LIBRARIES})
set_target_properties(mbpm PROPERTIES 
ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>
RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>
LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>
)

