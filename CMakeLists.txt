cmake_minimum_required (VERSION 3.10)

project(Maestro)

include_directories(include)
include_directories(thirdparty/tvm/include)
include_directories(thirdparty/tvm/3rdparty/dlpack/include)
include_directories(thirdparty/tvm/3rdparty/dmlc-core/include)

include(tvm_config.cmake)
add_subdirectory(thirdparty/tvm)

set(src_common
    src/evaluator/evaluator_gomoku
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
	${CMAKE_BINARY_DIR}/tvm/tvm_runtime.dll
)

set(targets
    CLI
    TestEntry
)

add_executable (CLI ${src_common} ${src_cli})
add_executable (TestEntry ${src_common} ${src_test})

foreach(tg ${targets})
    set_property(TARGET ${tg} PROPERTY CXX_STANDARD 17)
    target_link_libraries(${tg} tvm_runtime)
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