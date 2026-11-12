#define NOMINMAX
#include "InputManager.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>

using namespace KamataEngine;

namespace {
constexpr SHORT kDead = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

bool Btn(const XINPUT_STATE& s, WORD b) { return (s.Gamepad.wButtons & b) != 0; }

// Left stick or D-Pad
bool PadLeft(const XINPUT_STATE& s) { return s.Gamepad.sThumbLX < -kDead || Btn(s, XINPUT_GAMEPAD_DPAD_LEFT); }
bool PadRight(const XINPUT_STATE& s) { return s.Gamepad.sThumbLX > kDead || Btn(s, XINPUT_GAMEPAD_DPAD_RIGHT); }

// Menu navigation: either stick or D-Pad
bool MenuUp(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY > kDead || s.Gamepad.sThumbRY > kDead || Btn(s, XINPUT_GAMEPAD_DPAD_UP); }
bool MenuDown(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY < -kDead || s.Gamepad.sThumbRY < -kDead || Btn(s, XINPUT_GAMEPAD_DPAD_DOWN); }
bool MenuLeft(const XINPUT_STATE& s) { return PadLeft(s) || s.Gamepad.sThumbRX < -kDead; }
bool MenuRight(const XINPUT_STATE& s) { return PadRight(s) || s.Gamepad.sThumbRX > kDead; }

// Gameplay down (left stick / D-Pad only, so the right/aim stick never triggers drop-through)
bool PadDown(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY < -kDead || Btn(s, XINPUT_GAMEPAD_DPAD_DOWN); }
bool PadUp(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY > kDead || Btn(s, XINPUT_GAMEPAD_DPAD_UP); }

// スティックのデッドゾーンを除去して-1.0～1.0に変換
float NormalizeStickAxis(SHORT value, SHORT deadZone) {
	const float magnitude = std::abs(static_cast<float>(value));
	const float dead = static_cast<float>(deadZone);

	if (magnitude <= dead) {
		return 0.0f;
	}

	const float normalized = std::clamp((magnitude - dead) / (32767.0f - dead), 0.0f, 1.0f);

	return std::copysign(normalized, static_cast<float>(value));
}
} // namespace

InputManager::~InputManager() {
	ReleaseMouseCapture();
	SetSystemCursorVisible(true);
}

InputManager* InputManager::GetInstance() {
	static InputManager instance;
	return &instance;
}

// ShowCursorの表示カウンターを管理し、指定した表示状態に合わせる
void InputManager::SetSystemCursorVisible(bool visible) {
	if (systemCursorVisible_ == visible) {
		return;
	}

	int displayCount = 0;
	if (visible) {
		do {
			displayCount = ShowCursor(TRUE);
		} while (displayCount < 0);
	} else {
		do {
			displayCount = ShowCursor(FALSE);
		} while (displayCount >= 0);
	}

	systemCursorVisible_ = visible;
}

// カーソルをゲーム画面中央に固定して相対移動量を取得する
void InputManager::UpdateMouseCapture(bool lockCursor) {
	mouseDelta_ = {};

	if (!lockCursor) {
		ReleaseMouseCapture();
		return;
	}

	HWND window = GetForegroundWindow();
	if (!window || !IsWindow(window)) {
		ReleaseMouseCapture();
		return;
	}

	// 別のプロセスが前面にある場合はキャプチャを解除する
	DWORD processId = 0;
	GetWindowThreadProcessId(window, &processId);
	if (processId != GetCurrentProcessId()) {
		ReleaseMouseCapture();
		return;
	}

	RECT clientRect{};
	if (!GetClientRect(window, &clientRect) || clientRect.right <= clientRect.left || clientRect.bottom <= clientRect.top) {
		ReleaseMouseCapture();
		return;
	}

	POINT centerClient = {(clientRect.left + clientRect.right) / 2, (clientRect.top + clientRect.bottom) / 2};
	POINT centerScreen = centerClient;
	if (!ClientToScreen(window, &centerScreen)) {
		ReleaseMouseCapture();
		return;
	}

	POINT topLeft = {clientRect.left, clientRect.top};
	POINT bottomRight = {clientRect.right, clientRect.bottom};
	if (!ClientToScreen(window, &topLeft) || !ClientToScreen(window, &bottomRight)) {
		ReleaseMouseCapture();
		return;
	}

	const RECT screenRect = {topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};

	// キャプチャ開始時はカーソルを中央へ移動し、最初のフレームのdeltaを0にする
	if (!mouseCaptured_ || capturedWindow_ != reinterpret_cast<void*>(window)) {
		ReleaseMouseCapture();
		mouseCaptured_ = true;
		capturedWindow_ = reinterpret_cast<void*>(window);
		ClipCursor(&screenRect);
		SetCursorPos(centerScreen.x, centerScreen.y);

		mouseMovement_ = {static_cast<float>(centerClient.x), static_cast<float>(centerClient.y)};
		mouseX_ = centerClient.x;
		mouseY_ = centerClient.y;
		return;
	}

	// ウィンドウの移動・リサイズ後もクライアント領域にカーソルを制限する
	ClipCursor(&screenRect);

	POINT currentCursor{};
	if (GetCursorPos(&currentCursor)) {
		// 前フレームに中央へ戻しているため、中央との差がマウス移動量になる
		mouseDelta_ = {static_cast<float>(currentCursor.x - centerScreen.x), static_cast<float>(currentCursor.y - centerScreen.y)};
	}

	// 入力を取得した後に中央へ戻す
	SetCursorPos(centerScreen.x, centerScreen.y);

	mouseMovement_ = {static_cast<float>(centerClient.x), static_cast<float>(centerClient.y)};
	mouseX_ = centerClient.x;
	mouseY_ = centerClient.y;
}

