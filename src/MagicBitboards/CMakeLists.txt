file(GLOB MBB_SRC_FILES "Serializer.cpp")
add_library(magic_bitboards STATIC ${MBB_SRC_FILES})
target_include_directories(magic_bitboards PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

project(GenerateMagicBitboards)
add_executable(GenerateMagicBitboards "main.cpp")
target_link_libraries(GenerateMagicBitboards PUBLIC magic_bitboards)
