#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/RingObject.hpp>
#include <unordered_set>
#include <cfloat>
#include "Config.hpp"
#include "CheatMenu.hpp"

using namespace geode::prelude;

namespace {
	// If vertical speed turns out to be absolute (not flipped with gravity), set this to false.
	constexpr bool kVelocityIsRelative = true;

	// Object IDs
	constexpr int ID_GREEN_DASH_ORB = 1704;
	constexpr int ID_BLACK_ORB = 1330;
	constexpr int ID_BLUE_ORB = 84;
	constexpr int ID_YELLOW_PAD = 35;
	constexpr int ID_PINK_PAD = 140;
	constexpr int ID_RED_PAD = 1332;
	constexpr int ID_BLUE_PAD = 67;

	struct RuntimeState {
		bool reinject = false;           // true while the mod itself is sending input
		bool held[2] = {false, false};   // what the mod is currently holding
		bool gravLocked[2] = {false, false};
		bool gravTarget[2] = {false, false};
		int acCounter[2] = {0, 0};
		int acHoldLeft[2] = {0, 0};
		bool linkInit = false;
		std::unordered_set<GameObject*> padsInside[2];
		std::unordered_set<GameObject*> ringSeen[2];
		void reset() { *this = RuntimeState(); }
	};
	RuntimeState g;

	struct NearObj {
		GameObject* obj;
		CCRect rect;
	};
	std::vector<NearObj> s_near;

	bool pick(int sel, int idx) { return sel == 2 || sel == idx; }

	void sendButton(GJBaseGameLayer* l, bool isP1, bool down) {
		g.reinject = true;
		l->handleButton(down, 1, isP1);
		g.reinject = false;
	}

	double relVel(PlayerObject* p) {
		double v = p->m_yVelocity;
		return kVelocityIsRelative ? v : (p->m_isUpsideDown ? -v : v);
	}

	// Counts orbs of this id that the player just touched (not clicked yet)
	int countRings(PlayerObject* p, int id, int idx) {
		int n = 0;
		if (!p->m_touchingRings) return 0;
		for (auto* obj : CCArrayExt<GameObject*>(p->m_touchingRings)) {
			if (!obj || obj->m_objectID != id) continue;
			if (g.ringSeen[idx].insert(obj).second) ++n;
		}
		return n;
	}

	// Forget orbs the player is no longer touching
	void pruneRings(PlayerObject* p, int idx) {
		std::unordered_set<GameObject*> now;
		if (p->m_touchingRings) {
			for (auto* obj : CCArrayExt<GameObject*>(p->m_touchingRings)) {
				if (obj) now.insert(obj);
			}
		}
		for (auto it = g.ringSeen[idx].begin(); it != g.ringSeen[idx].end();) {
			if (!now.count(*it)) it = g.ringSeen[idx].erase(it);
			else ++it;
		}
	}

	bool isPad(int id) {
		return id == ID_YELLOW_PAD || id == ID_PINK_PAD || id == ID_RED_PAD || id == ID_BLUE_PAD;
	}

	// Collects solids + pads near the players (used by pad clicking and wave gap spam)
	void scanNearby(GJBaseGameLayer* l, bool dual) {
		s_near.clear();
		if (!l->m_objects || !l->m_player1) return;
		float x1 = l->m_player1->getPositionX();
		float xmin = x1, xmax = x1;
		if (dual && l->m_player2) {
			float x2 = l->m_player2->getPositionX();
			xmin = std::min(x1, x2);
			xmax = std::max(x1, x2);
		}
		xmin -= 60.f;
		xmax += 200.f;

		for (auto* obj : CCArrayExt<GameObject*>(l->m_objects)) {
			if (!obj) continue;
			float x = obj->getPositionX();
			if (x
