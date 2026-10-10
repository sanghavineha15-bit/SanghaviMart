cmake_minimum_required(VERSION 3.25)
project(sanghavimart VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Toolchain options ----
option(SANGHAVIMART_ENABLE_ASAN_UBSAN "Enable AddressSanitizer + UBSan" ON)
option(SANGHAVIMART_ENABLE_TSAN "Enable ThreadSanitizer (use单独 configure, incompatible with ASan)" OFF)
option(SANGHAVIMART_BUILD_TESTS "Build GoogleTest suites" ON)

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

if(SANGHAVIMART_ENABLE_ASAN_UBSAN AND NOT MSVC AND NOT SANGHAVIMART_ENABLE_TSAN)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

if(SANGHAVIMART_ENABLE_TSAN AND NOT MSVC)
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
endif()

# ---- Dependencies (vcpkg manifest mode provides these) ----
find_package(Drogon CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(CURL REQUIRED)

if(SANGHAVIMART_BUILD_TESTS)
  find_package(GTest CONFIG REQUIRED)
  enable_testing()
endif()

find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif()

# ---- Sources ----
file(GLOB_RECURSE SANGHAVIMART_SRC
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
# Exclude main.cc from the shared lib so tests can link without duplicate main
list(FILTER SANGHAVIMART_SRC EXCLUDE REGEX ".*/src/main\\.cc$")

file(GLOB_RECURSE SANGHAVIMART_HEADERS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_library(sanghavimart_lib STATIC ${SANGHAVIMART_SRC} ${SANGHAVIMART_HEADERS})
target_include_directories(sanghavimart_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(sanghavimart_lib PUBLIC
  Drogon::Drogon
  unofficial-sodium::sodium
  spdlog::spdlog
  nlohmann_json::nlohmann_json
  SQLite::SQLite3
  libpqxx::pqxx
  CURL::libcurl
)

add_executable(sanghavimart "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc")
target_link_libraries(sanghavimart PRIVATE sanghavimart_lib)

# ---- CSP views (Drogon auto-compiles .csp if drogon_ctl used; keep dir wired) ----
# Views in views/ are compiled by Drogon at runtime via config.json "views" path
# for dev simplicity; production uses compiled templates via drogon_ctl create view.
# To precompile: drogon_ctl create view ./views -o ./src/generated_views

# ---- Tests ----
if(SANGHAVIMART_BUILD_TESTS)
  file(GLOB_RECURSE SANGHAVIMART_TEST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cc")
  add_executable(sanghavimart_tests ${SANGHAVIMART_TEST_SRC})
  target_link_libraries(sanghavimart_tests PRIVATE
    sanghavimart_lib
    GTest::gtest GTest::gmock GTest::gtest_main
  )
  include(GoogleTest)
  gtest_discover_tests(sanghavimart_tests)
endif()

# ---- Static analysis targets ----
find_program(CPPCHECK_EXE NAMES cppcheck)
if(CPPCHECK_EXE)
  add_custom_target(cppcheck
    COMMAND ${CPPCHECK_EXE} --enable=all --std=c++20 --suppress=missingIncludeSystem
            --error-exitcode=1 "${CMAKE_CURRENT_SOURCE_DIR}/src"
    COMMENT "Running cppcheck"
    VERBATIM)
endif()

find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
  add_custom_target(format-check
    COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror
      ${SANGHAVIMART_SRC} ${SANGHAVIMART_HEADERS}
      "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc"
    COMMENT "Checking clang-format"
    VERBATIM)
endif()
