moorhuhn2_add_test(TARGET moorhuhn2_presentation_tests NAME integration.render.presentation
    SOURCE presentation.cpp LIBRARIES moorhuhn2_render moorhuhn2_assets
    LABELS render assets integration TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")

moorhuhn2_add_test(TARGET moorhuhn2_round_scene_tests NAME integration.render.round_scene
    SOURCE round_scene.cpp LIBRARIES moorhuhn2_round_scene moorhuhn2_assets
    LABELS render game assets integration TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets")
