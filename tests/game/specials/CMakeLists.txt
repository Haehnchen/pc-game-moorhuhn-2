moorhuhn2_add_test(TARGET moorhuhn2_multi_hit_tests NAME game.specials.multi_hit
    SOURCE multi_hit.cpp LIBRARIES moorhuhn2_game
    LABELS game specials TIMEOUT 30)
