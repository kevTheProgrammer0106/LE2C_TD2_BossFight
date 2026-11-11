#pragma once

#include "AudioSettings.h"
#include <KamataEngine.h>
#define _USE_MATH_DEFINES
#include <algorithm>
#include <cassert>
#include <math.h>
#include <cmath>
#include <numbers>
#include<array>
#include <string>

class Global {};

enum class Owner { kEmpty, kPlayer, kEnemy };

enum class DropItemCategory { kMelee, kFirearm, kUnknown };
enum class DropWeaponKind { kSabreLight, kSabreHeavy, kLongAxe, kSpear, kMusketLong, kMusketBayonet, kShotgun, kDualPistols, kUnknown };

struct AABB {
	KamataEngine::Vector3 min; //!< 最小点
	KamataEngine::Vector3 max; //!< 最大点
};
enum Corner {
	kTopLeft,     // 左上
	kTopRight,    // 右上
	kBottomLeft,  // 左下
	kBottomRight, // 右下

	kNumCorner // 要素数
};
// 当たり判定構造体
struct CollisionInfo {
	bool isHitTop = false;
	bool isHitGround = false;
	bool isHitWall = false;
	KamataEngine::Vector3 move = {};
};

enum class PartSlot { kBody, kHead, kHat, kCount };

// ---- Attack effect textures (slash / thrust / smash) ----
enum class AttackEffectType { kSlash, kSlashOpposite, kSlashDownward, kSlashAlt, kThrust, kSmash, kCount };

struct AttackEffectTextures {
	std::array<uint32_t, static_cast<size_t>(AttackEffectType::kCount)> tex{};
	void Set(AttackEffectType type, uint32_t handle) { tex[static_cast<size_t>(type)] = handle; }
	uint32_t Get(AttackEffectType type) const {
		const uint32_t handle = tex[static_cast<size_t>(type)];
		return handle != 0 ? handle : tex[static_cast<size_t>(AttackEffectType::kSlash)]; // missing art falls back to the plain slash
	}
};

inline constexpr float kFacingRotationYRight = 1.570796f;
inline constexpr float kFacingRotationYLeft = 4.712389f;

struct SRTOffset {
	KamataEngine::Vector3 scale{1.0f, 1.0f, 1.0f};
	KamataEngine::Vector3 rotation{0.0f, 0.0f, 0.0f};
	KamataEngine::Vector3 translation{0.0f, 0.0f, 0.0f};
};

struct ComboAttackPattern {
	SRTOffset anticipateStart; // 予備動作開始時の姿勢（通常はidle姿勢と同じでよい）
	SRTOffset anticipateEnd;   // 予備動作完了時の姿勢
	SRTOffset attackEnd;       // 攻撃振り切り時の姿勢
	SRTOffset recoveryEnd;     // 硬直終了時の姿勢（通常はidle姿勢に戻す）
};
enum class BodyAttackPhase { kAnticipate, kAttack, kRecovery };

inline SRTOffset MakeBodyPose(float rotX, float rotY, float rotZ, KamataEngine::Vector3 scale = {1.0f, 1.0f, 1.0f}, KamataEngine::Vector3 translation = {0.0f, 0.0f, 0.0f}) {
	SRTOffset pose;
	pose.scale = scale;
	pose.rotation = {rotX, rotY, rotZ};
	pose.translation = translation;
	return pose;
}

// Body swing for a combo weapon: 3 combo steps x (anticipate / peak / recovery). Same shape the player uses.
struct BodyComboPoseSet {
	std::array<SRTOffset, 3> anticipate;
	std::array<SRTOffset, 3> peak;
	std::array<SRTOffset, 3> recovery;
};
// Body pose for a thrust (lunge / bayonet).
struct BodyLungePoseSet {
	SRTOffset windUp;
	SRTOffset thrust;
};

// Carried by DroppableItem so a weapon keeps its state when dropped and picked up again.
//  Firearm: kReady = loaded,        kOnCooldown = unloaded (reloading)
//  Melee:   kReady = ready to swing, kOnCooldown = dropped mid-swing / recovering
enum class DropItemState : uint8_t { kReady = 0, kOnCooldown = 1 };

enum class LungeSource { kNone, kHeavySabre, kBayonet };

// Player special attack parameters (frames @60fps, speed per frame, cooldown seconds)
struct LungeSpec {
	float anticipateFrames = 10.0f;
	float attackFrames = 8.0f;
	float recoveryFrames = 15.0f;
	float speed = 0.3f;
	int damage = 50;
	float impactPower = 1.8f;
	float cooldown = 4.0f;
};

