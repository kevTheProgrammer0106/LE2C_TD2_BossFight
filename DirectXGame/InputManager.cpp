#define NOMINMAX
#include "InputManager.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>

using namespace KamataEngine;

namespace {
constexpr SHORT kDead = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

bool Btn(const XINPUT_STATE& s, WORD b) { return (s.Gamepad.wButtons & b) != 0; }

// 左スティックまたは十字キーによる左右入力。
bool PadLeft(const XINPUT_STATE& s) { return s.Gamepad.sThumbLX < -kDead || Btn(s, XINPUT_GAMEPAD_DPAD_LEFT); }
bool PadRight(const XINPUT_STATE& s) { return s.Gamepad.sThumbLX > kDead || Btn(s, XINPUT_GAMEPAD_DPAD_RIGHT); }

// メニュー操作：左右どちらかのスティック、または十字キー。
bool MenuUp(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY > kDead || s.Gamepad.sThumbRY > kDead || Btn(s, XINPUT_GAMEPAD_DPAD_UP); }
bool MenuDown(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY < -kDead || s.Gamepad.sThumbRY < -kDead || Btn(s, XINPUT_GAMEPAD_DPAD_DOWN); }
bool MenuLeft(const XINPUT_STATE& s) { return PadLeft(s) || s.Gamepad.sThumbRX < -kDead; }
bool MenuRight(const XINPUT_STATE& s) { return PadRight(s) || s.Gamepad.sThumbRX > kDead; }

// ゲームプレイ中の下入力。右スティックはエイム用なので、左スティックと十字キーのみを使う。
bool PadDown(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY < -kDead || Btn(s, XINPUT_GAMEPAD_DPAD_DOWN); }
bool PadUp(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY > kDead || Btn(s, XINPUT_GAMEPAD_DPAD_UP); }

// スティックのデッドゾーンを除去して-1.0～1.0に変換する。
float NormalizeStickAxis(SHORT value, SHORT deadZone) {
	const float magnitude = std::abs(static_cast<float>(value));
	const float dead = static_cast<float>(deadZone);

	if (magnitude <= dead) {
		return 0.0f;
	}

	const float normalized = std::clamp((magnitude - dead) / (32767.0f - dead), 0.0f, 1.0f);

	return std::copysign(normalized, static_cast<float>(value));
}
} // 無名名前空間

InputManager::~InputManager() {
	ReleaseMouseCapture();
	SetSystemCursorVisible(true);
}

InputManager* InputManager::GetInstance() {
	static InputManager instance;
	return &instance;
}

// ShowCursorの表示カウンターを調整し、指定した表示状態に合わせる。
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

// カーソルをゲーム画面中央に固定し、中央からの相対移動量を取得する。
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

	// 別プロセスのウィンドウが前面にある場合は、マウスのキャプチャを解除する。
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

	// キャプチャ開始時にカーソルを中央へ移動し、最初のフレームの移動量を0にする。
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

	// ウィンドウの移動やリサイズ後も、クライアント領域内にカーソルを制限する。
	ClipCursor(&screenRect);

	POINT currentCursor{};
	if (GetCursorPos(&currentCursor)) {
		// 前フレームで中央へ戻しているため、現在位置と中央の差がマウス移動量になる。
		mouseDelta_ = {static_cast<float>(currentCursor.x - centerScreen.x), static_cast<float>(currentCursor.y - centerScreen.y)};
	}

	// 入力を取得してからカーソルを中央へ戻す。
	SetCursorPos(centerScreen.x, centerScreen.y);

	mouseMovement_ = {static_cast<float>(centerClient.x), static_cast<float>(centerClient.y)};
	mouseX_ = centerClient.x;
	mouseY_ = centerClient.y;
}

// マウスのキャプチャと画面端の制限を解除する。
void InputManager::ReleaseMouseCapture() {
	if (mouseCaptured_) {
		ClipCursor(nullptr);
	}

	mouseCaptured_ = false;
	capturedWindow_ = nullptr;
	mouseDelta_ = {};
}

