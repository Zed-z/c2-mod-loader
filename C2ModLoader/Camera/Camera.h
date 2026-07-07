#pragma once

#include "ModApi.h"

namespace Camera {

enum class CameraMode {
	None = -1,
	Normal = 0,
	Orbit = 1,
	Freecam = 2
};
inline const char *CameraModeNames[] = {"None", "Normal", "Orbit", "Freecam"};

extern bool cameraEnabled;
extern bool orbitCamera;
extern CameraMode cameraMode;

void Setup();

void toggleOrbitCamera();
void toggleFreecam();

} // namespace Camera
