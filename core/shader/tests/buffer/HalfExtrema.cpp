#include "HalfExtremaKernel.hpp"
#include "HalfExtremaReference.hpp"
#include "ComputeCompiler.hpp"
#include "Fixtures.hpp"

void CheckHalfExtremaInputs(AnyBox::Vulkan::Context& context);

void CheckHalfExtrema(AnyBox::Vulkan::Context& context) {
    CheckHalfExtremaInputs(context);
    constexpr std::array<std::uint32_t, 16> corners{0, 0x8000, 1, 0x8001, 0x03FF, 0x0400, 0x3C00, 0xBC00, 0x7BFF, 0xFBFF, 0x7C00, 0xFC00, 0x7E13, 0xFE23, 0x7C01, 0xFD55};
    std::vector<std::uint32_t> input;
    for (unsigned bits = 0; bits < 65536; ++bits) {
        for (unsigned corner = 0; corner < corners.size(); ++corner) {
            input.push_back(bits | ((bits ^ 65535) << 16));
            input.push_back(corners[corner] | (corners[(corner + 5) % corners.size()] << 16));
        }
    }
    std::uint32_t random = 0x5D189BC0;
    for (unsigned index = 0; index < 131072; ++index) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        input.push_back(random);
    }
    for (bool maximum : {false, true}) {
        auto program = BufferTests::MakeHalfExtremaKernel(maximum);
        ShaderRecompiler::ShaderComputeInputInfo info;
        info.threadsNum[0] = 64;
        info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = info.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected;
        expected.reserve(input.size() / 2 * 5);
        for (std::size_t index = 0; index < input.size(); index += 2) {
            auto a = input[index];
            auto b = input[index + 1];
            auto low = BufferTests::HalfMinMaxReference(a & 65535, b & 65535, maximum);
            auto high = BufferTests::HalfMinMaxReference(a >> 16, b >> 16, maximum);
            expected.push_back(low);
            expected.push_back(low);
            expected.push_back(high);
            expected.push_back(BufferTests::HalfMinMaxReference(low, a & 65535, !maximum));
            expected.push_back(BufferTests::HalfMinMaxReference(high, a >> 16, !maximum));
        }
        BufferTests::RunGpu(context, shader, expected, {static_cast<std::uint32_t>(input.size() / 128), 1, 1}, std::as_bytes(std::span(input)));
    }
}
