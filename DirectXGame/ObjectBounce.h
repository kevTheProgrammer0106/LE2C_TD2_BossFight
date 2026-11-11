//#pragma once
//
//#define NOMINMAX
//
//#include "MapChipField.h"
//#include <KamataEngine.h>
//#include <algorithm>
//#include <cmath>
//
//namespace ObjectBounce {
//
//// Solid for a tumbling body. One-way platforms never count from the side or below.
//inline bool IsBlocking(MapChipField* field, const KamataEngine::Vector3& p, bool ignoreBarrier) { return field->IsSolidAtPosition(p, ignoreBarrier) && !field->IsOneWayAtPosition(p); }
//
//// Moves pos by vel one axis at a time and bounces off walls, ceilings and floors.
//// restitution: speed kept on a bounce. friction: horizontal speed kept when landing.
//// outLanded is set when the body hit a floor this step (use it to count floor bounces).
//inline void Step(MapChipField* field, KamataEngine::Vector3& pos, KamataEngine::Vector3& vel, float width, float height, float restitution, float friction, bool ignoreBarrier, bool& outLanded) {
//	using namespace KamataEngine;
//	outLanded = false;
//	if (!field) {
//		pos += vel;
//		return;
//	}
//
//	const float halfW = width * 0.5f;
//	const float halfH = height * 0.5f;
//
//	// ---- X axis ----
//	if (vel.x != 0.0f) {
//		float newX = pos.x + vel.x;
//		float sideX = newX + (vel.x > 0.0f ? halfW : -halfW);
//		bool hit = false;
//		for (float y : {pos.y - halfH + 0.05f, pos.y, pos.y + halfH - 0.05f}) {
//			if (IsBlocking(field, {sideX, y, 0.0f}, ignoreBarrier)) {
//				hit = true;
//				break;
//			}
//		}
//		if (hit) {
//			vel.x = -vel.x * restitution; // bounce off the wall
//		} else {
//			pos.x = newX;
//		}
//	}
//
//	// ---- Y axis ----
//	float newY = pos.y + vel.y;
//	const float sampleXs[3] = {pos.x - halfW + 0.01f, pos.x, pos.x + halfW - 0.01f};
//
//	if (vel.y > 0.0f) {
//		// ceiling
//		float topY = newY + halfH;
//		bool hit = false;
//		for (float x : sampleXs) {
//			if (IsBlocking(field, {x, topY, 0.0f}, ignoreBarrier)) {
//				hit = true;
//				break;
//			}
//		}
//		if (hit) {
//			vel.y = -vel.y * restitution;
//		} else {
//			pos.y = newY;
//		}
//	} else if (vel.y < 0.0f) {
//		// floor (platforms count only if the feet started above their top)
//		float prevFootY = pos.y - halfH;
//		float footY = newY - halfH;
//		bool landed = false;
//		float landTop = 0.0f;
//		for (float x : sampleXs) {
//			Vector3 sample = {x, footY, 0.0f};
//			if (!field->IsSolidAtPosition(sample, ignoreBarrier)) {
//				continue;
//			}
//			MapChipField::IndexSet index = field->GetMapChipIndexSetByPosition(sample);
//			MapChipField::Rect rect = field->GetRectByIndex(index.xIndex, index.yIndex);
//			if (field->IsOneWayAtPosition(sample) && prevFootY < rect.top - 0.001f) {
//				continue;
//			}
//			if (!landed || rect.top > landTop) {
//				landTop = rect.top;
//			}
//			landed = true;
//		}
//		if (landed) {
//			pos.y = landTop + halfH + 0.001f; // rest on the surface instead of stopping above it
//			vel.y = -vel.y * restitution;
//			vel.x *= friction;
//			outLanded = true;
//		} else {
//			pos.y = newY;
//		}
//	}
//}
//// 転がり回転の増分。右へ進むと負(時計回り)、左へ進むと正。速度が0に近い時は fallbackSign
//inline float TumbleSpin(float velocityX, float fallbackSign, float perSpeed, float minSpin, float maxSpin) {
//	const float sign = (std::abs(velocityX) > 0.01f) ? ((velocityX > 0.0f) ? -1.0f : 1.0f) : fallbackSign;
//	return sign * std::clamp(std::abs(velocityX) * perSpeed + minSpin, 0.0f, maxSpin);
//}
//} // namespace DeathBounce