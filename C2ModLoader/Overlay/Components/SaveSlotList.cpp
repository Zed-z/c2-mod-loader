#include "SaveSlotList.h"

#include "ModApi.h"
#include "Overlay/Fonts.h"
#include "imgui.h"

#include <string>

extern ModApi *api;

bool showSaveSlotList;

void RenderSaveSlotList() {
	if (!showSaveSlotList)
		return;

	ImGui::SetNextWindowSizeConstraints(ImVec2(300, 400), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::Begin("Save Slot List");
	ImGui::PushFont(Fonts::GetFontCode());
	for (int i = 0; i < SAVE_SLOT_NUMBER; i++) {
		SaveSlot *slot = api->GetSaveSlot(i);
		std::string slotId = std::string("slot") + std::to_string(i);
		std::string slotName = std::string("Slot ") + std::to_string(i + 1) + " - " + slot->name;
		/*if (i == api->AddressGetInt(ADDR_CURRENT_SAVE_SLOT)) {
			slotName += " (Current)";
		}*/

		if (ImGui::CollapsingHeader((slotName + "##" + slotId).c_str())) {
			ImGui::Indent();

			const float itemWidth = 128;

			ImGui::Text("Info");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputText((std::string("Name##name") + slotId).c_str(), slot->name, 4);
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputText((std::string("Tribe##tribe") + slotId).c_str(), slot->tribe, 16);

			ImGui::Text("Stats");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Heart Pots##heartpots") + slotId).c_str(), reinterpret_cast<int *>(&slot->heartPots));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Health##health") + slotId).c_str(), reinterpret_cast<int *>(&slot->health));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Total Crystals##totalcrystals") + slotId).c_str(), reinterpret_cast<int *>(&slot->crystals));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Level Crystals##levelcrystals") + slotId).c_str(), reinterpret_cast<int *>(&slot->levelCrystals));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Golden Gobbos##goldengobbos") + slotId).c_str(), reinterpret_cast<int *>(&slot->goldenGobbos));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Jigsaw Pieces##jigsawpieces") + slotId).c_str(), reinterpret_cast<int *>(&slot->jigsawPieces));

			ImGui::Text("Inventory");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Binoculars##binoculars") + slotId).c_str(), reinterpret_cast<int *>(&slot->binoculars));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Keys##keys") + slotId).c_str(), reinterpret_cast<int *>(&slot->keys));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Purple Gummis##purplegummis") + slotId).c_str(), reinterpret_cast<int *>(&slot->purpleGummis));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Blue Gummis##bluegummis") + slotId).c_str(), reinterpret_cast<int *>(&slot->blueGummis));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Green Gummis##greengummis") + slotId).c_str(), reinterpret_cast<int *>(&slot->greenGummis));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Clockwork Gobbos##clockworkgobbos") + slotId).c_str(), reinterpret_cast<int *>(&slot->clockworkGobbos));

			ImGui::Text("Boss");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Total Boss Hearts##totalbosshearts") + slotId).c_str(), reinterpret_cast<int *>(&slot->totalBossHearts));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Boss Hearts##bosshearts") + slotId).c_str(), reinterpret_cast<int *>(&slot->bossHearts));

			ImGui::Unindent();
		}
	}
	ImGui::PopFont();
	ImGui::End();
}
