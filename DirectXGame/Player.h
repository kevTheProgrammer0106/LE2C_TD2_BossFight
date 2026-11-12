#pragma once

#include "BaseBehavior.h"
#include "BlockDetector.h"
#include "Global.h"
#include <array>
#include <memory>

class MapChipField;

// Top-down player: moves on the X-Z plane with WASD, collides with blocks.
// The back of the player always faces the camera (yaw follows the camera, it never turns toward the move direction).
// Shift = dash (sprint). Shift again while dashing = barrel roll.
// Combat, weapons, HP, knockback and the old behaviors are NOT ported (see PlayerBehavior: empty base).
class Player {
public:
	Player();
	~Player();

	enum class MoveMode { kNormal, kDash, kRoll };

	void Initialize(const std::array<PartResources, static_cast<size_t>(PartSlot::kCount)>& partResources, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	KamataEngine::Vector3 GetWorldPosition() const { return worldTransform_.translation_; }
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	void ResetVelocity() { velocity_ = {}; }
	AABB GetAABB() const;

	MoveMode GetMoveMode() const { return moveMode_; }
	bool IsDashing() const { return moveMode_ == MoveMode::kDash; }
	bool IsRolling() const { return moveMode_ == MoveMode::kRoll; }

private:
	void Move();
	void MapChipCollisionUpdate();
	void UpdateBodyParts();

	void EnterDash(const KamataEngine::Vector3& dir);
	void EnterRoll(const KamataEngine::Vector3& dir, float sign);

	KamataEngine::Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	BlockDetector blockDetector_;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Vector3 velocity_ = {};

	std::array<BodyPart, static_cast<size_t>(PartSlot::kCount)> parts_;
	enum { body = 0, head = 1, hat = 2 };

	std::unique_ptr<BaseBehavior> behavior_; // empty for now

	// --- dash / barrel roll ---
	MoveMode moveMode_ = MoveMode::kNormal;
	float modeTimer_ = 0.0f;                             // seconds spent in the current dash / roll
	KamataEngine::Vector3 dashDir_ = {0.0f, 0.0f, -1.0f};
	KamataEngine::Vector3 rollDir_ = {0.0f, 0.0f, -1.0f}; // roll direction, locked when the roll starts
	float rollSign_ = -1.0f;                             // -1 = roll to the right, +1 = roll to the left
	SRTOffset currentPose_ = {};                         // smoothed body pose (AnimationManager::SmoothSRT)

	static inline float kAcceleration = 0.05f;
	static inline float kAttenuation = 0.1f;
	static inline float kLimitRunSpeed = 0.25f;
	static inline float kWidth = 0.8f; // X
	static inline float kDepth = 0.8f; // Z
	static inline float kHeight = 1.6f;

	// Dash / roll tuning (speed is units per frame @60fps, like kLimitRunSpeed)
	static inline float kDashSpeed = 0.45f;
	static inline float kDashDuration = 0.8f;
	static inline float kDashAccel = 0.25f; // per-frame ease toward the target velocity
	static inline float kRollSpeed = 0.55f;
	static inline float kRollDuration = 0.5f;

	// Body poses (rotation.x = lean forward). Roll adds a full 360 degree spin on top, see UpdateBodyParts.
	static inline SRTOffset kDashPose = {
	    {1.0f, 0.95f,  1.05f},
        {0.4f, 0.0f,   0.0f },
        {0.0f, -0.05f, 0.0f }
    };
	static inline SRTOffset kRollPose = {
	    {0.95f, 0.95f, 1.1f},
        {0.15f, 0.0f,  0.0f},
        {0.0f,  0.0f,  0.0f}
    };

	// same part layout as the side-view player (the model is centred on worldTransform_)
	static inline SRTOffset headOffset_ = {
	    {1.0f,   1.0f,  1.0f},
        {0.0f,   0.0f,  0.0f},
        {-0.05f, 0.72f, 0.0f}
    };
	static inline SRTOffset hatOffset_ = {
	    {1.0f, 1.0f, 1.0f  },
        {0.0f, 0.0f, 0.0f  },
        {0.0f, 0.1f, -0.11f}
    };
};