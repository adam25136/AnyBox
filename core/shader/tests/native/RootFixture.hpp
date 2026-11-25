#pragma once

#include <anybox/shader/NativeArguments.hpp>
#include "../buffer/Target.hpp"
#include "NativeFixtures.hpp"
#include <Recompiler.hpp>
#include <array>

namespace NativeTests {
struct RootFixture {
    std::vector<std::byte> data;
    AnyBox::Graphics::Container::RootSignatureLayout root{2, 0, 0, {}};
    std::array<AnyBox::Shader::BufferBinding, 1> buffers{};
    std::array<AnyBox::Graphics::Container::NativeRootBinding, 1> roots{};

    RootFixture(unsigned wave, unsigned slot) : data(Fixtures::MakeComputeProgram()) {
        using namespace AnyBox::Graphics::Container;
        using namespace AnyBox::Shader;
        Fixtures::Set(data, 0xD4, wave == 32 ? 0x20 : 0);
        Fixtures::Set(data, 0x13C, wave);
        Fixtures::Set(data, 0x140, 1);
        Fixtures::Set(data, 0x144, 1);
        Fixtures::Set(data, 0x138, 0x88);
        Fixtures::Set(data, 0x10C, 0);
        data.resize(0x1C0);
        Fixtures::Set(data, 0xC8, 0x80);
        root.parameters.push_back({RootParameterKind::Constants, 0, 0, 0, 1, 0, {}});
        buffers[0] = {ResourceClass::ConstantBuffer, 0, 0, 0, 1, BufferKind::Constant, 0, 4};
        roots[0] = {NativeRootKind::Constants, 0, 1, 0, static_cast<std::uint8_t>(slot), 0};
        std::vector<std::uint32_t> code;
        if (slot >= 16) code = {0xF4000200, 0xFA000000 | ((slot - 16) * 4), 0x7E060208};
        else code = {0x7E060200 | slot};
        code.insert(code.end(), {0x7E040282, 0x34020082, 0xDC708000, 0x007F0301, 0xBF810000});
        for (std::size_t offset = 0x168; offset < data.size(); offset += 4) Fixtures::Set(data, offset, 0xBF9F0000);
        for (std::size_t index = 0; index < code.size(); ++index) Fixtures::Set(data, 0x168 + index * 4, code[index]);
    }

    AnyBox::Shader::NativeComputeTranslation Translate() const {
        using namespace AnyBox::Shader;
        auto state = AnyBox::Graphics::Container::ReadNativeComputeState(data);
        NativeFrontend frontend(data);
        return LowerNativeCompute(frontend, state, ShaderRecompiler::ShaderFloatMode{state.floatControl, state.dx10Clamp, state.ieeeMode, false}, BufferTests::Target());
    }
};
}
