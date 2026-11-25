#pragma once

#include "Plan.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>

namespace AnyBox::Shader::Dxil::Flow {
class Builder {
public:
    explicit Builder(ShaderRecompiler::IrProgram& source);
    ShaderRecompiler::IrProgram Run();

private:
    using IrValue = ShaderRecompiler::IrValue;
    using IrBlock = ShaderRecompiler::IrBlock;
    struct State {
        IrBlock* block = nullptr;
        const Origin* origin = nullptr;
        std::unordered_map<const IrValue*, IrValue*> definitions;
        std::unordered_map<const IrValue*, IrValue*> entries;
        std::vector<std::pair<const IrValue*, IrValue*>> instructions;
    };
    void createBlocks();
    void createInstructions();
    void connectInstructions();
    void connectConditions();
    IrValue& constant(const IrValue& value);
    IrValue& operand(const IrValue& value, State& state);
    IrValue& entry(const IrValue& variable, State& state);
    IrValue& exit(const IrValue& variable, State& state);
    IrValue& phi(const IrValue& variable, State& state);
    IrValue& undefinedEntry(const IrValue& variable, State& state);
    ShaderRecompiler::IrProgram& source;
    Plan plan;
    ShaderRecompiler::IrProgram output;
    std::vector<State> states;
    std::unordered_map<const IrBlock*, std::uint32_t> ids;
    std::unordered_map<const IrValue*, IrValue*> constants;
};
}
