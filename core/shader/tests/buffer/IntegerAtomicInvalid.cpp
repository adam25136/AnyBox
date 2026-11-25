#include "FloatFixture.hpp"

void CheckIntegerAtomicInvalid() {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    for (bool subtract : {false, true}) {
        auto opcode = subtract ? 367u : 355u;
        auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, opcode), fixture.Integer(64, 0x200000000), fixture.Integer(32, 1)}; };
        auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> values) { fixture.Reject(*fixture.Call(subtract ? "xbox.global_atomic_sub" : "xbox.global_atomic_add", result, std::move(values))); };
        reject(word, {});
        reject(fixture.FloatType(), arguments());
        reject(fixture.IntegerType(64), arguments());
        auto extra = arguments();
        extra.push_back(fixture.Integer(32, 0));
        reject(word, std::move(extra));
        for (unsigned index = 0; index < 3; ++index) {
            auto values = arguments();
            values[index] = fixture.Integer(index == 1 ? 32 : 16, 0);
            reject(word, std::move(values));
        }
        auto values = arguments();
        values[0] = fixture.Integer(32, opcode + 1);
        reject(word, std::move(values));
        values = arguments();
        values[0] = fixture.Make<LLVMBC::Argument>(word, 0, false);
        reject(word, std::move(values));
        values = arguments();
        values[2] = fixture.Argument(0);
        reject(word, std::move(values));
    }
}
