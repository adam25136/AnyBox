#include "../native/BufferFixture.hpp"
#include <cstring>
#include <algorithm>

namespace {
void Check(AnyBox::Vulkan::Context& context, unsigned wave, unsigned slot, unsigned bits, unsigned components, bool store, bool bounded, unsigned boundsMode = 3) {
    using namespace AnyBox::Shader;
    auto fixture = NativeTests::MakeBuffer(wave, slot, bits, components, store, bounded, boundsMode);
    RootArgumentPlan plan(fixture.buffers, fixture.root);
    auto translated = fixture.Translate();
    BindNativeRootArguments(translated, fixture.roots, plan);
    auto compiled = CompileCompute(translated.program, translated.input, BufferTests::Target(), {0, 0, 0, 128});
    Fixtures::Require(translated.program.Info().buffers.empty() && translated.program.Resources().srtReads.empty());
    Fixtures::Require(compiled.bindings.layout.memoryOffsetCount == 0);
    for (const auto& descriptor : compiled.bindings.layout.descriptors) Fixtures::Require(descriptor.resources.empty());
    std::array<std::uint32_t, 16> arguments{};
    arguments[1] = store ? 2 : 1;
    auto bytes = bits / 8;
    std::vector<std::uint32_t> input(std::max<std::size_t>(16, wave * components * bytes / 4));
    std::vector<std::uint32_t> expected(store ? wave * components * bytes / 4 : wave * components, store ? UINT32_MAX : 0);
    auto* inputBytes = reinterpret_cast<std::byte*>(input.data());
    auto* outputBytes = reinterpret_cast<std::byte*>(expected.data());
    for (unsigned lane = 0; lane < wave; ++lane) {
        for (unsigned component = 0; component < components; ++component) {
            std::uint32_t value = store ? component + 1 : (lane * components + component + 1) * 17;
            if (bits < 32) value &= (1u << bits) - 1;
            auto offset = (lane * components + component) * bytes;
            std::memcpy(inputBytes + offset, &value, bytes);
            if (boundsMode == 0 || (boundsMode == 3 && bounded && lane + 1 == wave)) continue;
            if (store) std::memcpy(outputBytes + offset, &value, bytes);
            else expected[lane * components + component] = value;
        }
    }
    try { BufferTests::RunGpu(context, compiled, expected, {1, 1, 1}, std::as_bytes(std::span(input)), arguments); }
    catch (const std::exception& error) { throw std::runtime_error("Native buffer wave=" + std::to_string(wave) + " slot=" + std::to_string(slot) + " bits=" + std::to_string(bits) + " components=" + std::to_string(components) + " store=" + std::to_string(store) + " bounded=" + std::to_string(bounded) + " boundsMode=" + std::to_string(boundsMode) + ": " + error.what()); }
}
}

void CheckNativeBuffers(AnyBox::Vulkan::Context& context) {
    for (unsigned wave : {32u, 64u}) {
        for (unsigned slot : {0u, 16u}) {
            for (bool store : {false, true}) {
                for (unsigned boundsMode : {0u, 1u, 2u}) Check(context, wave, slot, 32, 1, store, true, boundsMode);
                for (bool bounded : {false, true}) {
                    for (unsigned bits : {8u, 16u, 32u}) Check(context, wave, slot, bits, 1, store, bounded);
                    for (unsigned components : {2u, 4u}) Check(context, wave, slot, 32, components, store, bounded);
                }
            }
        }
    }
}
