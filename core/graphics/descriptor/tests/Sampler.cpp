#include <anybox/descriptor/Sampler.hpp>
#include "SamplerReference.hpp"
#include <bit>
#include <stdexcept>

namespace {
template<class TException, class TCallable>
void RejectSampler(TCallable&& callable) {
    try { callable(); } catch (const TException&) { return; }
    throw std::runtime_error("Invalid sampler descriptor was accepted");
}
}

void CheckSamplers() {
    using namespace AnyBox::Descriptor;
    for (const auto& reference : SamplerReferences) {
        auto description = std::bit_cast<SamplerDescription>(reference.source);
        if (EncodeSampler(description, reference.index) != reference.words) throw std::runtime_error("Sampler descriptor differs from the original encoder");
    }
    SamplerDescription description;
    description.Flags = 64;
    RejectSampler<std::runtime_error>([&] { EncodeSampler(description); });
    description.Flags = 0;
    description.AddressU = 0;
    RejectSampler<std::invalid_argument>([&] { EncodeSampler(description); });
    description.AddressU = 6;
    RejectSampler<std::invalid_argument>([&] { EncodeSampler(description); });
    description.AddressU = 3;
    description.MagFilter = 4;
    RejectSampler<std::invalid_argument>([&] { EncodeSampler(description); });
    description.MagFilter = 1;
    description.MaxAnisotropy = 0;
    RejectSampler<std::invalid_argument>([&] { EncodeSampler(description); });
    description.MaxAnisotropy = 17;
    RejectSampler<std::invalid_argument>([&] { EncodeSampler(description); });
    description.MaxAnisotropy = 16;
    description.ComparisonFunction = 9;
    RejectSampler<std::invalid_argument>([&] { EncodeSampler(description); });
    description.ComparisonFunction = 1;
    description.PerformanceMip = 16;
    RejectSampler<std::invalid_argument>([&] { EncodeSampler(description); });
    description.PerformanceMip = 0;
    auto unmodified = EncodeSampler(description);
    description.BorderColorSwizzle[0] = 8;
    if (EncodeSampler(description) != unmodified) throw std::runtime_error("Disabled sampler swizzle affected its descriptor");
    description.Flags = 0x100;
    RejectSampler<std::invalid_argument>([&] { EncodeSampler(description); });
    description.BorderColorSwizzle[0] = 0;
    RejectSampler<std::out_of_range>([&] { EncodeSampler(description, 4096); });
}
