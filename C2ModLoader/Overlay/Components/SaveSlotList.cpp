#include "SaveSlotList.h"

#include "ModApi.h"
#include "Utils/Fonts.h"
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
		if (i == *currentSaveSlotIndex) {
			slotName += " [Current]";
		}

		if (ImGui::CollapsingHeader((slotName + "###" + slotId).c_str())) {
			ImGui::Indent();

			const float itemWidth = 128;

			ImGui::Text("Info");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputText((std::string("Name##name") + slotId).c_str(), slot->name, 8);
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputText((std::string("FileName##filename") + slotId).c_str(), slot->fileName, 32);

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

			ImGui::Text("Settings");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Effect Volume##effectvolume") + slotId).c_str(), reinterpret_cast<int *>(&slot->effectVolume));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Music Volume##musicvolume") + slotId).c_str(), reinterpret_cast<int *>(&slot->musicVolume));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Dialog Volume##dialogvolume") + slotId).c_str(), reinterpret_cast<int *>(&slot->dialogVolume));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Control Method##controlmethod") + slotId).c_str(), reinterpret_cast<int *>(&slot->controlMethod));

			ImGui::Text("Inventory");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Binoculars##binoculars") + slotId).c_str(), reinterpret_cast<int *>(&slot->binoculars));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Keys##keys") + slotId).c_str(), reinterpret_cast<int *>(&slot->keys));
			ImGui::SetNextItemWidth(itemWidth);
			if (api->GetGameVersion() == GAMEVER_US) {
				ImGui::InputInt((std::string("Purple Gummis##purplegummis") + slotId).c_str(), reinterpret_cast<int *>(&slot->purpleGummis));
				ImGui::SetNextItemWidth(itemWidth);
				ImGui::InputInt((std::string("Blue Gummis##bluegummis") + slotId).c_str(), reinterpret_cast<int *>(&slot->blueGummis));
				ImGui::SetNextItemWidth(itemWidth);
				ImGui::InputInt((std::string("Green Gummis##greengummis") + slotId).c_str(), reinterpret_cast<int *>(&slot->greenGummis));
				ImGui::SetNextItemWidth(itemWidth);
			} else {
				ImGui::InputInt((std::string("Red Jellies##redjellies") + slotId).c_str(), reinterpret_cast<int *>(&slot->redJellies));
				ImGui::SetNextItemWidth(itemWidth);
				ImGui::InputInt((std::string("Orange Jellies##orangejellies") + slotId).c_str(), reinterpret_cast<int *>(&slot->orangeJellies));
				ImGui::SetNextItemWidth(itemWidth);
				ImGui::InputInt((std::string("Green Jellies##greenjellies") + slotId).c_str(), reinterpret_cast<int *>(&slot->greenJellies));
				ImGui::SetNextItemWidth(itemWidth);
			}
			ImGui::InputInt((std::string("Clockwork Gobbos##clockworkgobbos") + slotId).c_str(), reinterpret_cast<int *>(&slot->clockworkGobbos));

			ImGui::Text("Boss");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Total Boss Hearts##totalbosshearts") + slotId).c_str(), reinterpret_cast<int *>(&slot->totalBossHearts));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Boss Hearts##bosshearts") + slotId).c_str(), reinterpret_cast<int *>(&slot->bossHearts));

			ImGui::Text("Goto Level Variables");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Goto Tribe##gototribe") + slotId).c_str(), reinterpret_cast<int *>(&slot->tribe0));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Goto Level##gotolevel") + slotId).c_str(), reinterpret_cast<int *>(&slot->level0));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Goto Map##gotomap") + slotId).c_str(), reinterpret_cast<int *>(&slot->map0));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Goto Type##gototype") + slotId).c_str(), reinterpret_cast<int *>(&slot->type0));

			ImGui::Unindent();
		}
	}
	ImGui::PopFont();
	ImGui::End();
}
