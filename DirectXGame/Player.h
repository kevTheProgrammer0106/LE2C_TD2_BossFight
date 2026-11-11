#pragma once

#include "BaseBehavior.h"
#include "BlockDetector.h"
#include "Global.h"
#include <array>
#include <memory>

class MapChipField;

// Top-down player: moves on the X-Z plane with WASD, collides with blocks, turns to face its movement.
// Combat, weapons, HP, knockback and the old behaviors are NOT ported (see PlayerBehavior: empty base).
class Player {
public:
	Player();
	~Player();

	void Initialize(const std::array<PartResources, static_cast<size_t>(PartSlot::kCount)>& partResources, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	KamataEngine::Vector3 GetWorldPosition() const { return worldTransform_.translation_; }
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	void ResetVelocity() { velocity_ = {}; }
	AABB GetAABB() const;

private:
	void Move();
	void MapChipCollisionUpdate();
	void UpdateBodyParts();

	KamataEngine::Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	BlockDetector blockDetector_;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Vector3 velocity_ = {};

	std::array<BodyPart, static_cast<size_t>(PartSlot::kCount)> parts_;
	enum { body = 0, head = 1, hat = 2 };

	std::unique_ptr<BaseBehavior> behavior_; // empty for now

	static inline float kAcceleration = 0.05f;
	static inline float kAttenuation = 0.1f;
	static inline float kLimitRunSpeed = 0.25f;
	static inline float kTurnRate = 0.25f;
	static inline float kWidth = 0.8f; // X
	static inline float kDepth = 0.8f; // Z
	static inline float kHeight = 1.6f;

	// same part layout as the side-view player (the model is centred on worldTransform_)
	static inline SRTOffset headOffset_ = {
	    {1.0f,   1.0f,  1.0f},
	    {0.0f,   0.0f,  0.0f},
	    {-0.05f, 0.72f, 0.0f}
    };
	static inline SRTOffset hatOffset_ = {
	    {1.0f, 1.0f, 1.0f },
	    {0.0f, 0.0f, 0.0f },
	    {0.0f, 0.1f, -0.11f}
    };
};
