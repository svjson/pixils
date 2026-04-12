BUILD_TYPE ?= Release
PREFIX ?= $(HOME)/.local

configure:
	cmake -S . -B build \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
		-DCMAKE_PREFIX_PATH=$(PREFIX)

build:
	cmake --build build

install:
	cmake --build build --target install

test:
	cmake --build build --target testpixils && cd build && ctest --output-on-failure

clean:
	rm -rf build
