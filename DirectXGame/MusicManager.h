#pragma once

#define NOMINMAX

#include <KamataEngine.h>
#include <string>

/// <summary>
/// メニュー(タイトル)BGM / インゲーム(探索用)BGM / アリーナ(戦闘用)BGMの3トラックを管理する。
/// ハンドル・音量等の実際の再生状態はstatic(全インスタンス共有)にしてある。これはTitleSceneと
/// GameSceneがそれぞれ自分のMusicManagerインスタンスを持つため — インスタンスをまたいでも
/// 同じ音声チャンネルを参照し続けられるようにするための設計(そうしないとRetry/Restartのたびに
/// BGMが二重に鳴ったり、ぶつ切りになったりする)。
/// Reset()はメニューに戻る時にのみ呼ぶ想定(GameSceneのデストラクタから)。
/// </summary>
class MusicManager {
public:
	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();

	// インゲーム/アリーナトラックの再生を開始する(ループON, 音量0から)。
	// GameScene::Initializeで1回だけ呼ぶ想定。
	void Initialize(uint32_t ingameSoundDataHandle, uint32_t arenaSoundDataHandle);

	// メニュートラックの再生を開始する(ループON, 音量0から)。
	// TitleScene::Initializeで呼ぶ想定。
	void InitializeMenuTrack(uint32_t menuSoundDataHandle);

	// 毎フレーム呼ぶ。フェードの進行(音量の補間)のみを行う。
	void Update();

	// アリーナ開始: アリーナBGMへクロスフェードする
	void StartArenaMusic();
	// アリーナクリア/離脱: インゲームBGMへクロスフェードして戻す
	void StopArenaMusic();

	// メニュートラックをフェードアウトして止める(ゲーム開始時にTitleSceneから呼ぶ)
	void StopMenuMusic();

	bool IsArenaMusicActive() const { return arenaActive_; }

	// 全トラックの再生状態をまっさらにする。メニューへ戻った時にのみ呼ぶ想定
	// (ハンドルはstaticで共有されているので、次に作られるTitleScene/GameSceneが
	// Initialize系を呼べばゼロから鳴らし直せるようにする)。
	void Reset();

private:
	static inline uint32_t ingameSoundDataHandle_ = 0;
	static inline uint32_t arenaSoundDataHandle_ = 0;
	static inline uint32_t menuSoundDataHandle_ = 0;

	static inline uint32_t ingameVoiceHandle_ = 0;
	static inline uint32_t arenaVoiceHandle_ = 0;
	static inline uint32_t menuVoiceHandle_ = 0;

	static inline float ingameVolumeCurrent_ = 0.0f;
	static inline float arenaVolumeCurrent_ = 0.0f;
	static inline float menuVolumeCurrent_ = 0.0f;
	static inline float ingameVolumeTarget_ = 1.0f;
	static inline float arenaVolumeTarget_ = 0.0f;
	static inline float menuVolumeTarget_ = 0.0f;

	static inline bool arenaActive_ = false;
	static inline bool initialized_ = false;

	static inline float kIngameBaseVolume = 0.3f;
	static inline float kArenaBaseVolume = 0.5f;
	static inline float kMenuBaseVolume = 0.5f;
	static inline float kFadeSpeed = 0.6f; 

	void ApplyVolumes();
};