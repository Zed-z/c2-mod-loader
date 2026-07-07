#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace Camera::Utils {

using std::sin, std::cos, std::min, std::max;

double LerpAngle(double a, double b, double t) {
	double diff = fmod(b - a + PI, 2 * PI);
	if (diff < 0) {
		diff += 2 * PI;
	}
	diff -= PI;
	return a + diff * t;
}

int RadiansToGameRotation(double radians_input) {
	radians_input = fmod(radians_input, 2 * PI);
	while (radians_input <= -PI) {
		radians_input += 2 * PI;
	}
	while (radians_input > PI) {
		radians_input -= 2 * PI;
	}

	double scaled_value = radians_input * (2048.0 / PI);

	int game_rotation_value = static_cast<int>(round(scaled_value));

	if (game_rotation_value == 2048) {
		game_rotation_value = -2048;
	}

	return game_rotation_value;
}

double GameRotationToRadians(int game_rotation) {
	return (game_rotation * PI) / 2048.0;
}

} // namespace Camera::Utils
