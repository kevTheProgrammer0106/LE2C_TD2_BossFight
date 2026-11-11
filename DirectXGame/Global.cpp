#include "Global.h"
#include "GlobalVariables.h"

using namespace KamataEngine;

namespace {
void RegisterSRT(const char* group, const std::string& key, const SRTOffset& v) {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->AddItem(group, key + "Scale", v.scale);
	gv->AddItem(group, key + "Rotation", v.rotation);
	gv->AddItem(group, key + "Translation", v.translation);
}
void ApplySRT(const char* group, const std::string& key, SRTOffset& v) {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	v.scale = gv->GetVector3Value(group, key + "Scale");
	v.rotation = gv->GetVector3Value(group, key + "Rotation");
	v.translation = gv->GetVector3Value(group, key + "Translation");
}
} // namespace

SRTOffset EvaluateBodyComboPose(const BodyComboPoseSet& set, BodyAttackPhase phase, float t, int comboIndex) {
	int idx = std::clamp(comboIndex, 0, 2);
	t = std::clamp(t, 0.0f, 1.0f);
	switch (phase) {
	case BodyAttackPhase::kAnticipate:
		return LerpSRT(SRTOffset{}, set.anticipate[idx], EaseIn(0.0f, 1.0f, t));
	case BodyAttackPhase::kAttack:
		return LerpSRT(set.anticipate[idx], set.peak[idx], EaseOut(0.0f, 1.0f, t));
	case BodyAttackPhase::kRecovery:
		return LerpSRT(set.recovery[idx], SRTOffset{}, EaseInOut(0.0f, 1.0f, t));
	}
	return SRTOffset{};
}

SRTOffset EvaluateBodyLungePose(const BodyLungePoseSet& set, BodyAttackPhase phase, float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	switch (phase) {
	case BodyAttackPhase::kAnticipate:
		return LerpSRT(SRTOffset{}, set.windUp, EaseIn(0.0f, 1.0f, t));
	case BodyAttackPhase::kAttack:
		return LerpSRT(set.windUp, set.thrust, EaseOut(0.0f, 1.0f, t));
	case BodyAttackPhase::kRecovery:
		return LerpSRT(set.thrust, SRTOffset{}, EaseInOut(0.0f, 1.0f, t));
	}
	return SRTOffset{};
}

void RegisterBodyComboPoseSet(const char* group, const char* prefix, const BodyComboPoseSet& set) {
	for (int i = 0; i < 3; ++i) {
		std::string base = std::string(prefix) + std::to_string(i);
		RegisterSRT(group, base + "Anticipate", set.anticipate[i]);
		RegisterSRT(group, base + "Peak", set.peak[i]);
		RegisterSRT(group, base + "Recovery", set.recovery[i]);
	}
}
void ApplyBodyComboPoseSet(const char* group, const char* prefix, BodyComboPoseSet& set) {
	for (int i = 0; i < 3; ++i) {
		std::string base = std::string(prefix) + std::to_string(i);
		ApplySRT(group, base + "Anticipate", set.anticipate[i]);
		ApplySRT(group, base + "Peak", set.peak[i]);
		ApplySRT(group, base + "Recovery", set.recovery[i]);
	}
}
void RegisterBodyLungePoseSet(const char* group, const char* prefix, const BodyLungePoseSet& set) {
	RegisterSRT(group, std::string(prefix) + "WindUp", set.windUp);
	RegisterSRT(group, std::string(prefix) + "Thrust", set.thrust);
}
void ApplyBodyLungePoseSet(const char* group, const char* prefix, BodyLungePoseSet& set) {
	ApplySRT(group, std::string(prefix) + "WindUp", set.windUp);
	ApplySRT(group, std::string(prefix) + "Thrust", set.thrust);
}

