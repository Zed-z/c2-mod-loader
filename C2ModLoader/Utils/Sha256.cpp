#include "Sha256.h"

#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <wincrypt.h>
#include <windows.h>

#pragma comment(lib, "advapi32.lib")

namespace Sha256 {

namespace {

std::string HashToHex(const BYTE *hash, DWORD hashLength) {
	std::ostringstream hashStream;
	hashStream << std::hex << std::uppercase << std::setfill('0');
	for (DWORD i = 0; i < hashLength; ++i) {
		hashStream << std::setw(2) << static_cast<int>(hash[i]);
	}
	return hashStream.str();
}

void DestroyHashContext(HCRYPTPROV hProv, HCRYPTHASH hHash) {
	if (hHash) {
		CryptDestroyHash(hHash);
	}
	if (hProv) {
		CryptReleaseContext(hProv, 0);
	}
}

bool CreateHashContext(HCRYPTPROV &hProv, HCRYPTHASH &hHash) {
	if (!CryptAcquireContextW(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
		return false;
	}

	if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
		CryptReleaseContext(hProv, 0);
		hProv = 0;
		return false;
	}

	return true;
}

std::string FinalizeHash(HCRYPTPROV hProv, HCRYPTHASH hHash) {
	BYTE hash[32];
	DWORD hashLen = sizeof(hash);

	if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
		DestroyHashContext(hProv, hHash);
		return "";
	}

	DestroyHashContext(hProv, hHash);
	return HashToHex(hash, hashLen);
}

} // namespace

std::string ComputeFileHash(const char *filePath) {
	HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		return "";
	}

	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	if (!CreateHashContext(hProv, hHash)) {
		CloseHandle(hFile);
		return "";
	}

	const DWORD BUFFER_SIZE = 65536;
	BYTE buffer[BUFFER_SIZE];
	DWORD bytesRead = 0;

	while (ReadFile(hFile, buffer, BUFFER_SIZE, &bytesRead, nullptr) && bytesRead > 0) {
		if (!CryptHashData(hHash, buffer, bytesRead, 0)) {
			DestroyHashContext(hProv, hHash);
			CloseHandle(hFile);
			return "";
		}
	}

	CloseHandle(hFile);
	return FinalizeHash(hProv, hHash);
}

std::string ComputeHash(const char *data) {
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	if (!CreateHashContext(hProv, hHash)) {
		return "";
	}

	DWORD dataLen = static_cast<DWORD>(strlen(data));
	if (!CryptHashData(hHash, reinterpret_cast<const BYTE *>(data), dataLen, 0)) {
		DestroyHashContext(hProv, hHash);
		return "";
	}

	return FinalizeHash(hProv, hHash);
}

} // namespace Sha256
