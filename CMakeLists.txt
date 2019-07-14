cmake_minimum_required (VERSION 3.10)

set(src_common

)

set(src_main
    main
)

set(src_tests
    test/test
)

set(src_MCTS
	MCTS/main
	MCTS/gomoku
	MCTS/MCT
)

add_executable (Main ${src_common} ${src_main})
add_executable (TestEntry ${src_common} ${src_tests})
add_executable (MCTSDemo ${src_common} ${src_MCTS})

set(targets
    Main
    TestEntry
	MCTSDemo
)

foreach(tg ${targets})
    set_property(TARGET ${tg} PROPERTY CXX_STANDARD 17)
endforeach()