void WorldTransformUpdate(KamataEngine::WorldTransform& worldTransform) {
	// スケール、回転、平行移動からワールド行列を計算
	worldTransform.matWorld_ = MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
	// 定数バッフアへの書き込み
	worldTransform.TransferMatrix(); 
}
Vector3 WorldToScreen(const Camera& camera, const Vector3& world) {
	Matrix4x4 vp = MakeViewportMatrix(0.0f, 0.0f, static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight), 0.0f, 1.0f);
	return TransformCoordinate(world, Multiply(camera.matView, Multiply(camera.matProjection, vp)));
}
Vector2 WorldToScreen(const Camera& camera, const Vector2& world) {
	// ビューポート行列を作成する
	Matrix4x4 vp = MakeViewportMatrix(0.0f, 0.0f, static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight), 0.0f, 1.0f);
	// ワールド座標からスクリーン座標への変換行列を作成する
	Matrix4x4 viewProjectionViewport = Multiply(camera.matView, Multiply(camera.matProjection, vp));
	return TransformCoordinate(world, viewProjectionViewport);
}
AimFacing ComputeAimFacing(const Vector2& aimDir, bool currentFacingRight) {
	float worldAngle = std::atan2(aimDir.y, aimDir.x); // 0 = +X, +pi/2 = up

	constexpr float kFlipHysteresis = 0.1f; // ラジアン。90度境界の不感帯(要調整)

	bool facingRight;
	if (currentFacingRight) {
		// 右向きの場合、cos(worldAngle) がマイナス方向にある程度超えるまでは右のまま
		facingRight = std::cos(worldAngle) >= -kFlipHysteresis;
	} else {
		// 左向きの場合、プラス方向にある程度超えるまでは左のまま
		facingRight = std::cos(worldAngle) > kFlipHysteresis;
	}

	float localAngle = facingRight ? worldAngle : (std::numbers::pi_v<float> - worldAngle);
	while (localAngle > std::numbers::pi_v<float>)
		localAngle -= 2.0f * std::numbers::pi_v<float>;
	while (localAngle <= -std::numbers::pi_v<float>)
		localAngle += 2.0f * std::numbers::pi_v<float>;

	return {facingRight, localAngle};
}
float TriangleWave(float timer, float speed) {
	float phase = std::fmod(timer * speed, 2.0f * std::numbers::pi_v<float>);
	return (std::sin(phase) + 1.0f) * 0.5f;
}
SRTOffset LerpSRT(const SRTOffset& a, const SRTOffset& b, float t) { return {Lerp(a.scale, b.scale, t), Lerp(a.rotation, b.rotation, t), Lerp(a.translation, b.translation, t)}; }

bool IsCollisionAABB(const AABB& a, const AABB& b) {
	if ((a.min.x <= b.max.x && a.max.x >= b.min.x) && // x軸
	    (a.min.y <= b.max.y && a.max.y >= b.min.y) && // y軸
	    (a.min.z <= b.max.z && a.max.z >= b.min.z))   // // z軸

	{
		return true;
	}
	return false;
}

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
	Matrix4x4 result{};
	result.m[0][0] = width / 2.0f;
	result.m[1][1] = -height / 2.0f;
	result.m[2][2] = maxDepth - minDepth;
	result.m[3][0] = left + width / 2.0f;
	result.m[3][1] = top + height / 2.0f;
	result.m[3][2] = minDepth;
	result.m[3][3] = 1.0f;
	return result;
}
Matrix4x4 Inverse(Matrix4x4 matrix) {
	float determinant =
	    matrix.m[0][0] * (matrix.m[1][1] * (matrix.m[2][2] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][2]) - matrix.m[1][2] * (matrix.m[2][1] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][1]) +
	                      matrix.m[1][3] * (matrix.m[2][1] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][1])) -
	    matrix.m[0][1] * (matrix.m[1][0] * (matrix.m[2][2] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][2]) - matrix.m[1][2] * (matrix.m[2][0] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][0]) +
	                      matrix.m[1][3] * (matrix.m[2][0] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][0])) +
	    matrix.m[0][2] * (matrix.m[1][0] * (matrix.m[2][1] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][1]) - matrix.m[1][1] * (matrix.m[2][0] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][0]) +
	                      matrix.m[1][3] * (matrix.m[2][0] * matrix.m[3][1] - matrix.m[2][1] * matrix.m[3][0])) -
	    matrix.m[0][3] * (matrix.m[1][0] * (matrix.m[2][1] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][1]) - matrix.m[1][1] * (matrix.m[2][0] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][0]) +
	                      matrix.m[1][2] * (matrix.m[2][0] * matrix.m[3][1] - matrix.m[2][1] * matrix.m[3][0]));
