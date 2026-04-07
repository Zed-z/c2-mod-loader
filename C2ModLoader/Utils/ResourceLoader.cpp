#include "ResourceLoader.h"

#include "Utils.h"
#include "imgui.h"

#include <d3d11.h>
#include <vector>
#include <wincodec.h>

namespace {

bool LoadResourceBytes(HMODULE module, int resourceId, const BYTE **resourceData, DWORD *resourceSize) {
	if (!module || !resourceData || !resourceSize)
		return false;

	HRSRC hResource = FindResourceW(module, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
	if (!hResource)
		return false;

	HGLOBAL hData = LoadResource(module, hResource);
	if (!hData)
		return false;

	void *data = LockResource(hData);
	if (!data)
		return false;

	const DWORD size = SizeofResource(module, hResource);
	if (size == 0)
		return false;

	*resourceData = static_cast<const BYTE *>(data);
	*resourceSize = size;
	return true;
}

} // namespace

namespace ResourceLoader {

bool LoadTextureFromPngResource(HMODULE module, int resourceId, ID3D11Device *device, ID3D11ShaderResourceView **outTexture, float *outWidth, float *outHeight) {
	if (!device || !outTexture)
		return false;

	if (!module)
		module = GetCallingModule();

	const BYTE *resourceData = nullptr;
	DWORD resourceSize = 0;
	if (!LoadResourceBytes(module, resourceId, &resourceData, &resourceSize))
		return false;

	HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	const bool comInitializedHere = SUCCEEDED(hrCom);

	IWICImagingFactory *factory = nullptr;
	IWICStream *stream = nullptr;
	IWICBitmapDecoder *decoder = nullptr;
	IWICBitmapFrameDecode *frame = nullptr;
	IWICFormatConverter *converter = nullptr;
	ID3D11Texture2D *texture = nullptr;
	D3D11_TEXTURE2D_DESC textureDesc = {};
	D3D11_SUBRESOURCE_DATA subresourceData = {};
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	std::vector<unsigned char> pixels;
	UINT width = 0;
	UINT height = 0;
	UINT stride = 0;
	UINT imageSize = 0;
	ID3D11ShaderResourceView *shaderResourceView = nullptr;

	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
		goto cleanup;

	if (FAILED(factory->CreateStream(&stream)))
		goto cleanup;

	if (FAILED(stream->InitializeFromMemory(const_cast<BYTE *>(resourceData), resourceSize)))
		goto cleanup;

	if (FAILED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder)))
		goto cleanup;

	if (FAILED(decoder->GetFrame(0, &frame)))
		goto cleanup;

	if (FAILED(factory->CreateFormatConverter(&converter)))
		goto cleanup;

	if (FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
		goto cleanup;

	if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0)
		goto cleanup;

	stride = width * 4;
	imageSize = stride * height;
	pixels.resize(imageSize);

	if (FAILED(converter->CopyPixels(nullptr, stride, imageSize, pixels.data())))
		goto cleanup;

	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	subresourceData.pSysMem = pixels.data();
	subresourceData.SysMemPitch = stride;

	if (FAILED(device->CreateTexture2D(&textureDesc, &subresourceData, &texture)))
		goto cleanup;

	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	if (FAILED(device->CreateShaderResourceView(texture, &srvDesc, &shaderResourceView)))
		goto cleanup;

cleanup:
	if (texture)
		texture->Release();
	if (converter)
		converter->Release();
	if (frame)
		frame->Release();
	if (decoder)
		decoder->Release();
	if (stream)
		stream->Release();
	if (factory)
		factory->Release();
	if (comInitializedHere)
		CoUninitialize();

	if (!shaderResourceView)
		return false;

	*outTexture = shaderResourceView;
	if (outWidth)
		*outWidth = static_cast<float>(width);
	if (outHeight)
		*outHeight = static_cast<float>(height);

	return true;
}

