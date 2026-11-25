#pragma once

namespace LLVMBC { class CallInst; }
namespace ShaderRecompiler { class IrBuilder; class IrValue; class IrProgram; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
ShaderRecompiler::IrValue& LowerLaneSwizzle(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder, const ShaderRecompiler::IrProgram& program);
}
