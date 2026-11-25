#pragma once

#include "RootDescriptorFixture.hpp"
#include <bit>

namespace NativeTests {
inline RootFixture MakeBuffer(unsigned wave, unsigned slot, unsigned bits, unsigned components, bool store, bool bounded, unsigned boundsMode = 3) {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    auto fixture = MakeRootDescriptor(wave, slot);
    if (store) {
        fixture.root.parameters[0].kind = RootParameterKind::UnorderedAccess;
        fixture.buffers[0].resourceClass = ResourceClass::UnorderedAccess;
    }
    auto stride = bits / 8 * components;
    auto length = (wave - static_cast<unsigned>(bounded)) * stride;
    std::vector<std::uint32_t> code;
    if (slot >= 16) code = {0xF4040200, 0xFA000000 | ((slot - 16) * 4)};
    else code = {0xBE880300 | slot, 0xBE890300 | (slot + 1)};
    code.insert(code.end(), {0xBE8A03FF, length, 0xBE8B03FF, 0x01014FAC | (boundsMode << 28)});
    code.push_back(0x34020080 | std::countr_zero(stride));
    auto opcode = bits == 8 ? 0x08u : bits == 16 ? 0x0Au : components == 1 ? 0x0Cu : components == 2 ? 0x0Du : 0x0Eu;
    if (store) {
        opcode += 0x10;
        for (unsigned component = 0; component < components; ++component) code.push_back(0x7E000280 | ((3 + component) << 17) | (component + 1));
    }
    code.insert(code.end(), {0xE0001000 | (opcode << 18), 0x7D020301});
    if (!store) {
        code.push_back(0x7E040282);
        code.push_back(0x34020080 | std::countr_zero(4 * components));
        for (unsigned component = 0; component < components; ++component) code.insert(code.end(), {0xDC708000 | (4 * component), 0x007F0001 | ((3 + component) << 8)});
    }
    code.push_back(0xBF810000);
    for (std::size_t offset = 0x168; offset < fixture.data.size(); offset += 4) Fixtures::Set(fixture.data, offset, 0xBF9F0000);
    for (std::size_t index = 0; index < code.size(); ++index) Fixtures::Set(fixture.data, 0x168 + index * 4, code[index]);
    return fixture;
}
}
