#include <anybox/shader/Native.hpp>
#include <anybox/ExceptionTest.hpp>
#include "NativeFixtures.hpp"
#include <array>

namespace {
std::vector<std::byte> Make(std::uint32_t first, std::uint32_t second) {
    auto data = Fixtures::MakeComputeProgram();
    data.resize(0x180);
    Fixtures::Set(data, 0xC8, 64);
    Fixtures::Set(data, 0x168, first);
    Fixtures::Set(data, 0x16C, second);
    Fixtures::Set(data, 0x170, 0xBF810000);
    for (std::size_t offset = 0x174; offset < data.size(); offset += 4) Fixtures::Set(data, offset, 0xBF9F0000);
    return data;
}
}

void CheckNativePermlaneDecoding() {
    using namespace ShaderRecompiler;
    for (auto opcode : {0x377u, 0x378u}) {
        for (unsigned controls = 0; controls < 4; ++controls) {
            for (unsigned destination : {0u, 31u, 255u}) {
                for (unsigned source : {0u, 127u, 255u}) {
                    auto first = 0xD4000000U | (opcode << 16) | (controls << 11) | destination;
                    auto second = (256 + source) | (6u << 9) | (7u << 18);
                    AnyBox::Shader::NativeFrontend ordinary(Make(first, second));
                    AnyBox::Shader::NativeFrontend xbox(Make(first | 0x4000, second));
                    const auto& value = xbox.GetDecoded().instructions.front();
                    const auto& reference = ordinary.GetDecoded().instructions.front();
                    Fixtures::Require(value.op == (opcode == 0x377 ? RdnaOpcode::VPermlane16B32 : RdnaOpcode::VPermlanex16B32));
                    Fixtures::Require(value.opcodeId == opcode && value.wordCount == 2);
                    Fixtures::Require(value.destination.opSel == ((controls & 1) != 0));
                    Fixtures::Require(value.destination.opSelHi == ((controls & 2) != 0));
                    Fixtures::Require(value.destination.reg == destination && value.source0.reg == source);
                    Fixtures::Require(value.source1.reg == 6 && value.source2.reg == 7 && value.sourceCount == 3);
                    Fixtures::Require(value.destination.kind == reference.destination.kind && value.source0.kind == reference.source0.kind);
                    Fixtures::Require(value.rawWords[0] == (first | 0x4000) && value.rawWords[1] == second);
                    Fixtures::Require(xbox.GetDecoded().code.data() == xbox.GetCode().data());
                    for (auto invalid : {0x100u, 0x2000u, 0x8000u}) {
                        auto data = Make(first | 0x4000 | invalid, second);
                        Fixtures::Reject<std::invalid_argument>([&] { AnyBox::Shader::NativeFrontend rejected(data); });
                    }
                    for (auto invalid : {1u << 27, 1u << 29}) {
                        auto data = Make(first | 0x4000, second | invalid);
                        Fixtures::Reject<std::invalid_argument>([&] { AnyBox::Shader::NativeFrontend rejected(data); });
                    }
                }
            }
        }
    }
}
