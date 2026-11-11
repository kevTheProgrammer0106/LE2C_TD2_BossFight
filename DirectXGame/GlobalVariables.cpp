#include "GlobalVariables.h"

#include <json.hpp>
using json = nlohmann::json;

#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace KamataEngine;

GlobalVariables* GlobalVariables::GetInstance() {
	static GlobalVariables instance;
	return &instance;
}

void GlobalVariables::CreateGroup(const std::string& groupName) { datas_[groupName]; }

// ------------------------------------------------------------------ ImGui
// 独立ウィンドウ + DebugMenuと同じタブバー。タブは1つ("All Variables")に全グループを入れる。
void GlobalVariables::Update() {
#ifdef _DEBUG
	static ImGuiTextFilter filter;

	if (!ImGui::Begin("Global Variables")) {
		ImGui::End();
		return;
	}

	if (ImGui::BeginTabBar("GlobalVariablesTabs")) {
		if (ImGui::BeginTabItem("All Variables")) {
			ImGui::TextDisabled("Save writes Resources/GlobalVariables/<Group>.json");
			filter.Draw("Filter (group / item)");
			ImGui::SameLine();
			if (ImGui::Button("Save All"))
				SaveAllFiles();

			if (statusTimer_ > 0.0f) {
				statusTimer_ -= 1.0f / 60.0f;
				ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "%s", statusMessage_.c_str());
			}
			ImGui::Separator();

			ImGui::BeginChild("GlobalVariablesList", ImVec2(0, 0), false);
			for (auto& [groupName, group] : datas_) {
				const bool groupMatches = filter.PassFilter(groupName.c_str());

				// フィルタ中は、グループ名にも項目名にも一致しないグループを隠す
				if (filter.IsActive() && !groupMatches) {
					bool anyItemMatches = false;
					for (auto& [itemName, item] : group.items) {
						if (filter.PassFilter(itemName.c_str())) {
							anyItemMatches = true;
							break;
						}
					}
					if (!anyItemMatches)
						continue;
				}

				ImGui::PushID(groupName.c_str());
				if (filter.IsActive())
					ImGui::SetNextItemOpen(true, ImGuiCond_Always);

				if (ImGui::CollapsingHeader(groupName.c_str())) {
					for (auto& [itemName, item] : group.items) {
						if (filter.IsActive() && !groupMatches && !filter.PassFilter(itemName.c_str()))
							continue;

						// min=max=0 は範囲制限なし(負値や100超えも編集できる)
						if (int32_t* i = std::get_if<int32_t>(&item.value)) {
							ImGui::DragInt(itemName.c_str(), i, 0.1f);
						} else if (float* f = std::get_if<float>(&item.value)) {
							ImGui::DragFloat(itemName.c_str(), f, 0.01f);
						} else if (Vector3* v = std::get_if<Vector3>(&item.value)) {
							ImGui::DragFloat3(itemName.c_str(), &v->x, 0.01f);
						}
					}
					if (ImGui::Button("Save"))
						SaveFile(groupName);
				}
				ImGui::PopID();
			}
			ImGui::EndChild();

			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
#endif
}

// ------------------------------------------------------------------ File IO
void GlobalVariables::SaveFile(const std::string& groupName) {
	auto itGroup = datas_.find(groupName);
#ifdef _DEBUG
	assert(itGroup != datas_.end());
#endif
	if (itGroup == datas_.end())
		return;

	json root = json::object();
	root[groupName] = json::object();

	for (const auto& [itemName, item] : itGroup->second.items) {
		if (const int32_t* i = std::get_if<int32_t>(&item.value)) {
			root[groupName][itemName] = *i;
		} else if (const float* f = std::get_if<float>(&item.value)) {
			root[groupName][itemName] = *f;
		} else if (const Vector3* v = std::get_if<Vector3>(&item.value)) {
			root[groupName][itemName] = json::array({v->x, v->y, v->z});
		}
	}

	if (!std::filesystem::exists(kDirectoryPath)) {
		std::filesystem::create_directories(kDirectoryPath);
	}

	std::string filePath = kDirectoryPath + groupName + ".json";
	std::ofstream ofs(filePath);
	if (ofs.fail()) {
		statusMessage_ = std::format("Failed to write {}", filePath);
		statusTimer_ = 4.0f;
		return;
	}
	ofs << std::setw(4) << root << std::endl;

	statusMessage_ = std::format("{}.json saved", groupName);
	statusTimer_ = 2.0f;
}

void GlobalVariables::SaveAllFiles() {
	for (const auto& [groupName, group] : datas_) {
		SaveFile(groupName);
	}
	statusMessage_ = std::format("Saved {} groups", datas_.size());
	statusTimer_ = 2.0f;
}

void GlobalVariables::LoadFiles() {
	if (!std::filesystem::exists(kDirectoryPath)) {
		return;
	}
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(kDirectoryPath)) {
		const std::filesystem::path& filePath = entry.path();
		if (filePath.extension().string() != ".json")
			continue;
		LoadFile(filePath.stem().string());
	}
}

void GlobalVariables::LoadFile(const std::string& groupName) {
	std::string filePath = kDirectoryPath + groupName + ".json";
	std::ifstream ifs(filePath);
	if (ifs.fail()) {
		statusMessage_ = std::format("Failed to open {}", filePath);
		statusTimer_ = 4.0f;
		return;
	}

	json root;
	ifs >> root;
	ifs.close();

	json::iterator itGroup = root.find(groupName);
	if (itGroup == root.end()) {
		return; // ファイル内にグループが無ければ無視
	}

	for (json::iterator itItem = itGroup->begin(); itItem != itGroup->end(); ++itItem) {
		const std::string& itemName = itItem.key();

		if (itItem->is_number_integer()) {
			SetValue(groupName, itemName, itItem->get<int32_t>());
		} else if (itItem->is_number_float()) {
			SetValue(groupName, itemName, itItem->get<float>());
		} else if (itItem->is_array() && itItem->size() == 3) {
			SetValue(groupName, itemName, Vector3{itItem->at(0), itItem->at(1), itItem->at(2)});
		}
	}
}