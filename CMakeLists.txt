cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(ChessEngine)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/nnue")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/MagicBitboards")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests")

file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(engine STATIC ${SRC_FILES})
add_executable(ChessEngine "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

target_include_directories(engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(engine PUBLIC nnue)
target_link_libraries(engine PUBLIC magic_bitboards)
target_link_libraries(ChessEngine PUBLIC engine)
