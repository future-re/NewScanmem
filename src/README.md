# Source layout

The supported build consumes the traditional headers under
`include/newscanmem/` and the ordinary translation unit `newscanmem.cpp`.

The historical `.cppm` files in this directory are retained as a recoverable
reference for the migration; they are not listed in CMake and are not compiled.
Do not add new implementation changes there.
