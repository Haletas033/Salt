#ifndef SALT_PREPROCESSOR_H
#define SALT_PREPROCESSOR_H
#include <fstream>
#include <sstream>
#include <string>
#include <map>

class Preprocessor {
private:
        void run();

public:
        std::string source{};
        std::map<std::string, std::string> defines;

        explicit Preprocessor(const std::ifstream& file) {
                std::stringstream buffer{};
                buffer << file.rdbuf();
                source = buffer.str();

                run();
        }
};

#endif //SALT_PREPROCESSOR_H
