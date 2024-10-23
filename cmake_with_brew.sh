rm -rf build
CXX=$(brew --prefix llvm)/bin/clang++ cmake -S . -B build
make -C build
