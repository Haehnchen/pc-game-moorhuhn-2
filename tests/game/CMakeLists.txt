moorhuhn2_add_test(TARGET moorhuhn2_round_tests NAME game.round
    SOURCE round.cpp LIBRARIES moorhuhn2_game
    LABELS game TIMEOUT 30)

add_subdirectory(specials)

moorhuhn2_add_test(TARGET moorhuhn2_scene_object_tests NAME game.scene_object
    SOURCE scene_object.cpp LIBRARIES moorhuhn2_game
    LABELS game TIMEOUT 30)
