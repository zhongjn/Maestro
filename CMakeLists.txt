cmake_minimum_required (VERSION 3.10)

set(src_common

)

set(src_main
    main
)

set(src_tests
    test/test
)

add_executable (Main ${src_common} ${src_main})
add_executable (TestEntry ${src_common} ${src_tests})

set(targets
    Main
    TestEntry
)

foreach(tg ${targets})
    set_property(TARGET ${tg} PROPERTY CXX_STANDARD 17)
endforeach()