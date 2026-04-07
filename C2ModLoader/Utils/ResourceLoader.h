#pragma once

#include <imgui.h>
#include <windows.h>

struct ID3D11Device;
struct ID3D11ShaderResourceView;

namespace ResourceLoader {

bool LoadTextureFromPngResource(HMODULE module, int resourceId, ID3D11Device *device, ID3D11ShaderResourceView **outTexture, float *outWidth, float *outHeight);
HICON LoadIconFromPngResource(HMODULE module, int resourceId, int targetSize);
ImFont *LoadTextFont(HMODULE module, int resourceId, float fontSize);

} // namespace ResourceLoader
