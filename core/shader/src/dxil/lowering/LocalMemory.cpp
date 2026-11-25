#include "Local.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <limits>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::LocalLowering;

bool LocalLowering::Emit(const LLVMBC::Instruction& instruction, ScalarLowering& scalar) {
    if (LLVMBC::isa<LLVMBC::CastInst>(&instruction) && instruction.getType() && instruction.getType()->getTypeID() == LLVMBC::Type::TypeID::PointerTyID) {
        (void)resolve(instruction, scalar);
        return true;
    }
    if (LLVMBC::isa<LLVMBC::AllocaInst>(&instruction) || LLVMBC::isa<LLVMBC::GetElementPtrInst>(&instruction)) {
        (void)resolve(instruction, scalar);
        return true;
    }
    if (auto* stored = LLVMBC::dyn_cast<LLVMBC::StoreInst>(&instruction)) {
        store(*stored, scalar);
        return true;
    }
    return false;
}

LocalLowering::Access LocalLowering::access(const Pointer& pointer, const LLVMBC::Type& type) {
    if (&type != pointer.element || (type.getTypeID() != LLVMBC::Type::TypeID::FloatTyID && (!type.isIntegerTy() || (type.getIntegerBitWidth() != 32 && type.getIntegerBitWidth() != 64)))) throw std::runtime_error("DXIL local memory access requires a matching float32, int32 or int64 element");
    auto bytes = size(type);
    if (pointer.allocation.bytes < bytes) throw std::out_of_range("DXIL local memory element exceeds its allocation");
    IrValue* active = &builder.ConstantBool(true);
    if (pointer.constant) {
        if (*pointer.constant < 0 || static_cast<std::uint64_t>(*pointer.constant) > pointer.allocation.bytes - bytes) throw std::out_of_range("DXIL constant local memory access is outside its allocation");
    } else {
        auto& outside = builder.Emit(IrOpcode::UGreaterThan64, IrType::Bool, {pointer.offset, &builder.ConstantU64(pointer.allocation.bytes - bytes)});
        auto& low = builder.CompositeExtract(*pointer.offset, 0);
        auto& high = builder.CompositeExtract(*pointer.offset, 1);
        (void)builder.Emit(IrOpcode::DescriptorFault, IrType::Void, {&outside, &low, &high, &builder.Constant(0)});
        program.Info().usesFaultBuffer = true;
        active = &builder.LogicalNot(outside);
    }
    auto& offset = builder.IAdd(builder.Constant(pointer.allocation.base), builder.CompositeExtract(*pointer.offset, 0));
    return {offset, *active};
}

MemoryFlags LocalLowering::flags(bool shared, unsigned words) {
    auto& metadata = program.Resources().memoryInfo;
    if (metadata.size() >= std::numeric_limits<std::uint32_t>::max()) throw std::length_error("DXIL local memory operation count exceeds the IR limit");
    MemoryFlags result{static_cast<std::uint32_t>(metadata.size()), 0};
    MemoryInfo memory;
    memory.kind = shared ? ResourceKind::Lds : ResourceKind::Scratch;
    memory.componentCount = memory.dataDwords = words;
    metadata.push_back(memory);
    return result;
}

IrValue& LocalLowering::Load(const LLVMBC::LoadInst& instruction, ScalarLowering& scalar) {
    auto pointer = resolve(*instruction.getPointerOperand(), scalar);
    auto location = access(pointer, *instruction.getType());
    bool wide = instruction.getType()->isIntegerTy() && instruction.getType()->getIntegerBitWidth() == 64;
    if (pointer.allocation.constants) {
        auto& value = constantWord(pointer, location);
        if (wide) return builder.ConstructU64(value, constantWord(pointer, location, 1));
        return instruction.getType()->getTypeID() == LLVMBC::Type::TypeID::FloatTyID ? builder.BitCastF32(value) : value;
    }
    auto type = wide ? IrType::Vec2U32 : IrType::U32;
    IrValue* value;
    if (pointer.allocation.shared) {
        value = &builder.Emit(wide ? IrOpcode::LoadSharedU32x2 : IrOpcode::LoadSharedU32, type, {&location.offset, &location.active}, flags(true, wide ? 2 : 1));
    } else {
        auto& resource = builder.Emit(IrOpcode::GetScratchResource, IrType::AddressResource, {});
        value = &builder.Emit(IrOpcode::LoadAddressU32, IrType::U32, {&resource, &location.offset, &builder.Constant(0), &location.active}, flags(false));
        if (wide) {
            auto& highOffset = builder.IAdd(location.offset, builder.Constant(4));
            auto& high = builder.Emit(IrOpcode::LoadAddressU32, IrType::U32, {&resource, &highOffset, &builder.Constant(0), &location.active}, flags(false));
            return builder.ConstructU64(*value, high);
        }
    }
    if (wide) return builder.ConstructU64(builder.CompositeExtract(*value, 0), builder.CompositeExtract(*value, 1));
    return instruction.getType()->getTypeID() == LLVMBC::Type::TypeID::FloatTyID ? builder.BitCastF32(*value) : *value;
}

void LocalLowering::store(const LLVMBC::StoreInst& instruction, ScalarLowering& scalar) {
    auto* source = instruction.getOperand(0);
    auto pointer = resolve(*instruction.getOperand(1), scalar);
    if (pointer.allocation.constants) throw std::runtime_error("DXIL cannot write a constant global variable");
    auto location = access(pointer, *source->getType());
    auto& input = scalar.Get(*source);
    bool wide = input.Type() == IrType::U64;
    auto& bits = input.Type() == IrType::F32 ? builder.BitCastU32(input) : input;
    if (pointer.allocation.shared) {
        if (wide) (void)builder.Emit(IrOpcode::WriteSharedU32x2, IrType::Void, {&location.offset, &builder.CompositeExtract(bits, 0), &builder.CompositeExtract(bits, 1), &location.active}, flags(true, 2));
        else (void)builder.Emit(IrOpcode::WriteSharedU32, IrType::Void, {&location.offset, &bits, &location.active}, flags(true));
        return;
    }
    auto& resource = builder.Emit(IrOpcode::GetScratchResource, IrType::AddressResource, {});
    auto& low = wide ? builder.CompositeExtract(bits, 0) : bits;
    (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&resource, &location.offset, &builder.Constant(0), &low, &location.active}, flags(false));
    if (wide) {
        auto& highOffset = builder.IAdd(location.offset, builder.Constant(4));
        (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&resource, &highOffset, &builder.Constant(0), &builder.CompositeExtract(bits, 1), &location.active}, flags(false));
    }
}
