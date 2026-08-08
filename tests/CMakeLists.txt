function(miles_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE milesChess_core)
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

miles_test(test_fen)
miles_test(test_zobrist)
miles_test(test_perft)
miles_test(test_uci_parse)
miles_test(test_puzzles)
miles_test(test_eval_symmetry)
miles_test(test_endgame)
miles_test(test_perft_deep)

set_tests_properties(test_fen test_zobrist test_perft test_uci_parse test_puzzles test_eval_symmetry test_endgame
                     PROPERTIES LABELS fast)
set_tests_properties(test_perft_deep PROPERTIES LABELS slow TIMEOUT 3600)

# The lookup tables are generator output pasted into the header by hand; check
# the paste is still faithful. gen_tables is EXCLUDE_FROM_ALL, so the test
# builds it on demand rather than relying on it already existing.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME build_gen_tables
             COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target gen_tables)
    add_test(NAME test_tables
             COMMAND ${Python3_EXECUTABLE}
                     ${CMAKE_SOURCE_DIR}/tools/check_tables.py
                     $<TARGET_FILE:gen_tables>
                     ${CMAKE_SOURCE_DIR}/src/LookupTables.h)
    set_tests_properties(build_gen_tables PROPERTIES LABELS fast FIXTURES_SETUP gen_tables)
    set_tests_properties(test_tables PROPERTIES LABELS fast FIXTURES_REQUIRED gen_tables)
endif()