#ifdef _DEBUG
	assert(determinant != 0.0f);
#endif
	float invDet = 1.0f / determinant;
	return matrix = {
	           (matrix.m[1][1] * (matrix.m[2][2] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][2]) - matrix.m[1][2] * (matrix.m[2][1] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][1]) +
	            matrix.m[1][3] * (matrix.m[2][1] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][1])) *
	               invDet,
	           -(matrix.m[0][1] * (matrix.m[2][2] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][2]) - matrix.m[0][2] * (matrix.m[2][1] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][1]) +
	             matrix.m[0][3] * (matrix.m[2][1] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][1])) *
	               invDet,
	           (matrix.m[0][1] * (matrix.m[1][2] * matrix.m[3][3] - matrix.m[1][3] * matrix.m[3][2]) - matrix.m[0][2] * (matrix.m[1][1] * matrix.m[3][3] - matrix.m[1][3] * matrix.m[3][1]) +
	            matrix.m[0][3] * (matrix.m[1][1] * matrix.m[3][2] - matrix.m[1][2] * matrix.m[3][1])) *
	               invDet,
	           -(matrix.m[0][1] * (matrix.m[1][2] * matrix.m[2][3] - matrix.m[1][3] * matrix.m[2][2]) - matrix.m[0][2] * (matrix.m[1][1] * matrix.m[2][3] - matrix.m[1][3] * matrix.m[2][1]) +
	             matrix.m[0][3] * (matrix.m[1][1] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][1])) *
	               invDet,

	           -(matrix.m[1][0] * (matrix.m[2][2] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][2]) - matrix.m[1][2] * (matrix.m[2][0] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][0]) +
	             matrix.m[1][3] * (matrix.m[2][0] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][0])) *
	               invDet,
	           (matrix.m[0][0] * (matrix.m[2][2] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][2]) - matrix.m[0][2] * (matrix.m[2][0] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][0]) +
	            matrix.m[0][3] * (matrix.m[2][0] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][0])) *
	               invDet,
	           -(matrix.m[0][0] * (matrix.m[1][2] * matrix.m[3][3] - matrix.m[1][3] * matrix.m[3][2]) - matrix.m[0][2] * (matrix.m[1][0] * matrix.m[3][3] - matrix.m[1][3] * matrix.m[3][0]) +
	             matrix.m[0][3] * (matrix.m[1][0] * matrix.m[3][2] - matrix.m[1][2] * matrix.m[3][0])) *
	               invDet,
	           (matrix.m[0][0] * (matrix.m[1][2] * matrix.m[2][3] - matrix.m[1][3] * matrix.m[2][2]) - matrix.m[0][2] * (matrix.m[1][0] * matrix.m[2][3] - matrix.m[1][3] * matrix.m[2][0]) +
	            matrix.m[0][3] * (matrix.m[1][0] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][0])) *
	               invDet,

	           (matrix.m[1][0] * (matrix.m[2][1] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][1]) - matrix.m[1][1] * (matrix.m[2][0] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][0]) +
	            matrix.m[1][3] * (matrix.m[2][0] * matrix.m[3][1] - matrix.m[2][1] * matrix.m[3][0])) *
	               invDet,
	           -(matrix.m[0][0] * (matrix.m[2][1] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][1]) - matrix.m[0][1] * (matrix.m[2][0] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][0]) +
	             matrix.m[0][3] * (matrix.m[2][0] * matrix.m[3][1] - matrix.m[2][1] * matrix.m[3][0])) *
	               invDet,
	           (matrix.m[0][0] * (matrix.m[1][1] * matrix.m[3][3] - matrix.m[1][3] * matrix.m[3][1]) - matrix.m[0][1] * (matrix.m[1][0] * matrix.m[3][3] - matrix.m[1][3] * matrix.m[3][0]) +
	            matrix.m[0][3] * (matrix.m[1][0] * matrix.m[3][1] - matrix.m[1][1] * matrix.m[3][0])) *
	               invDet,
	           -(matrix.m[0][0] * (matrix.m[1][1] * matrix.m[2][3] - matrix.m[1][3] * matrix.m[2][1]) - matrix.m[0][1] * (matrix.m[1][0] * matrix.m[2][3] - matrix.m[1][3] * matrix.m[2][0]) +
	             matrix.m[0][3] * (matrix.m[1][0] * matrix.m[2][1] - matrix.m[1][1] * matrix.m[2][0])) *
	               invDet,

	           -(matrix.m[1][0] * (matrix.m[2][1] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][1]) - matrix.m[1][1] * (matrix.m[2][0] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][0]) +
	             matrix.m[1][2] * (matrix.m[2][0] * matrix.m[3][1] - matrix.m[2][1] * matrix.m[3][0])) *
	               invDet,
	           (matrix.m[0][0] * (matrix.m[2][1] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][1]) - matrix.m[0][1] * (matrix.m[2][0] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][0]) +
	            matrix.m[0][2] * (matrix.m[2][0] * matrix.m[3][1] - matrix.m[2][1] * matrix.m[3][0])) *
	               invDet,
	           -(matrix.m[0][0] * (matrix.m[1][1] * matrix.m[3][2] - matrix.m[1][2] * matrix.m[3][1]) - matrix.m[0][1] * (matrix.m[1][0] * matrix.m[3][2] - matrix.m[1][2] * matrix.m[3][0]) +
	             matrix.m[0][2] * (matrix.m[1][0] * matrix.m[3][1] - matrix.m[1][1] * matrix.m[3][0])) *
	               invDet,
	           (matrix.m[0][0] * (matrix.m[1][1] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][1]) - matrix.m[0][1] * (matrix.m[1][0] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][0]) +
	            matrix.m[0][2] * (matrix.m[1][0] * matrix.m[2][1] - matrix.m[1][1] * matrix.m[2][0])) *
	               invDet,
	       };
}
#pragma region 行列計算
Matrix4x4 Multiply(Matrix4x4 matrix1, Matrix4x4 matrix2) {
	Matrix4x4 result;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] = 0.0f;
			for (int k = 0; k < 4; k++) {
				result.m[i][j] += matrix1.m[i][k] * matrix2.m[k][j];
			}
		}
	}

	return result;
}

