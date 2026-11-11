#define NOMINMAX

#include "BackgroundModel.h"
#include "GlobalVariables.h"

using namespace KamataEngine;

void BackgroundModel::Initialize(Camera* camera, Model* modelHill, Model* modelTown, Model* modelTrees, uint32_t texHill, uint32_t texTown, uint32_t texTrees) {

	camera_ = camera;

	// 背景モデルごとにモデルとテクスチャを設定する
	hill_.model = modelHill;
	hill_.textureHandle = texHill;

	town_.model = modelTown;
	town_.textureHandle = texTown;

	trees_.model = modelTrees;
	trees_.textureHandle = texTrees;

	// 各背景のワールド変換を初期化する
	hill_.worldTransform.Initialize();
	town_.worldTransform.Initialize();
	trees_.worldTransform.Initialize();

	// 背景ごとに独立したGlobalVariablesのグループを登録する
	RegisterGlobalVariables();

	// 初期設定を反映する
	Update();
	hill_.worldTransform.translation_.x = -30.0f;

	town_.worldTransform.translation_.x = 500.0f;

	trees_.worldTransform.translation_ = {3000.0f, -300.0f, 1000.0f};
}

void BackgroundModel::RegisterGlobalVariables() {
	RegisterPartGlobalVariables(hill_, "BackgroundHill");
	RegisterPartGlobalVariables(town_, "BackgroundTown");
	RegisterPartGlobalVariables(trees_, "BackgroundTrees");
}

void BackgroundModel::RegisterPartGlobalVariables(BackgroundPart& part, const std::string& groupName) {

	part.groupName = groupName;

	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(part.groupName);

	gv->AddItem(part.groupName, "Scale", part.config.scale);
	gv->AddItem(part.groupName, "Rotation", part.config.rotation);
	gv->AddItem(part.groupName, "Position", part.config.translation);
	gv->AddItem(part.groupName, "Follow", part.config.follow);
}

void BackgroundModel::ApplyGlobalVariables() {
	ApplyPartGlobalVariables(hill_);
	ApplyPartGlobalVariables(town_);
	ApplyPartGlobalVariables(trees_);
}

void BackgroundModel::ApplyPartGlobalVariables(BackgroundPart& part) {
	if (part.groupName.empty()) {
		return;
	}

	GlobalVariables* gv = GlobalVariables::GetInstance();

	// 各モデル専用の設定を取得する
	part.config.scale = gv->GetVector3Value(part.groupName, "Scale");

	part.config.rotation = gv->GetVector3Value(part.groupName, "Rotation");

	part.config.translation = gv->GetVector3Value(part.groupName, "Position");

	part.config.follow = gv->GetVector3Value(part.groupName, "Follow");
}

void BackgroundModel::Update() {
	if (!camera_) {
		return;
	}

	// GlobalVariablesの設定を各モデルに反映する
	ApplyGlobalVariables();
	UpdatePart(hill_);
	UpdatePart(town_);
	UpdatePart(trees_);
}

void BackgroundModel::UpdatePart(BackgroundPart& part) {
	// Followの値に応じてカメラに追従させ、Positionで個別の位置を調整する
	const Vector3& cam = camera_->translation_;
	const Vector3& follow = part.config.follow;
	const Vector3& position = part.config.translation;

	part.worldTransform.translation_ = {cam.x * follow.x + position.x, cam.y * follow.y + position.y, cam.z * follow.z + position.z};

	part.worldTransform.rotation_ = part.config.rotation;
	part.worldTransform.scale_ = part.config.scale;

	WorldTransformUpdate(part.worldTransform);
}

void BackgroundModel::Draw() {
	if (!camera_) {
		return;
	}

	// 登録されている背景モデルだけを描画する
	if (!hill_.model && !town_.model && !trees_.model) {
		return;
	}

	Model::PreDraw();

	DrawPart(hill_);
	DrawPart(town_);
	DrawPart(trees_);

	Model::PostDraw();
}

void BackgroundModel::DrawPart(BackgroundPart& part) {
	if (!part.model) {
		return;
	}

	// 各モデルに専用のワールド変換とテクスチャを適用する
	part.model->Draw(part.worldTransform, *camera_, part.textureHandle);
}