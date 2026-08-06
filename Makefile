BUILD_TYPE ?= Release

.PHONY: engine clean

engine:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build build --target engine -j

clean:
	rm -rf build
