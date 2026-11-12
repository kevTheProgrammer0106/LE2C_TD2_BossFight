#pragma once

#include "BlockDetector.h"
#include "Global.h"
#include <array>
#include <memory>

class MapChipField;
class PlayerBehavior;
class BaseMelee;
class WeaponHeavySabre;

// X-Z平面をWASDで移動し、ブロックとの衝突判定を行うトップダウン視点用プレイヤー。
// カーソルロック中はプレイヤーの背面をカメラ側へ向け、通常移動もカメラ基準で行う。
// フリーカーソル中はプレイヤーの向きを維持し、ダッシュ・ロール・攻撃がカメラ旋回の影響を受けないようにする。
// ダッシュ／攻撃中は開始時のYawを固定し、ロール終了後などは必要に応じてカメラの向きへ滑らかに戻す。
// Shift：ダッシュ。ダッシュ中にShift：バレルロール。
// 重厚なサーベル：左クリックで最大3段のコンボ（攻撃中に再入力すると次の攻撃へ連携）、
// 右クリックでクールダウン付きの突進攻撃、Qで武器の納刀／抜刀を切り替える。
// 状態遷移はPlayerBehaviorが管理する。
class Player {
public:
	Player();
	~Player();

	enum class MoveMode { kNormal, kDash, kRoll };

