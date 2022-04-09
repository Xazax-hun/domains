project('domains', 'cpp',
        version : '0.0.1',
        default_options : ['cpp_std=c++20', 'warning_level=3'])

# Dependencies
fmt_dep = dependency('fmt')

# Libraries
domains_static_sources = ['src/lexer.cpp', 'src/parser.cpp',
                          'src/ast.cpp', 'src/utils.cpp', 'src/eval.cpp', 'src/analysis.cpp']
domains_static_lib = static_library('libslox', domains_static_sources,
                                     dependencies: [fmt_dep])

# Executables
interpreter_sources = ['main.cpp']
executable('domains', interpreter_sources,
           link_with: domains_static_lib,
           dependencies: fmt_dep)

# Tests + test dependencies.
gtest_dep = dependency('gtest')
unittest_sources = ['test/main.cpp']
tests = executable('unittest', unittest_sources,
                   d_unittest: true,
                   install: false,
                   link_with: domains_static_lib,
                   dependencies: gtest_dep)
test('unittests', tests)