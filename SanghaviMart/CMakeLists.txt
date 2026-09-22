cmake_minimum_required(VERSION 3.25)
project(SanghaviMart CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build GoogleTest suite" ON)
option(ENABLE_ASAN "AddressSanitizer" OFF)
option(ENABLE_UBSAN "UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "ThreadSanitizer" OFF)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_library(SODIUM_LIB sodium REQUIRED)
find_path(SODIUM_INCLUDE_DIR sodium.h REQUIRED)
find_library(ARGON2_LIB argon2)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PQXX QUIET libpqxx)
endif()

file(GLOB_RECURSE APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/controllers/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/filters/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controller/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/*.cc
)

add_executable(${PROJECT_NAME} ${APP_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SODIUM_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    spdlog::spdlog
    CURL::libcurl
    nlohmann_json::nlohmann_json
    libpqxx::pqxx
    ${SODIUM_LIB}
)
if(ARGON2_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ARGON2_LIB})
endif()

foreach(SAN OPT VAL)
    if(${OPT})
        target_compile_options(${PROJECT_NAME} PRIVATE ${VAL})
        target_link_options(${PROJECT_NAME} PRIVATE ${VAL})
    endif()
endforeach()
# Expanded manually (foreach over pairs is clearer below)
if(ENABLE_ASAN)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
endif()
if(ENABLE_UBSAN)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined)
endif()
if(ENABLE_TSAN)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=thread)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=thread)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/config.json
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/frontend
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/frontend
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/static
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/static
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/db
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/db
)

# Static analysis targets (do not suppress; fix warnings)
find_program(CLANG_TIDY_EXE clang-tidy)
if(CLANG_TIDY_EXE)
    add_custom_target(tidy COMMAND ${CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR} ${APP_SOURCES})
endif()
find_program(CPPCHECK_EXE cppcheck)
if(CPPCHECK_EXE)
    add_custom_target(cppcheck COMMAND ${CPPCHECK_EXE} --enable=all --std=c++20
        --suppress=missingIncludeSystem -I ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/controllers)
endif()
find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
    add_custom_target(format-check COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror
        --style=file ${APP_SOURCES})
endif()

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc)
    if(TEST_SOURCES)
        add_executable(sanghavimart_tests ${TEST_SOURCES})
        target_include_directories(sanghavimart_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${SODIUM_INCLUDE_DIR})
        target_link_libraries(sanghavimart_tests PRIVATE
            GTest::gtest GTest::gmock spdlog::spdlog ${SODIUM_LIB} CURL::libcurl
            nlohmann_json::nlohmann_json)
        if(ENABLE_ASAN)
            target_compile_options(sanghavimart_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
            target_link_options(sanghavimart_tests PRIVATE -fsanitize=address)
        endif()
        if(ENABLE_UBSAN)
            target_compile_options(sanghavimart_tests PRIVATE -fsanitize=undefined)
            target_link_options(sanghavimart_tests PRIVATE -fsanitize=undefined)
        endif()
        include(GoogleTest)
        gtest_discover_tests(sanghavimart_tests)
    endif()
endif()
