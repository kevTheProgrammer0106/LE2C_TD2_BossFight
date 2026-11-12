
#pragma once

#define NOMINMAX
#include "Global.h"
#include "KamataEngine.h"

class Player;

// Initialize() leaves the camera in its default pose (what the title / level-select scenes need).
// The game scene then calls EnableThirdPerson(): the camera follows the target from behind.
class CameraController {
public:
	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();

	void Initialize(KamataEngine::Camera* camera);

	void EnableThirdPerson() {
		thirdPersonEnabled_ = true;
		needsSnap_ = true;
	}

	void Update();
	void Reset(); // snap to the target without smoothing

	void SetTarget(Player* target) {
		target_ = target;
		needsSnap_ = true;
		orbitInitialized_ = false;
	}

	void SetTargetOffset(KamataEngine::Vector3 offset) {
		targetOffset_ = offset;
		needsSnap_ = true;
	}

	void SetRotationOffset(KamataEngine::Vector3 rotation) {
		rotationOffset_ = rotation;
		needsSnap_ = true;
	}

	void SetHeight(float height) {
		targetOffset_.y = height;
		needsSnap_ = true;
	}

	float GetHeight() const { return targetOffset_.y; }

public:
	void UpdateLook(); // カメラ入力をプレイヤー移動より先に更新する

private:
	void InitializeOrbit();

	bool orbitInitialized_ = false;

	float orbitYaw_ = 0.0f;
	float lookPitchOffset_ = 0.0f;
	float appliedRotationOffsetY_ = 0.0f;

	static inline float kMouseSensitivity_ = 0.0025f;

	KamataEngine::Vector3 ComputeTargetPosition() const;
	void ApplyThirdPersonView();

	KamataEngine::Camera* camera_ = nullptr;
	Player* target_ = nullptr;

	bool thirdPersonEnabled_ = false;
	bool needsSnap_ = true;

	// プレイヤー基準のカメラ位置と回転のオフセット
	static inline KamataEngine::Vector3 targetOffset_ = {0.0f, 3.0f, -8.0f};
	static inline KamataEngine::Vector3 rotationOffset_ = {0.3f, 0.0f, 0.0f};

	static inline float kInterpolationRate = 0.1f;
	float currentInterpolation_ = 0.1f;
	static inline float kBoostInterpolationRate = 0.4f; // follow rate while dashing / rolling
};
