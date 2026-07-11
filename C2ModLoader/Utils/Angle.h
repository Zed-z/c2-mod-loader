#pragma once

namespace Angle {

constexpr double PI = 3.14159265358979323846;

double LerpAngle(double a, double b, double t);

int RadiansToGameRotation(double radians_input);

double GameRotationToRadians(int game_rotation);

} // namespace Angle
