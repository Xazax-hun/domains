#include <fmt/format.h>

#include <string_view>
#include <iostream>
#include <fstream>
#include <sstream>

#include "include/eval.h"
#include "include/cfg.h"
#include "include/parser.h"
#include "include/render.h"
#include "include/analyze.h"

using namespace std::literals;

bool runFile(std::string_view filePath, const std::optional<std::string>& analysisName, bool dumpCfg, bool svg)
{
    DiagnosticEmitter emitter(std::cout, std::cerr);
    std::ifstream file(filePath.data());
    std::stringstream fileContent;
    fileContent << file.rdbuf();
    Lexer lexer(std::move(fileContent).str(), emitter);
    auto tokens = lexer.lexAll();
    if (tokens.empty())
        return false;
    Parser parser(tokens, emitter);
    auto root = parser.parse();
    if (!root)
        return false;
    CFG cfg = createCfg(*root);
    if (dumpCfg)
        std::cout << print(cfg);
    Walk w = createRandomWalk(cfg);
    if (w.empty())
        return false;

    if (svg)
        std::cout << renderRandomWalkSVG(w) << "\n";
    else
    {
        for (auto step : w)
            std::cout << step.pos.x << " " << step.pos.y << "\n";
    }
    if (analysisName)
    {
        auto annotations = getAnalysisResults(*analysisName, cfg);
        if (!annotations)
            return false;

        std::cout << print(*root, *annotations) << "\n";
    }
    return true;
}

int main(int argc, const char* argv[])
{
    auto printHelp = [&]()
    {
        fmt::print("Usage: {} script [options]\n", argv[0]);
        fmt::print("Options:\n");
        fmt::print("  --cfg-dump\n");
        fmt::print("  --svg\n");
        fmt::print("  --analyze ANALYSIS_NAME\n");
        fmt::print("  --help\n");
        fmt::print("Available analyses:\n");
        for (const auto& analysis : getListOfAnalyses())
        {
            fmt::print("  {}\n", analysis);
        }
    };

    const char *file = nullptr;
    bool dumpCfg = false;
    bool svg = false;
    std::optional<std::string> analysisName;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            // Process flags.
            if (argv[i] == "--cfg-dump"sv)
            {
                dumpCfg = true;
                continue;
            }
            if (argv[i] == "--svg"sv)
            {
                svg = true;
                continue;
            }
            if (argv[i] == "--help"sv)
            {
                printHelp();
                return EXIT_SUCCESS;
            }
            if (argv[i] == "--analyze"sv)
            {
                if (i == argc - 1 || argv[i+1][0] == '-')
                {
                    fmt::print("Analysis name was not provided.");
                    return EXIT_FAILURE;
                }
                analysisName = argv[i+1];
                ++i;
                continue;
            }

            printHelp();
            return EXIT_FAILURE;
        }
        if (!file)
        {
            file = argv[i];
            continue;
        }

        printHelp();
        return EXIT_FAILURE;
    }

    if (!file)
    {
        fmt::print("error: input file not specified.\n");
        return EXIT_FAILURE;
    }


    return runFile(file, analysisName, dumpCfg, svg) ? EXIT_SUCCESS : EXIT_FAILURE;
}