#pragma once

#include <KamataEngine.h>
#include <cassert>
#include <string>
#include <vector>

// One row of Resources/stageData.csv:  name,timeLimit,chunksX,chunksZ   (the last two are optional, default 1)
struct StageData {
	std::string name;
	int32_t timeLimit = 0;
	int32_t chunksX = 1; // chunks along X
	int32_t chunksZ = 1; // chunks along Z
};

class StageManager {
public:
	// Kept so main.cpp's menu switch stays unchanged. Only the player position is remembered for now.
	struct CheckpointData {
		bool hasCheckpoint = false;
		KamataEngine::Vector3 playerPosition{};
	};
	void SetCheckpoint(const CheckpointData& data) { checkpoint_ = data; }
	const CheckpointData& GetCheckpoint() const { return checkpoint_; }
	bool HasCheckpoint() const { return checkpoint_.hasCheckpoint; }
	void ClearCheckpoint() { checkpoint_ = CheckpointData{}; }

	void LoadStageDataFile();

	const StageData& GetStageData(int32_t index) const {
#ifdef _DEBUG
		assert(index >= 0 && index < static_cast<int32_t>(stageDatas_.size()));
#endif
		return stageDatas_[index];
	}
	void SetCurrentStageIndex(int32_t index) {
#ifdef _DEBUG
		assert(index >= 0 && index < static_cast<int32_t>(stageDatas_.size()));
#endif
		currentStageIndex_ = index;
	}
	int32_t GetCurrentStageIndex() const { return currentStageIndex_; }
	int32_t GetStageCount() const { return static_cast<int32_t>(stageDatas_.size()); }
	void SetCurrentStageIndexByName(const std::string& name);
	const StageData& GetCurrentStageData() const { return GetStageData(currentStageIndex_); }

	// Chunk files: (0,0) "name.csv"   (x,0) "name_x1.csv"   (0,z) "name_z1.csv"   (x,z) "name_x1_z1.csv"
	// A chunk file that does not exist is fine - that chunk is just blank.
	std::string GetChunkFileName(int32_t stageIndex, uint32_t chunkX, uint32_t chunkZ) const;

private:
	int32_t currentStageIndex_ = 0;
	std::vector<StageData> stageDatas_;
	CheckpointData checkpoint_;
};
