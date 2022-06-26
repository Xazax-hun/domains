#ifndef ANALYSIS_TEST_SUPPORT_H
#define ANALYSIS_TEST_SUPPORT_H

#include "include/dataflow/solver.h"
#include "include/analyze.h"
#include "include/parser.h"
#include "include/cfg.h"

template<Domain D>
struct AnalysisTestResult
{
    ASTContext context;
    CFG cfg;
    std::vector<D> analysis;
    Annotations anns;
    std::vector<Polygon> covered;
};

template <CfgConcept Cfg, Domain D, AnalysisFunc<D, Cfg> getAnalysis,
          AnnotatorFunc<D, Cfg> AF, VisualizerFunc<D, Cfg> VF>
std::optional<AnalysisTestResult<D>> analyzeForTest(std::string_view str, std::ostream& output)
{
    DiagnosticEmitter emitter(output, output);
    Lexer lexer(std::string(str), emitter);
    auto tokens = lexer.lexAll();
    if (tokens.empty())
        return {};
    Parser parser(tokens, emitter);
    auto ctxt = parser.parse();
    if (!ctxt)
        return {};
    auto cfg = CFG::createCfg(ctxt->getRoot());
    auto results = getAnalysis(cfg);
    if (results.empty())
        return AnalysisTestResult<D>{std::move(*ctxt), std::move(cfg), {}, {}, {}};
    auto anns = AF(cfg, results);
    auto covered = VF(cfg, results);
    return AnalysisTestResult<D>{std::move(*ctxt), std::move(cfg), std::move(results),
                                 std::move(anns), std::move(covered)};
}

#endif