file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/tests/test_*.cpp)

foreach(SRC ${TEST_SRC_FILES})
    get_filename_component(test_name ${SRC} NAME_WE)
    set_source_files_properties(${SRC} PROPERTIES COMPILE_FLAGS "-fexceptions")
    add_executable(${test_name} ${SRC})
    add_dependencies(${test_name} gtest)
    target_link_libraries(${test_name}
        gtest_main
        pthread
        mc
        )
    add_test(${test_name} ${test_name})
endforeach(SRC)

add_executable(profile_client profile_client.cpp)
if(NOT APPLE)
    target_link_libraries(profile_client rt)
endif(NOT APPLE)
target_link_libraries(profile_client mc)

add_executable(debug_client debug_client.cpp)
if(NOT APPLE)
    target_link_libraries(debug_client rt)
endif(NOT APPLE)
target_link_libraries(debug_client mc)
