#pragma once

#define NOMINMAX

#include "Global.h"
#include <KamataEngine.h>

class GameScene;

/// <summary>
/// Single debug window (toggle: F3) combining scene/player/spawn tools with the live
/// enemy weapon / firearm / body offset tweaker. Debug builds only.
/// Needs `friend class DebugMenu;` in GameScene and in every class whose private statics it edits.
/// Hotkeys: F1 collision boxes, F2 freeze/unfreeze enemies, F3 this window, P debug camera.
///
/// 武器タブ(Heavy Sabre / Axe / Spontoon / Bayonet Musket / Shotgun / Pistols)は
/// GlobalVariablesの値を直接編集する(GetVector3Ptr等)。Save this tab でjsonへ保存。
/// </summary>
class DebugMenu {
public:
	void Draw(GameScene* gameScene);

private:
	void DrawSceneTab(GameScene* gs);
	void DrawPlayerTab(GameScene* gs);
	void DrawSpawnTab(GameScene* gs);
	void DrawEnemiesTab(GameScene* gs);
	void DrawSabreTab();
	void DrawHeavySabreTab();
	void DrawAxeTab();
	void DrawSpearTab();
	void DrawBayonetTab();
	void DrawShotgunTab();
	void DrawPistolsTab();
	void DrawBodyTab();
	void DrawCombatTab();
	void PushBodyOffsetsToEnemies(GameScene* gs);

	bool isOpen_ = true;

	// Per-instance AnimationManager offsets, pushed to every enemy each frame.
	bool bodyInitialized_ = false;
	bool pushBodyOffsets_ = true;
	SRTOffset bodyHead_{};
	SRTOffset bodyHat_{};

	float spawnOffsetX_ = 2.0f;
};