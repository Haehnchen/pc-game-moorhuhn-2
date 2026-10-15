moorhuhn2_add_test(TARGET moorhuhn2_audio_pcm_tests NAME integration.audio.pcm
    SOURCE pcm.cpp LIBRARIES moorhuhn2_audio moorhuhn2_assets SDL3_mixer::SDL3_mixer
    LABELS audio integration TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets"
    ENVIRONMENT "SDL_AUDIO_DRIVER=moorhuhn2-disabled")

moorhuhn2_add_test(TARGET moorhuhn2_audio_sink_tests NAME integration.audio.sink
    SOURCE audio_sink.cpp LIBRARIES moorhuhn2_audio
    LABELS audio app integration TIMEOUT 30
    ARGS "${PROJECT_SOURCE_DIR}/assets"
    ENVIRONMENT "SDL_AUDIO_DRIVER=moorhuhn2-disabled")
target_sources(moorhuhn2_audio_sink_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/app/audio_sink.cpp")
