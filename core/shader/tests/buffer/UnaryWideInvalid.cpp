#include "FloatFixture.hpp"

void CheckWideUnaryInvalid() {
    BufferTests::FloatFixture fixture;
    for (unsigned bits : {32u, 64u}) {
        for (bool reverse : {false, true}) {
            auto* name = reverse ? bits == 64 ? "dx.op.unary.i64" : "dx.op.unary.i32" : bits == 64 ? "dx.op.unaryBits.i64" : "dx.op.unaryBits.i32";
            auto* result = fixture.IntegerType(reverse ? bits : 32);
            auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, reverse ? 30 : 31), fixture.Integer(bits, 1)}; };
            auto reject = [&](LLVMBC::Type* type, LLVMBC::Vector<LLVMBC::Value*> values) { fixture.Reject(*fixture.Call(name, type, std::move(values))); };
            reject(result, {});
            auto extra = arguments();
            extra.push_back(fixture.Integer(32, 0));
            reject(result, std::move(extra));
            reject(fixture.FloatType(), arguments());
            reject(fixture.IntegerType(16), arguments());
            if (!reverse) reject(fixture.IntegerType(64), arguments());
            else reject(fixture.IntegerType(bits == 64 ? 32 : 64), arguments());
            for (unsigned index = 0; index < 2; ++index) {
                auto values = arguments();
                values[index] = fixture.Integer(16, 0);
                reject(result, std::move(values));
            }
            auto values = arguments();
            values[0] = fixture.Make<LLVMBC::Argument>(fixture.IntegerType(32), 0, false);
            reject(result, std::move(values));
            values = arguments();
            values[1] = fixture.Integer(bits == 64 ? 32 : 64, 0);
            reject(result, std::move(values));
            for (unsigned opcode : {0u, 29u, 35u, 255u, reverse ? 31u : 30u}) {
                values = arguments();
                values[0] = fixture.Integer(32, opcode);
                reject(result, std::move(values));
            }
        }
    }
}
