#pragma once
#include "KamataEngine.h"
#include <cstdint>
#include <string>
#include <vector>

// Top-down map. The grid lies on the world X-Z plane:
//   xIndex = column -> world X (0 .. +)      zIndex = ROW -> world Z (row 0 = first CSV row = highest Z)
// Cell (x, z) is centred on world (x, 0, z); the base chunk's bottom row sits at world z = 0.
//
// CSV cells are plain numbers (see MapChipType). Anything unknown reads as blank.
enum class MapChipType : uint8_t {
	kBlank = 0,  // 0
	kBlock = 1,  // 1
	kPlayer = 2, // 2  player spawn (the cell itself is empty floor)
	kBoss = 3,
};

struct MapChipDataUnit {
	MapChipType type = MapChipType::kBlank;
};

class MapChipField {
public:
	static inline const uint32_t kChunkBlockSize = 100; // cells per chunk side
	static inline const uint32_t kMaxChunksX = 5;
	static inline const uint32_t kMaxChunksZ = 5;

	// Chunk (cx, cz): cx grows toward +X, cz grows toward +Z. Chunk (0, 0) is the base chunk (bottom-left, bottom row at z = 0).
	void Initialize(uint32_t chunkCountX, uint32_t chunkCountZ);
	void ResetMapChipData();

	// Returns false when the file does not exist (the chunk stays blank) - there is no assert on a missing chunk.
	bool LoadChunkCsv(uint32_t chunkX, uint32_t chunkZ, const std::string& filePath);
	void UnloadChunk(uint32_t chunkX, uint32_t chunkZ);

	uint32_t GetChunkCountX() const { return chunkCountX_; }
	uint32_t GetChunkCountZ() const { return chunkCountZ_; }
	uint32_t GetNumBlockHorizontal() const { return totalBlockHorizontal_; } // X cells
	uint32_t GetNumBlockDepth() const { return totalBlockDepth_; }           // Z cells

	// Index = (xIndex column, zIndex row). Out of range reads as kBlank.
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t zIndex) const;
	void SetMapChipByIndex(uint32_t xIndex, uint32_t zIndex, MapChipType type);
	void ClearMapChipByIndex(uint32_t xIndex, uint32_t zIndex) { SetMapChipByIndex(xIndex, zIndex, MapChipType::kBlank); }

	KamataEngine::Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t zIndex) const;

	struct IndexSet {
		uint32_t xIndex;
		uint32_t zIndex;
	};
	// Out of range returns the "total" index on that axis, which reads as kBlank.
	IndexSet GetMapChipIndexSetByPosition(const KamataEngine::Vector3& position) const;

	struct Rect {
		float left;
		float right;
		float bottom; // min Z
		float top;    // max Z
	};
	Rect GetRectByIndex(uint32_t xIndex, uint32_t zIndex) const;

	static bool IsSolidType(MapChipType type) { return type == MapChipType::kBlock; }
	bool IsSolidAtPosition(const KamataEngine::Vector3& position) const;

	// World-space EDGES (not cell centres).
	float GetWorldMinX() const { return -kBlockSize * 0.5f; }
	float GetWorldMaxX() const { return GetWorldMinX() + static_cast<float>(totalBlockHorizontal_) * kBlockSize; }
	float GetWorldMinZ() const { return -kBlockSize * 0.5f; }
	float GetWorldMaxZ() const { return GetWorldMinZ() + static_cast<float>(totalBlockDepth_) * kBlockSize; }

private:
	static inline const float kBlockSize = 1.0f;

	uint32_t chunkCountX_ = 1;
	uint32_t chunkCountZ_ = 1;
	uint32_t totalBlockHorizontal_ = kChunkBlockSize;
	uint32_t totalBlockDepth_ = kChunkBlockSize;

	// data_[zIndex][xIndex]
	std::vector<std::vector<MapChipDataUnit>> data_;

	const MapChipDataUnit* GetUnit(uint32_t xIndex, uint32_t zIndex) const;
};
