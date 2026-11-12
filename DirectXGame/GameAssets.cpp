#include "GameAssets.h"

using namespace KamataEngine;

void GameAssets::Load() {
	// --- Skydome ---
	modelSkyDome.reset(Model::CreateFromOBJ("skydome", true));
	skyDomeTex = TextureManager::Load("./Resources/skydome/skydome.png");

	modelBackgroundHill.reset(Model::CreateFromOBJ("backgroundPlain"));
	modelBackgroundTown.reset(Model::CreateFromOBJ("backgroundTown"));
	modelBackgroundTrees.reset(Model::CreateFromOBJ("backgroundTrees"));
	hillTex = TextureManager::Load("./Resources/backgroundPlain/grassTexture.png");
	townTex = TextureManager::Load("./Resources/backgroundTown/grayTex.png");
	treesTex = TextureManager::Load("./Resources/backgroundTrees/trees.png");

	// --- Player ---
	modelPlayerHead.reset(Model::CreateFromOBJ("playerhead", true));
	modelPlayerHat.reset(Model::CreateFromOBJ("playerhat", true));
	modelPlayerBody.reset(Model::CreateFromOBJ("playerbody", true));
	playerBodyTex = TextureManager::Load("./Resources/playerBody/playerBody.png");
	playerHeadTex = TextureManager::Load("./Resources/playerHead/playerHead.png");
	playerHatTex = TextureManager::Load("./Resources/playerHat/playerShako.png");

	// --- Boss ---
	modelBoss.reset(Model::CreateFromOBJ("boss", true));
	bossTex = TextureManager::Load("./Resources/boss/White1x1.png");

	// --- Block: the old linkID 0 block (grass) ---
	modelBlock.reset(Model::CreateFromOBJ("blockGrass"));
	blockTex = TextureManager::Load("./Resources/blockGrass/blockGrass.png");
}
