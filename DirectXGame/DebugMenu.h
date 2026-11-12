#pragma once

#define NOMINMAX

#include "Global.h"
#include <KamataEngine.h>

class GameScene;

/// <summary>
/// Single debug window (Debug builds only), drawn from GameScene::Update.
///   F1 reload the scene   F3 show / hide this window   F4 collision boxes
/// (The mouse is locked to the camera while playing: press Alt to free it so you can use the window.)
///
/// Tabs: Scene / Player / Movement / Heavy Sabre / Camera.
/// The Heavy Sabre and Camera tabs edit GlobalVariables directly (live, applied by the next ApplyGlobalVariables()).
/// "Save this tab" writes the json of every group the tab uses.
/// Needs `friend class DebugMenu;` in GameScene, Player and WeaponHeavySabre.
/// </summary>
class DebugMenu {
public:
	void Draw(GameScene* gameScene);

private:
	void DrawSceneTab(GameScene* gs);
	void DrawPlayerTab(GameScene* gs);
	void DrawMovementTab(GameScene* gs);
	void DrawHeavySabreTab();
	void DrawCameraTab();

	bool isOpen_ = true;

	// debuff test values
	float stunDuration_ = 1.5f;
	float knockbackPower_ = 5.0f;
};