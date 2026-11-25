#pragma once

#include <IntermediateRepresentation/IrProgram.hpp>
#include <Optimization/ShaderStageInputInfo.hpp>
#include <optional>
#include <span>
#include <unordered_map>
#include <unordered_set>

namespace AnyBox::Shader::Control {
bool IsUniformArithmetic(ShaderRecompiler::IrOpcode opcode);

class Uniformity {
public:
    Uniformity(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& inputInfo);
    bool Check(const ShaderRecompiler::IrValue& expression);

private:
    using IrValue = ShaderRecompiler::IrValue;
    struct Node {
        bool uniform;
        bool needsControl;
        std::vector<const IrValue*> dependencies;
        std::vector<const IrValue*> users;
    };
    std::optional<std::vector<const IrValue*>> classify(const IrValue& value) const;
    void extend(std::span<const IrValue* const> roots);
    const ShaderRecompiler::ShaderComputeInputInfo& input;
    std::unordered_set<const IrValue*> controlRoots;
    std::vector<const IrValue*> controlUsers;
    bool controlsUniform = true;
    bool phiControlValid = true;
    std::unordered_map<const IrValue*, Node> nodes;
};
}
