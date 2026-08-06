BUILD_TYPE ?= Release

.PHONY: engine test clean

engine:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build build --target engine -j

test:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build build -j
	ctest --test-dir build -L fast --output-on-failure

clean:
	rm -rf build
