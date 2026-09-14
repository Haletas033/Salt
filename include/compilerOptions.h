#ifndef SALT_COMPILEROPTIONS_H
#define SALT_COMPILEROPTIONS_H

#include <string>

struct CompilerOptions {
        std::string inputFile;
        std::string outputFile = "output";
        bool emitIR = false;
        bool emitTokens = false;
        bool isLibrary = false;
        bool isDependency = false;
};

#endif //SALT_COMPILEROPTIONS_H
