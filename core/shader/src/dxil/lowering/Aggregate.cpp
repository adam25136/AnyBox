#include "Scalar.hpp"
#include <cast.hpp>

const LLVMBC::Value* AnyBox::Shader::Dxil::ScalarLowering::aggregateElement(const LLVMBC::ExtractValueInst& instruction) {
    const LLVMBC::Value* value = instruction.getAggregateOperand();
    if (!LLVMBC::isa<LLVMBC::CompositeConstructInst>(value) && !LLVMBC::isa<LLVMBC::ConstantAggregate>(value) && !LLVMBC::isa<LLVMBC::ConstantDataArray>(value)) return nullptr;
    for (unsigned position = 0; position < instruction.getNumIndices(); ++position) {
        auto index = instruction.getIndices()[position];
        if (auto* composite = LLVMBC::dyn_cast<LLVMBC::CompositeConstructInst>(value)) {
            if (index >= composite->getNumOperands()) throw std::runtime_error("DXIL aggregate extraction index is invalid");
            value = composite->getOperand(index);
        } else if (auto* aggregate = LLVMBC::dyn_cast<LLVMBC::ConstantAggregate>(value)) {
            if (index >= aggregate->getNumOperands()) throw std::runtime_error("DXIL constant aggregate extraction index is invalid");
            value = aggregate->getOperand(index);
        } else if (auto* array = LLVMBC::dyn_cast<LLVMBC::ConstantDataArray>(value)) {
            if (index >= array->getNumElements()) throw std::runtime_error("DXIL constant array extraction index is invalid");
            value = array->getElementAsConstant(index);
        } else throw std::runtime_error("DXIL nested extraction from an opaque aggregate is not implemented");
    }
    if (value->getType() != instruction.getType()) throw std::runtime_error("DXIL aggregate extraction type is invalid");
    return value;
}
