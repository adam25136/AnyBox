#pragma once

namespace LLVMBC { class CallInst; }
namespace ShaderRecompiler { class IrBuilder; class IrValue; class IrProgram; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
ShaderRecompiler::IrValue& LowerBallot64(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder, ShaderRecompiler::IrProgram& program);
}
