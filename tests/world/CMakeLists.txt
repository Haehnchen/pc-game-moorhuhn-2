moorhuhn2_add_test(TARGET moorhuhn2_scarecrow_tests NAME world.scarecrow
    SOURCE scarecrow.cpp LIBRARIES moorhuhn2_world
    LABELS world game specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_tree_tests NAME world.tree
    SOURCE tree.cpp LIBRARIES moorhuhn2_world
    LABELS world game specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_branch_tests NAME world.branch
    SOURCE branch.cpp LIBRARIES moorhuhn2_world
    LABELS world game TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_notice_tests NAME world.notice
    SOURCE notice.cpp LIBRARIES moorhuhn2_world
    LABELS world game ui specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_spider_tests NAME world.spider
    SOURCE spider.cpp LIBRARIES moorhuhn2_world
    LABELS world game specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_frog_tests NAME world.frog
    SOURCE frog.cpp LIBRARIES moorhuhn2_world
    LABELS world game specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_stone_tests NAME world.stone
    SOURCE stone.cpp LIBRARIES moorhuhn2_world
    LABELS world game TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_balloon_tests NAME world.balloon
    SOURCE balloon.cpp LIBRARIES moorhuhn2_world
    LABELS world game TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_boat_tests NAME world.boat
    SOURCE boat.cpp LIBRARIES moorhuhn2_world
    LABELS world game specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_village_tests NAME world.village
    SOURCE village.cpp LIBRARIES moorhuhn2_world
    LABELS world game specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_hunting_stand_tests NAME world.hunting_stand
    SOURCE hunting_stand.cpp LIBRARIES moorhuhn2_world
    LABELS world game TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_hidden_chicken_tests NAME world.hidden_chicken
    SOURCE hidden_chicken.cpp LIBRARIES moorhuhn2_world
    LABELS world game TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_rose_tests NAME world.rose
    SOURCE rose.cpp LIBRARIES moorhuhn2_world
    LABELS world game TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_big_chicken_tests NAME world.big_chicken
    SOURCE big_chicken.cpp LIBRARIES moorhuhn2_world
    LABELS world game TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_special_chicken_tests NAME world.special_chicken
    SOURCE special_chicken.cpp LIBRARIES moorhuhn2_world
    LABELS world game specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_airplane_tests NAME world.airplane
    SOURCE airplane.cpp LIBRARIES moorhuhn2_world
    LABELS world game TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_blimp_tests NAME world.blimp
    SOURCE blimp.cpp LIBRARIES moorhuhn2_world
    LABELS world game specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_persistent_leaves_tests NAME world.persistent_leaves
    SOURCE persistent_leaves.cpp LIBRARIES moorhuhn2_world
    LABELS world game TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_world_presentation_tests NAME world.presentation
    SOURCE presentation.cpp LIBRARIES moorhuhn2_render
    LABELS world render TIMEOUT 30)
