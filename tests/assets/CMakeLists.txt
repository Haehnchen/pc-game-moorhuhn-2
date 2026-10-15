moorhuhn2_add_test(TARGET moorhuhn2_asset_package_tests NAME assets.package
    SOURCE asset_package.cpp LIBRARIES moorhuhn2_package
    LABELS assets TIMEOUT 10
    ARGS "${CMAKE_CURRENT_BINARY_DIR}/fixtures")

moorhuhn2_add_test(TARGET moorhuhn2_asset_tests NAME assets.store
    SOURCE asset_store.cpp LIBRARIES moorhuhn2_assets
    LABELS assets TIMEOUT 90
    ARGS "${PROJECT_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/fixtures")
add_dependencies(moorhuhn2_asset_tests moorhuhn2_asset_package)