// 1. X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radian) {
	Matrix4x4 result = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, cosf(radian), sinf(radian), 0.0f, 0.0f, -sinf(radian), cosf(radian), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
	return result;
}
// 2. Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float radian) {
	Matrix4x4 result = {cosf(radian), 0.0f, -sinf(radian), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, sinf(radian), 0.0f, cosf(radian), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
	return result;
}
// 3. Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float radian) {
	Matrix4x4 result = {cosf(radian), sinf(radian), 0.0f, 0.0f, -sinf(radian), cosf(radian), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
	return result;
}

//
Matrix4x4 MakeScaleMatrix(Vector3 scale) {
	Matrix4x4 result;
	return result = {scale.x, 0.0f, 0.0f, 0.0f, 0.0f, scale.y, 0.0f, 0.0f, 0.0f, 0.0f, scale.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
}

Matrix4x4 MakeTranslateMatrix(Vector3 translate) {
	Matrix4x4 result;
	return result = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, translate.x, translate.y, translate.z, 1};
}

Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate) {
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
	Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

	Matrix4x4 rotateXYZMatrix = Multiply(rotateXMatrix, Multiply(rotateYMatrix, rotateZMatrix));
	Matrix4x4 worldMatrix = Multiply(scaleMatrix, Multiply(rotateXYZMatrix, translateMatrix));
	// worldMatrix = Multiply(worldMatrix, translateMatrix);

	return worldMatrix;
}

