#include "tensorstore/open.h"

// Minimal stub that forces the linker to pull in TensorStore symbols.
// This file exists solely to make ts_prebuild_marker a non-trivial executable
// so cmake compiles all TensorStore transitive dependencies during the prebuild.
int main() { return 0; }
