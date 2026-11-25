#include "FloatFixture.hpp"

void CheckWideWaveInvalid() {
    BufferTests::FloatFixture fixture;
    for (bool prefix : {false, true}) {
        auto* name = prefix ? "dx.op.wavePrefixOp.i64" : "dx.op.waveActiveOp.i64";
        auto* wide = fixture.IntegerType(64);
        auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, prefix ? 121 : 119), fixture.Integer(64, 1), fixture.Integer(8, 0), fixture.Integer(8, 1)}; };
        auto reject = [&](LLVMBC::Type* type, LLVMBC::Vector<LLVMBC::Value*> values) { fixture.Reject(*fixture.Call(name, type, std::move(values))); };
        reject(wide, {});
        reject(fixture.IntegerType(32), arguments());
        auto extra = arguments();
        extra.push_back(fixture.Integer(32, 0));
        reject(wide, std::move(extra));
        for (unsigned index = 0; index < 4; ++index) {
            auto values = arguments();
            values[index] = fixture.Integer(16, 0);
            reject(wide, std::move(values));
        }
        for (unsigned index : {0u, 2u, 3u}) {
            auto values = arguments();
            values[index] = fixture.Make<LLVMBC::Argument>(fixture.IntegerType(index == 0 ? 32 : 8), 0, false);
            reject(wide, std::move(values));
        }
        for (unsigned operation = 1; operation <= 6; ++operation) {
            auto values = arguments();
            values[2] = fixture.Integer(8, operation);
            reject(wide, std::move(values));
        }
        auto values = arguments();
        values[0] = fixture.Integer(32, prefix ? 119 : 121);
        reject(wide, std::move(values));
        values = arguments();
        values[3] = fixture.Integer(8, 2);
        reject(wide, std::move(values));
    }
}
