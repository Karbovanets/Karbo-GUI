cmake_minimum_required(VERSION 3.8)

enable_language(C)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ARCH_ID "${CMAKE_HOST_SYSTEM_PROCESSOR}")
if (CMAKE_OSX_ARCHITECTURES)
  set(ARCH_ID "${CMAKE_OSX_ARCHITECTURES}")
endif()
string(TOLOWER "${ARCH_ID}" ARCH_ID)
set(ARM_ID "${ARCH_ID}")
string(SUBSTRING "${ARM_ID}" 0 3 ARM_TEST)
if (ARM_TEST STREQUAL "arm")
  set(ARM 1)
endif()
if (ARM_ID STREQUAL "aarch64" OR ARM_ID STREQUAL "arm64" OR ARM_ID STREQUAL "armv8-a")
  set(ARM 1)
endif()

include(CryptoNoteWallet.cmake)
include(QREncode.cmake)

project(${WALLET_NAME})

execute_process(COMMAND git log -1 --pretty=format:%h
WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
OUTPUT_VARIABLE GIT_REVISION)

set(VERSION "")
include(cn_version.cmake)
configure_file("src/CryptoNoteWalletConfig.h.in" "CryptoNoteWalletConfig.h")
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/src/Info.plist.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/src/karbo-gui.desktop.in"
    "${CMAKE_CURRENT_BINARY_DIR}/karbo-gui.desktop"
    @ONLY
)

add_definitions (-DGIT_REVISION="${GIT_REVISION}")
add_definitions (-DWALLET_VERSION="${WALLET_VERSION}")

set(CRYPTONOTE_LIB cryptonote)

include_directories(${CMAKE_BINARY_DIR}
  src
  include
  cryptonote/external
  cryptonote/include
  cryptonote/src
  cryptonote/external/liblmdb
  libqrencode
)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network Svg)
if (UNIX AND NOT APPLE)
  find_package(Qt6 REQUIRED COMPONENTS DBus)
endif()

set(Boost_NO_BOOST_CMAKE ON)
set(Boost_USE_STATIC_LIBS ON)
if(WIN32)
  add_definitions(-D WIN32_LEAN_AND_MEAN -DNOMINMAX)
  set(Boost_USE_STATIC_RUNTIME OFF)
else(WIN32)
  set(Boost_USE_STATIC_RUNTIME ON)
endif(WIN32)

find_package(Boost REQUIRED COMPONENTS date_time filesystem program_options regex serialization system thread chrono coroutine context)
if ((${Boost_MAJOR_VERSION} EQUAL 1) AND (${Boost_MINOR_VERSION} EQUAL 54))
  message(SEND_ERROR "Boost version 1.57 is unsupported, more details are available here http://goo.gl/RrCFmA")
