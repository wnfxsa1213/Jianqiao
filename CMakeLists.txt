cmake_minimum_required(VERSION 3.16)
project(JianqiaoSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui GuiPrivate CoreTools)

set(PROJECT_SOURCES
    main.cpp
    JianqiaoCoreShell.cpp
    SystemInteractionModule.cpp
    AdminLoginView.cpp
    AdminModule.cpp
    AdminDashboardView.cpp
    WhitelistManagerView.cpp
    UserView.cpp
    UserModeModule.cpp
    HotkeyEditDialog.cpp
    AppCardWidget.cpp # Consider removing if AppCardWidget is fully replaced
    HoverIconWidget.cpp 
)

set(PROJECT_HEADERS
    JianqiaoCoreShell.h
    SystemInteractionModule.h
    AdminLoginView.h
    AdminModule.h
    AdminDashboardView.h
    WhitelistManagerView.h
    UserView.h
    UserModeModule.h
    common_types.h # common_types.h should be here if it contains Q_OBJECT or needs MOC/UIC
    HotkeyEditDialog.h
    AppCardWidget.h # Consider removing if AppCardWidget is fully replaced
    HoverIconWidget.h
)

add_executable(JianqiaoSystem
    ${PROJECT_SOURCES}
    # PROJECT_HEADERS are generally not listed directly in add_executable for compilation
    # CMAKE_AUTOMOC will scan sources for includes of headers with Q_OBJECT.
    # common_types.h # If it has Q_GADGET or similar it might need to be explicitly handled or just included by other moc'd headers.
)

target_include_directories(JianqiaoSystem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(JianqiaoSystem PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::GuiPrivate)

# Copy config.json to the output directory
set(CONFIG_FILE_NAME "config.json")
set(CONFIG_FILE_SOURCE "${CMAKE_SOURCE_DIR}/${CONFIG_FILE_NAME}")

add_custom_command(
    TARGET JianqiaoSystem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CONFIG_FILE_SOURCE}"
        "$<TARGET_FILE_DIR:JianqiaoSystem>/${CONFIG_FILE_NAME}"
    COMMENT "Copying ${CONFIG_FILE_NAME} to output directory"
)

# Copy AdminLoginView.qss to the output directory
set(QSS_FILE_NAME "AdminLoginView.qss")
set(QSS_FILE_SOURCE "${CMAKE_SOURCE_DIR}/${QSS_FILE_NAME}")

add_custom_command(
    TARGET JianqiaoSystem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QSS_FILE_SOURCE}"
        "$<TARGET_FILE_DIR:JianqiaoSystem>/${QSS_FILE_NAME}"
    COMMENT "Copying ${QSS_FILE_NAME} to output directory"
)

# Add a post-build step to run windeployqt
add_custom_command(
    TARGET JianqiaoSystem POST_BUILD
    COMMAND Qt6::windeployqt --dir "$<TARGET_FILE_DIR:JianqiaoSystem>" --plugindir "$<TARGET_FILE_DIR:JianqiaoSystem>/plugins" "$<TARGET_FILE:JianqiaoSystem>"
    COMMENT "Deploying Qt dependencies for JianqiaoSystem using windeployqt"
) 