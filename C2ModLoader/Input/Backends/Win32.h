#pragma once

#include "Input/Input.h"

#include <string>

namespace Input::Backends::Win32 {

class Backend : public IKeyboardBackend {
  public:
	void Setup() override;
	bool getKey(const std::string &keyName) override;
};

} // namespace Input::Backends::Win32
