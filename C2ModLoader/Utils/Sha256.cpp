#include "Sha256.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <wincrypt.h>
#include <windows.h>

#pragma comment(lib, "advapi32.lib")

namespace Sha256 {

std::string ComputeFileHash(const char *filePath) {
	HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		return "";
	}

	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;

	if (!CryptAcquireContextW(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
		CloseHandle(hFile);
		return "";
	}

	if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
		CryptReleaseContext(hProv, 0);
		CloseHandle(hFile);
		return "";
	}

	const DWORD BUFFER_SIZE = 65536;
	BYTE buffer[BUFFER_SIZE];
	DWORD bytesRead = 0;

	while (ReadFile(hFile, buffer, BUFFER_SIZE, &bytesRead, nullptr) && bytesRead > 0) {
		if (!CryptHashData(hHash, buffer, bytesRead, 0)) {
			CryptDestroyHash(hHash);
			CryptReleaseContext(hProv, 0);
			CloseHandle(hFile);
			return "";
		}
	}

	BYTE hash[32];
	DWORD hashLen = 32;
	if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		CloseHandle(hFile);
		return "";
	}

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	CloseHandle(hFile);

	// Convert hash to uppercase hex string
	std::ostringstream hashStream;
	for (int i = 0; i < 32; ++i) {
		hashStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
	}
	return hashStream.str();
}

} // namespace Sha256
