#pragma once

#include <vector>

namespace Launcher {

struct EmbeddedLicense {
	const char *displayName;
	const char *content;
};

const std::vector<EmbeddedLicense> &GetEmbeddedLicenses();

} // namespace Launcher
