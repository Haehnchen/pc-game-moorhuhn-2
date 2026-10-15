moorhuhn2_add_test(TARGET moorhuhn2_audio_mixing_tests NAME audio.mixing
    SOURCE mixing.cpp LIBRARIES moorhuhn2_audio_mixing
    LABELS audio rules TIMEOUT 10)

moorhuhn2_add_test(TARGET moorhuhn2_audio_resampling_tests NAME audio.resampling
    SOURCE resampling.cpp LIBRARIES moorhuhn2_audio_resampling
    LABELS audio rules TIMEOUT 10)

set(disabled_audio "SDL_AUDIO_DRIVER=moorhuhn2-disabled")
moorhuhn2_add_test(TARGET moorhuhn2_audio_tests NAME audio.offline
    SOURCE engine.cpp LIBRARIES moorhuhn2_audio moorhuhn2_assets SDL3_mixer::SDL3_mixer
    LABELS audio TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets" offline
    ENVIRONMENT "${disabled_audio}")
foreach(mode IN ITEMS unavailable policy)
    add_test(NAME audio.${mode} COMMAND moorhuhn2_audio_tests "${PROJECT_SOURCE_DIR}/assets" ${mode})
    set_tests_properties(audio.${mode} PROPERTIES LABELS audio TIMEOUT 90 ENVIRONMENT "${disabled_audio}")
endforeach()
add_test(NAME audio.dummy COMMAND moorhuhn2_audio_tests "${PROJECT_SOURCE_DIR}/assets" dummy)
set_tests_properties(audio.dummy PROPERTIES LABELS audio TIMEOUT 90 ENVIRONMENT "SDL_AUDIO_DRIVER=dummy")
add_test(NAME audio.surround COMMAND moorhuhn2_audio_tests "${PROJECT_SOURCE_DIR}/assets" dummy)
set_tests_properties(audio.surround PROPERTIES LABELS audio TIMEOUT 90
    ENVIRONMENT "SDL_AUDIO_DRIVER=dummy;SDL_AUDIO_FREQUENCY=48000;SDL_AUDIO_CHANNELS=6;SDL_AUDIO_FORMAT=S16")
