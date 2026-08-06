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
miles_test(test_perft_deep)

set_tests_properties(test_fen test_zobrist test_perft test_uci_parse
                     PROPERTIES LABELS fast)
set_tests_properties(test_perft_deep PROPERTIES LABELS slow TIMEOUT 3600)
