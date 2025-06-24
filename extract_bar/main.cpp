#include <iostream>
#include "file_proc.h"

int main(int argc, char* argv[]) {
	if (argc > 1) {
		FileProcessing _proc;
		return _proc.proc(argv[1]);
	}
	return 1;
}
