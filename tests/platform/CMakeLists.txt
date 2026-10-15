moorhuhn2_add_test(TARGET moorhuhn2_viewport_tests NAME platform.viewport
    SOURCE viewport.cpp LIBRARIES moorhuhn2_viewport
    LABELS platform unit TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_clock_tests NAME platform.clock
    SOURCE clock.cpp LIBRARIES moorhuhn2_clock
    LABELS platform unit TIMEOUT 30)

moorhuhn2_add_test(TARGET moorhuhn2_keyboard_tests NAME platform.keyboard
    SOURCE keyboard.cpp LIBRARIES moorhuhn2_keyboard
    LABELS platform unit TIMEOUT 30)
