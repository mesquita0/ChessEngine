file(GLOB NNUE_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Evaluate/*.cpp")
add_library(nnue STATIC ${NNUE_SRC_FILES})
target_include_directories(nnue PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Evaluate" "${CMAKE_CURRENT_SOURCE_DIR}/../")