endif ()

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# We have to look for Homebrew OpenSSL a bit differently
if (APPLE)
    find_program(HOMEBREW_EXECUTABLE brew)
    execute_process(COMMAND ${HOMEBREW_EXECUTABLE} --prefix openssl
            OUTPUT_VARIABLE HOMEBREW_OPENSSL
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (DEFINED HOMEBREW_OPENSSL)
        if (NOT DEFINED OPENSSL_ROOT_DIR)
            message(STATUS "Setting OpenSSL root to ${HOMEBREW_OPENSSL}")
            set(OPENSSL_ROOT_DIR "${HOMEBREW_OPENSSL}")
        endif ()
    endif ()
endif ()

find_package(OpenSSL)

if (NOT OPENSSL_FOUND)
    find_package(PkgConfig)

    if (PKG_CONFIG_FOUND)
        pkg_search_module(OPENSSL openssl)
    endif()
endif()

if (OPENSSL_FOUND)
    if (NOT MSVC)
        set(OPENSSL_LIBRARIES "${OPENSSL_LIBRARIES};dl")
    endif ()

    include_directories(${OPENSSL_INCLUDE_DIR})
    message(STATUS "OpenSSL Found: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL Libraries: ${OPENSSL_LIBRARIES}")
else ()
    message(STATUS "OpenSSL Found: No... Skipping...")
endif ()

set(CMAKE_AUTOMOC ON)

set(CRYPTONOTE_SOURCES
  cryptonote/external/miniupnpc/connecthostport.c
  cryptonote/external/miniupnpc/igd_desc_parse.c
  cryptonote/external/miniupnpc/minisoap.c
  cryptonote/external/miniupnpc/minissdpc.c
  cryptonote/external/miniupnpc/miniupnpc.c
  cryptonote/external/miniupnpc/miniwget.c
  cryptonote/external/miniupnpc/minixml.c
  cryptonote/external/miniupnpc/portlistingparse.c
  cryptonote/external/miniupnpc/receivedata.c
  cryptonote/external/miniupnpc/upnpcommands.c
  cryptonote/external/miniupnpc/upnpdev.c
  cryptonote/external/miniupnpc/upnperrors.c
  cryptonote/external/miniupnpc/upnpreplyparse.c
  cryptonote/src/BlockchainExplorer/BlockchainExplorer.cpp
  cryptonote/src/BlockchainExplorer/BlockchainExplorerDataBuilder.cpp
  cryptonote/src/BlockchainExplorer/BlockchainExplorerErrors.cpp
  cryptonote/src/Checkpoints/Checkpoints.cpp
  cryptonote/src/Common/Base58.cpp
  cryptonote/src/Common/CommandLine.cpp
  cryptonote/src/Common/ConsoleTools.cpp
  cryptonote/src/Common/DnsTools.cpp
  cryptonote/src/Common/FormatTools.cpp
  cryptonote/src/Common/JsonValue.cpp
  cryptonote/src/Common/MemoryInputStream.cpp
  cryptonote/src/Common/PasswordContainer.cpp
  cryptonote/src/Common/PathTools.cpp
  cryptonote/src/Common/StdInputStream.cpp
  cryptonote/src/Common/StdOutputStream.cpp
  cryptonote/src/Common/StreamTools.cpp
  cryptonote/src/Common/StringOutputStream.cpp
  cryptonote/src/Common/StringTools.cpp
  cryptonote/src/Common/StringView.cpp
  cryptonote/src/Common/Util.cpp
  cryptonote/src/Common/VectorOutputStream.cpp
  cryptonote/src/Common/ScopeExit.cpp
  cryptonote/src/crypto/blake256.c
  cryptonote/src/crypto/chacha8.cpp
  cryptonote/src/crypto/crypto-ops-data.c
  cryptonote/src/crypto/crypto-ops.c
  cryptonote/src/crypto/crypto-util.c
  cryptonote/src/crypto/crypto.cpp
  cryptonote/src/crypto/groestl.c
  cryptonote/src/crypto/hash-extra-blake.c
  cryptonote/src/crypto/hash-extra-groestl.c
  cryptonote/src/crypto/hash-extra-jh.c
  cryptonote/src/crypto/hash-extra-skein.c
  cryptonote/src/crypto/hash.c
  cryptonote/src/crypto/jh.c
  cryptonote/src/crypto/keccak.c
  cryptonote/src/crypto/oaes_lib.c
  cryptonote/src/crypto/skein.c
  cryptonote/src/crypto/slow-hash.c
  cryptonote/src/crypto/slow-hash.cpp
  cryptonote/src/crypto/tree-hash.c
  cryptonote/src/crypto/yespower.c
  cryptonote/src/CryptoNoteCore/Account.cpp
  cryptonote/src/CryptoNoteCore/Blockchain.cpp
  cryptonote/src/CryptoNoteCore/BlockchainIndices.cpp
  cryptonote/src/CryptoNoteCore/BlockchainMessages.cpp
  cryptonote/src/CryptoNoteCore/BlockIndex.cpp
  cryptonote/src/CryptoNoteCore/Core.cpp
  cryptonote/src/CryptoNoteCore/CoreConfig.cpp
  cryptonote/src/CryptoNoteCore/CryptoNoteBasic.cpp
  cryptonote/src/CryptoNoteCore/CryptoNoteBasicImpl.cpp
  cryptonote/src/CryptoNoteCore/CryptoNoteFormatUtils.cpp
  cryptonote/src/CryptoNoteCore/CryptoNoteSerialization.cpp
  cryptonote/src/CryptoNoteCore/CryptoNoteTools.cpp
  cryptonote/src/CryptoNoteCore/Currency.cpp
  cryptonote/src/CryptoNoteCore/Difficulty.cpp
  cryptonote/src/CryptoNoteCore/IBlock.cpp
  cryptonote/src/CryptoNoteCore/LMDBBlockchainDB.cpp
  cryptonote/src/CryptoNoteCore/Miner.cpp
  cryptonote/src/CryptoNoteCore/MinerConfig.cpp
  cryptonote/src/CryptoNoteCore/SwappedVector.cpp
  cryptonote/src/CryptoNoteCore/Transaction.cpp
  cryptonote/src/CryptoNoteCore/TransactionExtra.cpp
  cryptonote/src/CryptoNoteCore/TransactionPool.cpp
  cryptonote/src/CryptoNoteCore/TransactionPrefixImpl.cpp
  cryptonote/src/CryptoNoteCore/TransactionUtils.cpp
  cryptonote/src/CryptoNoteCore/UpgradeDetector.cpp
  cryptonote/src/CryptoNoteProtocol/CryptoNoteProtocolHandler.cpp
  cryptonote/src/HTTP/HttpClient.cpp
  cryptonote/src/HTTP/HttpParser.cpp
  cryptonote/src/HTTP/HttpParserErrorCodes.cpp
  cryptonote/src/HTTP/HttpRequest.cpp
  cryptonote/src/HTTP/HttpResponse.cpp
  cryptonote/src/HTTP/HttpServer.cpp
  cryptonote/src/InProcessNode/InProcessNode.cpp
  cryptonote/src/InProcessNode/InProcessNodeErrors.cpp
  cryptonote/src/Logging/CommonLogger.cpp
  cryptonote/src/Logging/ConsoleLogger.cpp
  cryptonote/src/Logging/FileLogger.cpp
  cryptonote/src/Logging/ILogger.cpp
  cryptonote/src/Logging/LoggerGroup.cpp
  cryptonote/src/Logging/LoggerManager.cpp
  cryptonote/src/Logging/LoggerMessage.cpp
  cryptonote/src/Logging/LoggerRef.cpp
  cryptonote/src/Logging/StreamLogger.cpp
  cryptonote/src/NodeRpcProxy/NodeErrors.cpp
  cryptonote/src/NodeRpcProxy/NodeRpcProxy.cpp
  cryptonote/src/P2p/LevinProtocol.cpp
  cryptonote/src/P2p/NetNode.cpp
  cryptonote/src/P2p/NetNodeConfig.cpp
  cryptonote/src/P2p/P2pConnectionContext.cpp
  cryptonote/src/P2p/PeerListManager.cpp
  cryptonote/src/Rpc/JsonRpc.cpp
  cryptonote/src/Rpc/RpcServer.cpp
  cryptonote/src/Rpc/RpcServerConfig.cpp
  cryptonote/src/Serialization/BinaryInputStreamSerializer.cpp
  cryptonote/src/Serialization/BinaryOutputStreamSerializer.cpp
  cryptonote/src/Serialization/BlockchainExplorerDataSerialization.cpp
  cryptonote/src/Serialization/JsonInputValueSerializer.cpp
  cryptonote/src/Serialization/JsonOutputStreamSerializer.cpp
  cryptonote/src/Serialization/KVBinaryInputStreamSerializer.cpp
  cryptonote/src/Serialization/KVBinaryOutputStreamSerializer.cpp
  cryptonote/src/Serialization/SerializationOverloads.cpp
  cryptonote/src/System/ContextGroup.cpp
  cryptonote/src/System/Dispatcher.cpp
  cryptonote/src/System/Event.cpp
  cryptonote/src/System/EventLock.cpp
  cryptonote/src/System/InterruptedException.cpp
  cryptonote/src/System/Ipv4Address.cpp
  cryptonote/src/System/SocketStream.cpp
  cryptonote/src/System/SslTcpStreambuf.cpp
  cryptonote/src/System/TcpConnection.cpp
  cryptonote/src/System/TcpConnector.cpp
  cryptonote/src/System/TcpListener.cpp
  cryptonote/src/System/TcpStream.cpp
  cryptonote/src/Transfers/BlockchainSynchronizer.cpp
  cryptonote/src/Transfers/SynchronizationState.cpp
  cryptonote/src/Transfers/TransfersConsumer.cpp
  cryptonote/src/Transfers/TransfersContainer.cpp
  cryptonote/src/Transfers/TransfersSubscription.cpp
  cryptonote/src/Transfers/TransfersSynchronizer.cpp
  cryptonote/src/Wallet/LegacyKeysImporter.cpp
  cryptonote/src/Wallet/TransactionBuilder.cpp
  cryptonote/src/Wallet/WalletAsyncContextCounter.cpp
  cryptonote/src/Wallet/WalletErrors.cpp
  cryptonote/src/Wallet/WalletGreen.cpp
  cryptonote/src/Wallet/WalletSerializationV1.cpp
  cryptonote/src/Wallet/WalletSerializationV2.cpp
  cryptonote/src/Wallet/WalletUtils.cpp
  cryptonote/src/WalletLegacy/KeysStorage.cpp
  cryptonote/src/WalletLegacy/WalletLegacySerialization.cpp
  cryptonote/src/WalletLegacy/WalletLegacySerializer.cpp
  cryptonote/src/WalletLegacy/WalletUnconfirmedTransactions.cpp
  cryptonote/src/WalletLegacy/WalletUserTransactionsCache.cpp
)

file(GLOB_RECURSE Mnemonics cryptonote/src/Mnemonics/*)

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE FORMS src/*.ui)

list(FILTER SOURCES EXCLUDE REGEX ".*/QJsonRpc/.*")
list(FILTER HEADERS EXCLUDE REGEX ".*/QJsonRpc/.*")

if(MSVC)
  include_directories(Platform/Windows)
  set(SOURCES ${SOURCES} ${CMAKE_SOURCE_DIR}/Platform/Windows/ApplicationEventHandler.cpp)
elseif(APPLE)
  include_directories(Platform/OSX)
  include_directories(Platform/Posix)
  set(SOURCES ${SOURCES} ${CMAKE_SOURCE_DIR}/Platform/OSX/ApplicationEventHandler.h ${CMAKE_SOURCE_DIR}/Platform/OSX/ApplicationEventHandler.mm ${CMAKE_SOURCE_DIR}/Platform/OSX/INotificationMonitorObserver.h ${CMAKE_SOURCE_DIR}/Platform/OSX/NotificationMonitor.h ${CMAKE_SOURCE_DIR}/Platform/OSX/NotificationMonitor.mm ${CMAKE_SOURCE_DIR}/Platform/OSX/WalletApplication.mm )
  set(SOURCES ${SOURCES} ${CMAKE_SOURCE_DIR}/Platform/Posix/System/MemoryMappedFile.cpp)
else()
  include_directories(Platform/Linux)
  include_directories(Platform/Posix)
  set(SOURCES ${SOURCES} ${CMAKE_SOURCE_DIR}/Platform/Linux/ApplicationEventHandler.cpp)
  set(SOURCES ${SOURCES} ${CMAKE_SOURCE_DIR}/Platform/Posix/System/MemoryMappedFile.cpp)
endif()


set(QRC src/resources.qrc)

qt_wrap_ui(UIS ${FORMS})
qt_add_resources(RCC ${QRC})

if (WIN32)
  if (NOT MSVC)
    message(FATAL_ERROR "Only MSVC is supported on this platform")
  endif ()

  set(STATIC ${MSVC} CACHE BOOL "Link libraries statically")

  add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_WIN32_WINNT=0x0601 /DSTATICLIB)
  include_directories(cryptonote/src/Platform/msc)

  set( CMAKE_CXX_FLAGS "-bigobj -EHsc")
  set(PLATFORM_DIR Windows)
  set(BUILD_PLATFORM WIN32)
  set(BUILD_RESOURCES src/karbowanecwallet.rc)

elseif (UNIX)

  set(CRYPTONOTE_SOURCES ${CRYPTONOTE_SOURCES} cryptonote/external/miniupnpc/minissdpc.c)

  if (APPLE)
    enable_language(ASM)
    file(GLOB_RECURSE OBJC_SOURCES src/*.mm)
    set(SOURCES ${SOURCES} ${OBJC_SOURCES})
    set(PLATFORM_DIR OSX)
    set(MACOSX_BUNDLE_INFO_STRING "Cryptonote GUI wallet")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_VERSION}.${VERSION_MINOR}.${VERSION_PATCH}")
    set(MACOSX_BUNDLE_BUNDLE_NAME CryptonoteWallet)
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_VERSION}.${VERSION_MINOR}.${VERSION_PATCH}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "$${VERSION_VERSION}.${VERSION_MINOR}.${VERSION_PATCH}")

    find_package(Qt6 REQUIRED COMPONENTS PrintSupport)

    include_directories(/usr/include/malloc)

    if (ARM)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -stdlib=libc++")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -D_DARWIN_C_SOURCE")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes -std=c++17 -stdlib=libc++")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -maes -D_DARWIN_C_SOURCE")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework OpenGL -framework CoreFoundation -framework Carbon -framework IOKit -L/usr/lib")

    set(MACOSX_BUNDLE_ICON_FILE Karbovanets.icns)
    set(APPLICATION_ICON src/images/Karbovanets.icns)
    set_source_files_properties(${APPLICATION_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    set(BUILD_PLATFORM MACOSX_BUNDLE)
    set(BUILD_RESOURCES ${APPLICATION_ICON})

    GET_TARGET_PROPERTY(QT_LIB_DIR "${Qt6Widgets_LIBRARIES}" LOCATION)
    GET_FILENAME_COMPONENT(QT_LIB_DIR "${QT_LIB_DIR}" PATH)

  else(APPLE)

    set(PLATFORM_DIR Linux)
    if (ARM)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes -std=c++17")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes -std=c11")
    endif()
  endif (APPLE)
endif ()

include_directories(cryptonote/src/Platform/${PLATFORM_DIR})
file(GLOB PLATFORM_SOURCES cryptonote/src/Platform/${PLATFORM_DIR}/System/*)
set(CRYPTONOTE_SOURCES ${CRYPTONOTE_SOURCES} ${PLATFORM_SOURCES} ${CRYPTONOTE_PLATFORM})

add_definitions(-DUPNP_STATIC -DMINIUPNP_STATICLIB -DUPNPC_BUILD_STATIC)

# LMDB library target (matches karbowanec standalone project build)
if (MSVC)
  set(_lmdb_src_dir "${CMAKE_SOURCE_DIR}/cryptonote/external/liblmdb")
  set(_lmdb_build_dir "${CMAKE_BINARY_DIR}/lmdb_external")
  if(CMAKE_BUILD_TYPE)
    set(_lmdb_config "${CMAKE_BUILD_TYPE}")
  else()
    set(_lmdb_config "Release")
  endif()

  message(STATUS "Configuring lmdb externally (${_lmdb_config}) in ${_lmdb_build_dir}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
      -G "Visual Studio 17 2022"
      -A x64
      -S "${_lmdb_src_dir}"
      -B "${_lmdb_build_dir}"
    RESULT_VARIABLE _lmdb_configure_result
    OUTPUT_QUIET
    ERROR_VARIABLE _lmdb_configure_error
  )
  if(NOT _lmdb_configure_result EQUAL 0)
    message(FATAL_ERROR "lmdb configure failed:\n${_lmdb_configure_error}")
  endif()

  message(STATUS "Building lmdb externally...")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${_lmdb_build_dir}" --config "${_lmdb_config}"
    RESULT_VARIABLE _lmdb_build_result
    OUTPUT_QUIET
    ERROR_VARIABLE _lmdb_build_error
  )
  if(NOT _lmdb_build_result EQUAL 0)
    message(FATAL_ERROR "lmdb build failed:\n${_lmdb_build_error}")
  endif()

  add_library(lmdb STATIC IMPORTED GLOBAL)
  set_target_properties(lmdb PROPERTIES
    IMPORTED_LOCATION "${_lmdb_build_dir}/${_lmdb_config}/lmdb.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${_lmdb_src_dir}"
  )
else()
  set(LMDB_SOURCES
    cryptonote/external/liblmdb/mdb.c
    cryptonote/external/liblmdb/midl.c)
  add_library(lmdb STATIC ${LMDB_SOURCES})
  target_include_directories(lmdb PUBLIC cryptonote/external/liblmdb)
  set_property(TARGET lmdb APPEND PROPERTY COMPILE_FLAGS "-fPIC")
endif()

add_library(${CRYPTONOTE_LIB} STATIC ${CRYPTONOTE_SOURCES})
add_library(Mnemonics ${Mnemonics})
set_target_properties(${CRYPTONOTE_LIB} PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)
if (APPLE OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_link_libraries(${CRYPTONOTE_LIB} ${Boost_LIBRARIES} lmdb -lresolv)
else ()
  target_link_libraries(${CRYPTONOTE_LIB} ${Boost_LIBRARIES} lmdb)
endif ()
if (UNIX AND NOT APPLE)
  target_link_libraries(${CRYPTONOTE_LIB} Qt6::DBus)
endif()
set_target_properties(Mnemonics PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)
target_link_libraries(Mnemonics ${Boost_LIBRARIES})

add_executable(${PROJECT_NAME} ${BUILD_PLATFORM} ${BUILD_RESOURCES} ${SOURCES} ${HEADERS} ${UIS} ${RCC})

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${CRYPTONOTE_LIB} ${QRENCODE_LIB} Mnemonics)
target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network Qt6::Svg)
if (UNIX AND NOT APPLE)
  target_link_libraries(${PROJECT_NAME} Qt6::DBus)
endif()

if (OPENSSL_FOUND)
  target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
  if (MSVC)
    target_link_libraries(${PROJECT_NAME} Rpcrt4 ws2_32 advapi32 crypt32 gdi32 user32)
  endif ()
endif ()

if (APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
  target_link_libraries(${PROJECT_NAME} Qt6::PrintSupport)
elseif (UNIX)
  target_link_libraries(${PROJECT_NAME} -lpthread)
elseif (WIN32)
  target_link_libraries(${PROJECT_NAME} Imm32 Iphlpapi Winmm UxTheme d3d9 dxguid Rpcrt4 Wtsapi32 WSock32 ws2_32 Dwmapi)
endif (APPLE)

# Installation

set(CPACK_PACKAGE_NAME ${WALLET_NAME})
set(CPACK_PACKAGE_VERSION ${WALLET_VERSION})
set(CPACK_PACKAGE_VENDOR "Karbowanec-project")
set(CPACK_PACKAGE_CONTACT "https://karbo.org")
set(CPACK_STRIP_FILES ON)
if (APPLE)
    set(CPACK_GENERATOR DragNDrop)
    set(CPACK_BUNDLE_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)
elseif (UNIX)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME} DESTINATION bin
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    install(FILES ${CMAKE_BINARY_DIR}/karbo-gui.desktop DESTINATION share/applications)
    install(FILES src/images/Karbovanets.png DESTINATION share/icons/hicolor/256x256/apps RENAME karbowanec.png)
endif (APPLE)

include(CPack)