	void Initialize(const std::array<PartResources, static_cast<size_t>(PartSlot::kCount)>& partResources, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	// ---- 武器 ----
	// modelがnullptrでもサーベルの攻撃判定は機能する（モデルの描画だけを行わない）。
	void EquipHeavySabre(KamataEngine::Model* model, uint32_t textureHandle);
	BaseMelee* GetMelee() const;
	WeaponHeavySabre* GetHeavySabre() const { return heavySabre_.get(); }
	bool IsWeaponEquipped() const;
	void SetWeaponEquipped(bool equipped);

	// 突進攻撃のクールダウン。突進を開始した時点でクールダウンを開始する。
	LungeSpec GetLungeSpec() const;
	void BeginLungeCooldown();
	bool CanLunge() const { return lungeCooldown_ <= 0.0f; }
	float GetLungeCooldownRemaining() const { return lungeCooldown_; }
	void ResetLungeHits() { lungeHit_ = false; }
	void MarkLungeHit() { lungeHit_ = true; } // 後で敵などの当たり判定処理から呼び出す。
	bool HasLungeHits() const { return lungeHit_; }

	// 攻撃フェーズ中のみ有効な攻撃判定。存在しない場合はfalseを返す。
	bool GetAttackHitbox(AABB& outBox) const;

	// ---- 状態／デバフ ----
	PlayerBehavior& GetBehavior() const { return *behavior_; }
	bool IsControlLocked() const { return controlLocked_; }
	void SetControlLocked(bool locked) { controlLocked_ = locked; }
	void SetStunned(float duration);
	void SetKnockback(const KamataEngine::Vector3& direction, float power); // X-Z平面上の方向を指定する。
	void ClearStun();
	void ClearKnockback();
	void ClearAllDebuffs();
	bool HasAnyDebuff() const;

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	KamataEngine::Vector3 GetWorldPosition() const { return worldTransform_.translation_; }
	float GetYaw() const { return worldTransform_.rotation_.y; } // 補間中または固定中のモデルのYaw。
	KamataEngine::Vector3 GetForward() const;                    // X-Z平面上でプレイヤーが向いている方向。
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	void ResetVelocity() { velocity_ = {}; }
	AABB GetAABB() const;
	AABB GetFootprintAABB() const; // ブロックとの衝突判定に使う、Y方向に薄いX-Z矩形。

	MoveMode GetMoveMode() const { return moveMode_; }
	bool IsDashing() const { return moveMode_ == MoveMode::kDash; }
	bool IsRolling() const { return moveMode_ == MoveMode::kRoll; }
	bool IsYawLocked() const { return yawLocked_; }

	// UI
	float GetHPRatio() const { return hp_ / kMaxHP; }
	float GetStaminaRatio() const { return stamina_ / kMaxStamina; }

private:
	friend class PlayerBehavior;
	friend class DebugMenu;

	void RegisterGlobalVariables(); // ダッシュ／ロール関連の調整値をGlobalVariablesへ登録する。
	void ApplyGlobalVariables();    // GlobalVariablesで設定された値を各パラメータへ反映する。
	void Move();                    // 通常移動／ダッシュ／ロール移動。PlayerBehaviorのkRoot状態から呼び出す。
	void MapChipCollisionUpdate();
	void UpdateBodyParts();
	void UpdateAttackInput();
	void UpdateWeapon();
	void ResetMoveMode() {
		moveMode_ = MoveMode::kNormal;
		modeTimer_ = 0.0f;
	}

	void EnterDash(const KamataEngine::Vector3& dir);
	void EnterRoll(const KamataEngine::Vector3& dir, float sign);

	// ---- 向き（Yaw） ----
	bool ShouldLockYaw() const; // ダッシュ中または攻撃中（攻撃を要求したフレームも含む）なら固定する。
	void UpdateFacing();        // 固定YawまたはカメラYawへ補間し、worldTransform_.rotation_.yへ反映する。

	KamataEngine::Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	BlockDetector blockDetector_;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Matrix4x4 bodyRotation_{}; // 武器や攻撃判定で共有する体の回転行列。
	KamataEngine::Vector3 velocity_ = {};

	std::array<BodyPart, static_cast<size_t>(PartSlot::kCount)> parts_;
	enum { body = 0, head = 1, hat = 2 };

	std::unique_ptr<PlayerBehavior> behavior_;
	std::unique_ptr<WeaponHeavySabre> heavySabre_;

	bool controlLocked_ = false;
	float lungeCooldown_ = 0.0f; // 秒単位。
	bool lungeHit_ = false;

	// --- ダッシュ／バレルロール ---
	MoveMode moveMode_ = MoveMode::kNormal;
	float modeTimer_ = 0.0f; // 現在のダッシュ／ロール状態が続いた時間（秒）。
	KamataEngine::Vector3 dashDir_ = {0.0f, 0.0f, -1.0f};
	KamataEngine::Vector3 rollDir_ = {0.0f, 0.0f, -1.0f}; // ロール方向（ローカル方向、または固定済みワールド方向）。
	bool rollDirectionIsWorld_ = false;                   // trueの場合、rollDir_はワールド方向として扱う。
	bool rollIsHorizontal_ = false;                       // true：A/Dの左右ロール、false：W/Sの前後ロール。
	float rollSign_ = -1.0f;                              // -1：右へロール、+1：左へロール。
	SRTOffset currentPose_ = {};                          // 補間後の移動姿勢（AnimationManager::SmoothSRT）。
	SRTOffset currentActionPose_ = {};                    // PlayerBehaviorから取得する補間後の攻撃姿勢。

	// --- 向きの状態 ---
	float facingYaw_ = 0.0f;    // モデルに実際に適用するYaw（補間後）。
	float lockedYaw_ = 0.0f;    // ダッシュ／攻撃開始時に固定するYaw。
	bool yawLocked_ = false;    // ダッシュ中または攻撃中にtrue。
	bool yawReleasing_ = false; // 固定解除後、カメラYawに追いつくまでtrue。
	bool yawDetached_ = false;  // フリーカーソル中はカメラ旋回に関係なくモデルのYawを維持する。

	static inline float kAcceleration = 0.05f;
	static inline float kAttenuation = 0.1f;
	static inline float kLimitRunSpeed = 0.25f;
	static inline float kWidth = 0.8f; // X方向の幅。
	static inline float kDepth = 0.8f; // Z方向の奥行き。
	static inline float kHeight = 1.6f;
	static inline float kMaxHP = 100.0f;
	static inline float kMaxStamina = 100.0f;
	float hp_ = kMaxHP;
	float stamina_ = kMaxStamina;

	// ダッシュ／ロールの調整値（速度は60fps時の1フレームあたりの移動量）。
	static inline float kDashSpeed = 0.45f;
	static inline float kDashDuration = 0.8f;
	static inline float kDashAccel = 0.25f; // 目標速度へ近づける1フレームあたりの補間率。
	static inline float kRollSpeed = 0.55f;
	static inline float kRollDuration = 0.5f;

	// Yawの補間速度（秒単位の指数補間）。値が大きいほど素早く追従する。
	// 約7の場合、時定数はおよそ0.14秒。
	static inline float kYawFollowSpeed = 25.0f; // 通常移動中：カメラへ素早く追従する。
	static inline float kYawLockSpeed = 25.0f;   // ダッシュ／攻撃開始時：固定Yawへ追従する。
	static inline float kYawReleaseSpeed = 7.0f; // ロール後などの固定解除時：ゆっくりカメラへ戻る。

	// 体の姿勢。左右ロールと前後ロールは別々のSRTOffsetで調整する。
	static inline SRTOffset kDashPose = {
	    {1.0f, 0.95f,  1.05f},
        {0.4f, 0.0f,   0.0f },
        {0.0f, -0.05f, 0.0f }
    };
	// A/Dで行う左右ロール用の姿勢。
	static inline SRTOffset kHorizontalRollPose = {
	    {0.95f, 0.95f, 1.1f},
        {0.15f, 0.0f,  0.0f},
        {0.0f,  0.0f,  0.0f}
    };
	// W/Sで行う前後ロール用の姿勢。左右ロールとは独立して調整できる。
	static inline SRTOffset kVerticalRollPose = {
	    {0.95f, 0.95f, 1.1f},
        {0.15f, 0.0f,  0.0f},
        {0.0f,  0.0f,  0.0f}
    };
	static inline float kRollRotationAngle = 2.0f * 3.14159265358979323846f; // ロール中の回転角度（ラジアン）。

	// 横視点版プレイヤーと同じパーツ構成。モデルの原点はworldTransform_の位置にある。
	static inline SRTOffset headOffset_ = {
	    {1.0f,   1.0f,  1.0f},
        {0.0f,   0.0f,  0.0f},
        {-0.05f, 0.72f, 0.0f}
    };
	static inline SRTOffset hatOffset_ = {
	    {1.0f, 1.0f, 1.0f  },
        {0.0f, 0.0f, 0.0f  },
        {0.0f, 0.1f, -0.11f}
    };
};
