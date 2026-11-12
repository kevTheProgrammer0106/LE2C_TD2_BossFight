#include "GameAssets.h"

#include <filesystem>

using namespace KamataEngine;

namespace {
// Heavy sabre asset names: Resources/<name>/<name>.obj and Resources/<name>/<name>.png
// Change this to match the folder of the old project's sabre model.
constexpr const char* kHeavySabreName = "weaponHeavySword";
} // namespace

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

	// --- Heavy sabre (checked first: CreateFromOBJ asserts on a missing file) ---
	const std::string sabreDir = std::string("Resources/") + kHeavySabreName + "/";
	if (std::filesystem::exists(sabreDir + kHeavySabreName + ".obj")) {
		modelHeavySabre.reset(Model::CreateFromOBJ(kHeavySabreName, true));
		const std::string texPath = sabreDir + kHeavySabreName + ".png";
		heavySabreTex = TextureManager::Load(std::filesystem::exists(texPath) ? "./" + texPath : std::string("./Resources/white1x1.png"));
	}

	// --- Block: the old linkID 0 block (grass) ---
	modelBlock.reset(Model::CreateFromOBJ("blockGrass"));
	blockTex = TextureManager::Load("./Resources/blockGrass/blockGrass.png");
}