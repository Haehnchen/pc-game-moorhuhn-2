moorhuhn2_add_test(TARGET moorhuhn2_image_bank_tests NAME render.image_bank
    SOURCE image_bank.cpp LIBRARIES moorhuhn2_render
    LABELS render TIMEOUT 10)

moorhuhn2_add_test(TARGET moorhuhn2_framebuffer_tests NAME render.framebuffer
    SOURCE framebuffer.cpp LIBRARIES moorhuhn2_render
    LABELS render TIMEOUT 10)

moorhuhn2_add_test(TARGET moorhuhn2_frame_catalog_tests NAME render.frame_catalog
    SOURCE frame_catalog.cpp LIBRARIES moorhuhn2_render
    LABELS render TIMEOUT 10)
