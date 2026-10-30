add_executable(test_eterlib
    test_eterlib.cpp
)

target_link_libraries(test_eterlib
    PRIVATE
        EterLib
)

add_test(NAME EterLib_SmokeTests COMMAND test_eterlib)

foreach(TEST_NAME test_grpmath test_detector)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PRIVATE EterLib)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# TA.1: shared test helpers and always-on asserts (tests/support/TestAssert.h)
foreach(_test_target test_eterlib test_grpmath test_detector)
    target_include_directories(${_test_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(${_test_target} PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
endforeach()
