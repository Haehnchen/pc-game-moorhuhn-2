moorhuhn2_add_test(TARGET moorhuhn2_clock_input_tests NAME integration.game.clock_input
    SOURCE clock_input.cpp LIBRARIES moorhuhn2_game moorhuhn2_clock
    LABELS integration game TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_numeric_tests NAME integration.game.numeric
    SOURCE numeric.cpp LIBRARIES moorhuhn2_game
    LABELS integration game world TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_round_order_tests NAME integration.game.round_order
    SOURCE round_order.cpp LIBRARIES moorhuhn2_game
    LABELS integration game world TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_round_grants_tests NAME integration.game.round_grants
    SOURCE round_grants.cpp LIBRARIES moorhuhn2_game
    LABELS integration game world specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_multi_hit_round_tests NAME integration.game.multi_hit
    SOURCE multi_hit.cpp LIBRARIES moorhuhn2_game
    LABELS integration game world specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_house88_tests NAME integration.game.house88
    SOURCE house88.cpp LIBRARIES moorhuhn2_game
    LABELS integration game specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_balloon_hero_tests NAME integration.game.balloon_hero
    SOURCE balloon_hero.cpp LIBRARIES moorhuhn2_game
    LABELS integration game world specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_persistent_leaves_round_tests NAME integration.game.persistent_leaves
    SOURCE persistent_leaves.cpp LIBRARIES moorhuhn2_game
    LABELS integration game world TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_flying_targets_round_tests NAME integration.game.flying_targets
    SOURCE flying_targets.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets
    LABELS integration world game render assets specials TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_special_chickens_round_tests NAME integration.game.special_chickens
    SOURCE special_chickens.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets
    LABELS integration world game render assets specials TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_rose_round_tests NAME integration.game.rose
    SOURCE rose.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets moorhuhn2_ui
    LABELS integration world game render assets TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_hidden_chicken_round_tests NAME integration.game.hidden_chicken
    SOURCE hidden_chicken.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets moorhuhn2_ui
    LABELS integration world game render assets TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_hunting_stand_round_tests NAME integration.game.hunting_stand
    SOURCE hunting_stand.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets moorhuhn2_ui
    LABELS integration world game render assets TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_scarecrow_round_tests NAME integration.game.scarecrow
    SOURCE scarecrow.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets moorhuhn2_ui
    LABELS integration world game render assets specials TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_tree_round_tests NAME integration.game.trees
    SOURCE trees.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets moorhuhn2_ui
    LABELS integration world game render assets specials TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_branch_round_tests NAME integration.game.branches
    SOURCE branches.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets moorhuhn2_ui
    LABELS integration world game render assets TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_spider_round_tests NAME integration.game.spider
    SOURCE spider.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets moorhuhn2_ui
    LABELS integration world game render assets specials TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_frog_round_tests NAME integration.game.frog
    SOURCE frog.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets
    LABELS integration world game render assets specials TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_village_round_tests NAME integration.game.village
    SOURCE village.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets moorhuhn2_ui
    LABELS integration world game render assets specials TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_stone_round_tests NAME integration.game.stones
    SOURCE stones.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets moorhuhn2_ui
    LABELS integration world game render assets TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_eight_stones_tests NAME integration.game.eight_stones
    SOURCE eight_stones.cpp LIBRARIES moorhuhn2_game
    LABELS integration game world specials TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_boat_round_tests NAME integration.game.boat
    SOURCE boat.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_render moorhuhn2_assets
    LABELS integration world game render assets TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_notice_render_tests NAME integration.game.notice
    SOURCE notice.cpp LIBRARIES moorhuhn2_world moorhuhn2_ui
    LABELS integration world ui TIMEOUT 30)
