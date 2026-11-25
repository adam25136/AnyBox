#pragma once

#include <IntermediateRepresentation/IrProgram.hpp>
#include <optional>

namespace ShaderRecompiler { struct ShaderComputeInputInfo; }

namespace AnyBox::Shader::Dxil::Flow::Predication {
struct Plan {
    ShaderRecompiler::IrBlock* header;
    ShaderRecompiler::IrBlock* positive;
    ShaderRecompiler::IrBlock* negative;
    ShaderRecompiler::IrBlock* merge;
    ShaderRecompiler::IrValue* condition;
};

ShaderRecompiler::BlockInfo& Information(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::IrBlock& block);
std::optional<Plan> Find(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrBlock& header);
bool CanMask(const ShaderRecompiler::IrBlock& block);
void Mask(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrBlock& block, ShaderRecompiler::IrValue& active);
void Apply(ShaderRecompiler::IrProgram& program, const Plan& plan);
std::size_t Run(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input);
}
