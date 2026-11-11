#pragma once
#include "Global.h"
#include "KamataEngine.h"

class MapChipField;

/// <summary>
///	天球
/// </summary>
class Skydome {
public:
	void RegisterGlobalVariables(const std::string& groupName);
	void ApplyGlobalVariables();

	~Skydome();
	void Initialize(KamataEngine::Vector3 scale, KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	// マップチップフィールドを設定するセッター
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

private:
	KamataEngine::Camera* camera_ = nullptr;
	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_{};
	uint32_t textureHandle_ = 0;
	KamataEngine::Model* model_ = nullptr;

	std::string groupName_;

	KamataEngine::Vector3 scale_ = {1.0f, 1.0f, 1.0f};
	KamataEngine::Vector3 rotation_ = {0.0f, 0.0f, 0.0f};
	KamataEngine::Vector3 translation_ = {0.0f, 0.0f, 0.0f};

	MapChipField* mapChipField_ = nullptr;
};
