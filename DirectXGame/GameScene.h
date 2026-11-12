#pragma once
#include "KamataEngine.h"
#include <array>
#include <memory>
#include <vector>

#include "CameraController.h"
#include "DebugMenu.h"
#include "Fade.h"
#include "GameAssets.h"
#include "GameUI.h"
#include "Global.h"
#include "InputManager.h"
#include "MapChipField.h"
#include "Boss.h"
#include "Player.h"

class StageManager;

/// Top-down gameplay scene. Only the player (with the heavy sabre) spawns and moves; there are no enemies, items or HUD yet.
/// Debug builds: F1 reload scene, F3 debug window, F4 collision boxes (see DebugMenu).
class GameScene final {
public:
	~GameScene();

	void Initialize(StageManager* stageDataManager);
	void Update();
	void Draw();

	// --- flags read by main.cpp's scene switch (unchanged) ---
	bool isFinished() const { return finished_; }
	bool isReturnToMenuRequested() const { return returnToMenuRequested_; }
	bool isLevelCleared() const { return levelCleared_; }
	bool isCreditsRequested() const { return creditsRequested_; }
	bool isRestartRequested() const { return restartRequested_; }
	bool isReloadRequested() const { return reloadRequested_; }

	Player* GetPlayer() const { return player_.get(); }

private:
	friend class DebugMenu;

	// Loads every chunk of the stage into mapChipField_. Chunks on the X axis (cx,0) and Z axis (0,cz) come first;
	// the x+z chunk (cx,cz) is generated when BOTH its x chunk and its z chunk are occupied.
	void LoadStageChunks();
	// Builds the block objects of one chunk and spawns whatever its number codes ask for (player).
	void GenerateChunk(uint32_t chunkX, uint32_t chunkZ);
	void SpawnPlayer(const KamataEngine::Vector3& position);

	// Debug wireframes (player / footprint / facing / attack hitbox / weapon / nearby blocks). Toggled with showCollisionDebug_.
	void DrawCollisionDebug();

	void SpawnBoss(const KamataEngine::Vector3& position);
	StageManager* stageManager_ = nullptr;

	KamataEngine::Camera camera_;
	std::unique_ptr<CameraController> cameraController_;

	GameAssets assets_;
	std::unique_ptr<MapChipField> mapChipField_;
	std::vector<std::unique_ptr<KamataEngine::WorldTransform>> blocks_;
	std::vector<std::vector<bool>> chunkOccupied_; // [chunkX][chunkZ]

	std::unique_ptr<Player> player_;
	std::unique_ptr<Boss> boss_;
	std::unique_ptr<Fade> fade_;
	std::unique_ptr<GameUI> gameUI_;

	DebugMenu debugMenu_;
	bool showCollisionDebug_ = false;

	bool finished_ = false;
	bool returnToMenuRequested_ = false;
	bool levelCleared_ = false;
	bool creditsRequested_ = false;
	bool restartRequested_ = false;
	bool reloadRequested_ = false;
};