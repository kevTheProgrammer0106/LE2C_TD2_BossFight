#pragma once

#include <cstdint>

/// Generic request/resolve state machine used by Player (and later enemies).
///
/// Flow (once per frame, call Update()):
///   1. a pending Request() is resolved: OnExit(old) -> state changes -> OnEnter(new, old)
///   2. OnUpdate(current) runs
///
/// A concrete behavior defines its own state ids (a plain enum is enough) and overrides the three hooks.
/// Requests are only applied at the start of the next Update(), so a state can safely ask for another one
/// from inside OnUpdate() without being torn down mid-frame.
class BaseBehavior {
public:
	using StateId = int32_t;
	static constexpr StateId kNoState = -1;

	virtual ~BaseBehavior() = default;

	// Enter the first state immediately (OnEnter runs now, not on the next Update).
	void Start(StateId initial) {
		state_ = kNoState;
		previous_ = kNoState;
		request_ = initial;
		ResolveRequest();
	}

	// Leave the current state (OnExit runs) and go idle. Update() does nothing until Start()/Request().
	void Stop() {
		if (state_ != kNoState) {
			OnExit(state_, kNoState);
			previous_ = state_;
			state_ = kNoState;
		}
		request_ = kNoState;
	}

	void Update() {
		ResolveRequest();
		if (state_ != kNoState) {
			OnUpdate(state_);
			++stateFrames_;
		}
	}

	void Request(StateId state) { request_ = state; }
	void ClearRequest() { request_ = kNoState; }

	StateId GetState() const { return state_; }
	StateId GetPreviousState() const { return previous_; }
	StateId GetRequest() const { return request_; }
	bool IsIn(StateId state) const { return state_ == state; }
	bool IsRequested(StateId state) const { return request_ == state; }
	uint32_t GetStateFrames() const { return stateFrames_; } // frames spent in the current state (0 on the frame it was entered)

protected:
	virtual void OnEnter(StateId /*state*/, StateId /*previous*/) {}
	virtual void OnUpdate(StateId /*state*/) {}
	virtual void OnExit(StateId /*state*/, StateId /*next*/) {}

private:
	void ResolveRequest() {
		if (request_ == kNoState) {
			return;
		}
		const StateId next = request_;
		request_ = kNoState;
		if (state_ != kNoState) {
			OnExit(state_, next);
		}
		previous_ = state_;
		state_ = next;
		stateFrames_ = 0;
		OnEnter(state_, previous_);
	}

	StateId state_ = kNoState;
	StateId previous_ = kNoState;
	StateId request_ = kNoState;
	uint32_t stateFrames_ = 0;
};