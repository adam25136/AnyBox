#pragma once

#include <array>

namespace LLVMBC { class CallInst; }
namespace ShaderRecompiler { class IrBuilder; class IrValue; class IrProgram; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
ShaderRecompiler::IrValue& LowerBitCount64(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder);
std::array<ShaderRecompiler::IrValue*, 2> ReadBitCountWords(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder, unsigned opcode);
ShaderRecompiler::IrValue& LowerMaskBitCount64(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder, const ShaderRecompiler::IrProgram& program);
}
