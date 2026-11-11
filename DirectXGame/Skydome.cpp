#include "Skydome.h"
#include "GlobalVariables.h"

using namespace KamataEngine;

void Skydome::RegisterGlobalVariables(const std::string& groupName) {
	groupName_ = groupName;

	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(groupName_);
	gv->AddItem(groupName_, "Scale", scale_);
	gv->AddItem(groupName_, "Rotation", rotation_);
	gv->AddItem(groupName_, "Position", translation_);
}

void Skydome::ApplyGlobalVariables() {
	if (groupName_.empty()) {
		return;
	}
	GlobalVariables* gv = GlobalVariables::GetInstance();
	scale_ = gv->GetVector3Value(groupName_, "Scale");
	rotation_ = gv->GetVector3Value(groupName_, "Rotation");
	translation_ = gv->GetVector3Value(groupName_, "Position");
}

void Skydome::Initialize(Vector3 scale, Model* model, uint32_t textureHandle, Camera* camera, const Vector3& position) {

	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	// GlobalVariablesで使用する初期値もここで設定する
	scale_ = {scale.x, scale.y, scale.z};
	rotation_ = {0.0f, 0.0f, 0.0f};
	translation_ = position;

	worldTransform_.Initialize();
	worldTransform_.scale_ = scale_;
	worldTransform_.rotation_ = rotation_;
	worldTransform_.translation_ = translation_;
}
void Skydome::Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	Vector3 defaultScale = {1.0f, 1.0f, 1.0f};
	Initialize(defaultScale, model, textureHandle, camera, position);
}

Skydome::~Skydome() {}

void Skydome::Update() {
	ApplyGlobalVariables();

	worldTransform_.scale_ = scale_;
	worldTransform_.rotation_ = rotation_;
	worldTransform_.translation_ = translation_;
	WorldTransformUpdate(worldTransform_);
}

void Skydome::Draw() {
	model_->PreDraw();
	model_->Draw(worldTransform_, *camera_, textureHandle_);
	model_->PostDraw();
}