#include "Buffer.hpp"
#include "Scalar.hpp"

using namespace ShaderRecompiler;
using namespace AnyBox::Shader;
using namespace AnyBox::Shader::Dxil;

void BufferLowering::typedLoad(const LLVMBC::CallInst& instruction, ScalarLowering& scalar) {
    if (instruction.getNumArgOperands() != 4 || constant(instruction, 0, 32) != 138) throw std::runtime_error("Xbox typed load has an invalid signature");
    auto* result = instruction.getType();
    if (!result || result->getTypeID() != LLVMBC::Type::TypeID::StructTyID || result->getStructNumElements() != 5) throw std::runtime_error("Xbox typed load result must contain five words");
    for (unsigned index = 0; index < 5; ++index) {
        auto* member = result->getStructElementType(index);
        if (!member->isIntegerTy() || member->getIntegerBitWidth() != 32) throw std::runtime_error("Xbox typed load result member must be a 32-bit integer");
    }
    auto format = constant(instruction, 3, 32);
    auto numeric = format >> 4;
    auto data = format & 15;
    if ((numeric != 4 && numeric != 5) || (data != 1 && data != 2 && data != 4)) throw std::runtime_error("Xbox typed load format is not implemented: " + std::to_string(format));
    auto bits = data * 8;
    auto handle = handles.find(instruction.getArgOperand(1));
    if (handle == handles.end()) throw std::runtime_error("Xbox typed load requires a previously created resource handle");
    const auto& binding = *handle->second.binding;
    if (binding.kind != BufferKind::Raw || (binding.resourceClass != ResourceClass::ShaderResource && binding.resourceClass != ResourceClass::UnorderedAccess)) throw std::runtime_error("Xbox typed load requires a raw shader resource or unordered access buffer");
    auto* offset = instruction.getArgOperand(2);
    if (!offset->getType()->isIntegerTy() || offset->getType()->getIntegerBitWidth() != 32) throw std::runtime_error("Xbox typed load requires a 32-bit byte offset");

    auto view = bindings.ResolveBuffer(binding, scalar.Get(*handle->second.index), handle->second.nonUniform, builder);
    if (view.address.Type() != IrType::U64 || view.byteLength.Type() != IrType::U64) throw std::logic_error("Xbox typed load binding requires a 64-bit address and size");
    auto& location = builder.ConstructU64(scalar.Get(*offset), builder.Constant(0));
    Access access{binding, view, location, nullptr};
    auto part = component(access, bits, 0);
    IrValue* value = &read(part.address, part.active, bits, binding.globallyCoherent);
    if (numeric == 5 && bits < 32) {
        auto& shift = builder.Constant(32 - bits);
        value = &builder.ShiftRightArithmetic(builder.ShiftLeftLogical(*value, shift), shift);
    }
    aggregates.emplace(&instruction, std::array<IrValue*, 5>{value});
}
