.DEFAULT_GOAL := build

CMAKE ?= cmake
CTEST ?= ctest
BUILD_DIR ?= build/dev
BUILD_TYPE ?= Debug
GENERATOR ?= Ninja
CMAKE_ARGS ?=
BUILD_JOBS ?= 4
BUILD_TARGETS ?=
TEST_JOBS ?= 2
TESTS ?=
CTEST_ARGS ?=
PYTHON ?= python3
RELEASE_BUILD_DIR ?= build/release
RELEASE_OUTPUT_DIR ?= dist
SANITIZER_BUILD_DIR ?= build/sanitized
PACKAGE_ARGS ?=

.PHONY: configure build run test check check-sanitizers format-check release help

configure:
	$(CMAKE) -S . -B "$(BUILD_DIR)" -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" $(CMAKE_ARGS)

build: configure
	$(CMAKE) --build "$(BUILD_DIR)" --config "$(BUILD_TYPE)" --parallel "$(BUILD_JOBS)" $(if $(strip $(BUILD_TARGETS)),--target $(BUILD_TARGETS))

run: build
	$(CMAKE) --build "$(BUILD_DIR)" --config "$(BUILD_TYPE)" --target run

test check: build
	$(CTEST) --test-dir "$(BUILD_DIR)" --output-on-failure --no-tests=error --parallel "$(TEST_JOBS)" $(if $(strip $(TESTS)),-R '$(TESTS)') $(CTEST_ARGS)

check-sanitizers:
	$(MAKE) check BUILD_DIR="$(SANITIZER_BUILD_DIR)" BUILD_TYPE=Debug CMAKE_ARGS="$(CMAKE_ARGS) -DMOORHUHN2_SANITIZERS=ON"

format-check: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target format-check

release:
	@$(PYTHON) -c 'import sys; sys.exit(sys.version_info < (3, 11) and "make release requires Python 3.11+")'
	$(CMAKE) -S . -B "$(RELEASE_BUILD_DIR)" -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF $(CMAKE_ARGS)
	$(CMAKE) --build "$(RELEASE_BUILD_DIR)" --config Release --target moorhuhn2 --parallel "$(BUILD_JOBS)"
	$(PYTHON) -B tools/packaging/package_release.py --build-dir "$(RELEASE_BUILD_DIR)" --output-dir "$(RELEASE_OUTPUT_DIR)" $(PACKAGE_ARGS)

help:
	@printf '%s\n' 'make build         Configure and build incrementally.' 'make run           Build, then open Moorhuhn 2.' 'make release       Build native Release/LTO ZIPs from a clean checkout.' 'make test/check    Build and run all tests (two at a time).' 'make check-sanitizers  Run tests with ASan and UBSan.' 'make format-check  Check project C++ formatting.' 'Focused checks: make test BUILD_TARGETS="target1 target2" TESTS="test-name-regex"' 'Options: BUILD_DIR=build/dev BUILD_TYPE=Debug GENERATOR=Ninja CMAKE_ARGS=' '         BUILD_JOBS=4 TEST_JOBS=2 BUILD_TARGETS= TESTS= CTEST_ARGS=' '         RELEASE_BUILD_DIR=build/release RELEASE_OUTPUT_DIR=dist PYTHON=python3 PACKAGE_ARGS='
