#include "include/eval.h"
#include "include/lexer.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <fmt/format.h>

bool runFile(std::string_view filePath, bool dumpAst)
{
    DiagnosticEmitter emitter(std::cout, std::cerr);
    std::ifstream file(filePath.data());
    std::stringstream fileContent;
    fileContent << file.rdbuf();
    Lexer lexer(fileContent.str(), emitter);
    auto tokens = lexer.lexAll();
    for (auto t : tokens)
    {
        fmt::print("{}\n", print(t));
    }
}