// マウスキャプチャと画面端の制限を解除する
void InputManager::ReleaseMouseCapture() {
	if (mouseCaptured_) {
		ClipCursor(nullptr);
	}

	mouseCaptured_ = false;
	capturedWindow_ = nullptr;
	mouseDelta_ = {};
}

void InputManager::SetCursorLockAllowed(bool allowed) {
	// 状態が変わったときだけモードを切り替える
	if (cursorModeInitialized_ && cursorLockAllowed_ == allowed) {
		return;
	}

	cursorModeInitialized_ = true;
	cursorLockAllowed_ = allowed;
	lockCursor_ = allowed;
	cursorToggleCooldownFrames_ = 0;

	// UIではOSカーソルを隠して自由移動にし、Gameplayでは初期ロックする
	ReleaseMouseCapture();
	SetSystemCursorVisible(false);
}

void InputManager::Update() {
	Input* input = Input::GetInstance();

	// --- Controller snapshot (we keep our own previous frame so edges don't depend on the engine) ---
	previousControllerState_ = controllerState_;
	XINPUT_STATE polled{};
	if (input->GetJoystickState(0, polled)) {
		controllerState_ = polled;
	} else {
		controllerState_ = XINPUT_STATE{}; // disconnected: everything released
	}
	const XINPUT_STATE& cur = controllerState_;
	const XINPUT_STATE& prev = previousControllerState_;

	auto padEdge = [&](WORD b) { return Btn(cur, b) && !Btn(prev, b); };
	const bool rtNow = cur.Gamepad.bRightTrigger > kTriggerThreshold_;
	const bool rtPrev = prev.Gamepad.bRightTrigger > kTriggerThreshold_;
	const bool ltNow = cur.Gamepad.bLeftTrigger > kTriggerThreshold_;
	const bool ltPrev = prev.Gamepad.bLeftTrigger > kTriggerThreshold_;

	// --- Movement ---
	moveLeftHeld_ = input->PushKey(DIK_A) || input->PushKey(DIK_LEFT) || PadLeft(cur);
	moveRightHeld_ = input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT) || PadRight(cur);
	moveLeftTriggered_ = input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT) || (PadLeft(cur) && !PadLeft(prev));
	moveRightTriggered_ = input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT) || (PadRight(cur) && !PadRight(prev));

	// --- Jump ---
	jumpHeld_ = input->PushKey(DIK_SPACE) || Btn(cur, XINPUT_GAMEPAD_A);
	jumpTriggered_ = input->TriggerKey(DIK_SPACE) || padEdge(XINPUT_GAMEPAD_A);

	// --- Actions ---
	weaponSwitchTriggered_ = input->TriggerKey(DIK_Q) || padEdge(XINPUT_GAMEPAD_Y);
	pauseTriggered_ = input->TriggerKey(DIK_ESCAPE) || padEdge(XINPUT_GAMEPAD_START);
	interactTriggered_ = input->TriggerKey(DIK_F) || padEdge(XINPUT_GAMEPAD_X);
	attackUpHeld_ = input->PushKey(DIK_W) || PadUp(cur);
	attackDownHeld_ = input->PushKey(DIK_S) || PadDown(cur);

	// --- Mouse ---
	mouseMovement_ = input->GetMousePosition();
	mouseX_ = static_cast<int>(mouseMovement_.x);
	mouseY_ = static_cast<int>(mouseMovement_.y);

	primaryMouseTriggered_ = input->IsTriggerMouse(0);
	primaryMouseHeld_ = input->IsPressMouse(0);
	secondaryMouseHeld_ = input->IsPressMouse(1);
	secondaryMouseTriggered_ = input->IsTriggerMouse(1);

	// --- Mouse cursor mode ---
	if (cursorLockAllowed_) {
		// クールダウンを毎フレーム減らし、Altの連続切り替えを防ぐ
		if (cursorToggleCooldownFrames_ > 0) {
			--cursorToggleCooldownFrames_;
		}

		// 押した瞬間だけ判定し、長押し中に毎フレーム反転しないようにする
		const bool altTriggered = input->TriggerKey(DIK_LALT) || input->TriggerKey(DIK_RALT);
		if (altTriggered && cursorToggleCooldownFrames_ == 0) {
			lockCursor_ = !lockCursor_;
			cursorToggleCooldownFrames_ = kCursorToggleCooldownFrames_;
		}

		UpdateMouseCapture(lockCursor_);

		// ロックに失敗して非アクティブな場合はカーソルを表示する
		SetSystemCursorVisible(!lockCursor_ || !mouseCaptured_);
	} else {
		// UI中はAlt切り替えを禁止。OSカーソルは隠し、UIMenuのカーソルを表示する
		lockCursor_ = false;
		cursorToggleCooldownFrames_ = 0;
		UpdateMouseCapture(false);
		SetSystemCursorVisible(false);
	}

	// --- TPS movement axis ---
	const bool leftKey = input->PushKey(DIK_A) || input->PushKey(DIK_LEFT);
	const bool rightKey = input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT);
	const bool forwardKey = input->PushKey(DIK_W);
	const bool backwardKey = input->PushKey(DIK_S);

	const bool leftDpad = Btn(cur, XINPUT_GAMEPAD_DPAD_LEFT);
	const bool rightDpad = Btn(cur, XINPUT_GAMEPAD_DPAD_RIGHT);
	const bool upDpad = Btn(cur, XINPUT_GAMEPAD_DPAD_UP);
	const bool downDpad = Btn(cur, XINPUT_GAMEPAD_DPAD_DOWN);

	float x = 0.0f;
	float y = 0.0f;

	// キーボード、D-Pad、左スティックの順に入力を取得
	if (leftKey || rightKey) {
		x = static_cast<float>(rightKey) - static_cast<float>(leftKey);
	} else if (leftDpad || rightDpad) {
		x = static_cast<float>(rightDpad) - static_cast<float>(leftDpad);
	} else {
		x = NormalizeStickAxis(cur.Gamepad.sThumbLX, kStickDeadZone_);
	}

	if (forwardKey || backwardKey) {
		y = static_cast<float>(forwardKey) - static_cast<float>(backwardKey);
	} else if (upDpad || downDpad) {
		y = static_cast<float>(upDpad) - static_cast<float>(downDpad);
	} else {
		y = NormalizeStickAxis(cur.Gamepad.sThumbLY, kStickDeadZone_);
	}

	// 斜め移動を正規化
	const float moveLength = std::sqrt(x * x + y * y);
	if (moveLength > 1.0f) {
		x /= moveLength;
		y /= moveLength;
	}
	moveAxis_ = {x, y};

	// --- Combined mouse + pad ---
	primaryTriggered_ = primaryMouseTriggered_ || (rtNow && !rtPrev);
	primaryHeld_ = primaryMouseHeld_ || rtNow;
	blockHeld_ = secondaryMouseHeld_ || ltNow || Btn(cur, XINPUT_GAMEPAD_LEFT_SHOULDER);
	secondaryTriggered_ = secondaryMouseTriggered_ || (ltNow && !ltPrev) || padEdge(XINPUT_GAMEPAD_LEFT_SHOULDER);

	// --- Menu navigation edges ---
	menuUpTriggered_ = input->TriggerKey(DIK_W) || input->TriggerKey(DIK_UP) || (MenuUp(cur) && !MenuUp(prev));
	menuDownTriggered_ = input->TriggerKey(DIK_S) || input->TriggerKey(DIK_DOWN) || (MenuDown(cur) && !MenuDown(prev));
	menuLeftTriggered_ = input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT) || (MenuLeft(cur) && !MenuLeft(prev));
	menuRightTriggered_ = input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT) || (MenuRight(cur) && !MenuRight(prev));

	// --- Right stick aim ---
	const float ax = static_cast<float>(cur.Gamepad.sThumbRX);
	const float ay = static_cast<float>(cur.Gamepad.sThumbRY);
	const float aimLength = std::sqrt(ax * ax + ay * ay);
	const float aimDeadZone = static_cast<float>(kAimDeadZone_);

	if (aimLength > aimDeadZone) {
		const float magnitude = std::clamp((aimLength - aimDeadZone) / (32767.0f - aimDeadZone), 0.0f, 1.0f);
		aimStick_ = {(ax / aimLength) * magnitude, (ay / aimLength) * magnitude};
		aimStickActive_ = true;
	} else {
		aimStick_ = {0.0f, 0.0f};
		aimStickActive_ = false;
	}
}

float InputManager::GetMoveAxisX() const { return moveAxis_.x; }

bool InputManager::IsControllerButtonTriggered(WORD button) const { return Btn(controllerState_, button) && !Btn(previousControllerState_, button); }

bool InputManager::IsControllerButtonHeld(WORD button) const { return Btn(controllerState_, button); }
