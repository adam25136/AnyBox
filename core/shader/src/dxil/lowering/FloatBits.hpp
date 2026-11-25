#pragma once

namespace LLVMBC { class CallInst; }
namespace ShaderRecompiler { class IrBuilder; class IrValue; }

namespace AnyBox::Shader::Dxil {
class ScalarLowering;
ShaderRecompiler::IrValue& FlushFloat32(ShaderRecompiler::IrBuilder& builder, ShaderRecompiler::IrValue& value);
ShaderRecompiler::IrValue& ReciprocalSqrt32(ShaderRecompiler::IrBuilder& builder, ShaderRecompiler::IrValue& value);
ShaderRecompiler::IrValue& LowerUnaryFloat(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder);
ShaderRecompiler::IrValue& LowerCopySign(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, ShaderRecompiler::IrBuilder& builder);
}
