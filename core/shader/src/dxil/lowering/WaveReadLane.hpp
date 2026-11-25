#pragma once

namespace LLVMBC { class CallInst; }
namespace ShaderRecompiler { class IrValue; class IrBuilder; class IrProgram; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
ShaderRecompiler::IrValue& LowerWaveReadLane(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder, const ShaderRecompiler::IrProgram& program, unsigned bits, bool floating = false);
}
