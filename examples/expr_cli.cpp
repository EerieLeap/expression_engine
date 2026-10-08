// Compiles an expression, prints its bytecode and evaluates it.
//
//   expr_cli "x * 4 + 1.6" x=2
//   expr_cli "(x >> 8) & 0xFF" x=4660

#include <charconv>
#include <cstdio>
#include <string_view>
#include <vector>

#include <eerie_leap/expression_engine/expression_engine.hpp>

namespace ee = eerie_leap::expression_engine;

namespace {

struct Binding {
    std::string_view name;
    ee::Value value;
};

bool ParseBinding(std::string_view argument, Binding& binding) {
    const auto equals = argument.find('=');
    if(equals == std::string_view::npos || equals == 0) {
        return false;
    }
    binding.name = argument.substr(0, equals);
    const std::string_view number = argument.substr(equals + 1);
    const auto [ptr, ec] = std::from_chars(number.data(), number.data() + number.size(), binding.value);
    return ec == std::errc{} && ptr == number.data() + number.size();
}

} // namespace

int main(int argc, char** argv) {
    if(argc < 2) {
        std::fputs("usage: expr_cli <expression> [name=value ...]\n", stderr);
        return 2;
    }

    const std::string_view text{argv[1]};
    std::vector<Binding> bindings;
    for(int i = 2; i < argc; ++i) {
        Binding binding{};
        if(!ParseBinding(argv[i], binding)) {
            std::fprintf(stderr, "bad binding: %s\n", argv[i]);
            return 2;
        }
        bindings.push_back(binding);
    }

    const auto program = ee::Compile(text);
    if(!program) {
        const ee::CompileError& error = program.error();
        std::fprintf(
            stderr,
            "error: %.*s at %u\n%.*s\n%*s^\n",
            static_cast<int>(ee::Describe(error.code).size()),
            ee::Describe(error.code).data(),
            static_cast<unsigned>(error.position),
            static_cast<int>(text.size()),
            text.data(),
            static_cast<int>(error.position),
            ""
        );
        return 1;
    }

    ee::Disassemble(*program, [](std::string_view line) {
        std::printf("%.*s\n", static_cast<int>(line.size()), line.data());
    });

    std::vector<ee::Value> variables(program->VariableCount(), 0.0f);
    for(std::size_t index = 0; index < program->VariableCount(); ++index) {
        const std::string_view name = program->VariableName(index);
        bool bound = false;
        for(const Binding& binding : bindings) {
            if(binding.name == name) {
                variables[index] = binding.value;
                bound = true;
            }
        }
        if(!bound) {
            std::fprintf(stderr, "unbound variable: %.*s\n", static_cast<int>(name.size()), name.data());
            return 1;
        }
    }

    std::printf("= %.9g\n", static_cast<double>(program->Evaluate(variables)));
    return 0;
}
