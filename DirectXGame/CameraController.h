#pragma once

#define NOMINMAX
#include "Global.h"
#include "KamataEngine.h"

class Player;

// Initialize() leaves the camera in its default pose (what the title / level-select scenes need).
// The game scene then calls EnableTopDown(): the camera sits straight above the target looking down (-Y),
// screen-up = +Z, screen-right = +X, and Update() follows the target.
class CameraController {
public:
	void Initialize(KamataEngine::Camera* camera);
	void EnableTopDown() { topDown_ = true; }
	void Update();
	void Reset(); // snap to the target without smoothing

	void SetTarget(Player* target) { target_ = target; }
	void SetHeight(float height) { height_ = height; }
	float GetHeight() const { return height_; }

	// world-space EDGES; the view never shows outside them (the view is centred if the world is smaller than the screen)
	void SetWorldBounds(float minX, float maxX, float minZ, float maxZ) {
		minX_ = minX;
		maxX_ = maxX;
		minZ_ = minZ;
		maxZ_ = maxZ;
		hasBounds_ = true;
	}

private:
	KamataEngine::Vector3 ComputeTargetPosition() const;
	void ApplyTopDownView(); // builds the straight-down view matrix directly (no Euler-angle convention involved)
	float ClampAxis(float value, float minEdge, float maxEdge, float halfView) const;

	KamataEngine::Camera* camera_ = nullptr;
	Player* target_ = nullptr;
	bool topDown_ = false;

	float height_ = 30.0f; // camera Y above the ground
	static inline float kInterpolationRate = 0.1f;

	bool hasBounds_ = false;
	float minX_ = 0.0f, maxX_ = 0.0f, minZ_ = 0.0f, maxZ_ = 0.0f;
};