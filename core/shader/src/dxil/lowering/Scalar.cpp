#include "Scalar.hpp"
#include "Buffer.hpp"
#include "Local.hpp"
#include "HalfScalar.hpp"
#include <cast.hpp>
#include <stdexcept>
#include <bit>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

ScalarLowering::ScalarLowering(IrProgram& program, IrBuilder& builder) : program(program), builder(builder) {}

void ScalarLowering::SetBuffers(BufferLowering& value) {
    if (buffers) throw std::logic_error("DXIL buffer lowering is already configured");
    buffers = &value;
}

void ScalarLowering::Emit(const LLVMBC::Instruction& instruction) {
    if (LLVMBC::isa<LLVMBC::CompositeConstructInst>(&instruction)) return;
    if (locals && locals->Emit(instruction, *this)) return;
    auto* call = LLVMBC::dyn_cast<LLVMBC::CallInst>(&instruction);
    if (call && buffers && buffers->Emit(*call, *this)) return;
    (void)Get(instruction);
}

void ScalarLowering::SetLocals(LocalLowering& value) {
    if (locals) throw std::logic_error("DXIL local lowering is already configured");
    locals = &value;
}

IrValue& ScalarLowering::Get(const LLVMBC::Value& value) {
    if (auto found = values.find(&value); found != values.end()) return *found->second;
    if (auto* extraction = LLVMBC::dyn_cast<LLVMBC::ExtractValueInst>(&value)) {
        if (auto* constituent = aggregateElement(*extraction)) return Get(*constituent);
    }
    if (auto* calledInstruction = LLVMBC::dyn_cast<LLVMBC::CallInst>(&value)) {
        auto& output = call(*calledInstruction);
        values.emplace(&value, &output);
        return output;
    }
    auto resultType = GetType(value);
    auto bits = resultType == IrType::F32 || resultType == IrType::F16 ? 0u : width(value);
    IrValue* output = nullptr;
    if (auto* loadedInstruction = LLVMBC::dyn_cast<LLVMBC::LoadInst>(&value)) {
        if (!locals) throw std::runtime_error("DXIL local load has no local storage configuration");
        output = &locals->Load(*loadedInstruction, *this);
    } else if (auto* atomicInstruction = LLVMBC::dyn_cast<LLVMBC::AtomicRMWInst>(&value)) {
        if (!locals) throw std::runtime_error("DXIL shared atomic has no local storage configuration");
        output = &locals->Atomic(*atomicInstruction, *this);
    } else if (LLVMBC::isa<LLVMBC::UndefValue>(&value)) {
        if (resultType == IrType::F32) output = &builder.ConstantF32(0);
        else if (resultType == IrType::F16) output = &builder.ConstantF16(0);
        else if (resultType == IrType::Bool) output = &builder.ConstantBool(false);
        else if (resultType == IrType::U64) output = &builder.ConstantU64(0);
        else output = &builder.Constant(0);
    } else if (auto* floatingConstant = LLVMBC::dyn_cast<LLVMBC::ConstantFP>(&value)) {
        auto data = static_cast<std::uint32_t>(floatingConstant->getValueAPF().bitcastToAPInt().getZExtValue());
        if (resultType == IrType::F16) output = &builder.ConstantF16(static_cast<std::uint16_t>(data));
        else if (resultType == IrType::F32) output = &builder.ConstantF32(std::bit_cast<float>(data));
        else throw std::runtime_error("DXIL floating constant type is not implemented");
    } else if (auto* integerConstant = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(&value)) {
        auto data = integerConstant->getUniqueInteger().getZExtValue();
        if (bits == 64) output = &builder.ConstantU64(data);
        else if (bits == 1) output = &builder.ConstantBool((data & 1) != 0);
        else output = &builder.Constant(static_cast<std::uint32_t>(bits == 32 ? data : data & ((1u << bits) - 1)));
    } else if (auto* binaryInstruction = LLVMBC::dyn_cast<LLVMBC::BinaryOperator>(&value)) {
        output = resultType == IrType::F32 ? &floatBinary(*binaryInstruction) : &binary(*binaryInstruction);
    } else if (auto* unaryInstruction = LLVMBC::dyn_cast<LLVMBC::UnaryOperator>(&value)) {
        output = &negate(*unaryInstruction);
    } else if (auto* floatingComparison = LLVMBC::dyn_cast<LLVMBC::FCmpInst>(&value)) {
        output = &floatCompare(*floatingComparison);
    } else if (auto* integerComparison = LLVMBC::dyn_cast<LLVMBC::ICmpInst>(&value)) {
        output = &compare(*integerComparison);
    } else if (auto* conversion = LLVMBC::dyn_cast<LLVMBC::CastInst>(&value)) {
        if (IsHalf(value) || IsHalf(*conversion->getOperand(0))) output = &LowerHalfCast(*conversion, *this, builder);
        else output = resultType == IrType::F32 || conversion->getOperand(0)->getType()->getTypeID() == LLVMBC::Type::TypeID::FloatTyID ? &floatCast(*conversion) : &cast(*conversion);
    } else if (auto* extraction = LLVMBC::dyn_cast<LLVMBC::ExtractValueInst>(&value)) {
        if (buffers) {
            output = buffers->Extract(*extraction, *this);
            if (output) { values.emplace(&value, output); return *output; }
        }
        if (extraction->getNumIndices() != 1) throw std::runtime_error("DXIL nested aggregate extraction is not implemented");
        auto index = extraction->getIndices()[0];
        auto* sourceType = extraction->getAggregateOperand()->getType();
        if (sourceType && sourceType->getTypeID() == LLVMBC::Type::TypeID::StructTyID && (index >= sourceType->getStructNumElements() || sourceType->getStructElementType(index) != extraction->getType())) throw std::runtime_error("DXIL aggregate extraction type differs from its member");
        auto& aggregate = Get(*extraction->getAggregateOperand());
        if (!IsVectorType(aggregate.Type()) || index >= ComponentCount(aggregate.Type()) || (bits != 32 && resultType != IrType::F16)) throw std::runtime_error("DXIL aggregate extraction has an incompatible result type or index");
        output = &builder.CompositeExtract(aggregate, index);
        if (resultType == IrType::F16) {
            if (output->Type() != IrType::U32) throw std::runtime_error("DXIL packed half extraction requires an integer representation");
            output = &builder.BitCastF16(*output);
        }
    } else if (auto* selection = LLVMBC::dyn_cast<LLVMBC::SelectInst>(&value)) {
        if (GetType(*selection->getOperand(0)) != IrType::Bool || GetType(*selection->getOperand(1)) != resultType || GetType(*selection->getOperand(2)) != resultType) throw std::runtime_error("DXIL select operand types do not match");
        auto& condition = Get(*selection->getOperand(0));
        if (condition.Type() != IrType::Bool) throw std::runtime_error("DXIL select requires a scalar boolean condition");
        output = &builder.Select(condition, Get(*selection->getOperand(1)), Get(*selection->getOperand(2)));
    } else {
        auto diagnostic = "DXIL scalar value is not implemented: kind=" + std::to_string(static_cast<unsigned>(value.get_value_kind()));
        throw std::runtime_error(diagnostic);
    }
    if (output->Type() != resultType) throw std::logic_error("DXIL scalar lowering produced an incompatible result type");
    values.emplace(&value, output);
    return *output;
}
