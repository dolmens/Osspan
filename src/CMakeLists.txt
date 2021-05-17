include(FindCURL)
include(FindOpenSSL)

if(NOT CURL_FOUND)
message(FATAL_ERROR "Could not find curl")
endif()

if(NOT OPENSSL_FOUND)
message(FATAL_ERROR "Could not find openssl")
endif()

set(CRYPTO_LIBS ${OPENSSL_LIBRARIES})
set(CRYPTO_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIRS})
set(CRYPTO_LIBS_ABSTRACT_NAME crypto ssl)

set(CLIENT_LIBS ${CURL_LIBRARIES})
set(CLIENT_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
set(CLIENT_LIBS_ABSTRACT_NAME curl)

find_package(wxWidgets 3.0 REQUIRED COMPONENTS aui xrc adv core xml base)
find_package(sqlite3 REQUIRED)

file(GLOB_RECURSE LOCALES "${CMAKE_SOURCE_DIR}/locales/*.mo")

file(GLOB_RECURSE SHARED_SUPPORTS "${CMAKE_SOURCE_DIR}/SharedSupport/*")

add_executable(Osspan MACOSX_BUNDLE
    osspanapp.cc
    options.cc
    window_state_manager.cc
    mainfrm.cc
    toolbar.mm
    executor.cc
    global_executor.cc
    string_format.cc
    storage.cc
    statusbar.cc
    traffic.cc
    traffic_setting_dialog.cc
    led.cc
    oss_regions.cc
    oss_client.cc
    oss_site_config.cc
    site.cc
    local_site.cc
    oss_site.cc
    directory_compare.cc
    uri_box.cc
    local_uri_box.cc
    oss_uri_box.cc
    dndobjects.cc
    file_list_ctrl.cc
    local_list_ctrl.cc
    local_list_view.cc
    oss_list_ctrl.cc
    oss_list_view.cc
    explorer.cc
    multi_tab_explorer.cc
    task_list.cc
    task_list_view_model.cc
    task_list_ctrl.cc
    sync_task_dialog.cc
    schedule_list.cc
    schedule_list_ctrl.cc
    oss_browse_dialog.cc
    site_manager_dialog.cc
    create_bucket_dialog.cc
    remove_objects_dialog.cc
    create_directory_dialog.cc
    input_dialog.cc
    dialogex.cc
    panelex.cc
    theme_provider.cc
    system_image_list.cc
    utils.cc
    ${LOCALES}
    ${SHARED_SUPPORTS}
    ${CMAKE_SOURCE_DIR}/Osspan.icns
    )

foreach (LC ${LOCALES})
    file(RELATIVE_PATH LC_RELATIVE_PATH "${CMAKE_SOURCE_DIR}/locales" ${LC})
    get_filename_component(DST_RELATIVE_PATH ${LC_RELATIVE_PATH} DIRECTORY)
    set_property(SOURCE ${LC} PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${DST_RELATIVE_PATH}")
endforeach()

set_source_files_properties(${CMAKE_SOURCE_DIR}/Osspan.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

foreach (SP ${SHARED_SUPPORTS})
    file(RELATIVE_PATH SP_RELATIVE_PATH "${CMAKE_SOURCE_DIR}/SharedSupport" ${SP})
    get_filename_component(DST_RELATIVE_PATH ${SP_RELATIVE_PATH} DIRECTORY)
    set_property(SOURCE ${SP} PROPERTY MACOSX_PACKAGE_LOCATION "SharedSupport/${DST_RELATIVE_PATH}")
endforeach()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ${wxWidgets_DEFINITIONS})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${wxWidgets_LIBRARIES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/osssdk/sdk/include)
target_link_libraries(${PROJECT_NAME} cpp-sdk)
target_link_libraries(${PROJECT_NAME} ${CRYPTO_LIBS})
target_link_libraries(${PROJECT_NAME} ${CLIENT_LIBS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/yaml/include)
target_link_libraries(${PROJECT_NAME}
    yaml-cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SQLite3_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME}
    ${SQLite3_LIBRARIES})

install(TARGETS Osspan
    BUNDLE DESTINATION . COMPONENT Runtime
    )

# Note Mac specific extension .app
set(APPS "\${CMAKE_INSTALL_PREFIX}/${APP_NAME}.app")

# Directories to look for dependencies
set(DIRS ${CMAKE_BINARY_DIR})

install(CODE "include(BundleUtilities)
    fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\")")

set(CPACK_GENERATOR "DRAGNDROP")
include(CPack)
