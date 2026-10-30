add_executable(test_eterlib
    test_eterlib.cpp
)

target_link_libraries(test_eterlib
    PRIVATE
        EterLib
)

add_test(NAME EterLib_SmokeTests COMMAND test_eterlib)

# TA.1: shared test helpers and always-on asserts (tests/support/TestAssert.h)
foreach(_test_target test_eterlib)
    target_include_directories(${_test_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(${_test_target} PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
endforeach()
