#include <anybox/descriptor/Texture.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>

void CheckColorTargets();
void CheckStorageViews();
void CheckSamplers();

namespace {
template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    try { callable(); } catch (const TException&) { return; }
    throw std::runtime_error("Invalid texture descriptor was accepted");
}

void CheckReferences() {
    using namespace AnyBox::Descriptor;
    TextureViewDescription view;
    view.address = 0xFF1234567800;
    auto words = EncodeTexture2D({63, 17, 1, 1, 56, 27}, view);
    constexpr ImageWords single{0x12345678, 0x838000FF, 0x8004000F, 0xD1B00FAC, 0, 0x80400000, 0, 0};
    if (words != single) throw std::runtime_error("Single texture descriptor differs from its native reference");

    view.firstMip = 2;
    view.mipCount = 5;
    view.minimumLod = 2.5f;
    words = EncodeTexture2D({1024, 1024, 1, 11, 56, 27}, view);
    constexpr ImageWords mips{0x12345678, 0xC38280FF, 0x80FFC0FF, 0xD1B62FAC, 0, 0x804000A0, 0, 0};
    if (words != mips) throw std::runtime_error("Mip texture descriptor differs from its native reference");

    view.firstMip = 1;
    view.mipCount = UINT32_MAX;
    view.firstLayer = 2;
    view.minimumLod = 0;
    words = EncodeTexture2D({256, 256, 6, 9, 56, 27}, view);
    constexpr ImageWords arrays{0x12345678, 0xC38100FF, 0x803FC03F, 0xD1B81FAC, 0x00020005, 0x80400080, 0, 0};
    if (words != arrays) throw std::runtime_error("Array texture descriptor differs from its native reference");
}

void CheckInvalid() {
    using namespace AnyBox::Descriptor;
    Texture2DDescription texture{63, 17, 1, 1, 56, 27};
    TextureViewDescription view;
    view.address = 1;
    Reject<std::invalid_argument>([&] { EncodeTexture2D(texture, view); });
    view.address = 0;
    view.firstMip = 1;
    Reject<std::out_of_range>([&] { EncodeTexture2D(texture, view); });
    view.firstMip = 0;
    view.layerCount = 2;
    Reject<std::out_of_range>([&] { EncodeTexture2D(texture, view); });
    view.layerCount = 1;
    view.componentMapping = 7;
    Reject<std::invalid_argument>([&] { EncodeTexture2D(texture, view); });
    view.componentMapping = 0x1688;
    view.minimumLod = std::numeric_limits<float>::quiet_NaN();
    Reject<std::invalid_argument>([&] { EncodeTexture2D(texture, view); });
    view.minimumLod = 0;
    texture.swizzleMode = 31;
    Reject<std::runtime_error>([&] { EncodeTexture2D(texture, view); });
}
}

int main() {
    try { CheckReferences(); CheckInvalid(); CheckColorTargets(); CheckStorageViews(); CheckSamplers(); return 0; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
