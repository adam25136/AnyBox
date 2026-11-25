#include "FloatFixture.hpp"
#include "HalfPackReference.hpp"

void CheckHalfPackInputs(AnyBox::Vulkan::Context& context) {
    BufferTests::FloatFixture fixture;
    auto* word = fixture.IntegerType(32);
    auto arguments = [&] { return LLVMBC::Vector<LLVMBC::Value*>{fixture.Integer(32, 130), fixture.Argument(0)}; };
    auto reject = [&](LLVMBC::Type* type, LLVMBC::Vector<LLVMBC::Value*> values) { fixture.Reject(*fixture.Call("dx.op.legacyF32ToF16", type, std::move(values))); };
    reject(word, {});
    reject(fixture.FloatType(), arguments());
    auto extra = arguments();
    extra.push_back(fixture.Argument(0));
    reject(word, std::move(extra));
    for (unsigned index = 0; index < 2; ++index) {
        auto values = arguments();
        values[index] = fixture.Integer(16, 0);
        reject(word, std::move(values));
    }
    auto values = arguments();
    values[0] = fixture.Integer(32, 131);
    reject(word, std::move(values));
    values = arguments();
    values[0] = fixture.Make<LLVMBC::Argument>(word, 0, false);
    reject(word, std::move(values));
    values = arguments();
    values[1] = fixture.Integer(32, 0);
    reject(word, std::move(values));
    std::array<std::uint32_t, 16> input{0, 0x80000000, 0x3F800001, 0x3F801000, 0xBF803FFF, 0x477FE000, 0x477FF000, 0x47800000, 0x7F800000, 0xFF800000, 0x7F812345, 0xFFC12345, 0x33800000, 0x337FFFFF, 0x387FFFFF, 0x38800000};
    for (unsigned index = 0; index < input.size(); ++index) {
        auto expected = BufferTests::HalfPackReference(input[index]);
        auto* opcode = fixture.Integer(32, 130);
        for (auto* value : {fixture.Argument(index), static_cast<LLVMBC::Value*>(fixture.FloatConstant(input[index]))}) {
            auto* call = fixture.Call("dx.op.legacyF32ToF16", word, {opcode, value});
            fixture.Expect(*call, expected);
        }
    }
    fixture.Run(context, input);
}
