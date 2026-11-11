#define NOMINMAX
#include "StageManager.h"

#include <Windows.h>

#include <algorithm>
#include <fstream>
#include <sstream>

void StageManager::LoadStageDataFile() {
	// "stageData.csv" is the new name; the old project called it "stageDatas.csv" - accept both
	std::ifstream file("Resources/stageData.csv");
	if (!file.is_open()) {
		file.open("Resources/stageDatas.csv");
	}
	if (!file.is_open()) {
		OutputDebugStringA("[StageManager] Resources/stageData.csv (or stageDatas.csv) not found - using a default stage\n");
		StageData fallback;
		fallback.name = "field01";
		fallback.timeLimit = 300;
		stageDatas_.push_back(fallback);
		return;
	}

	std::stringstream stageDataCsv;
	stageDataCsv << file.rdbuf();
	file.close();

	std::string line;
	while (std::getline(stageDataCsv, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		if (line.empty()) {
			continue;
		}

		StageData stageData;
		std::istringstream lineStream(line);
		std::string word;

		std::getline(lineStream, word, ',');
		if (word.empty()) {
			continue;
		}
		stageData.name = word;

		// a column that is not a plain number keeps its default instead of throwing
		auto readInt = [&lineStream, &word](int32_t& out) {
			if (std::getline(lineStream, word, ',') && !word.empty()) {
				try {
					out = std::stoi(word);
				} catch (...) {
				}
			}
		};
		readInt(stageData.timeLimit);
		readInt(stageData.chunksX);
		readInt(stageData.chunksZ);
		stageData.chunksX = std::clamp(stageData.chunksX, 1, 5);
		stageData.chunksZ = std::clamp(stageData.chunksZ, 1, 5);

		stageDatas_.push_back(stageData);
	}

	if (stageDatas_.empty()) { // file exists but has no usable row
		OutputDebugStringA("[StageManager] stage data file is empty - using a default stage\n");
		StageData fallback;
		fallback.name = "field01";
		fallback.timeLimit = 300;
		stageDatas_.push_back(fallback);
	}
}

void StageManager::SetCurrentStageIndexByName(const std::string& name) {
	for (size_t i = 0; i < stageDatas_.size(); ++i) {
		if (stageDatas_[i].name == name) {
			currentStageIndex_ = static_cast<int32_t>(i);
			return;
		}
	}
	OutputDebugStringA(("[StageManager] no stage named '" + name + "' - keeping the current stage\n").c_str());
}

std::string StageManager::GetChunkFileName(int32_t stageIndex, uint32_t chunkX, uint32_t chunkZ) const {
	std::string path = "Resources/fields/" + GetStageData(stageIndex).name;
	if (chunkX > 0) {
		path += "_x" + std::to_string(chunkX);
	}
	if (chunkZ > 0) {
		path += "_z" + std::to_string(chunkZ);
	}
	return path + ".csv";
}