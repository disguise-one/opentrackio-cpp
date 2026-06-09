# Copyright 2025 Mo-Sys Engineering Ltd
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”),
# to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Include(FetchContent)

# Catch 2 test framework
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.0
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build libraries as static" FORCE)
FetchContent_MakeAvailable(Catch2)

# Camdkit to provide test data/samples.
FetchContent_Declare(
  testdata
  GIT_REPOSITORY git@github.com:disguise-one/opentrackio-testdata.git
  GIT_TAG        v1.0.1
)
FetchContent_MakeAvailable(testdata)

# Json
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")
find_package(nlohmann_json REQUIRED)

# Json validator
FetchContent_Declare(nlohmann_json_schema_validator
        GIT_REPOSITORY https://github.com/pboettch/json-schema-validator
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        GIT_TAG 2.3.0
)
set(nlohmann_json_schema_validator_SHARED_LIBS OFF CACHE BOOL "Build nlohmann_json_schema_validator as a static library" FORCE)
FetchContent_MakeAvailable(nlohmann_json_schema_validator)

# Compile
add_executable(tests
    test.cpp
)
target_compile_features(tests PUBLIC cxx_std_20)
target_include_directories(tests
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../include"
        "${CMAKE_CURRENT_LIST_DIR}/../external"
)

# Copies the testdata contents to the exe in the tree.
add_custom_command(TARGET tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${testdata_SOURCE_DIR}
                $<TARGET_FILE_DIR:tests>/testdata
        COMMENT "Copying test resources to target.")


# Linkage
target_link_libraries(tests
    PRIVATE
        ${PROJECT_NAME}
        Catch2::Catch2WithMain
        nlohmann_json_schema_validator
)

if(NOT BUILD_STATIC_LIBS)
    message(STATUS "Adding post build shared library copy for tests.")
    add_custom_command(TARGET tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                $<TARGET_RUNTIME_DLLS:tests> $<TARGET_FILE_DIR:tests>
        COMMAND_EXPAND_LISTS
    )
endif()