void InputManager::SetCursorLockAllowed(bool allowed) {
	// モードの状態が変化した場合のみ切り替え処理を行う。
	if (cursorModeInitialized_ && cursorLockAllowed_ == allowed) {
		return;
	}

	cursorModeInitialized_ = true;
	cursorLockAllowed_ = allowed;
	lockCursor_ = allowed;
	cursorToggleCooldownFrames_ = 0;

	// UIではOSカーソルを隠して自由移動にし、ゲームプレイでは初期状態をロックにする。
	ReleaseMouseCapture();
	SetSystemCursorVisible(false);
}

void InputManager::Update() {
	Input* input = Input::GetInstance();

	// --- コントローラー状態の取得（押した瞬間の判定をエンジンに依存させないよう前フレームも保持） ---
	previousControllerState_ = controllerState_;
	XINPUT_STATE polled{};
	if (input->GetJoystickState(0, polled)) {
		controllerState_ = polled;
	} else {
		controllerState_ = XINPUT_STATE{}; // 未接続の場合はすべての入力が離された状態にする。
	}
	const XINPUT_STATE& cur = controllerState_;
	const XINPUT_STATE& prev = previousControllerState_;

	auto padEdge = [&](WORD b) { return Btn(cur, b) && !Btn(prev, b); };
	const bool rtNow = cur.Gamepad.bRightTrigger > kTriggerThreshold_;
	const bool rtPrev = prev.Gamepad.bRightTrigger > kTriggerThreshold_;
	const bool ltNow = cur.Gamepad.bLeftTrigger > kTriggerThreshold_;
	const bool ltPrev = prev.Gamepad.bLeftTrigger > kTriggerThreshold_;

	// --- 移動 ---
	moveLeftHeld_ = input->PushKey(DIK_A) || input->PushKey(DIK_LEFT) || PadLeft(cur);
	moveRightHeld_ = input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT) || PadRight(cur);
	moveLeftTriggered_ = input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT) || (PadLeft(cur) && !PadLeft(prev));
	moveRightTriggered_ = input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT) || (PadRight(cur) && !PadRight(prev));

	// --- ジャンプ ---
	jumpHeld_ = input->PushKey(DIK_SPACE) || Btn(cur, XINPUT_GAMEPAD_A);
	jumpTriggered_ = input->TriggerKey(DIK_SPACE) || padEdge(XINPUT_GAMEPAD_A);

	// --- アクション ---
	weaponSwitchTriggered_ = input->TriggerKey(DIK_Q) || padEdge(XINPUT_GAMEPAD_Y);
	pauseTriggered_ = input->TriggerKey(DIK_ESCAPE) || padEdge(XINPUT_GAMEPAD_START);
	interactTriggered_ = input->TriggerKey(DIK_F) || padEdge(XINPUT_GAMEPAD_X);
	attackUpHeld_ = input->PushKey(DIK_W) || PadUp(cur);
	attackDownHeld_ = input->PushKey(DIK_S) || PadDown(cur);
	dashTriggered_ = input->TriggerKey(DIK_LSHIFT) || input->TriggerKey(DIK_RSHIFT) || padEdge(XINPUT_GAMEPAD_B);

	// --- マウス ---
	mouseMovement_ = input->GetMousePosition();
	mouseX_ = static_cast<int>(mouseMovement_.x);
	mouseY_ = static_cast<int>(mouseMovement_.y);

	primaryMouseTriggered_ = input->IsTriggerMouse(0);
	primaryMouseHeld_ = input->IsPressMouse(0);
	secondaryMouseHeld_ = input->IsPressMouse(1);
	secondaryMouseTriggered_ = input->IsTriggerMouse(1);

	// --- マウスカーソルのモード ---
	if (cursorLockAllowed_) {
		// クールダウンを毎フレーム減らし、Altによる連続切り替えを防ぐ。
		if (cursorToggleCooldownFrames_ > 0) {
			--cursorToggleCooldownFrames_;
		}

		// 押した瞬間だけ判定し、長押し中に毎フレームモードが反転しないようにする。
		const bool altTriggered = input->TriggerKey(DIK_LALT) || input->TriggerKey(DIK_RALT);
		if (altTriggered && cursorToggleCooldownFrames_ == 0) {
			lockCursor_ = !lockCursor_;
			cursorToggleCooldownFrames_ = kCursorToggleCooldownFrames_;
		}

		UpdateMouseCapture(lockCursor_);

		// ロックできていない場合（非アクティブ時など）はカーソルを表示する。
		SetSystemCursorVisible(!lockCursor_ || !mouseCaptured_);
	} else {
		// UI中はAltによる切り替えを禁止する。OSカーソルを隠し、UIMenu独自のカーソルを表示する。
		lockCursor_ = false;
		cursorToggleCooldownFrames_ = 0;
		UpdateMouseCapture(false);
		SetSystemCursorVisible(false);
	}

	// --- フリーカーソル中のカメラ旋回 ---
	if (IsFreeCursorMode()) {
		if (IsFreeOrbiting()) {
			if (freeOrbitMouseInitialized_) {
				// 前フレームのマウス座標との差分を、カメラ旋回用の移動量として扱う。
				mouseDelta_ = {mouseMovement_.x - previousFreeOrbitMousePosition_.x, mouseMovement_.y - previousFreeOrbitMousePosition_.y};
			} else {
				// 旋回を開始した最初のフレームは移動量を0にして、視点が急に飛ぶのを防ぐ。
				mouseDelta_ = {};
				freeOrbitMouseInitialized_ = true;
			}
			previousFreeOrbitMousePosition_ = mouseMovement_;
		} else {
			// 右クリックを離したら差分計測をリセットする。
			mouseDelta_ = {};
			freeOrbitMouseInitialized_ = false;
		}
	} else {
		// ロック中はUpdateMouseCapture()が取得した相対移動量をそのまま使う。
		freeOrbitMouseInitialized_ = false;
	}

	// --- TPS用の移動軸 ---
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

	// キーボード、十字キー、左スティックの優先順で入力を取得する。
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

	// 斜め移動の入力ベクトルを正規化する。
	const float moveLength = std::sqrt(x * x + y * y);
	if (moveLength > 1.0f) {
		x /= moveLength;
		y /= moveLength;
	}
	moveAxis_ = {x, y};

	// --- マウス入力とパッド入力の統合 ---
	primaryTriggered_ = primaryMouseTriggered_ || (rtNow && !rtPrev);
	primaryHeld_ = primaryMouseHeld_ || rtNow;
	const bool freeOrbiting = IsFreeOrbiting();
	blockHeld_ = (!freeOrbiting && secondaryMouseHeld_) || ltNow || Btn(cur, XINPUT_GAMEPAD_LEFT_SHOULDER);
	secondaryTriggered_ = (!freeOrbiting && secondaryMouseTriggered_) || (ltNow && !ltPrev) || padEdge(XINPUT_GAMEPAD_LEFT_SHOULDER);

	// --- メニュー操作の押した瞬間の判定 ---
	menuUpTriggered_ = input->TriggerKey(DIK_W) || input->TriggerKey(DIK_UP) || (MenuUp(cur) && !MenuUp(prev));
	menuDownTriggered_ = input->TriggerKey(DIK_S) || input->TriggerKey(DIK_DOWN) || (MenuDown(cur) && !MenuDown(prev));
	menuLeftTriggered_ = input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT) || (MenuLeft(cur) && !MenuLeft(prev));
	menuRightTriggered_ = input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT) || (MenuRight(cur) && !MenuRight(prev));

	// --- 右スティックによるエイム ---
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
