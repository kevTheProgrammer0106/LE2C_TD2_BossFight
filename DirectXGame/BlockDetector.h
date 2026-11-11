#pragma once

#include "Global.h"
#include "MapChipField.h"
#include <KamataEngine.h>

// Top-down block collision on the X-Z plane (ported from the side-view BlockDetector; Y/gravity/platform logic removed).
// The entity is a rectangle: kWidth along X, kDepth along Z, centred on worldTransform_.translation_.
class BlockDetector {
public:
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	// Resolve collisionInfo.move (x and z used) against the blocks. Sets isHitWall when it was clipped.
	void MapChipCollision(CollisionInfo& collisionInfo);

	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3& center, Corner corner) const;

	void SetWorldTransform(const KamataEngine::WorldTransform& worldTransform) { worldTransform_.translation_ = worldTransform.translation_; }
	void SetSize(float width, float depth) {
		kWidth = width;
		kDepth = depth;
	}

private:
	void LeftCollision(CollisionInfo& collisionInfo);
	void RightCollision(CollisionInfo& collisionInfo);
	void UpCollision(CollisionInfo& collisionInfo);   // +Z
	void DownCollision(CollisionInfo& collisionInfo); // -Z

	bool IsSolid(const KamataEngine::Vector3& position) const { return mapChipField_->IsSolidAtPosition(position); }

	KamataEngine::WorldTransform worldTransform_;
	float kWidth = 0.8f;
	float kDepth = 0.8f;
	float kBlank = 0.01f;

	MapChipField* mapChipField_ = nullptr;
};
