#include "Offline.hpp"
#include <anybox/shader/Requirements.hpp>
#include <filesystem>
#include <fstream>

AnyBox::Shader::ComputeArtifact AnyBox::Graphics::LoadPreparedCompute(std::span<const std::byte> code) {
    auto digest = Shader::HashShader(code);
    auto length = GetEnvironmentVariableW(L"ANYBOX_SHADER_CACHE", nullptr, 0);
    if (length == 0) throw std::runtime_error("ANYBOX_SHADER_CACHE must identify the prepared shader directory");
    std::wstring directory(length, L'\0');
    auto written = GetEnvironmentVariableW(L"ANYBOX_SHADER_CACHE", directory.data(), length);
    if (written == 0 || written >= length) throw std::runtime_error("Prepared shader directory could not be read consistently");
    directory.resize(written);
    auto name = Shader::DigestName(digest) + ".abxc";
    auto path = std::filesystem::path(directory) / name;
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("Prepared compute shader is missing: " + name);
    auto size = input.tellg();
    if (size <= 0 || static_cast<std::uint64_t>(size) > UINT32_MAX) throw std::length_error("Prepared compute shader size is invalid");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), size)) throw std::runtime_error("Prepared compute shader could not be read completely");
    auto artifact = Shader::DecodeComputeArtifact(bytes, digest);
    auto required = Shader::GetComputeWaveSize(code);
    if (required && artifact.waveSize != *required) throw std::runtime_error("Prepared compute shader wave size does not match its source");
    return artifact;
}
