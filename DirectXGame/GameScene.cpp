#define NOMINMAX
#include "GameScene.h"
#include "StageManager.h"
#include <Windows.h>
#include <algorithm>
#include <string>

using namespace KamataEngine;

namespace {
void Log(const std::string& message) { OutputDebugStringA(("[GameScene] " + message + "\n").c_str()); }
} // namespace

GameScene::~GameScene() = default;

void GameScene::Initialize(StageManager* stageDataManager) {
	stageManager_ = stageDataManager;

	CameraController::RegisterGlobalVariables();
	CameraController::ApplyGlobalVariables();

	camera_.farZ = 10000.0f;
	camera_.Initialize();

	assets_.Load();

	// --- map: stageData.csv decides the chunk counts, nothing else ---
	// (no stage rows at all -> a single blank chunk, so the scene still opens instead of asserting)
	const bool hasStage = stageManager_->GetStageCount() > 0 && stageManager_->GetCurrentStageIndex() >= 0 && stageManager_->GetCurrentStageIndex() < stageManager_->GetStageCount();
	if (!hasStage) {
		Log("no valid stage selected - opening a blank map");
	}
	mapChipField_ = std::make_unique<MapChipField>();
	if (hasStage) {
		const StageData& stage = stageManager_->GetCurrentStageData();
		mapChipField_->Initialize(static_cast<uint32_t>(stage.chunksX), static_cast<uint32_t>(stage.chunksZ));
		LoadStageChunks();
	} else {
		mapChipField_->Initialize(1, 1);
		chunkOccupied_.assign(1, std::vector<bool>(1, true));
	}

	// --- camera ---
	cameraController_ = std::make_unique<CameraController>();
	cameraController_->Initialize(&camera_);

	// --- build blocks + spawn from the number codes ---
	for (uint32_t cz = 0; cz < mapChipField_->GetChunkCountZ(); ++cz) {
		for (uint32_t cx = 0; cx < mapChipField_->GetChunkCountX(); ++cx) {
			if (chunkOccupied_[cx][cz]) {
				GenerateChunk(cx, cz);
			}
		}
	}

	// no player code in the CSV: fall back to the base chunk's bottom-left corner so the scene is still usable
	if (!player_) {
		Log("no player spawn (2) in the CSV - spawning at the bottom-left corner");
		SpawnPlayer(mapChipField_->GetMapChipPositionByIndex(0, mapChipField_->GetNumBlockDepth() - 1));
	}

	fade_ = std::make_unique<Fade>();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

void GameScene::LoadStageChunks() {
	const int32_t stageIndex = stageManager_->GetCurrentStageIndex();
	const uint32_t countX = mapChipField_->GetChunkCountX();
	const uint32_t countZ = mapChipField_->GetChunkCountZ();

	chunkOccupied_.assign(countX, std::vector<bool>(countZ, false));

	// the base chunk is always part of the map (blank if its file is missing)
	if (!mapChipField_->LoadChunkCsv(0, 0, stageManager_->GetChunkFileName(stageIndex, 0, 0))) {
		Log("base chunk file not found: " + stageManager_->GetChunkFileName(stageIndex, 0, 0));
	}
	chunkOccupied_[0][0] = true;

	// x chunks (cx, 0) and z chunks (0, cz): occupied when their file exists
	for (uint32_t cx = 1; cx < countX; ++cx) {
		chunkOccupied_[cx][0] = mapChipField_->LoadChunkCsv(cx, 0, stageManager_->GetChunkFileName(stageIndex, cx, 0));
	}
	for (uint32_t cz = 1; cz < countZ; ++cz) {
		chunkOccupied_[0][cz] = mapChipField_->LoadChunkCsv(0, cz, stageManager_->GetChunkFileName(stageIndex, 0, cz));
	}

	// x+z chunks: generated when both its x chunk and its z chunk are occupied.
	// Its own file (name_x1_z1.csv) fills it if present; without one it is generated blank.
	for (uint32_t cz = 1; cz < countZ; ++cz) {
		for (uint32_t cx = 1; cx < countX; ++cx) {
			if (chunkOccupied_[cx][0] && chunkOccupied_[0][cz]) {
				mapChipField_->LoadChunkCsv(cx, cz, stageManager_->GetChunkFileName(stageIndex, cx, cz));
				chunkOccupied_[cx][cz] = true;
			}
		}
	}
}

void GameScene::GenerateChunk(uint32_t chunkX, uint32_t chunkZ) {
	const uint32_t size = MapChipField::kChunkBlockSize;
	const uint32_t xStart = chunkX * size;
	// chunk row 0 is the bottom chunk, so it owns the LAST rows of the table
	const uint32_t rowStart = (mapChipField_->GetChunkCountZ() - 1 - chunkZ) * size;

	for (uint32_t row = rowStart; row < rowStart + size; ++row) {
		for (uint32_t col = xStart; col < xStart + size; ++col) {
			switch (mapChipField_->GetMapChipTypeByIndex(col, row)) {
			case MapChipType::kBlock: {
				auto block = std::make_unique<WorldTransform>();
				block->Initialize();
				block->translation_ = mapChipField_->GetMapChipPositionByIndex(col, row);
				WorldTransformUpdate(*block);
				blocks_.push_back(std::move(block));
			} break;
			case MapChipType::kPlayer:
				if (!player_) { // only one player; extra spawn cells are ignored
					SpawnPlayer(mapChipField_->GetMapChipPositionByIndex(col, row));
				}
				break;
			default:
				break;
			}
		}
	}
}

void GameScene::SpawnPlayer(const Vector3& position) {
	player_ = std::make_unique<Player>();
	player_->SetMapChipField(mapChipField_.get());

	const std::array<PartResources, static_cast<size_t>(PartSlot::kCount)> parts = {
	    {{assets_.modelPlayerBody.get(), assets_.playerBodyTex}, {assets_.modelPlayerHead.get(), assets_.playerHeadTex}, {assets_.modelPlayerHat.get(), assets_.playerHatTex}}
    };

	player_->Initialize(parts, &camera_, position);

	cameraController_->SetTarget(player_.get());
	cameraController_->EnableThirdPerson();
	cameraController_->Reset();
}

void GameScene::Update() {
	fade_->Update();
	if (fade_->status_ == Fade::Status::FadeIn && fade_->IsFinished()) {
		fade_->Stop();
	}
	// Escape goes back to the title menu (main.cpp's switch handles the rest)
	if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
		returnToMenuRequested_ = true;
		return;
	}
	CameraController::ApplyGlobalVariables();


	// カメラの向きを先に更新
	cameraController_->UpdateLook();

	if (player_) {
		player_->Update();
	}
	cameraController_->Update();
}

void GameScene::Draw() {
	if (!blocks_.empty()) {
		Model* model = assets_.modelBlock.get();
		model->PreDraw();
		for (auto& block : blocks_) {
			model->Draw(*block, camera_, assets_.blockTex);
		}
		model->PostDraw();
	}

	if (player_) {
		player_->Draw();
	}

	fade_->Draw();
}