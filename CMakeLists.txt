cmake_minimum_required (VERSION 3.10)

set(src_common
    evaluator/evaluator_gomoku
    util/expirable
    util/lazy
    util/nullable
    util/spin_lock
    game/game_base
    game/game_gomoku
    game/game_test
    play/player
    play/match
    play/round
    search/search_base
    search/search_graph
    search/search_tree
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
set_target_properties(Main PROPERTIES LINK_FLAGS "/PROFILE")

foreach(tg ${targets})
    set_property(TARGET ${tg} PROPERTY CXX_STANDARD 17)
endforeach()