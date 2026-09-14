#include <fstream>

#include "compiler.h"

int main(const int argc, char **argv) {
        CompilerOptions options = Compiler::parseArgs(argc, argv);

        if (options.inputFile.empty())
                return 0;

        Compiler compiler(options);
}
