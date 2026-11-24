#include <anybox/graphics/commands/Inline.hpp>
#include <stdexcept>

using namespace AnyBox::Graphics::Commands;
using AnyBox::Graphics::Container::RootParameterKind;

std::uint32_t AnyBox::Graphics::Commands::RootHeader(std::uint32_t parameter) {
    if (parameter >= 64) throw std::out_of_range("Inline root header parameter exceeds its six-bit field");
    return 0xC0000000U | (parameter << 10);
}

InlineCommand InlineReader::Next(const RootState* root) {
    if (words.empty()) throw std::out_of_range("Inline command stream is empty");
    auto header = words.front();
    if (header == PipelineHeader) {
        if (words.size() < PipelineWordCount) throw std::invalid_argument("Inline pipeline binding is truncated");
        auto address = words[1] | (static_cast<std::uint64_t>(words[2]) << 32);
        if (address == 0 || (address & 15) != 0) throw std::invalid_argument("Inline pipeline address is null or misaligned");
        for (unsigned index = 3; index < PipelineWordCount; ++index) {
            if (words[index] != 0) throw std::invalid_argument("Inline pipeline reserved field is nonzero");
        }
        words = words.subspan(PipelineWordCount);
        return PipelineBind{address};
    }
    if (header == 0x4002C400 || header == 0x4002E800 || header == 0x4002E804) {
        if (words.size() < 4) throw std::invalid_argument("Inline dispatch record is truncated");
        Dispatch result{{words[1], words[2], words[3]}, header};
        words = words.subspan(4);
        return result;
    }
    if ((header & 0xFFC00003U) != 0xC0000000U) throw std::runtime_error("Inline command header is not implemented");
    if (!root) throw std::logic_error("Inline root update requires a bound root signature");
    auto parameter = (header >> 10) & 63;
    auto offset = (header >> 2) & 255;
    auto count = ((header >> 16) & 63) + 1;
    auto kind = root->GetKind(parameter);
    if (kind != RootParameterKind::Constants) {
        if (count != 1 || offset != 0) throw std::invalid_argument("Inline root descriptor or table header is invalid");
        count = root->GetWordCount(parameter);
    }
    auto size = root->GetWordCount(parameter);
    if (offset > size || count > size - offset) throw std::out_of_range("Inline root update exceeds the parameter size");
    if (words.size() - 1 < count) throw std::invalid_argument("Inline root update is truncated");
    RootWrite result{parameter, offset, words.subspan(1, count)};
    words = words.subspan(1 + count);
    return result;
}
