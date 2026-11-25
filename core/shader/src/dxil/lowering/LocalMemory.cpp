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
    bool floating = type.getTypeID() == LLVMBC::Type::TypeID::FloatTyID || type.getTypeID() == LLVMBC::Type::TypeID::HalfTyID;
    bool integer = type.isIntegerTy() && (type.getIntegerBitWidth() == 16 || type.getIntegerBitWidth() == 32 || type.getIntegerBitWidth() == 64);
    if (&type != pointer.element || (!floating && !integer)) throw std::runtime_error("DXIL local memory access requires a matching supported scalar element");
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

MemoryFlags LocalLowering::flags(bool shared, unsigned words, unsigned bits) {
    auto& metadata = program.Resources().memoryInfo;
    if (metadata.size() >= std::numeric_limits<std::uint32_t>::max()) throw std::length_error("DXIL local memory operation count exceeds the IR limit");
    MemoryFlags result{static_cast<std::uint32_t>(metadata.size()), 0};
    MemoryInfo memory;
    memory.kind = shared ? ResourceKind::Lds : ResourceKind::Scratch;
    memory.dataBits = bits;
    memory.componentCount = memory.dataDwords = words;
    metadata.push_back(memory);
    return result;
}
