#include "Prepare.hpp"
#include "Compilation.hpp"
#include "Files.hpp"
#include <anybox/graphics/container/Embedded.hpp>
#include <anybox/graphics/container/Dxil.hpp>
#include <iostream>
#include <unordered_set>

void AnyBox::Shader::Tools::PrepareCompute(const std::filesystem::path& binary, const std::filesystem::path& directory) {
    auto data = ReadInput(binary);
    auto containers = Graphics::Container::FindEmbeddedDxbc(data);
    if (containers.empty()) throw std::runtime_error("Input contains no embedded DXBC containers");
    std::filesystem::create_directories(directory);
    std::unordered_set<std::string> processed;
    unsigned compiled = 0;
    unsigned reused = 0;
    unsigned skipped = 0;
    std::cout << "containers=" << containers.size() << std::endl;
    for (const auto& item : containers) {
        Graphics::Container::DxbcView container(item.data);
        auto part = container.FindPart(0x4C495844);
        if (!part) throw std::runtime_error("Embedded container has no supported DXIL program at offset " + std::to_string(item.offset));
        Graphics::Container::DxilView program(part->data);
        if ((program.GetProgramVersion() >> 16) != 5) {
            ++skipped;
            continue;
        }
        auto digest = HashShader(item.data);
        auto name = DigestName(digest);
        if (!processed.insert(name).second) continue;
        std::cout << "offset=" << item.offset << " source=" << name << std::endl;
        auto path = directory / (name + ".abxc");
        if (std::filesystem::exists(path)) {
            (void)DecodeComputeArtifact(ReadInput(path), digest);
            ++reused;
            std::cout << "cached source=" << name << std::endl;
        } else {
            auto artifact = CompileDxil(item.data);
            WriteOutput(path, EncodeComputeArtifact(artifact));
            ++compiled;
            std::cout << "compiled source=" << name << " words=" << artifact.code.size() << std::endl;
        }
    }
    std::cout << "compute=" << processed.size() << " compiled=" << compiled << " cached=" << reused << " non_compute=" << skipped << std::endl;
    if (!std::cout) throw std::runtime_error("Cannot write shader preparation results");
}
