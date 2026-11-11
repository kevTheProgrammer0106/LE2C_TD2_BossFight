#define NOMINMAX
#include "BlockDetector.h"
#include <algorithm>
#include <array>
#include <cmath>

using namespace KamataEngine;

// Corner naming stays from the side view: "Top" = +Z (screen up), "Bottom" = -Z.
Vector3 BlockDetector::CornerPosition(const Vector3& center, Corner corner) const {
	const Vector3 offsetTable[kNumCorner] = {
	    {-kWidth / 2.0f, 0.0f, kDepth / 2.0f },
	    {kWidth / 2.0f,  0.0f, kDepth / 2.0f },
	    {-kWidth / 2.0f, 0.0f, -kDepth / 2.0f},
	    {kWidth / 2.0f,  0.0f, -kDepth / 2.0f},
	};
	const Vector3& o = offsetTable[static_cast<uint32_t>(corner)];
	return {center.x + o.x, center.y + o.y, center.z + o.z};
}

// X first, then Z: the caller applies the X move before Z is resolved so the entity slides along walls.
void BlockDetector::MapChipCollision(CollisionInfo& collisionInfo) {
	if (!mapChipField_) {
		return;
	}
	LeftCollision(collisionInfo);
	RightCollision(collisionInfo);

	// Z is tested from where the entity will be after its (already clipped) X move
	const Vector3 saved = worldTransform_.translation_;
	worldTransform_.translation_.x += collisionInfo.move.x;
	UpCollision(collisionInfo);
	DownCollision(collisionInfo);
	worldTransform_.translation_ = saved;
}

void BlockDetector::LeftCollision(CollisionInfo& collisionInfo) {
	if (collisionInfo.move.x >= 0.0f)
		return;
	const Vector3 next = worldTransform_.translation_ + Vector3(collisionInfo.move.x, 0.0f, 0.0f);
	const Vector3 corners[2] = {CornerPosition(next, kTopLeft), CornerPosition(next, kBottomLeft)};

	float clamped = collisionInfo.move.x;
	bool hit = false;
	for (const Vector3& corner : corners) {
		if (!IsSolid(corner))
			continue;
		const MapChipField::IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(corner);
		const MapChipField::Rect rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.zIndex);
		clamped = std::max(clamped, std::max(0.0f, (rect.right - worldTransform_.translation_.x) + (kWidth / 2.0f) + kBlank));
		hit = true;
	}
	if (hit) {
		collisionInfo.move.x = clamped;
		collisionInfo.isHitWall = true;
	}
}

void BlockDetector::RightCollision(CollisionInfo& collisionInfo) {
	if (collisionInfo.move.x <= 0.0f)
		return;
	const Vector3 next = worldTransform_.translation_ + Vector3(collisionInfo.move.x, 0.0f, 0.0f);
	const Vector3 corners[2] = {CornerPosition(next, kTopRight), CornerPosition(next, kBottomRight)};

	float clamped = collisionInfo.move.x;
	bool hit = false;
	for (const Vector3& corner : corners) {
		if (!IsSolid(corner))
			continue;
		const MapChipField::IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(corner);
		const MapChipField::Rect rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.zIndex);
		clamped = std::min(clamped, std::min(0.0f, (rect.left - worldTransform_.translation_.x) - (kWidth / 2.0f) - kBlank));
		hit = true;
	}
	if (hit) {
		collisionInfo.move.x = clamped;
		collisionInfo.isHitWall = true;
	}
}

void BlockDetector::UpCollision(CollisionInfo& collisionInfo) {
	if (collisionInfo.move.z <= 0.0f)
		return;
	const Vector3 next = worldTransform_.translation_ + Vector3(0.0f, 0.0f, collisionInfo.move.z);
	const Vector3 corners[2] = {CornerPosition(next, kTopLeft), CornerPosition(next, kTopRight)};

	float clamped = collisionInfo.move.z;
	bool hit = false;
	for (const Vector3& corner : corners) {
		if (!IsSolid(corner))
			continue;
		const MapChipField::IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(corner);
		const MapChipField::Rect rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.zIndex);
		clamped = std::min(clamped, std::min(0.0f, (rect.bottom - worldTransform_.translation_.z) - (kDepth / 2.0f) - kBlank));
		hit = true;
	}
	if (hit) {
		collisionInfo.move.z = clamped;
		collisionInfo.isHitWall = true;
	}
}

void BlockDetector::DownCollision(CollisionInfo& collisionInfo) {
	if (collisionInfo.move.z >= 0.0f)
		return;
	const Vector3 next = worldTransform_.translation_ + Vector3(0.0f, 0.0f, collisionInfo.move.z);
	const Vector3 corners[2] = {CornerPosition(next, kBottomLeft), CornerPosition(next, kBottomRight)};

	float clamped = collisionInfo.move.z;
	bool hit = false;
	for (const Vector3& corner : corners) {
		if (!IsSolid(corner))
			continue;
		const MapChipField::IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(corner);
		const MapChipField::Rect rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.zIndex);
		clamped = std::max(clamped, std::max(0.0f, (rect.top - worldTransform_.translation_.z) + (kDepth / 2.0f) + kBlank));
		hit = true;
	}
	if (hit) {
		collisionInfo.move.z = clamped;
		collisionInfo.isHitWall = true;
	}
}
