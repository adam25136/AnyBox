#pragma once

namespace LLVMBC { class CallInst; }
namespace ShaderRecompiler { class IrValue; class IrBuilder; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
ShaderRecompiler::IrValue& LowerFloatToHalf(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder);
}
