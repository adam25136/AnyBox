#pragma once

#include <filesystem>

namespace AnyBox::Shader::Tools {
void PrepareCompute(const std::filesystem::path& binary, const std::filesystem::path& directory);
}
