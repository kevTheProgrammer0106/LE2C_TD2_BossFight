#pragma once

#include "BaseBehavior.h"
#include "BlockDetector.h"
#include "Global.h"

class MapChipField;

class Boss {

public:                               // ↓モデルのパーツを格納する構造体
	void Initialize(const std::array<PartResources, static_cast<size_t>(PartSlot::kCount)>& partResources, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

private:

	KamataEngine::Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	BlockDetector blockDetector_; // ブロックを検知するためのクラス

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Vector3 velocity_ = {};

	std::array<BodyPart, static_cast<size_t>(PartSlot::kCount)> parts_; // モデルのパーツを格納する配列
	enum { body = 0};

	static inline float kWidth = 0.8f; // X
	static inline float kDepth = 0.8f; // Z
	static inline float kHeight = 1.6f;


};
