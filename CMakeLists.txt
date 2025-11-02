cmake_minimum_required(VERSION 3.18)
project(d3d12_renderdoc_crash_repro)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY bin)

macro(download_extract URL DST_FOLDER FOLDER_NAME)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip)
        message(STATUS "Downloading ${URL} and unpacking to ${DST_FOLDER}/${FOLDER_NAME} .")
        file(
            DOWNLOAD
            ${URL}
            ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip
        )
    else()
        message(STATUS "${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip already exists. No download required.")
    endif()
    if(NOT EXISTS ${DST_FOLDER}/${FOLDER_NAME})
        message(STATUS "Extracting ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip to ${DST_FOLDER}/${FOLDER_NAME} .")
        file(
            ARCHIVE_EXTRACT
            INPUT ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip
            DESTINATION ${DST_FOLDER}/${FOLDER_NAME}
        )
    else()
        message(STATUS "${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip is already extracted to ${DST_FOLDER}/${FOLDER_NAME} .")
    endif()
endmacro()

macro(copy_if_not_exist SRC DST)
    if(NOT EXISTS ${DST}) # Should check for contents in case of directory.
        message(STATUS "Copying ${SRC} to ${DST} .")
        file(COPY ${SRC} DESTINATION ${DST})
    else()
        message(STATUS "${SRC} already exists at ${DST} .")
    endif()
endmacro()

macro(copy_dll_if_not_exist SRC DST)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        copy_if_not_exist(${SRC} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUTPUTCONFIG}/${DST})
    endforeach()
endmacro()

download_extract(
    https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.618.2
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    d3d12_agility_1.618.2
)

copy_dll_if_not_exist(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/d3d12_agility_1.618.2/build/native/bin/x64/
    D3D12
)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    copy_if_not_exist(
        ${CMAKE_CURRENT_SOURCE_DIR}/shader.bin
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUTPUTCONFIG}/)
endforeach()

add_executable(repro_01 ${CMAKE_CURRENT_SOURCE_DIR}/repro_01/main.cpp)
target_link_libraries(
    repro_01 PUBLIC
    d3d12.lib
    dxgi.lib)
target_include_directories(
    repro_01 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/d3d12_agility_1.618.2/build/native/include)
set_target_properties(repro_01 PROPERTIES CXX_STANDARD 20)

add_executable(repro_02 ${CMAKE_CURRENT_SOURCE_DIR}/repro_02/main.cpp)
target_link_libraries(
    repro_02 PUBLIC
    d3d12.lib
    dxgi.lib)
target_include_directories(
    repro_02 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/d3d12_agility_1.618.2/build/native/include)
set_target_properties(repro_02 PROPERTIES CXX_STANDARD 20)
