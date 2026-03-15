#include "EmbeddedLicenses.h"
#include "GeneratedEmbeddedLicenses.h"

namespace Launcher {

const std::vector<EmbeddedLicense>& GetEmbeddedLicenses() {
    return GetGeneratedEmbeddedLicenses();
}

}
