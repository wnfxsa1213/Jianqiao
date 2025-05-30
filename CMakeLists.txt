cmake_minimum_required(VERSION 3.16)
project(JianqiaoSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui GuiPrivate CoreTools Concurrent)

set(PROJECT_RESOURCES
    resources.qrc
)

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
    HotkeyEditDialog.cpp # Consider removing if AppCardWidget is fully replaced
    AppCardWidget.cpp # Consider removing if AppCardWidget is fully replaced
    HoverIconWidget.cpp 
    DetectionResultDialog.cpp
    AppStatusModel.cpp
    AppStatusBar.cpp
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
    DetectionResultDialog.h
    AppStatusModel.h
    AppStatusBar.h
)

add_executable(JianqiaoSystem WIN32
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
    ${PROJECT_RESOURCES}
)

target_include_directories(JianqiaoSystem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(JianqiaoSystem PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::GuiPrivate Qt6::Concurrent dwmapi)

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

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc")
set(APP_VERSION_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/version.rc")
if(MSVC)
    if(EXISTS ${APP_ICON_RESOURCE_WINDOWS})
        target_sources(JianqiaoSystem PRIVATE ${APP_ICON_RESOURCE_WINDOWS})
    endif()
    if(EXISTS ${APP_VERSION_RESOURCE_WINDOWS})
        target_sources(JianqiaoSystem PRIVATE ${APP_VERSION_RESOURCE_WINDOWS})
    endif()
    set(APP_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/JianqiaoSystem.exe.manifest")
    if(EXISTS ${APP_MANIFEST})
        set_target_properties(JianqiaoSystem PROPERTIES
            LINK_FLAGS "/MANIFESTUAC:level='requireAdministrator' /MANIFEST:EMBED /MANIFESTINPUT:${APP_MANIFEST}"
        )
    endif()
endif() 