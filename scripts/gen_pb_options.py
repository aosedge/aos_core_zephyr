#!/usr/bin/python3

import argparse
import re
import sys

parser = argparse.ArgumentParser()

parser.add_argument("file", help="input file")
parser.add_argument("-o", dest="output_file", help="output file")

args = parser.parse_args()

with open(args.file, encoding="utf-8") as file:
    input_lines = [line.rstrip() for line in file]

with open(args.output_file, "w", encoding="utf-8") if args.output_file else sys.stdout as file:
    file.write(
        """#include <iostream>
#include <fstream>
#include <regex>
#include <string>

"""
    )

    # write required headers

    file.writelines([line + "\n" for line in input_lines if line.strip().startswith("#include")])
    file.write("\n")

    # GetVarValue function

    file.write("static size_t GetVarValue(const std::string& name)\n{")

    for line in input_lines:
        # remove comments
        line = line.split("#")[0].split("//")[0]
        if len(line) == 0:
            continue

        for config in re.findall(r"\${([^}]+)}", line):
            file.write('    if (name == "{}") {{\n'.format(config))
            file.write("        return {};\n".format(config))
            file.write("    }\n\n")

    file.write('    throw std::runtime_error(name + " not found");\n')
    file.write("}\n\n")

    file.write(
        """bool StartsWith(const std::string& str, const std::string& prefix)
{
    return str.compare(0, prefix.length(), prefix) == 0;
}

static std::string TrimWhiteSpaces(const std::string& line)
{
    auto start = line.find_first_not_of(" \\t\\n\\r");
    if (start == std::string::npos) {
        return "";
    }

    auto end = line.find_last_not_of(" \\t\\n\\r");

    return line.substr(start, end - start + 1);
}

static std::string RemoveComments(const std::string& line)
{
    auto result = line;

    auto pos = result.find("#");
    if (pos != std::string::npos) {
        result = result.substr(0, pos);
    }

    pos = result.find("//");
    if (pos != std::string::npos) {
        result = result.substr(0, pos);
    }

    return result;
}

static std::string ProcessLine(const std::string& line)
{
    auto        woComment = RemoveComments(line);
    std::regex  pattern(R"(\$\{([^}]*)\})");
    std::string result;

    auto begin = std::sregex_iterator(woComment.begin(), woComment.end(), pattern);
    auto end   = std::sregex_iterator();
    auto pos   = 0;

    for (auto i = begin; i != end; i++) {
        std::smatch match = *i;

        result += line.substr(pos, match.position(0) - pos) + std::to_string(GetVarValue(match[1]));
        pos = match.position(0) + match.length();
    }

    return result + line.substr(pos, line.size() - pos + 1);
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }

    std::ifstream inputFile(argv[1]);
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file: " << argv[1] << std::endl;
        return 1;
    }

    std::ofstream outputFile(argv[2]);
    if (!outputFile.is_open()) {
        std::cerr << "Error creating file: " << argv[2] << std::endl;
        return 1;
    }

    std::string line;

    while (std::getline(inputFile, line)) {
        auto tmpLine = TrimWhiteSpaces(line);

        if (StartsWith(tmpLine, "#include")) {
            continue;
        }

        outputFile << ProcessLine(tmpLine) << std::endl;
    }

    inputFile.close();
    outputFile.close();

    return 0;
}
"""
    )

    if file is not sys.stdout:
        file.close()
