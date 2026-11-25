#include <anybox/shader/NativeArguments.hpp>
#include <anybox/shader/NativeVertex.hpp>
#include <anybox/shader/NativePixel.hpp>
#include "../buffer/Target.hpp"
#include "NativeFixtures.hpp"
#include <array>

void CheckSpirvValidation(std::span<const std::uint32_t> code);

namespace {
std::vector<std::byte> MakeGraphicsProgram(bool pixel) {
    auto data = Fixtures::MakeComputeProgram();
    data.resize(0x1C0);
    Fixtures::Set(data, 0x100, pixel ? 5 : 4);
    Fixtures::Set(data, 0xC8, 0x80);
    for (std::size_t offset = 0x168; offset < data.size(); offset += 4) Fixtures::Set(data, offset, 0xBF9F0000);
    std::array<std::uint32_t, 6> instructions{0x7E000208, 0x7E0202FF, 0x3F800000, pixel ? 0xF8001801u : 0xF80008CFu, pixel ? 0u : 0x01000000u, 0xBF810000};
    for (std::size_t index = 0; index < instructions.size(); ++index) Fixtures::Set(data, 0x168 + index * 4, instructions[index]);
    return data;
}
}

void CheckNativeGraphicsRootArguments() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    using namespace ShaderRecompiler;
    using namespace Fixtures;
    RootSignatureLayout root{2, 0, 0, {}};
    root.parameters.push_back({RootParameterKind::Constants, 1, 0, 0, 1, 0, {}});
    root.parameters.push_back({RootParameterKind::Constants, 5, 0, 0, 1, 0, {}});
    std::array<BufferBinding, 1> bindings{{{ResourceClass::ConstantBuffer, 0, 0, 0, 1, BufferKind::Constant, 0, 4}}};
    Reject<std::runtime_error>([&] { RootArgumentPlan plan(bindings, root); });
    Reject<std::invalid_argument>([&] { RootArgumentPlan plan(bindings, root, 0, 8); });

    for (bool pixel : {false, true}) {
        auto data = MakeGraphicsProgram(pixel);
        auto state = ReadNativeStageState(data);
        NativeFrontend frontend(data);
        auto target = BufferTests::Target();
        std::vector<std::uint32_t> capabilities(target.supportedCapabilities.begin(), target.supportedCapabilities.end());
        capabilities.push_back(spv::CapabilityClipDistance);
        target.supportedCapabilities = capabilities;
        RootArgumentPlan plan(bindings, root, 0, pixel ? 5u : 1u);
        Require(plan.GetRanges().size() == 1 && plan.GetRanges()[0].parameter == (pixel ? 1u : 0u));
        std::array<NativeRootBinding, 1> roots{{{NativeRootKind::Constants, 0, 1, 0, 8, static_cast<std::uint8_t>(pixel ? 1 : 0)}}};
        ShaderVertexInputInfo vertexInput;
        vertexInput.hostSubgroupSize = target.subgroupSize;
        ShaderPixelInputInfo pixelInput;
        pixelInput.targetOutputMode[0] = 1;
        auto translate = [&] { return pixel ? LowerNativePixel(frontend, state, pixelInput, 9, target) : LowerNativeVertex(frontend, state, vertexInput, 9, target); };
        auto program = translate();
        BindNativeGraphicsRootArguments(program, roots, plan);
        Require(program.Resources().userDataCount == 1);
        auto compiled = pixel ? CompilePixel(program, pixelInput, target, {0, 0, 0, 128}) : CompileVertex(program, vertexInput, target, {0, 0, 0, 128});
        CheckSpirvValidation(compiled.code);
        Require(compiled.bindings.layout.userDataRegisters == std::vector<std::uint32_t>{0});
        Reject<std::logic_error>([&] { BindNativeGraphicsRootArguments(program, roots, plan); });

        auto badRoots = roots;
        badRoots[0].firstUserRegister = 7;
        auto missing = translate();
        Reject<std::runtime_error>([&] { BindNativeGraphicsRootArguments(missing, badRoots, plan); });
        badRoots = roots;
        badRoots[0].firstUserRegister = 9;
        auto overlap = translate();
        Reject<std::invalid_argument>([&] { BindNativeGraphicsRootArguments(overlap, badRoots, plan); });
        auto wrongStage = translate();
        wrongStage.Resources().stage = IrShaderStage::Compute;
        Reject<std::invalid_argument>([&] { BindNativeGraphicsRootArguments(wrongStage, roots, plan); });
    }
}
