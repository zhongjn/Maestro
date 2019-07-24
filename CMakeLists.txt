cmake_minimum_required (VERSION 3.10)

set(src_common
    util/expirable
    util/lazy
    util/nullable
    util/spin_lock
    game_base
    game_gomoku
    game_test
    search_base
    search_graph
    search_tree
)

set(src_main
    main
)

set(src_tests
    test/test
    test/01_lazy
    test/02_gomoku
    test/03_expirable
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