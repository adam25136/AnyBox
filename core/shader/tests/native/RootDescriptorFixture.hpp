#pragma once

#include "RootFixture.hpp"

namespace NativeTests {
inline RootFixture MakeRootDescriptor(unsigned wave, unsigned slot) {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    RootFixture fixture(wave, slot);
    fixture.root.parameters[0] = {RootParameterKind::ShaderResource, 0, 0, 0, 0, 0, {}};
    fixture.buffers[0] = {ResourceClass::ShaderResource, 0, 0, 0, 1, BufferKind::Raw};
    fixture.roots[0].kind = NativeRootKind::Descriptor;
    fixture.roots[0].registerCount = 2;
    std::vector<std::uint32_t> code;
    if (slot >= 16) code = {0xF4040200, 0xFA000000 | ((slot - 16) * 4), 0xF4000304, 0xFA000000};
    else code = {0xF4000300 | (slot / 2), 0xFA000000};
    code.insert(code.end(), {0x7E06020C, 0x7E040282, 0x34020082, 0xDC708000, 0x007F0301, 0xBF810000});
    for (std::size_t offset = 0x168; offset < fixture.data.size(); offset += 4) Fixtures::Set(fixture.data, offset, 0xBF9F0000);
    for (std::size_t index = 0; index < code.size(); ++index) Fixtures::Set(fixture.data, 0x168 + index * 4, code[index]);
    return fixture;
}
}
