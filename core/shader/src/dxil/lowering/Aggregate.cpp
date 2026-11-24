#include "Scalar.hpp"
#include <cast.hpp>

const LLVMBC::Value* AnyBox::Shader::Dxil::ScalarLowering::aggregateElement(const LLVMBC::ExtractValueInst& instruction) {
    const LLVMBC::Value* value = instruction.getAggregateOperand();
    if (!LLVMBC::isa<LLVMBC::CompositeConstructInst>(value)) return nullptr;
    for (unsigned position = 0; position < instruction.getNumIndices(); ++position) {
        auto* composite = LLVMBC::dyn_cast<LLVMBC::CompositeConstructInst>(value);
        if (!composite) throw std::runtime_error("DXIL nested extraction from an opaque aggregate is not implemented");
        auto index = instruction.getIndices()[position];
        if (index >= composite->getNumOperands()) throw std::runtime_error("DXIL aggregate extraction index is invalid");
        value = composite->getOperand(index);
    }
    if (value->getType() != instruction.getType()) throw std::runtime_error("DXIL aggregate extraction type is invalid");
    return value;
}
