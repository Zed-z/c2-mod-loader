#include "Inputs.h"

#include "ModApi.h"
#include "Utils/Fonts.h"
#include "imgui.h"
#include <bitset>
#include <sstream>

extern ModApi *api;

bool showInputs;

void RenderInputs() {
	if (!showInputs)
		return;

	Inputs inputs = api->GetInputs();
	std::bitset<32> inputBits(inputs.raw);

	int analogStrength = api->AddressGetInt(ADDR_ANALOG_STRENGTH);

	int saveSlotOffset = api->AddressGetInt(ADDR_CURRENT_SAVE_SLOT) * ADDR_SAVE_SLOT_OFFSET;
	int controlScheme = api->AddressGetInt(ADDR_CONTROL_SCHEME_SLOT + saveSlotOffset);

	ImGui::Begin("Inputs");
	ImGui::PushFont(Fonts::GetFontCode());

	std::ostringstream ss0;
	ss0 << "Inputs: " << inputBits << " (" << inputs.raw << ")";

	std::ostringstream ss1;
	ss1 << "User input | Up: " << inputs.up << ", Down: " << inputs.down << ", Left: " << inputs.left << ", Right: " << inputs.right;

	std::ostringstream ss2;
	ss2 << "Effective  | Up: " << inputs.effectiveUp << ", Down: " << inputs.effectiveDown << ", Left: " << inputs.effectiveLeft << ", Right: " << inputs.effectiveRight;

	std::ostringstream ss3;
	ss3 << "Analog Strength: " << analogStrength;

	std::ostringstream ss4;
	ss4 << "Jump: " << inputs.jump << ", Attack: " << inputs.attack;

	std::ostringstream ss5;
	ss5 << "Flip: " << inputs.flip << ", Step Left: " << inputs.stepLeft << ", Step Right: " << inputs.stepRight;

	std::ostringstream ss6;
	ss6 << "Inv Use : " << inputs.invUse << ", Inv Left: " << inputs.invLeft << ", Inv Right: " << inputs.invRight;

	std::ostringstream ss7;
	ss7 << "Control Method: " << ControlSchemeNames[controlScheme];

	ImGui::Text(ss0.str().c_str());
	ImGui::Text(ss1.str().c_str());
	ImGui::Text(ss2.str().c_str());
	ImGui::Text(ss3.str().c_str());
	ImGui::Text(ss4.str().c_str());
	ImGui::Text(ss5.str().c_str());
	ImGui::Text(ss6.str().c_str());
	ImGui::Text(ss7.str().c_str());

	ImGui::PopFont();
	ImGui::End();
}
