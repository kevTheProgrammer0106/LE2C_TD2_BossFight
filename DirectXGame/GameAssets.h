#pragma once

#include "Global.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include <cstdint>
#include <memory>

/// Models / textures used by the menu scenes and the top-down gameplay scene.
/// (Enemies, other weapons, effects, HUD, audio and the other tile types were not ported.)
class GameAssets {
public:
	void Load();

	// --- Skydome (menu scenes) ---
	std::unique_ptr<KamataEngine::Model> modelSkyDome;
	uint32_t skyDomeTex = 0;

	// --- Background models (menu scenes) ---
	std::unique_ptr<KamataEngine::Model> modelBackgroundHill;
	std::unique_ptr<KamataEngine::Model> modelBackgroundTown;
	std::unique_ptr<KamataEngine::Model> modelBackgroundTrees;
	uint32_t hillTex = 0;
	uint32_t townTex = 0;
	uint32_t treesTex = 0;

	// --- Player ---
	std::unique_ptr<KamataEngine::Model> modelPlayerBody;
	std::unique_ptr<KamataEngine::Model> modelPlayerHead;
	std::unique_ptr<KamataEngine::Model> modelPlayerHat;
	uint32_t playerBodyTex = 0;
	uint32_t playerHeadTex = 0;
	uint32_t playerHatTex = 0;

	// --- Heavy sabre (null when the OBJ is missing: the weapon still works, it just isn't drawn) ---
	std::unique_ptr<KamataEngine::Model> modelHeavySabre;
	uint32_t heavySabreTex = 0;

	// --- Block (the only tile type for now) ---
	std::unique_ptr<KamataEngine::Model> modelBlock;
	uint32_t blockTex = 0;
};