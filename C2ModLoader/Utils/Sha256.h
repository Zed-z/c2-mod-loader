#include <string>
#include <windows.h>

namespace Sha256 {

std::string ComputeFileHash(const char *filePath);
std::string ComputeHash(const char *data);

}