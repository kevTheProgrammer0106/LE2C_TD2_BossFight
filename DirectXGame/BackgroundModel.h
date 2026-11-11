#pragma once

#define NOMINMAX

#include "Global.h"
#include <KamataEngine.h>
#include <cstdint>
#include <string>

/// <summary>
/// 3種類の背景モデルを管理するクラス。
/// Hill、Town、Treesそれぞれの位置・回転・スケールを
/// GlobalVariablesで個別に調整できる。
/// </summary>
class BackgroundModel {
public:
	struct Config {
		KamataEngine::Vector3 translation{0.0f, 0.0f, 0.0f};
		KamataEngine::Vector3 rotation{0.0f, 0.0f, 0.0f};
		KamataEngine::Vector3 scale{1.0f, 1.0f, 1.0f};
		KamataEngine::Vector3 follow{0.0f, 0.0f, 0.0f};
	};

	// 背景モデルごとに使用するデータを管理する
	struct BackgroundPart {
		KamataEngine::Model* model = nullptr;
		uint32_t textureHandle = 0;
		KamataEngine::WorldTransform worldTransform{};
		std::string groupName;
		Config config{};
	};

	// 3つの背景モデルとテクスチャを設定する
	void Initialize(
	    KamataEngine::Camera* camera, KamataEngine::Model* modelHill, KamataEngine::Model* modelTown, KamataEngine::Model* modelTrees, uint32_t texHill, uint32_t texTown, uint32_t texTrees);

	// 背景ごとのGlobalVariablesを登録する
	void RegisterGlobalVariables();

	// GlobalVariablesの値を各背景に反映する
	void ApplyGlobalVariables();

	void Update();
	void Draw();

private:
	// 個別の背景設定を登録・適用する
	void RegisterPartGlobalVariables(BackgroundPart& part, const std::string& groupName);

	void ApplyPartGlobalVariables(BackgroundPart& part);

	// 各背景のワールド変換を更新する
	void UpdatePart(BackgroundPart& part);

	// 指定された背景モデルを描画する
	void DrawPart(BackgroundPart& part);

	KamataEngine::Camera* camera_ = nullptr;

	BackgroundPart hill_;
	BackgroundPart town_;
	BackgroundPart trees_;
};