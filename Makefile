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

clean:
	rm -rf build
