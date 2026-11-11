#define NOMINMAX
#include "Player.h"
#include "MapChipField.h"
#include "PlayerBehavior.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

Player::Player() = default;
Player::~Player() = default;

void Player::Initialize(const std::array<PartResources, static_cast<size_t>(PartSlot::kCount)>& partResources, Camera* camera, const Vector3& position) {
#ifdef _DEBUG
	assert(camera);
#endif
	for (size_t i = 0; i < parts_.size(); ++i) {
		parts_[i].model = partResources[i].model;
		parts_[i].textureHandle = partResources[i].textureHandle;
		parts_[i].transform.Initialize();
	}
	camera_ = camera;
	velocity_ = {};

	worldTransform_.Initialize();
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_.y = 0.0f; // facing +Z (up the screen)
	worldTransform_.translation_ = position;

	behavior_ = std::make_unique<PlayerBehavior>(this);
	behavior_->Enter();

	UpdateBodyParts();
}

void Player::Update() {
	if (behavior_) {
		behavior_->Update();
	}
	Move();
	MapChipCollisionUpdate();
	FaceMovement();
	UpdateBodyParts();
}

// WASD on the X-Z plane: W = +Z (screen up), S = -Z, D = +X, A = -X
void Player::Move() {
	Input* input = Input::GetInstance();

	Vector3 dir = {};
	if (input->PushKey(DIK_D))
		dir.x += 1.0f;
	if (input->PushKey(DIK_A))
		dir.x -= 1.0f;
	if (input->PushKey(DIK_W))
		dir.z += 1.0f;
	if (input->PushKey(DIK_S))
		dir.z -= 1.0f;

	const float len = std::sqrt(dir.x * dir.x + dir.z * dir.z);
	if (len > 0.0f) {
		dir.x /= len; // diagonals are not faster
		dir.z /= len;
		velocity_.x += dir.x * kAcceleration;
		velocity_.z += dir.z * kAcceleration;

		const float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
		if (speed > kLimitRunSpeed) {
			velocity_.x *= kLimitRunSpeed / speed;
			velocity_.z *= kLimitRunSpeed / speed;
		}
	} else {
		velocity_.x *= (1.0f - kAttenuation);
		velocity_.z *= (1.0f - kAttenuation);
		if (std::abs(velocity_.x) < 0.0005f)
			velocity_.x = 0.0f;
		if (std::abs(velocity_.z) < 0.0005f)
			velocity_.z = 0.0f;
	}
}

void Player::MapChipCollisionUpdate() {
	if (!mapChipField_) {
		worldTransform_.translation_ += Vector3(velocity_.x, 0.0f, velocity_.z);
		return;
	}
	blockDetector_.SetMapChipField(mapChipField_);
	blockDetector_.SetSize(kWidth, kDepth);
	blockDetector_.SetWorldTransform(worldTransform_);

	CollisionInfo info{};
	info.move = {velocity_.x, 0.0f, velocity_.z};
	blockDetector_.MapChipCollision(info);

	worldTransform_.translation_ += info.move;

	// the world edge is not a block: keep the player inside it even where the CSV has no border
	worldTransform_.translation_.x = std::clamp(worldTransform_.translation_.x, mapChipField_->GetWorldMinX() + kWidth * 0.5f, mapChipField_->GetWorldMaxX() - kWidth * 0.5f);
	worldTransform_.translation_.z = std::clamp(worldTransform_.translation_.z, mapChipField_->GetWorldMinZ() + kDepth * 0.5f, mapChipField_->GetWorldMaxZ() - kDepth * 0.5f);

	// a wall stops the velocity on that axis only, so the player slides along it
	if (std::abs(info.move.x) < std::abs(velocity_.x))
		velocity_.x = 0.0f;
	if (std::abs(info.move.z) < std::abs(velocity_.z))
		velocity_.z = 0.0f;
}

void Player::FaceMovement() {
	const float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
	if (speed < 0.01f) {
		return;
	}
	// model rotation_.y = 0 faces +Z, pi/2 faces +X
	const float target = std::atan2(velocity_.x, velocity_.z);
	float diff = target - worldTransform_.rotation_.y;
	constexpr float kPi = std::numbers::pi_v<float>;
	while (diff > kPi)
		diff -= 2.0f * kPi;
	while (diff < -kPi)
		diff += 2.0f * kPi;
	worldTransform_.rotation_.y += diff * kTurnRate;
}

void Player::UpdateBodyParts() {
	WorldTransformUpdate(worldTransform_);

	parts_[body].transform.translation_ = worldTransform_.translation_;
	parts_[body].transform.rotation_ = worldTransform_.rotation_;
	parts_[body].transform.scale_ = worldTransform_.scale_;
	WorldTransformUpdate(parts_[body].transform);

	const Matrix4x4 bodyRot = Multiply(MakeRotateXMatrix(parts_[body].transform.rotation_.x), Multiply(MakeRotateYMatrix(parts_[body].transform.rotation_.y), MakeRotateZMatrix(parts_[body].transform.rotation_.z)));
	parts_[head].transform.translation_ = parts_[body].transform.translation_ + TransformNormal(headOffset_.translation, bodyRot);
	parts_[head].transform.rotation_ = parts_[body].transform.rotation_ + headOffset_.rotation;
	parts_[head].transform.scale_ = {
	    parts_[body].transform.scale_.x * headOffset_.scale.x, parts_[body].transform.scale_.y * headOffset_.scale.y, parts_[body].transform.scale_.z * headOffset_.scale.z};
	WorldTransformUpdate(parts_[head].transform);

	const Matrix4x4 headRot = Multiply(MakeRotateXMatrix(parts_[head].transform.rotation_.x), Multiply(MakeRotateYMatrix(parts_[head].transform.rotation_.y), MakeRotateZMatrix(parts_[head].transform.rotation_.z)));
	parts_[hat].transform.translation_ = parts_[head].transform.translation_ + TransformNormal(hatOffset_.translation, headRot);
	parts_[hat].transform.rotation_ = parts_[head].transform.rotation_ + hatOffset_.rotation;
	parts_[hat].transform.scale_ = {parts_[head].transform.scale_.x * hatOffset_.scale.x, parts_[head].transform.scale_.y * hatOffset_.scale.y, parts_[head].transform.scale_.z * hatOffset_.scale.z};
	WorldTransformUpdate(parts_[hat].transform);
}

void Player::Draw() {
	Model::PreDraw();
	for (auto& part : parts_) {
		if (part.model) {
			part.model->Draw(part.transform, *camera_, part.textureHandle);
		}
	}
	Model::PostDraw();
}

AABB Player::GetAABB() const {
	const Vector3 p = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {p.x - kWidth / 2.0f, p.y - kHeight / 2.0f, p.z - kDepth / 2.0f};
	aabb.max = {p.x + kWidth / 2.0f, p.y + kHeight / 2.0f, p.z + kDepth / 2.0f};
	return aabb;
}
