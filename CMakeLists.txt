cmake_minimum_required (VERSION 3.10)

project(Maestro)

set(LIBTORCH_DIR "D:/Program Files/libtorch")
include_directories(${LIBTORCH_DIR}/include/torch/csrc/api/include)
include_directories(${LIBTORCH_DIR}/include)
include_directories(include)

set(src_common
    src/evaluator/eval_gomoku_simplistic
    src/evaluator/eval_gomoku_nn
    src/util/expirable
    src/util/lazy
    src/util/nullable
    src/util/spin_lock
    src/game/game_base
    src/game/game_gomoku
    src/game/game_test
    src/play/player
    src/play/match
    src/play/round
    src/search/search_base
    src/search/search_graph
    src/search/search_tree
)

set(src_cli
    src/cli/cli_main
)

set(src_test
    src/test/test_main
    src/test/01_lazy
    src/test/02_gomoku
    src/test/03_expirable
)

set(copy_files
    
)

set(targets
    CLI
    TestEntry
    Exp2
    Exp3
)

add_executable (CLI ${src_common} ${src_cli})
add_executable (TestEntry ${src_common} ${src_test})
add_executable (Exp2 ${src_common} src/test/exp2)
add_executable (Exp3 ${src_common} src/test/exp3)
add_executable (Exp4 ${src_common} src/test/exp4)

foreach(tg ${targets})
    target_link_libraries(${tg} ${LIBTORCH_DIR}/lib/torch.lib)
    target_link_libraries(${tg} ${LIBTORCH_DIR}/lib/c10.lib)

    set_property(TARGET ${tg} PROPERTY CXX_STANDARD 17)

    foreach(file ${copy_files})
	    add_custom_command(
		    TARGET ${tg} POST_BUILD
		    COMMAND ${CMAKE_COMMAND} -E copy_if_different
				    ${file}
				    ${CMAKE_BINARY_DIR})
    endforeach()
endforeach()

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()