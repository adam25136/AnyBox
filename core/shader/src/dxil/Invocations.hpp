#pragma once

#include <string_view>

namespace LLVMBC { class Function; }

namespace AnyBox::Shader::Dxil {
bool IsInvocationLocalIntrinsic(std::string_view name);
bool HasIndependentInvocations(const LLVMBC::Function& function);
}
