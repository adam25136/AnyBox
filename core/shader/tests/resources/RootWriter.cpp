#include "Fixtures.hpp"
#include <anybox/shader/RootArgumentWriter.hpp>
#include <array>

using namespace AnyBox::Shader;
using namespace AnyBox::Graphics::Container;

namespace {
class Resources : public IRootResources {
public:
    RootBufferValue ResolveDescriptor(const BufferBinding& binding, std::uint64_t address) override {
        Fixtures::Require(binding.resourceClass == ResourceClass::ShaderResource && address == 0x1ABCDEF12);
        ++descriptorCalls;
        return {address, 64};
    }
    RootBufferValue ResolveTable(const BufferBinding& binding, std::uint32_t address, std::uint32_t offset) override {
        Fixtures::Require(binding.resourceClass == ResourceClass::UnorderedAccess && address == 0x5000 && offset >= 6 && offset <= 7);
        ++tableCalls;
        return {std::uint64_t{0x100000000} | offset, 0x300000004};
    }
    void CheckCounts() const { Fixtures::Require(descriptorCalls == 1 && tableCalls == 2); }

private:
    unsigned descriptorCalls = 0;
    unsigned tableCalls = 0;
};
}

void CheckRootWriter() {
    std::array bindings{
        BufferBinding{ResourceClass::ShaderResource, 0, 0, 1, 1, BufferKind::Raw},
        BufferBinding{ResourceClass::ConstantBuffer, 0, 0, 2, 1, BufferKind::Constant, 0, 8},
        BufferBinding{ResourceClass::UnorderedAccess, 0, 0, 3, 2, BufferKind::Raw}
    };
    RootSignatureLayout root{};
    root.version = 2;
    RootParameter descriptor{};
    descriptor.kind = RootParameterKind::ShaderResource;
    descriptor.shaderRegister = 1;
    RootParameter constants{};
    constants.kind = RootParameterKind::Constants;
    constants.shaderRegister = 2;
    constants.wordCount = 4;
    RootParameter table{};
    table.kind = RootParameterKind::Table;
    table.ranges = {{RootRangeKind::UnorderedAccess, 2, 3, 0, 0, 6}};
    root.parameters = {descriptor, constants, table};
    RootArgumentPlan plan(bindings, root, 3);
    AnyBox::Graphics::Commands::RootState state(root);
    std::array address{0xABCDEF12u, 1u};
    std::array values{99u, 100u};
    std::array pointer{0x5000u};
    state.Write(0, 0, address);
    state.Write(1, 0, values);
    state.Write(2, 0, pointer);
    std::array registers{18u, 7u, 5u, 11u, 8u, 3u, 14u, 6u, 12u};
    RootArgumentWriter writer(plan, registers);
    Fixtures::Require(!writer.RequiresVolatileTables(root));
    auto legacy = root;
    legacy.version = 1;
    Fixtures::Require(writer.RequiresVolatileTables(legacy));
    auto volatileRoot = root;
    volatileRoot.parameters[2].ranges[0].flags = 1;
    Fixtures::Require(writer.RequiresVolatileTables(volatileRoot));
    std::array<std::uint32_t, 9> destination{};
    Resources resources;
    writer.Write(state, resources, destination);
    std::array expected{3u, 99u, 64u, 6u, 100u, 0xABCDEF12u, 3u, 0u, 1u};
    Fixtures::Require(destination == expected && writer.GetWordCount() == expected.size());
    resources.CheckCounts();

    Fixtures::Reject<std::invalid_argument>([&] { writer.Write(state, resources, std::span(destination).first(8)); });
    for (auto invalid : {std::array{3u, 3u}, std::array{2u, 3u}, std::array{19u, 128u}}) {
        Fixtures::Reject<std::invalid_argument>([&] { RootArgumentWriter selected(plan, invalid); });
    }
    std::array uninitialized{9u};
    RootArgumentWriter selected(plan, uninitialized);
    std::array<std::uint32_t, 1> output{};
    Fixtures::Reject<std::logic_error>([&] { selected.Write(state, resources, output); });
    auto wrongRoot = root;
    wrongRoot.parameters[1].wordCount = 3;
    AnyBox::Graphics::Commands::RootState wrongState(wrongRoot);
    Fixtures::Reject<std::invalid_argument>([&] { selected.Write(wrongState, resources, output); });
    RootArgumentWriter empty(plan, {});
    Fixtures::Require(!empty.RequiresVolatileTables(legacy));
    empty.Write(state, resources, {});
}
