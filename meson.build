project('domains', 'cpp',
        version : '0.0.2',
        default_options : ['cpp_std=c++20', 'warning_level=3'])

# Dependencies
fmt_dep = dependency('fmt')
cairo_dep = dependency('cairo', required: false)

# Compiler arguments
if cairo_dep.found()
  add_project_arguments('-DHAVE_CAIRO', language : 'cpp')
endif

# Embedding version number.
version_dep = vcs_tag(input: 'src/version.cpp.in',
                      output: 'version.cpp')

# Libraries
domains_static_sources = [version_dep, 'src/lexer.cpp', 'src/parser.cpp',
                          'src/ast.cpp', 'src/utils.cpp', 'src/eval.cpp',
                          'src/cfg.cpp', 'src/render.cpp', 'src/dataflow/sign_analysis.cpp',
                          'src/dataflow/interval_analysis.cpp',
                          'src/dataflow/reachable_operations_analysis.cpp',
                          'src/analyze.cpp']
domains_static_lib = static_library('libslox', domains_static_sources,
                                     dependencies: [fmt_dep, cairo_dep])

# Executables
interpreter_sources = ['main.cpp']
executable('domains', interpreter_sources,
           link_with: domains_static_lib,
           dependencies: fmt_dep)

# Tests + test dependencies.
gtest_dep = dependency('gtest')
unittest_sources = ['unittest/main.cpp', 'unittest/lexer.cpp', 'unittest/parser.cpp',
                    'unittest/cfg.cpp', 'unittest/eval.cpp', 'unittest/dataflow/domains.cpp',
                    'unittest/dataflow/sign_analysis.cpp', 'unittest/dataflow/interval_analysis.cpp',
                    'unittest/dataflow/reachable_op_analysis.cpp']
tests = executable('unittest', unittest_sources,
                   d_unittest: true,
                   install: false,
                   link_with: domains_static_lib,
                   dependencies: gtest_dep)
test('unittests', tests)