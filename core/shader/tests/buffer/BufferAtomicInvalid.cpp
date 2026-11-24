#include "BufferAtomicFixture.hpp"
#include "Fixtures.hpp"

namespace BufferTests { void CheckBufferAtomicInvalid(); }

void BufferTests::CheckBufferAtomicInvalid() {
    for (bool structured : {false, true}) {
        BufferAtomicFixture fixture(structured);
        auto reject = [&](LLVMBC::Vector<LLVMBC::Value*> arguments) { Fixtures::Reject<std::runtime_error>([&] { (void)fixture.scalar.Get(*fixture.Call(std::move(arguments))); }); };
        reject({});
        for (unsigned position : {0u, 2u, 3u, 4u, 5u, 6u}) {
            auto arguments = fixture.Arguments(0);
            arguments[position] = fixture.Integer(16, 0);
            reject(std::move(arguments));
        }
        for (unsigned mode : {9u, 200u, UINT32_MAX}) reject(fixture.Arguments(mode));
        auto wrongOpcode = fixture.Arguments(0);
        wrongOpcode[0] = fixture.Integer(32, 77);
        reject(std::move(wrongOpcode));
        auto third = fixture.Arguments(0);
        third[5] = fixture.Integer(32, 0);
        reject(std::move(third));
        auto dynamic = fixture.Arguments(0);
        dynamic[2] = fixture.Bind(fixture.builder.Constant(0));
        reject(std::move(dynamic));
    }
    BufferAtomicFixture readOnly(false, 0);
    Fixtures::Reject<std::runtime_error>([&] { (void)readOnly.scalar.Get(*readOnly.Call(readOnly.Arguments(0))); });
}
