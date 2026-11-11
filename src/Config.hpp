#pragma once
#include <Geode/Geode.hpp>

// Player selection values: 0 = Player 1, 1 = Player 2, 2 = Both
namespace cfg {
	struct Config {
		bool greenDash = false;       int greenDashPlayers = 2;
		bool noOrbEffects = false;
		bool blackOrb = false;
		bool blackOrbUfoEasy = false;
		bool jumpPads = false;
		bool gravityPads = false;
		bool maintainGravity = false; int maintainPlayers = 2;

		bool straightFly = false;     int flyPlayers = 2;
		float flyThr[2] = {0.3f, 0.3f};

		bool straightUfo = false;     int ufoPlayers = 2;
		float ufoThr[2] = {10.f, 10.f};
		float ufoY[2] = {165.f, 165.f};

		bool autoClick[2] = {false, false};
		int acEvery[2] = {4, 4};
		int acHold[2] = {1, 1};
		bool acSwift[2] = {false, false};

		bool noclip = false;          int noclipPlayers = 2;
		bool autoSwift = false;

		bool link = false;            int linkMode = 0;    // 0 = copy, 1 = invert
		int linkDriver = 0;                                // 0 = P1 drives, 1 = P2 drives

		bool wave = false;
		bool waveOn[2] = {true, true};
		float waveThr[2] = {4.f, 4.f};
		float waveCenter[2] = {165.f, 165.f};
		bool waveBlue[2] = {false, false};
	};

	Config& get();   // loads saved values the first time it is called
	void save();
}
