#include "Local.hpp"
#include <cast.hpp>
#include <limits>
#include <unordered_set>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::LocalLowering;

void LocalLowering::preparePrivateGlobals(const LLVMBC::Function& function) {
    std::vector<const LLVMBC::Value*> pending;
    for (auto& block : function) {
        for (auto& instruction : block) {
            for (unsigned index = 0; index < instruction.getNumOperands(); ++index) pending.push_back(instruction.getOperand(index));
        }
    }
    builder.SetInsertionPoint(program.EntryBlock());
    std::unordered_set<const LLVMBC::Value*> visited;
    while (!pending.empty()) {
        auto* value = pending.back();
        pending.pop_back();
        if (!visited.insert(value).second) continue;
        if (auto* variable = LLVMBC::dyn_cast<LLVMBC::GlobalVariable>(value)) {
            auto* type = variable->getType();
            if (!variable->isConstant() && type && type->getTypeID() == LLVMBC::Type::TypeID::PointerTyID && type->getPointerAddressSpace() == 0) (void)privateAllocation(*variable);
        } else if (auto* expression = LLVMBC::dyn_cast<LLVMBC::ConstantExpr>(value)) {
            for (unsigned index = 0; index < expression->getNumOperands(); ++index) pending.push_back(expression->getOperand(index));
        }
    }
}

LocalLowering::Allocation LocalLowering::privateAllocation(const LLVMBC::GlobalVariable& variable) {
    auto* type = variable.getType();
    if (!type || type->getTypeID() != LLVMBC::Type::TypeID::PointerTyID || type->getPointerAddressSpace() != 0 || variable.isConstant() || variable.getLinkage() != LLVMBC::GlobalVariable::InternalLinkage) throw std::runtime_error("DXIL private global requires an internal writable address-space-zero variable");
    if (!variable.hasInitializer()) throw std::runtime_error("DXIL private global has no initializer");
    if (variable.getInitializer()->getType() != type->getPointerElementType()) throw std::runtime_error("DXIL private global initializer type differs from its allocation");
    auto bytes = size(*type->getPointerElementType());
    auto wordCount = (static_cast<std::uint64_t>(bytes) + 3) / 4;
    auto base = static_cast<std::uint64_t>(program.Info().scratchDwords) * 4;
    if (base + wordCount * 4 > std::numeric_limits<std::uint32_t>::max()) throw std::length_error("DXIL private globals exceed the native scratch address range");
    std::vector<std::uint32_t> words;
    if (!LLVMBC::isa<LLVMBC::UndefValue>(variable.getInitializer())) {
        words.reserve(static_cast<std::size_t>(wordCount));
        appendConstants(*variable.getInitializer(), *type->getPointerElementType(), words);
        if (words.size() != wordCount) throw std::logic_error("DXIL private global initializer size differs from its allocation");
    }
    Allocation allocation{static_cast<std::uint32_t>(base), bytes};
    if (!allocations.emplace(&variable, allocation).second) throw std::logic_error("DXIL private global was allocated twice");
    program.Info().scratchDwords += static_cast<std::uint32_t>(wordCount);
    if (words.empty()) return allocation;

    auto& resource = builder.Emit(IrOpcode::GetScratchResource, IrType::AddressResource, {});
    for (std::uint32_t index = 0; index < words.size(); ++index) {
        auto& offset = builder.Constant(allocation.base + index * 4);
        (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&resource, &offset, &builder.Constant(0), &builder.Constant(words[index]), &builder.ConstantBool(true)}, flags(false));
    }
    return allocation;
}
