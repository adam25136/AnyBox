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
    if (&type != pointer.element || (type.getTypeID() != LLVMBC::Type::TypeID::FloatTyID && (!type.isIntegerTy() || type.getIntegerBitWidth() != 32))) throw std::runtime_error("DXIL local memory access requires a matching float32 or int32 element");
    IrValue* active = &builder.ConstantBool(true);
    if (pointer.constant) {
        if (*pointer.constant < 0 || static_cast<std::uint64_t>(*pointer.constant) > pointer.allocation.bytes - 4) throw std::out_of_range("DXIL constant local memory access is outside its allocation");
    } else {
        auto& outside = builder.Emit(IrOpcode::UGreaterThan64, IrType::Bool, {pointer.offset, &builder.ConstantU64(pointer.allocation.bytes - 4)});
        auto& low = builder.CompositeExtract(*pointer.offset, 0);
        auto& high = builder.CompositeExtract(*pointer.offset, 1);
        (void)builder.Emit(IrOpcode::DescriptorFault, IrType::Void, {&outside, &low, &high, &builder.Constant(0)});
        program.Info().usesFaultBuffer = true;
        active = &builder.LogicalNot(outside);
    }
    auto& offset = builder.IAdd(builder.Constant(pointer.allocation.base), builder.CompositeExtract(*pointer.offset, 0));
    return {offset, *active};
}

MemoryFlags LocalLowering::flags(bool shared) {
    auto& metadata = program.Resources().memoryInfo;
    if (metadata.size() >= std::numeric_limits<std::uint32_t>::max()) throw std::length_error("DXIL local memory operation count exceeds the IR limit");
    MemoryFlags result{static_cast<std::uint32_t>(metadata.size()), 0};
    MemoryInfo memory;
    memory.kind = shared ? ResourceKind::Lds : ResourceKind::Scratch;
    metadata.push_back(memory);
    return result;
}

IrValue& LocalLowering::Load(const LLVMBC::LoadInst& instruction, ScalarLowering& scalar) {
    auto pointer = resolve(*instruction.getPointerOperand(), scalar);
    auto location = access(pointer, *instruction.getType());
    if (pointer.allocation.shared) {
        auto& value = builder.Emit(IrOpcode::LoadSharedU32, IrType::U32, {&location.offset, &location.active}, flags(true));
        return instruction.getType()->getTypeID() == LLVMBC::Type::TypeID::FloatTyID ? builder.BitCastF32(value) : value;
    }
    auto& resource = builder.Emit(IrOpcode::GetScratchResource, IrType::AddressResource, {});
    auto& value = builder.Emit(IrOpcode::LoadAddressU32, IrType::U32, {&resource, &location.offset, &builder.Constant(0), &location.active}, flags(false));
    return instruction.getType()->getTypeID() == LLVMBC::Type::TypeID::FloatTyID ? builder.BitCastF32(value) : value;
}

void LocalLowering::store(const LLVMBC::StoreInst& instruction, ScalarLowering& scalar) {
    auto* source = instruction.getOperand(0);
    auto pointer = resolve(*instruction.getOperand(1), scalar);
    auto location = access(pointer, *source->getType());
    auto& input = scalar.Get(*source);
    auto& value = input.Type() == IrType::F32 ? builder.BitCastU32(input) : input;
    if (pointer.allocation.shared) {
        (void)builder.Emit(IrOpcode::WriteSharedU32, IrType::Void, {&location.offset, &value, &location.active}, flags(true));
        return;
    }
    auto& resource = builder.Emit(IrOpcode::GetScratchResource, IrType::AddressResource, {});
    (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&resource, &location.offset, &builder.Constant(0), &value, &location.active}, flags(false));
}
