moorhuhn2_add_test(TARGET moorhuhn2_screens_tests NAME ui.screens
    SOURCE screens.cpp LIBRARIES moorhuhn2_ui
    LABELS ui TIMEOUT 15)

moorhuhn2_add_test(TARGET moorhuhn2_scores_tests NAME ui.scores
    SOURCE scores.cpp LIBRARIES moorhuhn2_ui
    LABELS ui storage TIMEOUT 15
    ARGS "${PROJECT_SOURCE_DIR}/tests/fixtures/behavior/ui/original-default.sav" "${CMAKE_CURRENT_BINARY_DIR}/fixtures")

moorhuhn2_add_test(TARGET moorhuhn2_house88_results_tests NAME ui.house88_results
    SOURCE house88_results.cpp LIBRARIES moorhuhn2_ui
    LABELS ui specials TIMEOUT 15)

moorhuhn2_add_test(TARGET moorhuhn2_outro_tests NAME ui.outro
    SOURCE outro.cpp LIBRARIES moorhuhn2_ui
    LABELS ui TIMEOUT 15)

moorhuhn2_add_test(TARGET moorhuhn2_results_transition_tests NAME ui.results_transition
    SOURCE results_transition.cpp LIBRARIES moorhuhn2_ui
    LABELS ui TIMEOUT 15)

moorhuhn2_add_test(TARGET moorhuhn2_round_exit_tests NAME ui.round_exit
    SOURCE round_exit.cpp LIBRARIES moorhuhn2_ui
    LABELS ui TIMEOUT 15)

moorhuhn2_add_test(TARGET moorhuhn2_results_celebration_tests NAME ui.results_celebration
    SOURCE results_celebration.cpp LIBRARIES moorhuhn2_ui
    LABELS ui TIMEOUT 15)

moorhuhn2_add_test(TARGET moorhuhn2_results_details_tests NAME ui.results_details
    SOURCE results_details.cpp LIBRARIES moorhuhn2_ui
    LABELS ui TIMEOUT 15)

moorhuhn2_add_test(TARGET moorhuhn2_title_tests NAME ui.title
    SOURCE title.cpp LIBRARIES moorhuhn2_ui
    LABELS ui TIMEOUT 15)

moorhuhn2_add_test(TARGET moorhuhn2_loading_tests NAME ui.loading
    SOURCE loading.cpp LIBRARIES moorhuhn2_ui
    LABELS ui TIMEOUT 15)
