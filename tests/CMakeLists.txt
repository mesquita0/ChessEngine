project(EngineTest)
add_executable(EngineTest "${CMAKE_CURRENT_SOURCE_DIR}/EngineTest/main.cpp")
target_link_libraries(EngineTest PUBLIC engine)

project(NNUETest)
add_executable(NNUETest "${CMAKE_CURRENT_SOURCE_DIR}/NNUETest/main.cpp")
target_link_libraries(NNUETest PUBLIC engine)

project(PerftTest)
add_executable(PerftTest "${CMAKE_CURRENT_SOURCE_DIR}/PerftTest/main.cpp")
target_link_libraries(PerftTest PUBLIC engine)
