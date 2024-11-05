set(CMAKE_CXX_STANDARD 20)

FetchContent_Declare(
        gtest_repo
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG 1d17ea1)
FetchContent_GetProperties(gtest_repo)

    FetchContent_MakeAvailable(gtest_repo)
    add_library(
            lib_gtest
            ${gtest_repo_SOURCE_DIR}/googletest/src/gtest-all.cc)
    target_include_directories(
            lib_gtest
            PUBLIC ${gtest_repo_SOURCE_DIR}/googletest/include/
            PRIVATE ${gtest_repo_SOURCE_DIR}/googletest)
    set_target_properties(lib_gtest PROPERTIES LINKER_LANGUAGE CXX)


add_executable(unit_test
        main.cpp
        AluTest.cpp
        CpuTest.cpp
)

target_link_libraries(unit_test embedded_sim_lib lib_gtest)
set_target_properties(unit_test PROPERTIES LINKER_LANGUAGE CXX)