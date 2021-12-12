cmake_minimum_required(VERSION 3.15.0)

project(vls VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/install)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###################### GUI ######################

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Multimedia REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Multimedia REQUIRED)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resources/winresources.rc")

set(PROJECT_SOURCES
    main.cpp
    src/application.ui
    src/vls_common.cpp
    src/application.cpp
    include/application.h
    include/vls_common.h
)

# add resources
if(QT_VERSION_MAJOR EQUAL 6)
    qt_add_resources(PROJECT_SOURCES resources/resources.qrc)
else()
    qt5_add_resources(PROJECT_SOURCES resources/resources.qrc)
endif()


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(vls WIN32
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES} ${APP_ICON_RESOURCE_WINDOWS}
    )
else()
    add_executable(vls WIN32 ${PROJECT_SOURCES} ${APP_ICON_RESOURCE_WINDOWS}
    )
endif()

target_link_libraries(vls PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia)
target_include_directories(vls PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")


if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(vls)
endif()


install(TARGETS vls RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})


######### build server cli ###################

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${Boost_LIBRARY_DIR})

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.77.0 COMPONENTS date_time program_options REQUIRED)


if(Boost_FOUND)
    file(GLOB VOSK_SRCS vosk_api/*.cpp vosk_api/*.h )
    add_executable(asr_server src/asr_server.cpp ${VOSK_SRCS})

    target_include_directories(
        asr_server
        PRIVATE
        vosk_api
        ${Boost_INCLUDE_DIR}
    )

    target_link_libraries(
        asr_server
        ${Boost_LIBRARIES}
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libvosk.lib
    )
    install(TARGETS asr_server RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
    file(GLOB VOSK_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.dll")
    install(FILES ${VOSK_DLLS} DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()


