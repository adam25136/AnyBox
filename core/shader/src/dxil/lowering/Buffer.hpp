#pragma once

#include "../ResourceLayout.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace AnyBox::Shader::Dxil {
class ScalarLowering;

struct BufferAddress {
    ShaderRecompiler::IrValue& address;
    ShaderRecompiler::IrValue& byteLength;
};

struct InlineConstants {
    std::vector<ShaderRecompiler::IrValue*> words;
};

using ConstantBufferBinding = std::variant<BufferAddress, InlineConstants>;

class IResourceBindings {
public:
    virtual ~IResourceBindings() = default;
    virtual BufferAddress ResolveBuffer(const BufferBinding& binding, ShaderRecompiler::IrValue& index, bool nonUniform, ShaderRecompiler::IrBuilder& builder) = 0;
    virtual ConstantBufferBinding ResolveConstantBuffer(const BufferBinding& binding, ShaderRecompiler::IrValue& index, bool nonUniform, ShaderRecompiler::IrBuilder& builder) { return ResolveBuffer(binding, index, nonUniform, builder); }
};

class BufferLowering {
public:
    BufferLowering(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrBuilder& builder, const ResourceLayout& layout, IResourceBindings& bindings);
    bool Emit(const LLVMBC::CallInst& instruction, ScalarLowering& scalar);
    ShaderRecompiler::IrValue* Extract(const LLVMBC::ExtractValueInst& instruction, ScalarLowering& scalar);
    ShaderRecompiler::IrValue& Atomic(const LLVMBC::CallInst& instruction, ScalarLowering& scalar);

private:
    using IrValue = ShaderRecompiler::IrValue;
    struct Access {
        const BufferBinding& binding;
        BufferAddress view;
        IrValue& offset;
        IrValue* structureOffset;
    };
    struct Component {
        IrValue& address;
        IrValue& active;
    };
    void load(const LLVMBC::CallInst& instruction, unsigned bits, ScalarLowering& scalar, bool floating = false);
    void typedLoad(const LLVMBC::CallInst& instruction, ScalarLowering& scalar);
    void constantBuffer(const LLVMBC::CallInst& instruction, unsigned bits, ScalarLowering& scalar, bool floating);
    void constantWords(const LLVMBC::CallInst& instruction, std::span<IrValue* const> words, IrValue& row, unsigned bits, std::uint32_t byteSize, bool floating);
    void store(const LLVMBC::CallInst& instruction, unsigned bits, ScalarLowering& scalar, bool floating = false);
    Access resolve(const LLVMBC::CallInst& instruction, ScalarLowering& scalar, bool write, unsigned indexPosition = 2);
    Component component(const Access& access, unsigned bits, unsigned index);
    static std::uint32_t constant(const LLVMBC::CallInst& instruction, unsigned index, unsigned bits);
    ShaderRecompiler::MemoryFlags memory(unsigned bits, bool coherent);
    IrValue& read(IrValue& address, IrValue& active, unsigned bits, bool coherent);
    void write(IrValue& address, IrValue& active, IrValue& value, unsigned bits, bool coherent);
    ShaderRecompiler::IrProgram& program;
    ShaderRecompiler::IrBuilder& builder;
    const ResourceLayout& layout;
    IResourceBindings& bindings;
    std::unordered_map<const LLVMBC::Value*, ResourceHandle> handles;
    std::unordered_map<const LLVMBC::Value*, std::array<IrValue*, 5>> aggregates;
    std::unordered_set<const LLVMBC::Value*> stores;
};
}
