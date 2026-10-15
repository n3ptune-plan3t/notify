cmake_minimum_required(VERSION 3.21)
project(QtNotifyCenter VERSION 0.3.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets DBus)

qt_add_executable(qt-notify-centerctl src/ctl.cpp)
target_link_libraries(qt-notify-centerctl PRIVATE Qt6::Core Qt6::DBus)

qt_add_executable(qt-notify-center
    src/main.cpp src/appconfig.h src/appconfig.cpp
    src/notificationserver.h src/notificationserver.cpp
    src/panelcontroller.h src/panelcontroller.cpp
    src/notificationpanel.h src/notificationpanel.cpp
    src/notificationpopup.h src/notificationpopup.cpp
)
target_link_libraries(qt-notify-center PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus)

include(GNUInstallDirs)
install(TARGETS qt-notify-center qt-notify-centerctl RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES packaging/qt-notify-center.desktop DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
install(FILES packaging/qt-notify-center.service DESTINATION "${CMAKE_INSTALL_LIBDIR}/systemd/user" OPTIONAL)
install(FILES config/config.ini.example DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/qt-notify-center")
