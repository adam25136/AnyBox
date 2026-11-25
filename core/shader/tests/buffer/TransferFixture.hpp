#pragma once

#include "../resources/Fixture.hpp"
#include "lowering/Function.hpp"
#include <optional>

namespace AnyBox::Vulkan { class Context; }

namespace BufferTests {
class TransferBindings : public AnyBox::Shader::Dxil::IResourceBindings {
public:
    explicit TransferBindings(unsigned outputBytes) : outputBytes(outputBytes) {}
    AnyBox::Shader::Dxil::BufferAddress ResolveBuffer(const AnyBox::Shader::BufferBinding& binding, ShaderRecompiler::IrValue& index, bool nonUniform, ShaderRecompiler::IrBuilder& builder) override {
        if (!index.HasImmediate() || index.ImmediateU32() != 4 || nonUniform) throw std::runtime_error("Unexpected transfer test resource selection");
        bool input = binding.resourceClass == AnyBox::Shader::ResourceClass::ShaderResource;
        return {builder.ConstantU64(input ? 0x100000000 : 0x200000000), builder.ConstantU64(input ? 64 : outputBytes)};
    }

private:
    unsigned outputBytes;
};

class TransferFixture {
public:
    TransferFixture(unsigned bits, bool structured, bool readOnly = false, bool floating = false) : bits(bits), floating(floating), bindings(15 * bits / 2) {
        auto& context = source.Context();
        auto& module = source.Module();
        auto* resources = source.Node({source.Node({source.Buffer(0, structured ? 32 : 0)}), source.Node({source.Buffer(1, 0)}), source.Null(), source.Null()});
        module.add_named_metadata("dx.resources", context.construct<LLVMBC::NamedMDNode>(&module, "dx.resources", LLVMBC::Vector<LLVMBC::MDNode*>{resources}));
        layout.emplace(module);
        block = context.construct<LLVMBC::BasicBlock>(context);
        auto* input = source.Handle(0, 4);
        auto* output = source.Handle(1, 4);
        block->add_instruction(input);
        block->add_instruction(output);
        auto* element = floating ? (bits == 16 ? LLVMBC::Type::getHalfTy(context) : LLVMBC::Type::getFloatTy(context)) : LLVMBC::Type::getIntTy(context, bits);
        auto* word = LLVMBC::Type::getInt32Ty(context);
        auto* result = LLVMBC::StructType::get(context, {element, element, element, element, word}, "result");
        auto payload = bits / 2;
        auto* offset = structured ? static_cast<LLVMBC::Value*>(integer(32, 32 - payload)) : LLVMBC::UndefValue::get(word);
        auto suffix = (floating ? "f" : "i") + std::to_string(bits);
        auto* loaded = call("dx.op.rawBufferLoad." + suffix, bits, result, {integer(32, 139), input, integer(32, structured ? 1 : 64 - payload), offset, integer(8, 15), integer(32, bits / 8)});
        std::array<LLVMBC::Value*, 4> values{};
        for (unsigned index = 0; index < 4; ++index) {
            auto* extracted = context.construct<LLVMBC::ExtractValueInst>(element, loaded, LLVMBC::Vector<unsigned>{index});
            block->add_instruction(extracted);
            values[index] = extracted;
        }
        for (unsigned mask = 1; mask <= 15; ++mask) store(readOnly ? input : output, (mask - 1) * payload, mask, values);
        store(output, 15 * payload, 15, values);
        block->add_instruction(context.construct<LLVMBC::ReturnInst>(nullptr));
    }
    const std::vector<AnyBox::Shader::BufferBinding>& GetBindings() const { return layout->GetBindings(); }
    ShaderRecompiler::IrProgram Lower() { return Lower(bindings, 0); }
    ShaderRecompiler::IrProgram Lower(AnyBox::Shader::Dxil::IResourceBindings& arguments, std::uint32_t userRegisterCount) {
        auto& context = source.Context();
        auto* type = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{});
        auto* function = context.construct<LLVMBC::Function>(type, 1024, source.Module());
        function->set_basic_blocks({block});
        ShaderRecompiler::IrProgram output;
        output.Resources().userDataCount = userRegisterCount;
        AnyBox::Shader::Dxil::FunctionLowering(output, *layout, arguments).Run(*function);
        return output;
    }

private:
    LLVMBC::ConstantInt* integer(unsigned width, unsigned value) {
        return LLVMBC::ConstantInt::get(LLVMBC::Type::getIntTy(source.Context(), width), value);
    }
    LLVMBC::CallInst* call(const std::string& name, unsigned id, LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        auto& context = source.Context();
        auto& module = source.Module();
        module.add_value_name(id, LLVMBC::String(name.begin(), name.end()));
        LLVMBC::Vector<LLVMBC::Type*> parameters;
        for (auto* argument : arguments) parameters.push_back(argument->getType());
        auto* signature = context.construct<LLVMBC::FunctionType>(context, result, std::move(parameters));
        auto* declaration = context.construct<LLVMBC::Function>(signature, id, module);
        auto* instruction = context.construct<LLVMBC::CallInst>(signature, declaration, std::move(arguments));
        block->add_instruction(instruction);
        return instruction;
    }
    void store(LLVMBC::Value* handle, unsigned offset, unsigned mask, const std::array<LLVMBC::Value*, 4>& values) {
        auto& context = source.Context();
        auto* word = LLVMBC::Type::getInt32Ty(context);
        auto* element = floating ? (bits == 16 ? LLVMBC::Type::getHalfTy(context) : LLVMBC::Type::getFloatTy(context)) : LLVMBC::Type::getIntTy(context, bits);
        LLVMBC::Vector<LLVMBC::Value*> arguments{integer(32, 140), handle, integer(32, offset), LLVMBC::UndefValue::get(word)};
        for (unsigned index = 0; index < 4; ++index) arguments.push_back(mask & (1u << index) ? values[index] : LLVMBC::UndefValue::get(element));
        arguments.push_back(integer(8, mask));
        arguments.push_back(integer(32, bits / 8));
        auto suffix = (floating ? "f" : "i") + std::to_string(bits);
        (void)call("dx.op.rawBufferStore." + suffix, 100 + bits, LLVMBC::Type::getVoidTy(context), std::move(arguments));
    }
    ResourceTests::Fixture source;
    unsigned bits;
    bool floating;
    TransferBindings bindings;
    std::optional<AnyBox::Shader::Dxil::ResourceLayout> layout;
    LLVMBC::BasicBlock* block;
};

void CheckTransfers(AnyBox::Vulkan::Context& context);
void CheckFloatTransfers(AnyBox::Vulkan::Context& context);
}
