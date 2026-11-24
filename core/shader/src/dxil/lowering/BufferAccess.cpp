#include "Buffer.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <limits>

using namespace ShaderRecompiler;
using namespace AnyBox::Shader;
using namespace AnyBox::Shader::Dxil;

std::uint32_t BufferLowering::constant(const LLVMBC::CallInst& instruction, unsigned index, unsigned bits) {
    auto* value = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(index));
    if (!value || !value->getType()->isIntegerTy() || value->getType()->getIntegerBitWidth() != bits) throw std::runtime_error("DXIL buffer constant has an invalid integer type");
    return static_cast<std::uint32_t>(value->getUniqueInteger().getZExtValue());
}

BufferLowering::Access BufferLowering::resolve(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, bool write, unsigned indexPosition) {
    auto handle = handles.find(instruction.getArgOperand(1));
    if (handle == handles.end()) throw std::runtime_error("DXIL buffer access requires a previously created resource handle");
    const auto& binding = *handle->second.binding;
    if ((write && binding.resourceClass != ResourceClass::UnorderedAccess) || (!write && binding.resourceClass != ResourceClass::ShaderResource && binding.resourceClass != ResourceClass::UnorderedAccess)) throw std::runtime_error("DXIL buffer access uses an incompatible resource class");
    if (binding.kind != BufferKind::Raw && binding.kind != BufferKind::Structured) throw std::runtime_error("DXIL buffer access requires a raw or structured buffer");
    auto* index = instruction.getArgOperand(indexPosition);
    auto* offset = instruction.getArgOperand(indexPosition + 1);
    if (!index->getType()->isIntegerTy() || index->getType()->getIntegerBitWidth() != 32 || !offset->getType()->isIntegerTy() || offset->getType()->getIntegerBitWidth() != 32) throw std::runtime_error("DXIL buffer indices must be 32-bit integers");
    auto& selected = scalar.Get(*handle->second.index);
    auto view = bindings.ResolveBuffer(binding, selected, handle->second.nonUniform, builder);
    if (view.address.Type() != IrType::U64 || view.byteLength.Type() != IrType::U64) throw std::logic_error("DXIL buffer binding requires a 64-bit address and size");
    auto& element = builder.ConstructU64(scalar.Get(*index), builder.Constant(0));
    if (binding.kind == BufferKind::Raw) {
        if (!LLVMBC::isa<LLVMBC::UndefValue>(offset)) throw std::runtime_error("DXIL raw byte address buffer access must not specify a structure offset");
        return {binding, view, element, nullptr};
    }
    auto& structureOffset = builder.ConstructU64(scalar.Get(*offset), builder.Constant(0));
    auto& start = builder.Emit(IrOpcode::IMul64, IrType::U64, {&element, &builder.ConstantU64(binding.stride)});
    auto& location = builder.Emit(IrOpcode::IAdd64, IrType::U64, {&start, &structureOffset});
    return {binding, view, location, &structureOffset};
}

BufferLowering::Component BufferLowering::component(const Access& access, unsigned bits, unsigned index) {
    auto& location = builder.Emit(IrOpcode::IAdd64, IrType::U64, {&access.offset, &builder.ConstantU64(index * (bits / 8))});
    auto& end = builder.Emit(IrOpcode::IAdd64, IrType::U64, {&location, &builder.ConstantU64(bits / 8)});
    auto& outside = builder.Emit(IrOpcode::UGreaterThan64, IrType::Bool, {&end, &access.view.byteLength});
    IrValue* active = &builder.LogicalNot(outside);
    if (access.structureOffset) {
        auto& componentEnd = builder.Emit(IrOpcode::IAdd64, IrType::U64, {access.structureOffset, &builder.ConstantU64((index + 1) * (bits / 8))});
        auto& exceedsElement = builder.Emit(IrOpcode::UGreaterThan64, IrType::Bool, {&componentEnd, &builder.ConstantU64(access.binding.stride)});
        active = &builder.LogicalAnd(*active, builder.LogicalNot(exceedsElement));
    }
    auto& address = builder.Emit(IrOpcode::IAdd64, IrType::U64, {&access.view.address, &location});
    return {address, *active};
}

MemoryFlags BufferLowering::memory(unsigned bits, bool coherent) {
    MemoryInfo description;
    description.kind = ResourceKind::Global;
    description.addressIsFull = true;
    description.dataBits = bits == 64 ? 32 : bits;
    description.dataDwords = bits == 64 ? 2 : 1;
    description.componentCount = description.dataDwords;
    description.coherent = coherent;
    auto& metadata = program.Resources().memoryInfo;
    if (metadata.size() >= std::numeric_limits<std::uint32_t>::max()) throw std::length_error("DXIL memory operation count exceeds the IR limit");
    MemoryFlags flags{static_cast<std::uint32_t>(metadata.size()), 0};
    metadata.push_back(description);
    return flags;
}
