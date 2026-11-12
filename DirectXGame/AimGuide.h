#pragma once

#define NOMINMAX

#include "Global.h"
#include <KamataEngine.h>
#include <array>

/// <summary>
/// 銃の狙い方向を示す照準ライン(パーティクルモデルの点列)。プレイヤーの実際の射線を表示する。w
/// 点は目標へ向かって流れるように動き、壁で止まる時は終端マーカーが赤くなる。
/// </summary>
class AimGuide {
public:
	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();
	static float GetMaxLength() { return kMaxLength; }

	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera);
	// origin: 発射位置 / dir: 正規化済みの狙い方向 / length: 線の長さ / blocked: 壁で止まっている / ready: 撃てる状態
	void Update(bool visible, const KamataEngine::Vector3& origin, const KamataEngine::Vector2& dir, float length, bool blocked, bool ready);
	void Draw();

private:
	struct Dot {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::ObjectColor objectColor;
	};
	static constexpr size_t kMaxDots = 64;

	std::array<Dot, kMaxDots> dots_;
	Dot endMarker_;
	size_t dotCount_ = 0;
	bool visible_ = false;
	float time_ = 0.0f;

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	uint32_t textureHandle_ = 0;

	static inline float kMaxLength = 20.0f;
	static inline float kStartOffset = 0.9f; // 体から少し離れたところから始める
	static inline float kDotSpacing = 0.8f;
	static inline float kDotSize = 0.14f;
	static inline float kEndMarkerSize = 0.4f;
	static inline float kScrollSpeed = 3.0f; // 点が流れる速さ(単位/秒)
	static inline float kAlpha = 0.7f;
	static inline float kNotReadyAlphaScale = 0.4f;
	static inline float kDepthOffset = -0.3f; // カメラ側へ少し寄せる
	static inline KamataEngine::Vector3 kColor = {1.0f, 1.0f, 1.0f};
	static inline KamataEngine::Vector3 kBlockedColor = {1.0f, 0.3f, 0.2f};
};