#pragma once

#include <IntermediateRepresentation/IrBuilder.hpp>
#include <instruction.hpp>
#include <type.hpp>
#include <unordered_map>

namespace AnyBox::Shader::Dxil {
class BufferLowering;
class LocalLowering;
class ScalarLowering {
public:
    ScalarLowering(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrBuilder& builder);
    ShaderRecompiler::IrValue& Get(const LLVMBC::Value& value);
    void Bind(const LLVMBC::Value& source, ShaderRecompiler::IrValue& value);
    void SetBuffers(BufferLowering& buffers);
    void SetLocals(LocalLowering& locals);
    void Emit(const LLVMBC::Instruction& instruction);
    static ShaderRecompiler::IrType GetType(const LLVMBC::Value& value);

private:
    using IrValue = ShaderRecompiler::IrValue;
    static unsigned width(const LLVMBC::Value& value);
    static const LLVMBC::Value* aggregateElement(const LLVMBC::ExtractValueInst& instruction);
    IrValue& unsignedValue(IrValue& value);
    IrValue& narrow(IrValue& value, unsigned bits);
    IrValue& signedValue(IrValue& value, unsigned bits);
    IrValue& binary(const LLVMBC::BinaryOperator& instruction);
    IrValue& wideBinary(const LLVMBC::BinaryOperator& instruction);
    IrValue& wideCompare(const LLVMBC::ICmpInst& instruction);
    IrValue& compare(const LLVMBC::ICmpInst& instruction);
    IrValue& cast(const LLVMBC::CastInst& instruction);
    IrValue& call(const LLVMBC::CallInst& instruction);
    IrValue& barrier(const LLVMBC::CallInst& instruction);
    IrValue& globalLoad(const LLVMBC::CallInst& instruction, unsigned words);
    IrValue& globalFloatAtomic(const LLVMBC::CallInst& instruction, bool maximum);
    IrValue& thread(const LLVMBC::CallInst& instruction, unsigned opcode);
    IrValue& waveFirst(const LLVMBC::CallInst& instruction);
    IrValue& waveBuiltin(const LLVMBC::CallInst& instruction, unsigned opcode);
    IrValue& waveCount(const LLVMBC::CallInst& instruction);
    IrValue& waveReduction(const LLVMBC::CallInst& instruction);
    IrValue& waveInteger(const LLVMBC::CallInst& instruction, bool prefix);
    IrValue& bitField(const LLVMBC::CallInst& instruction, bool insert);
    IrValue& unaryBits(const LLVMBC::CallInst& instruction);
    IrValue& legacyHalf(const LLVMBC::CallInst& instruction);
    IrValue& floatBinaryIntrinsic(const LLVMBC::CallInst& instruction);
    IrValue& floatTernaryIntrinsic(const LLVMBC::CallInst& instruction);
    IrValue& floatBinary(const LLVMBC::BinaryOperator& instruction);
    IrValue& floatCompare(const LLVMBC::FCmpInst& instruction);
    IrValue& floatCast(const LLVMBC::CastInst& instruction);
    IrValue& negate(const LLVMBC::UnaryOperator& instruction);
    IrValue& divide(IrValue& left, IrValue& right, bool isSigned, bool remainder, unsigned bits);
    ShaderRecompiler::IrProgram& program;
    ShaderRecompiler::IrBuilder& builder;
    BufferLowering* buffers = nullptr;
    LocalLowering* locals = nullptr;
    std::unordered_map<const LLVMBC::Value*, IrValue*> values;
};
}
