#pragma once

namespace Overlay::Toast {

extern bool showToastInfo;
extern bool showToastDebug;
extern bool showToastWarning;
extern bool showToastError;

void Setup();

void RenderToasts();

} // namespace Overlay::Toast
