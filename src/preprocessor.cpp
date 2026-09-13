#include <preprocessor.h>

void Preprocessor::run() {
        std::istringstream stream(source);
        std::string line;
        std::string output;

        while (std::getline(stream, line)) {
                const size_t start = line.find_first_not_of(" \t");
                if (start != std::string::npos && line.substr(start, 7) == "#define") {
                        std::istringstream lineStream(line.substr(start + 7));
                        std::string name, value;
                        lineStream >> name;
                        lineStream >> value;
                        defines[name] = value;
                } else {
                        output += line + '\n';
                }
        }

        for (const auto& [name, value] : defines) {
                size_t pos = 0;
                while ((pos = output.find(name, pos)) != std::string::npos) {
                        output.replace(pos, name.size(), value);
                        pos += value.size();
                }
        }

        source = output;
}
