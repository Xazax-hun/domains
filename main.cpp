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
    bool dotsOnly = false;
    int iterations = 1;
    int loopiness = 1;
    std::optional<std::string> analysisName;
};

bool runFile(std::string_view filePath, Config config)
{
    DiagnosticEmitter emitter(std::cout, std::cerr);
    std::ifstream file(filePath.data());
    if (!file)
    {
        fmt::print(stderr, "Unable to open file '{}'.\n", filePath);
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
    Annotations annotations;
    std::vector<Polygon> covered;
    if (config.analysisName)
    {
        auto analysisResult = getAnalysisResults(*config.analysisName, cfg);
        if (!analysisResult)
        {
            fmt::print(stderr, "Failed to run analysis '{}'.\n", *config.analysisName);
            return false;
        }
        annotations = std::move(analysisResult->annotations);
        covered = std::move(analysisResult->covered);
    }

    std::vector<Walk> walks;
    for (int i = 0; i < config.iterations; ++i)
    {
        walks.push_back(createRandomWalk(cfg, config.loopiness));
        if (walks.back().empty())
            return false;
        if (!config.svg)
        {
            if (config.iterations > 1)
                fmt::print("{}. execution:\n", i + 1);
            for (auto step : walks.back())
                fmt::print("{{ x: {}, y: {} }}\n", step.pos.x, step.pos.y);
        }
    }
    if (config.svg)
        fmt::print("{}\n", renderRandomWalkSVG(walks, covered, config.dotsOnly));
    else if (config.analysisName)
        fmt::print("{}\n", print(*root, annotations));
    return true;
}

std::optional<int> toInt(const std::string& str)
{
    try
    {
        std::size_t pos{};
        int ret = std::stoi(str, &pos);
        if (pos != str.size())
            return {};
        return ret;
    }
    catch(...)
    {
        return {};
    }
}
} // anonymous

int main(int argc, const char* argv[])
{
    auto printHelp = [=]()
    {
        fmt::print("Usage: {} script [options]\n", argv[0]);
        fmt::print("Options:\n");
        fmt::print("  --cfg-dump\n");
        fmt::print("  --svg\n");
        fmt::print("  --dots-only\n");
        fmt::print("  --executions NUMBER\n");
        fmt::print("  --loopiness NUMBER\n");
        fmt::print("  --analyze ANALYSIS_NAME\n");
        fmt::print("  --help\n");
        fmt::print("Available analyses:\n");
        for (const auto& analysis : getListOfAnalyses())
            fmt::print("  {}\n", analysis);
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
            if (argv[i] == "--dots-only"sv)
            {
                config.dotsOnly = true;
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
                    fmt::print(stderr, "Analysis name was not provided.");
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
                    fmt::print(stderr, "Execution count was not provided.");
                    return EXIT_FAILURE;
                }
                auto nextNum = toInt(argv[i+1]);
                if (!nextNum || *nextNum < 1)
                {
                    fmt::print(stderr, "Invalid execution count.");
                    return EXIT_FAILURE;
                }
                config.iterations = *nextNum;
                ++i;
                continue;
            }
            if (argv[i] == "--loopiness"sv)
            {
                if (i == argc - 1 || argv[i+1][0] == '-')
                {
                    fmt::print(stderr, "Loopiness was not provided.");
                    return EXIT_FAILURE;
                }
                auto nextNum = toInt(argv[i+1]);
                if (!nextNum || *nextNum < 1)
                {
                    fmt::print(stderr, "Invalid loopiness.");
                    return EXIT_FAILURE;
                }
                config.loopiness = *nextNum;
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

        fmt::print(stderr, "error: multiple input files specified.\n");
        printHelp();
        return EXIT_FAILURE;
    }

    if (config.dotsOnly && !config.svg)
        fmt::print(stderr, "warning: --dots-only is redundant without --svg.\n");

    if (file.empty())
    {
        fmt::print(stderr, "error: input file not specified.\n");
        printHelp();
        return EXIT_FAILURE;
    }

    return runFile(file, config) ? EXIT_SUCCESS : EXIT_FAILURE;
}
