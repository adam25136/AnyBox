#include "Buffer.hpp"
#include "Scalar.hpp"
#include <cast.hpp>
#include <function.hpp>
#include <string_view>

using namespace ShaderRecompiler;
using namespace AnyBox::Shader::Dxil;

BufferLowering::BufferLowering(IrProgram& program, IrBuilder& builder, const ResourceLayout& layout, IResourceBindings& bindings) : program(program), builder(builder), layout(layout), bindings(bindings) {}

bool BufferLowering::Emit(const LLVMBC::CallInst& instruction, ScalarLowering& scalar) {
    auto* function = instruction.getCalledFunction();
    if (!function) throw std::runtime_error("DXIL indirect resource call is not implemented");
    const auto& name = function->getName();
    auto identifier = std::string_view(name.data(), name.size());
    if (identifier == "dx.op.createHandle") {
        if (!handles.contains(&instruction)) handles.emplace(&instruction, layout.ResolveHandle(instruction));
        return true;
    }
    unsigned bits;
    if (identifier == "xbox.TypedLoad.i32") {
        if (!aggregates.contains(&instruction)) typedLoad(instruction, scalar);
        return true;
    }
    if (identifier == "dx.op.cbufferLoadLegacy.i32" || identifier == "dx.op.cbufferLoadLegacy.i64" || identifier == "dx.op.cbufferLoadLegacy.f32") {
        if (!aggregates.contains(&instruction)) constantBuffer(instruction, identifier == "dx.op.cbufferLoadLegacy.i64" ? 64 : 32, scalar, identifier == "dx.op.cbufferLoadLegacy.f32");
        return true;
    }
    if (identifier == "dx.op.rawBufferLoad.f32" || identifier == "dx.op.rawBufferLoad.f16") {
        if (!aggregates.contains(&instruction)) load(instruction, identifier.ends_with("f16") ? 16 : 32, scalar, true);
        return true;
    }
    if (identifier == "dx.op.rawBufferStore.f32" || identifier == "dx.op.rawBufferStore.f16") {
        if (!stores.contains(&instruction)) {
            store(instruction, identifier.ends_with("f16") ? 16 : 32, scalar, true);
            stores.insert(&instruction);
        }
        return true;
    }
    if (identifier == "dx.op.rawBufferStore.i16" || identifier == "dx.op.rawBufferStore.i32" || identifier == "dx.op.rawBufferStore.i64") {
        bits = identifier == "dx.op.rawBufferStore.i16" ? 16 : identifier == "dx.op.rawBufferStore.i32" ? 32 : 64;
        if (!stores.contains(&instruction)) {
            store(instruction, bits, scalar);
            stores.insert(&instruction);
        }
        return true;
    }
    if (identifier == "dx.op.rawBufferLoad.i16") bits = 16;
    else if (identifier == "dx.op.rawBufferLoad.i32") bits = 32;
    else if (identifier == "dx.op.rawBufferLoad.i64") bits = 64;
    else return false;
    if (!aggregates.contains(&instruction)) load(instruction, bits, scalar);
    return true;
}

IrValue* BufferLowering::Extract(const LLVMBC::ExtractValueInst& instruction, ScalarLowering& scalar) {
    auto* input = instruction.getAggregateOperand();
    auto found = aggregates.find(input);
    if (found == aggregates.end()) {
        auto* call = LLVMBC::dyn_cast<LLVMBC::CallInst>(input);
        if (!call || !Emit(*call, scalar)) return nullptr;
        found = aggregates.find(input);
        if (found == aggregates.end()) throw std::runtime_error("DXIL opaque resource handle cannot be extracted");
    }
    if (instruction.getNumIndices() != 1) throw std::runtime_error("DXIL nested buffer result extraction is not implemented");
    auto index = instruction.getIndices()[0];
    if (index >= input->getType()->getStructNumElements() || index >= 5) throw std::runtime_error("DXIL buffer result extraction index is invalid");
    if (index == 4) throw std::runtime_error("DXIL sparse buffer residency feedback is not implemented");
    auto* value = found->second.at(index);
    if (!value) throw std::runtime_error("DXIL buffer result extracts a component absent from its load mask");
    auto* type = instruction.getType();
    auto* expected = input->getType()->getStructElementType(index);
    if (type != expected) throw std::runtime_error("DXIL buffer extraction result type does not match its component");
    return value;
}
