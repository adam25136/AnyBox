#pragma once

namespace LLVMBC { class CallInst; }
namespace ShaderRecompiler { class IrValue; class IrBuilder; class IrProgram; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
ShaderRecompiler::IrValue& LowerWaveBitCount(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder, const ShaderRecompiler::IrProgram& program, bool prefix);
}