Vector3 TransformCoordinate(const Vector3& vector, const Matrix4x4& m) {
	Vector3 result;
	float w;
	result.x = vector.x * m.m[0][0] + vector.y * m.m[1][0] + vector.z * m.m[2][0] + m.m[3][0];
	result.y = vector.x * m.m[0][1] + vector.y * m.m[1][1] + vector.z * m.m[2][1] + m.m[3][1];
	result.z = vector.x * m.m[0][2] + vector.y * m.m[1][2] + vector.z * m.m[2][2] + m.m[3][2];
	w = vector.x * m.m[0][3] + vector.y * m.m[1][3] + vector.z * m.m[2][3] + m.m[3][3];
//#ifdef _DEBUG
//	assert(w != 0.0f);
//#endif
	result.x /= w;
	result.y /= w;
	result.z /= w;
	return result;
}
Vector2 TransformCoordinate(const Vector2& vector, const Matrix4x4& m) {
	Vector2 result{};
	// Vector2 は Z = 0 として座標変換する
	result.x = vector.x * m.m[0][0] + vector.y * m.m[1][0] + m.m[3][0];
	result.y = vector.x * m.m[0][1] + vector.y * m.m[1][1] + m.m[3][1];
	// 透視投影に必要な同次座標 W を計算する
	float w = vector.x * m.m[0][3] + vector.y * m.m[1][3] + m.m[3][3];
	if (std::abs(w) < 1.0e-6f) {
		return {};
	}
	result.x /= w;
	result.y /= w;
	return result;
}
Vector3 TransformNormal(const Vector3& vector, const Matrix4x4& m) {
	Vector3 result;

	result.x = vector.x * m.m[0][0] + vector.y * m.m[1][0] + vector.z * m.m[2][0];

	result.y = vector.x * m.m[0][1] + vector.y * m.m[1][1] + vector.z * m.m[2][1];

	result.z = vector.x * m.m[0][2] + vector.y * m.m[1][2] + vector.z * m.m[2][2];

	return result;
}
#pragma endregion

float Lerp(float start, float end, float t) { return (1.0f - t) * start + t * end; };

//Vector3 Lerp(Vector3 start, Vector3 end, float t) {
//	Vector3 result;
//	result.x = Lerp(start.x, end.x, t);
//	result.y = Lerp(start.y, end.y, t);
//	result.z = Lerp(start.z, end.z, t);
//	return result;
//};

//
Vector3 Lerp(Vector3 start, Vector3 end, float t) {
	Vector3 result;
	result.x = start.x + (end.x - start.x) * t;
	result.y = start.y + (end.y - start.y) * t;
	result.z = start.z + (end.z - start.z) * t;
	return result;
};
float EaseIn(float start, float end, float t) {
	// 加速するイージング（開始がゆっくり）
	float num = 1.0f - cosf((static_cast<float>(M_PI) / 2.0f) * t);
	return Lerp(start, end, num);
};

float EaseOut(float start, float end, float t) {
	// 減速するイージング（終了がゆっくり）
	float num = sinf((static_cast<float>(M_PI) / 2.0f) * t);
	return Lerp(start, end, num);
};
 float EaseInOut(float start, float end, float t)
{
	float num;
	// return -(Math.cos(Math.PI * x) - 1) / 2;
	num = -(cosf(static_cast<float>(M_PI) * t) - 1.0f) / 2;
	return Lerp(start, end, num);
};