#ifndef ANALYSIS_TEST_SUPPORT_H
#define ANALYSIS_TEST_SUPPORT_H

#include "include/dataflow/solver.h"
#include "include/analyze.h"
#include "include/parser.h"
#include "include/cfg.h"

template<Domain D>
struct AnalysisTestResult
{
    Node root;
    CFG cfg;
    std::vector<D> analysis;
    Annotations anns;
    std::vector<Polygon> covered;
    Parser parser; // Owns the nodes.
};

template <Domain D, AnalysisFunc<D> getAnalysis, AnnotatorFunc<D> AF, VisualizerFunc<D> VF>
std::optional<AnalysisTestResult<D>> analyzeForTest(std::string_view str, std::ostream& output)
{
    DiagnosticEmitter emitter(output, output);
    Lexer lexer(std::string(str), emitter);
    auto tokens = lexer.lexAll();
    if (tokens.empty())
        return {};
    Parser parser(tokens, emitter);
    auto root = parser.parse();
    if (!root)
        return {};
    auto cfg = createCfg(*root);
    auto results = getAnalysis(cfg);
    auto anns = AF(cfg, results);
    auto covered = VF(cfg, results);
    return AnalysisTestResult<D>{*root, std::move(cfg), std::move(results),
                                 std::move(anns), std::move(covered), std::move(parser)};
}

#endif