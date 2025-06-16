cmake_minimum_required(VERSION 3.16)

project(Cinephile VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt Components Needed:
# Widgets: Standard UI elements
# Svg: For SVG icon support
# Network: For QNetworkAccessManager (Firebase REST, Google token exchange)
# Core: For QUrl, QUrlQuery, QUuid, QCryptographicHash, QTcpServer, QTcpSocket, QDesktopServices etc.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Svg Network Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg Network Core)

set(PROJECT_SOURCES
        main.cpp
        MainWindow.cpp
        MainWindow.h
        LoginWidget.cpp
        LoginWidget.h
        DashboardWidget.cpp
        DashboardWidget.h
        HomePage.h
        HomePage.cpp
        MovieCard.h
        MovieCard.cpp
        MovieDetailWidget.h
        MovieDetailWidget.cpp
        clickablelabel.h
        clickablelabel.cpp
        Theme.h
        resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Cinephile
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        usermanager.h usermanager.cpp
        firestoreservice.h firestoreservice.cpp
        profilepagewidget.h profilepagewidget.cpp
        genremovieswidget.h
        genremovieswidget.cpp
        CategoryFilterWidget.h
        CategoryFilterWidget.cpp
        WatchPage.h
        WatchPage.cpp
        LibraryPageWidget.h
        LibraryPageWidget.cpp
        recommendationpagewidget.h recommendationpagewidget.cpp
        chatbotwidget.h chatbotwidget.cpp
        configmanager.h configmanager.cpp

    )
else()
    if(ANDROID)
        add_library(Cinephile SHARED ${PROJECT_SOURCES})
    else()
        add_executable(Cinephile ${PROJECT_SOURCES})
    endif()
endif()
if(WIN32)
    set(RC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/app.rc)
    file(WRITE ${RC_FILE} "IDI_ICON1 ICON \"${CMAKE_CURRENT_SOURCE_DIR}/logo.ico\"")
    set_property(TARGET Cinephile APPEND PROPERTY SOURCES ${RC_FILE})
endif()
target_link_libraries(Cinephile PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Core
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Cinephile)
endif()
set_target_properties(Cinephile PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Cinephile
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Cinephile)
endif()