HICON LoadIconFromPngResource(HMODULE module, int resourceId, int targetSize) {
	if (targetSize <= 0)
		return nullptr;

	if (!module)
		module = GetCallingModule();

	const BYTE *resourceData = nullptr;
	DWORD resourceSize = 0;
	if (!LoadResourceBytes(module, resourceId, &resourceData, &resourceSize))
		return nullptr;

	HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	const bool comInitializedHere = SUCCEEDED(hrCom);

	IWICImagingFactory *factory = nullptr;
	IWICStream *stream = nullptr;
	IWICBitmapDecoder *decoder = nullptr;
	IWICBitmapFrameDecode *frame = nullptr;
	IWICBitmapScaler *scaler = nullptr;
	IWICFormatConverter *converter = nullptr;
	HBITMAP colorBitmap = nullptr;
	HBITMAP maskBitmap = nullptr;
	HICON icon = nullptr;

	std::vector<unsigned char> pixels;
	UINT stride = 0;
	UINT imageSize = 0;
	void *dibBits = nullptr;
	HDC hdc = nullptr;
	BITMAPV5HEADER bi = {};
	ICONINFO ii = {};

	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
		goto cleanup;

	if (FAILED(factory->CreateStream(&stream)))
		goto cleanup;

	if (FAILED(stream->InitializeFromMemory(const_cast<BYTE *>(resourceData), resourceSize)))
		goto cleanup;

	if (FAILED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder)))
		goto cleanup;

	if (FAILED(decoder->GetFrame(0, &frame)))
		goto cleanup;

	if (FAILED(factory->CreateBitmapScaler(&scaler)))
		goto cleanup;

	if (FAILED(scaler->Initialize(frame, (UINT)targetSize, (UINT)targetSize, WICBitmapInterpolationModeFant)))
		goto cleanup;

	if (FAILED(factory->CreateFormatConverter(&converter)))
		goto cleanup;

	if (FAILED(converter->Initialize(scaler, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
		goto cleanup;

	stride = (UINT)targetSize * 4;
	imageSize = stride * (UINT)targetSize;
	pixels.resize(imageSize);

	if (FAILED(converter->CopyPixels(nullptr, stride, imageSize, pixels.data())))
		goto cleanup;

	bi.bV5Size = sizeof(BITMAPV5HEADER);
	bi.bV5Width = targetSize;
	bi.bV5Height = -targetSize;
	bi.bV5Planes = 1;
	bi.bV5BitCount = 32;
	bi.bV5Compression = BI_BITFIELDS;
	bi.bV5RedMask = 0x00FF0000;
	bi.bV5GreenMask = 0x0000FF00;
	bi.bV5BlueMask = 0x000000FF;
	bi.bV5AlphaMask = 0xFF000000;

	hdc = GetDC(nullptr);
	colorBitmap = CreateDIBSection(hdc, reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS, &dibBits, nullptr, 0);
	if (hdc)
		ReleaseDC(nullptr, hdc);
	if (!colorBitmap || !dibBits)
		goto cleanup;

	memcpy(dibBits, pixels.data(), imageSize);

	maskBitmap = CreateBitmap(targetSize, targetSize, 1, 1, nullptr);
	if (!maskBitmap)
		goto cleanup;

	ii.fIcon = TRUE;
	ii.xHotspot = 0;
	ii.yHotspot = 0;
	ii.hbmColor = colorBitmap;
	ii.hbmMask = maskBitmap;
	icon = CreateIconIndirect(&ii);

cleanup:
	if (maskBitmap)
		DeleteObject(maskBitmap);
	if (colorBitmap)
		DeleteObject(colorBitmap);
	if (converter)
		converter->Release();
	if (scaler)
		scaler->Release();
	if (frame)
		frame->Release();
	if (decoder)
		decoder->Release();
	if (stream)
		stream->Release();
	if (factory)
		factory->Release();
	if (comInitializedHere)
		CoUninitialize();

	return icon;
}

ImFont *LoadTextFont(HMODULE module, int resourceId, float fontSize) {
	ImGuiIO &io = ImGui::GetIO();

	if (!module)
		module = GetCallingModule();

	if (!module)
		return io.Fonts->AddFontDefault();

	HRSRC hResource = FindResourceW(module, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
	if (!hResource)
		return io.Fonts->AddFontDefault();

	HGLOBAL hData = LoadResource(module, hResource);
	if (!hData)
		return io.Fonts->AddFontDefault();

	void *resourceData = LockResource(hData);
	if (!resourceData)
		return io.Fonts->AddFontDefault();

	const DWORD resourceSize = SizeofResource(module, hResource);
	if (resourceSize == 0)
		return io.Fonts->AddFontDefault();

	void *fontDataCopy = IM_ALLOC(resourceSize);
	if (!fontDataCopy)
		return io.Fonts->AddFontDefault();
	memcpy(fontDataCopy, resourceData, resourceSize);

	ImFontConfig fontConfig;
	fontConfig.FontDataOwnedByAtlas = true;
	ImFont *font = io.Fonts->AddFontFromMemoryTTF(fontDataCopy, (int)resourceSize, fontSize, &fontConfig);
	if (!font)
		return io.Fonts->AddFontDefault();

	return font;
}

} // namespace ResourceLoader
