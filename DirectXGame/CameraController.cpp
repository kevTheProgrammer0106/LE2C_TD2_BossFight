#define NOMINMAX
#include "CameraController.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

void CameraController::Initialize(Camera* camera) {
	assert(camera);
	camera_ = camera;
	camera_->Initialize(); // default pose; the menu scenes rely on it
}

float CameraController::ClampAxis(float value, float minEdge, float maxEdge, float halfView) const {
	if (maxEdge - minEdge <= halfView * 2.0f) {
		return (minEdge + maxEdge) * 0.5f;
	}
	return std::clamp(value, minEdge + halfView, maxEdge - halfView);
}

Vector3 CameraController::ComputeTargetPosition() const {
	const Vector3 p = target_->GetWorldTransform().translation_;
	Vector3 pos = {p.x, height_, p.z};

	if (hasBounds_) {
		const float halfViewZ = std::tan(camera_->fovAngleY * 0.5f) * height_;
		const float halfViewX = halfViewZ * camera_->aspectRatio;
		pos.x = ClampAxis(pos.x, minX_, maxX_, halfViewX);
		pos.z = ClampAxis(pos.z, minZ_, maxZ_, halfViewZ);
	}
	return pos;
}

void CameraController::Reset() {
	if (!camera_ || !target_ || !topDown_) {
		return;
	}
	camera_->translation_ = ComputeTargetPosition();
	ApplyTopDownView();
}

// Look-at for "eye straight above, looking down -Y, screen-up = +Z, screen-right = +X"
// (left-handed, row-vector layout - the same layout the engine's own view matrix uses).
void CameraController::ApplyTopDownView() {
	camera_->rotation_ = {std::numbers::pi_v<float> / 2.0f, 0.0f, 0.0f};
	camera_->UpdateMatrix(); // fills the projection (and a view that we overwrite below)

	const Vector3 eye = camera_->translation_;
	const Vector3 xAxis = {1.0f, 0.0f, 0.0f};
	const Vector3 yAxis = {0.0f, 0.0f, 1.0f};
	const Vector3 zAxis = {0.0f, -1.0f, 0.0f};

	Matrix4x4 view{};
	view.m[0][0] = xAxis.x;
	view.m[0][1] = yAxis.x;
	view.m[0][2] = zAxis.x;
	view.m[0][3] = 0.0f;
	view.m[1][0] = xAxis.y;
	view.m[1][1] = yAxis.y;
	view.m[1][2] = zAxis.y;
	view.m[1][3] = 0.0f;
	view.m[2][0] = xAxis.z;
	view.m[2][1] = yAxis.z;
	view.m[2][2] = zAxis.z;
	view.m[2][3] = 0.0f;
	view.m[3][0] = -(xAxis.x * eye.x + xAxis.y * eye.y + xAxis.z * eye.z);
	view.m[3][1] = -(yAxis.x * eye.x + yAxis.y * eye.y + yAxis.z * eye.z);
	view.m[3][2] = -(zAxis.x * eye.x + zAxis.y * eye.y + zAxis.z * eye.z);
	view.m[3][3] = 1.0f;

	camera_->matView = view;
	camera_->TransferMatrix();
}

void CameraController::Update() {
	if (!camera_) {
		return;
	}
	if (topDown_) {
		if (target_) {
			camera_->translation_ = Lerp(camera_->translation_, ComputeTargetPosition(), kInterpolationRate);
		}
		ApplyTopDownView();
		return;
	}
	camera_->UpdateMatrix();
}