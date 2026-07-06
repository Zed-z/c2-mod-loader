#pragma once

#include "Input/Input.h"

namespace Input::Backends::SDL3 {

class Backend : public IControllerBackend {
  public:
	void PollInput() override;
	ModernInput GetState() override;
	void Setup() override;
	void vibrate(int strength, int durationMs) override;
};

} // namespace Input::Backends::SDL3