SRTOffset EvaluateBodyComboPose(const BodyComboPoseSet& set, BodyAttackPhase phase, float t, int comboIndex);
SRTOffset EvaluateBodyLungePose(const BodyLungePoseSet& set, BodyAttackPhase phase, float t);

// GlobalVariables glue so every set is tunable the same way (keys: <prefix><combo><phase><Scale|Rotation|Translation>)
void RegisterBodyComboPoseSet(const char* group, const char* prefix, const BodyComboPoseSet& set);
void ApplyBodyComboPoseSet(const char* group, const char* prefix, BodyComboPoseSet& set);
void RegisterBodyLungePoseSet(const char* group, const char* prefix, const BodyLungePoseSet& set);
void ApplyBodyLungePoseSet(const char* group, const char* prefix, BodyLungePoseSet& set);

struct BodyComboPattern {
	SRTOffset anticipateStart; // 予備動作開始時の姿勢（通常はidle姿勢と同じでよい）
	SRTOffset anticipateEnd;   // 予備動作完了時の姿勢
	SRTOffset attackEnd;       // 攻撃振り切り時の姿勢
	SRTOffset recoveryEnd;     // 硬直終了時の姿勢（通常はidle姿勢に戻す）
};

struct PartResources {
	KamataEngine::Model* model = nullptr;
	uint32_t textureHandle = 0;
};
struct BodyPart {
	KamataEngine::WorldTransform transform;
	KamataEngine::Model* model = nullptr;
	uint32_t textureHandle = 0;
};
enum class ReactionPose
{
	kNone,
	kKnockback,
	kStun, kBlock
};

struct AimFacing {
	bool facingRight;
	float localAngle; // rotation_.z to feed the gun, already mirrored for the flip side
};

KamataEngine::Vector3 WorldToScreen(const KamataEngine::Camera& camera, const KamataEngine::Vector3& world);
// ワールド座標からスクリーン座標へ変換（Vector2）
KamataEngine::Vector2 WorldToScreen(const KamataEngine::Camera& camera, const KamataEngine::Vector2& world);

AimFacing ComputeAimFacing(const KamataEngine::Vector2& aimDir, bool currentFacingRight);

float TriangleWave(float timer, float speed);
SRTOffset LerpSRT(const SRTOffset& a, const SRTOffset& b, float t);

bool IsCollisionAABB(const AABB& a, const AABB& b);

/// <summary>
///	行列計算・転送する
/// </summary>
void WorldTransformUpdate(KamataEngine::WorldTransform& worldTransform);

KamataEngine::Matrix4x4 Multiply(KamataEngine::Matrix4x4 matrix1, KamataEngine::Matrix4x4 matrix2);

KamataEngine::Matrix4x4 Inverse(KamataEngine::Matrix4x4 matrix);

// 1. X軸回転行列
KamataEngine::Matrix4x4 MakeRotateXMatrix(float radian);
// 2. Y軸回転行列
KamataEngine::Matrix4x4 MakeRotateYMatrix(float radian);
// 3. Z軸回転行列
KamataEngine::Matrix4x4 MakeRotateZMatrix(float radian);

KamataEngine::Matrix4x4 MakeScaleMatrix(KamataEngine::Vector3 scale);
KamataEngine::Matrix4x4 MakeTranslateMatrix(KamataEngine::Vector3 translate);

KamataEngine::Matrix4x4 MakeAffineMatrix(KamataEngine::Vector3 scale, KamataEngine::Vector3 rotation, KamataEngine::Vector3 translation);

KamataEngine::Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

KamataEngine::Vector3 TransformCoordinate(const KamataEngine::Vector3& vector, const KamataEngine::Matrix4x4& m);
// 座標変換（Vector2）
KamataEngine::Vector2 TransformCoordinate(const KamataEngine::Vector2& vector, const KamataEngine::Matrix4x4& m);


KamataEngine::Vector3 TransformNormal(const KamataEngine::Vector3& vector, const KamataEngine::Matrix4x4& m);

float Lerp(float start, float end, float t);

KamataEngine::Vector3 Lerp(KamataEngine::Vector3 start, KamataEngine::Vector3 end, float t);

float EaseIn(float start, float end, float t);
float EaseOut(float start, float end, float t);
float EaseInOut(float start, float end, float t);