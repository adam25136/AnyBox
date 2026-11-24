#include "Local.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <bit>
#include <limits>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::LocalLowering;

LocalLowering::Pointer LocalLowering::resolve(const LLVMBC::Value& value, ScalarLowering& scalar) {
    if (auto found = pointers.find(&value); found != pointers.end()) return found->second;
    Pointer pointer{};
    if (auto found = allocations.find(&value); found != allocations.end()) {
        pointer = {found->second, value.getType()->getPointerElementType(), &builder.ConstantU64(0), 0};
    } else if (auto* variable = LLVMBC::dyn_cast<LLVMBC::GlobalVariable>(&value)) {
        auto allocation = variable->isConstant() ? constantAllocation(*variable) : sharedAllocation(*variable);
        pointer = {std::move(allocation), value.getType()->getPointerElementType(), &builder.ConstantU64(0), 0};
    } else if (auto* instruction = LLVMBC::dyn_cast<LLVMBC::GetElementPtrInst>(&value)) {
        pointer = element(*instruction, scalar);
    } else if (auto* conversion = LLVMBC::dyn_cast<LLVMBC::CastInst>(&value)) {
        if (conversion->getOpcode() != LLVMBC::Instruction::BitCast) throw std::runtime_error("DXIL memory pointer conversion is not implemented");
        pointer = castPointer(*conversion->getOperand(0), conversion->getType(), scalar);
    } else if (auto* expression = LLVMBC::dyn_cast<LLVMBC::ConstantExpr>(&value)) {
        if (expression->getOpcode() == LLVMBC::Instruction::BitCast && expression->getNumOperands() == 1) pointer = castPointer(*expression->getOperand(0), expression->getType(), scalar);
        else {
            if (expression->getOpcode() != LLVMBC::Instruction::GetElementPtr) throw std::runtime_error("DXIL constant pointer operation is not implemented");
            LLVMBC::Vector<LLVMBC::Value*> operands;
            for (unsigned index = 0; index < expression->getNumOperands(); ++index) operands.push_back(expression->getOperand(index));
            LLVMBC::GetElementPtrInst elementPointer(expression->getType(), std::move(operands), false);
            pointer = element(elementPointer, scalar);
        }
    } else {
        throw std::runtime_error("DXIL pointer does not reference a supported memory allocation: kind=" + std::to_string(static_cast<unsigned>(value.get_value_kind())));
    }
    pointers.emplace(&value, pointer);
    return pointer;
}

LocalLowering::Pointer LocalLowering::castPointer(const LLVMBC::Value& source, LLVMBC::Type* target, ScalarLowering& scalar) {
    auto* original = source.getType();
    if (!original || original->getTypeID() != LLVMBC::Type::TypeID::PointerTyID || !target || target->getTypeID() != LLVMBC::Type::TypeID::PointerTyID) throw std::runtime_error("DXIL memory bitcast requires pointer operands");
    if (original->getPointerAddressSpace() != target->getPointerAddressSpace()) throw std::runtime_error("DXIL memory bitcast cannot change its address space");
    auto pointer = resolve(source, scalar);
    auto space = pointer.allocation.shared ? 3u : 0u;
    if (target->getPointerAddressSpace() != space) throw std::runtime_error("DXIL memory bitcast address space does not match its allocation");
    pointer.element = target->getPointerElementType();
    if (!pointer.element) throw std::runtime_error("DXIL memory bitcast has no destination element");
    (void)size(*pointer.element);
    return pointer;
}

LocalLowering::Pointer LocalLowering::element(const LLVMBC::GetElementPtrInst& instruction, ScalarLowering& scalar) {
    if (instruction.getNumOperands() < 2) throw std::runtime_error("DXIL local pointer has no element index");
    auto pointer = resolve(*instruction.getOperand(0), scalar);
    for (unsigned index = 1; index < instruction.getNumOperands(); ++index) {
        if (index != 1) {
            if (pointer.element->getTypeID() != LLVMBC::Type::TypeID::ArrayTyID) throw std::runtime_error("DXIL nested local pointer index requires an array");
            pointer.element = pointer.element->getArrayElementType();
        }
        auto stride = size(*pointer.element);
        auto* source = instruction.getOperand(index);
        auto* type = source->getType();
        if (!type || !type->isIntegerTy() || (type->getIntegerBitWidth() != 32 && type->getIntegerBitWidth() != 64)) throw std::runtime_error("DXIL local pointer index must be a 32-bit or 64-bit integer");
        auto* constant = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(source);
        if (constant && pointer.constant) {
            auto raw = constant->getUniqueInteger().getZExtValue();
            auto selected = type->getIntegerBitWidth() == 32 ? static_cast<std::int64_t>(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(raw))) : std::bit_cast<std::int64_t>(raw);
            if (selected > std::numeric_limits<std::int64_t>::max() / stride || selected < std::numeric_limits<std::int64_t>::min() / stride) throw std::runtime_error("DXIL constant local pointer multiplication overflows");
            auto bytes = selected * stride;
            if ((bytes > 0 && *pointer.constant > std::numeric_limits<std::int64_t>::max() - bytes) || (bytes < 0 && *pointer.constant < std::numeric_limits<std::int64_t>::min() - bytes)) throw std::runtime_error("DXIL constant local pointer addition overflows");
            *pointer.constant += bytes;
            pointer.offset = &builder.ConstantU64(static_cast<std::uint64_t>(*pointer.constant));
            continue;
        }
        auto& value = scalar.Get(*source);
        IrValue* extended = &value;
        if (type->getIntegerBitWidth() == 32) extended = &builder.ConstructU64(value, builder.ShiftRightArithmetic(value, builder.Constant(31)));
        auto& bytes = builder.Emit(IrOpcode::IMul64, IrType::U64, {extended, &builder.ConstantU64(stride)});
        pointer.offset = &builder.Emit(IrOpcode::IAdd64, IrType::U64, {pointer.offset, &bytes});
        pointer.constant.reset();
    }
    auto* type = instruction.getType();
    auto space = pointer.allocation.shared ? 3u : 0u;
    if (!type || type->getTypeID() != LLVMBC::Type::TypeID::PointerTyID || type->getPointerAddressSpace() != space || type->getPointerElementType() != pointer.element) throw std::runtime_error("DXIL memory pointer result type does not match its element or address space");
    return pointer;
}
