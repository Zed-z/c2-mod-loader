#pragma once

#include <string>

namespace RegistryManager {

void InstallHooks();
void SetEnabled(bool enabled);
void SetManagedKeys(const std::string &managedKeys);

} // namespace RegistryManager
