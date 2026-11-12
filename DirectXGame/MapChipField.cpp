#define NOMINMAX

#include "MapChipField.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace KamataEngine;

void MapChipField::ResetMapChipData() { data_.assign(totalBlockDepth_, std::vector<MapChipDataUnit>(totalBlockHorizontal_)); }

void MapChipField::Initialize(uint32_t chunkCountX, uint32_t chunkCountZ) {
	chunkCountX_ = std::clamp(chunkCountX, 1u, kMaxChunksX);
	chunkCountZ_ = std::clamp(chunkCountZ, 1u, kMaxChunksZ);
	totalBlockHorizontal_ = kChunkBlockSize * chunkCountX_;
	totalBlockDepth_ = kChunkBlockSize * chunkCountZ_;
	ResetMapChipData();
}

bool MapChipField::LoadChunkCsv(uint32_t chunkX, uint32_t chunkZ, const std::string& filePath) {
	if (chunkX >= chunkCountX_ || chunkZ >= chunkCountZ_) {
		return false;
	}
	std::ifstream file(filePath);
	if (!file.is_open()) {
		return false;
	}

	// chunk row 0 (cz = 0) is the BOTTOM chunk, so its last CSV line lands on the lowest rows (z = 0 for the base chunk)
	const uint32_t xOffset = chunkX * kChunkBlockSize;
	const uint32_t rowOffset = (chunkCountZ_ - 1 - chunkZ) * kChunkBlockSize;

	std::string line;
	for (uint32_t i = 0; i < kChunkBlockSize && std::getline(file, line); ++i) {
		std::istringstream lineStream(line);
		std::string word;
		for (uint32_t j = 0; j < kChunkBlockSize && std::getline(lineStream, word, ','); ++j) {
			if (word.empty()) {
				continue;
			}
			int value = 0;
			try {
				value = std::stoi(word);
			} catch (...) {
				continue; // not a number: leave the cell as it is
			}
			MapChipType type = MapChipType::kBlank;
			switch (value) {
			case static_cast<int>(MapChipType::kBlock):
				type = MapChipType::kBlock;
				break;
			case static_cast<int>(MapChipType::kPlayer):
				type = MapChipType::kPlayer;
				break;
			case static_cast<int>(MapChipType::kBoss):
				type = MapChipType::kBoss;
				break;
			default:
				break;
			}
			SetMapChipByIndex(xOffset + j, rowOffset + i, type);
		}
	}
	return true;
}

void MapChipField::UnloadChunk(uint32_t chunkX, uint32_t chunkZ) {
	if (chunkX >= chunkCountX_ || chunkZ >= chunkCountZ_) {
		return;
	}
	const uint32_t xStart = chunkX * kChunkBlockSize;
	const uint32_t rowStart = (chunkCountZ_ - 1 - chunkZ) * kChunkBlockSize;
	for (uint32_t r = rowStart; r < rowStart + kChunkBlockSize; ++r) {
		for (uint32_t c = xStart; c < xStart + kChunkBlockSize; ++c) {
			SetMapChipByIndex(c, r, MapChipType::kBlank);
		}
	}
}

const MapChipDataUnit* MapChipField::GetUnit(uint32_t xIndex, uint32_t zIndex) const {
	if (xIndex >= totalBlockHorizontal_ || zIndex >= totalBlockDepth_) {
		return nullptr;
	}
	return &data_[zIndex][xIndex];
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t zIndex) const {
	const MapChipDataUnit* unit = GetUnit(xIndex, zIndex);
	return unit ? unit->type : MapChipType::kBlank;
}

void MapChipField::SetMapChipByIndex(uint32_t xIndex, uint32_t zIndex, MapChipType type) {
	if (xIndex >= totalBlockHorizontal_ || zIndex >= totalBlockDepth_) {
		return;
	}
	data_[zIndex][xIndex].type = type;
}

// index -> world. x starts at 0; the LAST row is z = 0 and rows above it (smaller zIndex) are +Z.
Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t zIndex) const {
	const float worldZ = kBlockSize * (static_cast<float>(totalBlockDepth_) - 1.0f - static_cast<float>(zIndex));
	return Vector3(kBlockSize * static_cast<float>(xIndex), 0.0f, worldZ);
}

// world -> index (exact inverse of the above). Out of range returns the "total" index, which reads as kBlank.
MapChipField::IndexSet MapChipField::GetMapChipIndexSetByPosition(const Vector3& position) const {
	constexpr float kLimit = 1.0e6f; // keeps the float -> int conversion defined

	// floor, not a cast: a cast truncates toward zero, which is wrong for negatives
	const int32_t worldXIndex = static_cast<int32_t>(std::floor(std::clamp((position.x + kBlockSize * 0.5f) / kBlockSize, -kLimit, kLimit)));
	const int32_t worldZIndex = static_cast<int32_t>(std::floor(std::clamp((position.z + kBlockSize * 0.5f) / kBlockSize, -kLimit, kLimit)));
	const int32_t rowIndex = static_cast<int32_t>(totalBlockDepth_) - 1 - worldZIndex;

	IndexSet indexSet = {};
	indexSet.xIndex = (worldXIndex < 0 || worldXIndex >= static_cast<int32_t>(totalBlockHorizontal_)) ? totalBlockHorizontal_ : static_cast<uint32_t>(worldXIndex);
	indexSet.zIndex = (rowIndex < 0 || rowIndex >= static_cast<int32_t>(totalBlockDepth_)) ? totalBlockDepth_ : static_cast<uint32_t>(rowIndex);
	return indexSet;
}

MapChipField::Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t zIndex) const {
	const Vector3 center = GetMapChipPositionByIndex(xIndex, zIndex);
	Rect rect;
	rect.left = center.x - kBlockSize * 0.5f;
	rect.right = center.x + kBlockSize * 0.5f;
	rect.bottom = center.z - kBlockSize * 0.5f;
	rect.top = center.z + kBlockSize * 0.5f;
	return rect;
}

bool MapChipField::IsSolidAtPosition(const Vector3& position) const {
	const IndexSet index = GetMapChipIndexSetByPosition(position);
	return IsSolidType(GetMapChipTypeByIndex(index.xIndex, index.zIndex));
}
