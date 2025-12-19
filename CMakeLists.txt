cmake_minimum_required(VERSION 3.16)
project(plasma-clock-oled VERSION 1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WaylandClient Svg DBus)
find_package(LayerShellQt REQUIRED)

add_executable(plasma-clock-oled
    src/main.cpp
    src/ClockWidget.cpp
    src/KDEClockConfig.cpp
    resources/resources.qrc
)

target_link_libraries(plasma-clock-oled PRIVATE
    Qt6::Widgets
    Qt6::Svg
    LayerShellQt::Interface
)

install(TARGETS plasma-clock-oled DESTINATION bin)
install(FILES resources/plasma-clock-oled.desktop DESTINATION share/applications)
install(FILES resources/plasma-clock-oled.svg DESTINATION share/icons/hicolor/scalable/apps)
install(FILES resources/oled-clock-widget.svg DESTINATION share/icons/hicolor/scalable/apps)

# Package metadata
set(PKG_DESCRIPTION_SUMMARY "A moving clock widget to prevent OLED burn-in")

# CPack configuration
set(CPACK_PACKAGE_NAME "plasma-clock-oled")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PKG_DESCRIPTION_SUMMARY}")
set(CPACK_PACKAGE_VENDOR "Ustek RFID")
set(CPACK_PACKAGE_CONTACT "software@ustek-rfid.com")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ustek-rfid/plasma-clock-oled")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

include(CPack)

# Custom target to generate Arch Linux .pkg.tar.zst
add_custom_target(package_arch
    COMMAND bash "${CMAKE_SOURCE_DIR}/cmake/make-arch-pkg.sh"
            "${CMAKE_BINARY_DIR}"
            "${PROJECT_NAME}"
            "${PROJECT_VERSION}"
            "${PKG_DESCRIPTION_SUMMARY}"
    DEPENDS plasma-clock-oled
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Generating Arch Linux package..."
)
