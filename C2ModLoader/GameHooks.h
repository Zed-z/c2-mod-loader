#pragma once

#include <vector>

namespace GameHooks {

void RegisterHook(int hook_type, void(__stdcall *func)());

void ApplyHooks();

} // namespace GameHooks
