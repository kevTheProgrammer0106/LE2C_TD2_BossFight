#include "Boss.h"

using namespace KamataEngine;

void Boss::Initialize(const std::array<PartResources, static_cast<size_t>(PartSlot::kCount)>& partResources, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {

	// 一旦ボスの体だけ表示
	parts_[0].model = partResources[0].model;
	parts_[0].textureHandle = partResources[0].textureHandle;
	parts_[0].transform.Initialize();

	camera_ = camera;
	velocity_ = {};
	worldTransform_.Initialize();
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_.y = 0.0f; // facing +Z (up the screen)
	worldTransform_.translation_ = position;
}

void Boss::Update() {

// ボスの挙動はまだ実装されていないので、今は真ん中に立つだけ
	
}

void Boss::Draw() {

	Model::PreDraw();
	for (auto& part : parts_) {
		if (part.model) {
			part.model->Draw(part.transform, *camera_, part.textureHandle);
		}
	}
	Model::PostDraw();
}
