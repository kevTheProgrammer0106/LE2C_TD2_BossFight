#pragma once

#include "Global.h"
//#include "VisionSensor.h"
#include <KamataEngine.h>

/// <summary>
/// Debug-only wireframe helpers built on PrimitiveDrawer::DrawLine3d.
/// PrimitiveDrawer uses whichever camera GameScene last set via SetCamera()
/// (debug camera or the normal camera), so the boxes follow both.
/// </summary>
namespace DebugDraw {

inline void Box(const AABB& box, const KamataEngine::Vector4& color) {
	using namespace KamataEngine;
	PrimitiveDrawer* drawer = PrimitiveDrawer::GetInstance();

	// corner index bits: x = bit0, y = bit1, z = bit2
	Vector3 corners[8];
	for (int i = 0; i < 8; ++i) {
		corners[i] = {(i & 1) ? box.max.x : box.min.x, (i & 2) ? box.max.y : box.min.y, (i & 4) ? box.max.z : box.min.z};
	}

	// 12 edges: connect every pair of corners that differ by exactly one bit
	for (int i = 0; i < 8; ++i) {
		for (int bit = 1; bit <= 4; bit <<= 1) {
			int j = i | bit;
			if (j != i) {
				drawer->DrawLine3d(corners[i], corners[j], color);
			}
		}
	}
}

namespace Color {
inline const KamataEngine::Vector4 kPlayer = {0.2f, 1.0f, 0.2f, 1.0f};        // green
inline const KamataEngine::Vector4 kBlock = {0.3f, 0.5f, 1.0f, 1.0f};         // blue (player block box)
inline const KamataEngine::Vector4 kEnemy = {1.0f, 0.9f, 0.1f, 1.0f};         // yellow
inline const KamataEngine::Vector4 kAttack = {1.0f, 0.15f, 0.15f, 1.0f};      // red (active hitboxes)
inline const KamataEngine::Vector4 kProjectile = {0.2f, 0.9f, 1.0f, 1.0f};    // cyan
inline const KamataEngine::Vector4 kObject = {0.8f, 0.4f, 1.0f, 1.0f};        // purple (items, gates, switches, health blocks)
inline const KamataEngine::Vector4 kVision = {1.0f, 1.0f, 1.0f, 1.0f};        // white (vision cone, player not seen)
inline const KamataEngine::Vector4 kVisionSpotted = {1.0f, 0.3f, 0.3f, 1.0f}; // red-ish (vision cone, player spotted)
inline const KamataEngine::Vector4 kCameraZone = {1.0f, 0.6f, 0.1f, 1.0f};    // orange (camera trigger tile, idle)
inline const KamataEngine::Vector4 kCameraZoneActive = {0.2f, 1.0f, 0.6f, 1.0f}; // mint (camera trigger tile, zoom active)
inline const KamataEngine::Vector4 kCameraFocus = {1.0f, 0.2f, 1.0f, 1.0f};      // magenta (zoom point / camera position)
inline const KamataEngine::Vector4 kBlastFatal = {1.0f, 0.1f, 0.1f, 1.0f};
inline const KamataEngine::Vector4 kBlastNormal = {1.0f, 0.6f, 0.1f, 1.0f};
} // namespace Color

//// Draws an enemy's vision triangle (eye -> left, eye -> right, left -> right).
//// Skipped until the sensor has produced a cone at least once, so no stray line from the origin.
//inline void VisionCone(const VisionSensor& sensor, bool spotted) {
//	if (!sensor.HasValidCone())
//		return;
//	using namespace KamataEngine;
//	PrimitiveDrawer* drawer = PrimitiveDrawer::GetInstance();
//	const VisionSensor::VisionTriangle& tri = sensor.GetVisionTriangle();
//	const Vector4& color = spotted ? Color::kVisionSpotted : Color::kVision;
//	drawer->DrawLine3d(tri.center, tri.left, color);
//	drawer->DrawLine3d(tri.center, tri.right, color);
//	drawer->DrawLine3d(tri.left, tri.right, color);
//}

inline void BoxAt(const KamataEngine::Vector3& center, float halfX, float halfY, float halfZ, const KamataEngine::Vector4& color) {
	AABB box;
	box.min = {center.x - halfX, center.y - halfY, center.z - halfZ};
	box.max = {center.x + halfX, center.y + halfY, center.z + halfZ};
	Box(box, color);
}

inline void Circle(const KamataEngine::Vector3& center, float radius, const KamataEngine::Vector4& color, int segments = 48) {
	using namespace KamataEngine;
	PrimitiveDrawer* drawer = PrimitiveDrawer::GetInstance();
	const float step = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segments);
	Vector3 prev = {center.x + radius, center.y, center.z};
	for (int i = 1; i <= segments; ++i) {
		float a = step * static_cast<float>(i);
		Vector3 next = {center.x + std::cos(a) * radius, center.y + std::sin(a) * radius, center.z};
		drawer->DrawLine3d(prev, next, color);
		prev = next;
	}
}
inline void Line(const KamataEngine::Vector3& a, const KamataEngine::Vector3& b, const KamataEngine::Vector4& color) { KamataEngine::PrimitiveDrawer::GetInstance()->DrawLine3d(a, b, color); }
// Line a -> b with an arrowhead at b, so direction is readable.
inline void Arrow(const KamataEngine::Vector3& a, const KamataEngine::Vector3& b, const KamataEngine::Vector4& color) {
	using namespace KamataEngine;
	Line(a, b, color);

	Vector3 d = {b.x - a.x, b.y - a.y, b.z - a.z};
	float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
	if (len < 0.0001f)
		return;
	d = {d.x / len, d.y / len, d.z / len};

	// side1 = d x up (fallback to right if d is vertical), side2 = d x side1
	Vector3 s1 = {d.z, 0.0f, -d.x};
	float s1Len = std::sqrt(s1.x * s1.x + s1.z * s1.z);
	if (s1Len < 0.0001f)
		s1 = {1.0f, 0.0f, 0.0f};
	else
		s1 = {s1.x / s1Len, 0.0f, s1.z / s1Len};
	Vector3 s2 = {d.y * s1.z - d.z * s1.y, d.z * s1.x - d.x * s1.z, d.x * s1.y - d.y * s1.x};

	float head = std::min(len * 0.2f, 2.0f);
	float wing = head * 0.4f;
	Vector3 base = {b.x - d.x * head, b.y - d.y * head, b.z - d.z * head};
	const float signs[4][2] = {
	    {1,  0 },
        {-1, 0 },
        {0,  1 },
        {0,  -1}
    };
	for (const auto& sg : signs) {
		Vector3 tip = {base.x + (s1.x * sg[0] + s2.x * sg[1]) * wing, base.y + (s1.y * sg[0] + s2.y * sg[1]) * wing, base.z + (s1.z * sg[0] + s2.z * sg[1]) * wing};
		Line(b, tip, color);
	}
}
} // namespace DebugDraw