BUILD_TYPE ?= Release
PREFIX ?= $(HOME)/.local
CTEST_PARALLEL_LEVEL ?= $(shell nproc)

.PHONY: configure build install test test-core test-libraries test-examples \
	test-cmake test-proof test-proof-libraries test-proof-examples benchmark \
	assets clean

configure:
	cmake -S . -B build \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
		-DCMAKE_PREFIX_PATH=$(PREFIX)

build:
	cmake --build build

install:
	cmake --build build --target install

test: test-core test-libraries test-examples

test-core: build
	cd build && ctest --output-on-failure --parallel $(CTEST_PARALLEL_LEVEL) -L core

test-libraries: build
	cd build && ctest --output-on-failure --parallel $(CTEST_PARALLEL_LEVEL) -L libraries
	sh scripts/run_proof_tests.sh --root lib

test-examples: build
	cd build && ctest --output-on-failure --parallel $(CTEST_PARALLEL_LEVEL) -L examples
	sh scripts/run_proof_tests.sh --root examples

test-cmake: build
	cd build && ctest --output-on-failure --parallel $(CTEST_PARALLEL_LEVEL)

test-proof: build
	sh scripts/run_proof_tests.sh

test-proof-libraries: build
	sh scripts/run_proof_tests.sh --root lib

test-proof-examples: build
	sh scripts/run_proof_tests.sh --root examples

benchmark:
	cmake --build build --target benchmarkpixils
	build/lib/pixils/benchmark/benchmarkpixils $(BENCHMARK_ARGS)

assets:
	bash scripts/generate_assets.sh

clean:
	rm -rf build
