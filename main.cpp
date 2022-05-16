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

namespace
{
struct Config
{
    bool dumpCfg = false;
    bool svg = false;
    int iterations = 1;
    std::optional<std::string> analysisName;
};

bool runFile(std::string_view filePath, Config config)
{
    DiagnosticEmitter emitter(std::cout, std::cerr);
    std::ifstream file(filePath.data());
    if (!file)
    {
        fmt::print("Unable to open file '{}'.\n", filePath);
        return false;
    }
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
    if (config.dumpCfg)
        fmt::print("{}\n", print(cfg));
    if (config.analysisName)
    {
        auto annotations = getAnalysisResults(*config.analysisName, cfg);
        if (!annotations)
            return false;

        fmt::print("{}\n", print(*root, *annotations));
        return true;
    }

    std::vector<Walk> walks;
    for (int i = 0; i < config.iterations; ++i)
    {
        if (config.iterations > 1)
            fmt::print("{}. execution:\n", i + 1);
        walks.push_back(createRandomWalk(cfg));
        if (walks.back().empty())
            return false;
        else
        {
            for (auto step : walks.back())
                fmt::print("{{ x: {}, y: {} }}\n", step.pos.x, step.pos.y);
        }
    }
    if (config.svg)
        fmt::print("{}\n", renderRandomWalkSVG(walks));
    return true;
}
} // anonymous

int main(int argc, const char* argv[])
{
    auto printHelp = [&]()
    {
        fmt::print("Usage: {} script [options]\n", argv[0]);
        fmt::print("Options:\n");
        fmt::print("  --cfg-dump\n");
        fmt::print("  --svg\n");
        fmt::print("  --executions NUMBER\n");
        fmt::print("  --analyze ANALYSIS_NAME\n");
        fmt::print("  --help\n");
        fmt::print("Available analyses:\n");
        for (const auto& analysis : getListOfAnalyses())
        {
            fmt::print("  {}\n", analysis);
        }
    };

    std::string_view file;
    Config config;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            // Process flags.
            if (argv[i] == "--cfg-dump"sv)
            {
                config.dumpCfg = true;
                continue;
            }
            if (argv[i] == "--svg"sv)
            {
                config.svg = true;
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
                config.analysisName = argv[i+1];
                ++i;
                continue;
            }
            if (argv[i] == "--executions"sv)
            {
                if (i == argc - 1 || argv[i+1][0] == '-')
                {
                    fmt::print("Execution count was not provided.");
                    return EXIT_FAILURE;
                }
                bool fail = false;
                std::size_t pos{};
                try
                {
                    config.iterations = std::stoi(argv[i+1], &pos);
                }
                catch(...)
                {
                    fail = true;
                }
                if (fail || config.iterations < 1 || pos != strlen(argv[i+1]))
                {
                    fmt::print("Invalid execution count.");
                    return EXIT_FAILURE;
                }
                ++i;
                continue;
            }

            printHelp();
            return EXIT_FAILURE;
        }
        if (file.empty())
        {
            file = argv[i];
            continue;
        }

        printHelp();
        return EXIT_FAILURE;
    }

    if (file.empty())
    {
        fmt::print("error: input file not specified.\n");
        printHelp();
        return EXIT_FAILURE;
    }

    return runFile(file, config) ? EXIT_SUCCESS : EXIT_FAILURE;
}
