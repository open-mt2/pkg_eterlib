add_executable(test_eterlib
    test_eterlib.cpp
)

target_link_libraries(test_eterlib
    PRIVATE
        EterLib
)

add_test(NAME EterLib_SmokeTests COMMAND test_eterlib)
