BUILD_TYPE ?= Release
CLANG_FORMAT ?= clang-format
SOURCES := $(wildcard src/*.h src/*.cpp tests/*.h tests/*.cpp tools/*.cpp)

.PHONY: engine play test bench benchcmp tactics sprt setup-tools gen-tables check-tables format format-check clean

engine:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build build --target engine -j

# Play against the engine in a browser. Pass flags through PLAY_ARGS, e.g.
# `make play PLAY_ARGS="--host 0.0.0.0 --hash 256"`; see `./play.py --help`.
play: engine
	python3 play.py $(PLAY_ARGS)

test:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build build -j
	ctest --test-dir build -L fast --output-on-failure

bench: engine
	./build/engine bench $(BENCH_ARGS)

benchcmp:
	python3 scripts/benchcmp.py $(REF) $(NEW) $(BENCHCMP_ARGS)

tactics:
	python3 scripts/tactics.py $(TACTICS_ARGS)

sprt:
	./scripts/sprt.sh $(REF) $(NEW)

setup-tools:
	./scripts/setup_tools.sh

# Build the offline table generator. Run it as `./build/gen_tables <table>` and
# paste the output over the matching array in src/LookupTables.h; `gen_tables`
# with no argument lists the table names.
gen-tables:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build build --target gen_tables -j

# Confirm the tables committed in src/LookupTables.h still match their generator.
check-tables: gen-tables
	python3 tools/check_tables.py ./build/gen_tables src/LookupTables.h

# src/LookupTables.h is skipped through .clang-format-ignore
format:
	$(CLANG_FORMAT) -i $(SOURCES)

format-check:
	$(CLANG_FORMAT) --dry-run -Werror $(SOURCES)

clean:
	rm -rf build
