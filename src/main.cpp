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

	int countRings(PlayerObject* p, int id, bool isP1) {
		int n = 0;
		if (!p->m_touchingRings) return 0;
		for (auto* obj : CCArrayExt<GameObject*>(p->m_touchingRings)) {
			if (!obj || obj->m_objectID != id) continue;
			bool done = isP1 ? obj->m_hasBeenActivatedP1 : obj->m_hasBeenActivatedP2;
			if (!done) ++n;
		}
		return n;
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
			if (x < xmin - 30.f || x > xmax + 30.f) continue;
			if (obj->m_objectType != GameObjectType::Solid && !isPad(obj->m_objectID)) continue;
			s_near.push_back({obj, obj->getObjectRect()});
		}
	}

	void runCheats(GJBaseGameLayer* l) {
		auto& c = cfg::get();
		bool dual = l->m_gameState.m_isDualMode;
		PlayerObject* players[2] = {l->m_player1, l->m_player2};

		if (c.jumpPads || c.gravityPads || c.wave) scanNearby(l, dual);
		else s_near.clear();

		// 2P control link: in invert mode the follower starts held
		if (c.link && c.linkMode == 1 && dual && !g.linkInit) {
			g.linkInit = true;
			bool followerIsP1 = c.linkDriver == 1;
			sendButton(l, followerIsP1, true);
		}
		if (!c.link) g.linkInit = false;

		for (int i = 0; i < 2; ++i) {
			PlayerObject* p = players[i];
			if (!p || p->m_isDead || (i == 1 && !dual)) continue;
			bool isP1 = i == 0;

			int taps = 0;
			bool ctrl = false;
			bool want = g.held[i];

			// --- Maintain gravity -------------------------------------------------
			if (c.maintainGravity && pick(c.maintainPlayers, i)) {
				if (!g.gravLocked[i]) {
					g.gravLocked[i] = true;
					g.gravTarget[i] = p->m_isUpsideDown;
				} else if (p->m_isUpsideDown != g.gravTarget[i]) {
					p->flipGravity(g.gravTarget[i], true);
				}
			} else {
				g.gravLocked[i] = false;
			}

			// --- Orbs -----------------------------------------------------------
			if (c.greenDash && pick(c.greenDashPlayers, i)) {
				taps += std::min(countRings(p, ID_GREEN_DASH_ORB, isP1), 8);
			}
			if (c.blackOrb || c.blackOrbUfoEasy) {
				int n = countRings(p, ID_BLACK_ORB, isP1);
				if (n > 0) {
					// easy mode: in UFO, don't click while already moving in the drop direction
					bool skip = c.blackOrbUfoEasy && p->m_isBird && relVel(p) < 0;
					if (!skip) taps += std::min(n, 8);
				}
			}
			if (c.wave && c.waveOn[i] && c.waveBlue[i]) {
				taps += std::min(countRings(p, ID_BLUE_ORB, isP1), 8);
			}

			// --- Pads -----------------------------------------------------------
			if (c.jumpPads || c.gravityPads) {
				auto rect = p->getObjectRect();
				std::unordered_set<GameObject*> now;
				for (auto& n : s_near) {
					int id = n.obj->m_objectID;
					bool jump = id == ID_YELLOW_PAD || id == ID_PINK_PAD || id == ID_RED_PAD;
					bool grav = id == ID_BLUE_PAD;
					if (!((jump && c.jumpPads) || (grav && c.gravityPads))) continue;
					if (!rect.intersectsRect(n.rect)) continue;
					now.insert(n.obj);
					if (!g.padsInside[i].count(n.obj)) ++taps;
				}
				g.padsInside[i] = std::move(now);
			}

			// --- Auto straight fly (ship) --------------------------------------------
			if (c.straightFly && pick(c.flyPlayers, i) && p->m_isShip) {
				ctrl = true;
				double v = relVel(p);
				double t = c.flyThr[i];
				if (v < -t) want = true;
				else if (v > t) want = false;
			}

			// --- Auto straight UFO ----------------------------------------------------
			if (c.straightUfo && pick(c.ufoPlayers, i) && p->m_isBird) {
				float py = p->getPositionY();
				float target = c.ufoY[i];
				float t = c.ufoThr[i];
				bool below = p->m_isUpsideDown ? (py > target + t) : (py < target - t);
				if (below && relVel(p) <= 0) ++taps;
			}

			// --- Moving wave gap spam ---------------------------------------------------
			if (c.wave && c.waveOn[i] && p->m_isDart) {
				float px = p->getPositionX();
				float py = p->getPositionY();
				float above = FLT_MAX, below = -FLT_MAX;
				for (auto& n : s_near) {
					if (n.obj->m_objectType != GameObjectType::Solid) continue;
					if (n.rect.getMaxX() < px - 10.f || n.rect.getMinX() > px + 60.f) continue;
					if (n.rect.getMidY() > py) above = std::min(above, n.rect.getMinY());
					else below = std::max(below, n.rect.getMaxY());
				}
				float mid = (above != FLT_MAX && below != -FLT_MAX)
					? (above + below) / 2.f
					: c.waveCenter[i];
				float err = py - mid;
				float thr = c.waveThr[i];
				bool holdGoesUp = !p->m_isUpsideDown;
				ctrl = true;
				if (err < -thr) want = holdGoesUp;        // below the band -> move up
				else if (err > thr) want = !holdGoesUp;   // above the band -> move down
				else want = !g.held[i];                   // inside the band -> spam
			}

			// --- AutoClicker ----------------------------------------------------------
			if (c.autoClick[i]) {
				if (!c.acSwift[i]) ctrl = true;
				if (++g.acCounter[i] >= std::max(1, c.acEvery[i])) {
					g.acCounter[i] = 0;
					if (c.acSwift[i]) ++taps;
					else g.acHoldLeft[i] = std::max(1, c.acHold[i]);
				}
				if (!c.acSwift[i]) {
					if (g.acHoldLeft[i] > 0) { want = true; --g.acHoldLeft[i]; }
					else want = false;
				}
			}

			// --- Auto swift -------------------------------------------------------------
			if (c.autoSwift) ++taps;

			// --- Apply input -------------------------------------------------------
			if (taps > 0) {
				for (int k = 0; k < taps; ++k) {
					sendButton(l, isP1, true);
					sendButton(l, isP1, false);
				}
				g.held[i] = false;
			}
			if (ctrl) {
				if (want != g.held[i]) {
					sendButton(l, isP1, want);
					g.held[i] = want;
				}
			} else if (g.held[i]) {
				sendButton(l, isP1, false);
				g.held[i] = false;
			}
		}
	}
}

class $modify(CheatBaseGameLayer, GJBaseGameLayer) {
	void update(float dt) {
		GJBaseGameLayer::update(dt);
		auto pl = PlayLayer::get();
		auto self = static_cast<GJBaseGameLayer*>(this);
		if (!pl || static_cast<GJBaseGameLayer*>(pl) != self) return;
		if (!m_player1) return;
		runCheats(self);
	}

	// 2P control link: mirror / invert the driver's input onto the other player
	void handleButton(bool down, int button, bool isPlayer1) {
		GJBaseGameLayer::handleButton(down, button, isPlayer1);

		auto& c = cfg::get();
		if (g.reinject || !c.link || button != 1) return;
		if (!m_gameState.m_isDualMode) return;
		bool driverIsP1 = c.linkDriver == 0;
		if (isPlayer1 != driverIsP1) return;

		bool follow = c.linkMode == 0 ? down : !down;
		g.reinject = true;
		GJBaseGameLayer::handleButton(follow, 1, !isPlayer1);
		g.reinject = false;
	}
};

class $modify(CheatPlayLayer, PlayLayer) {
	void resetLevel() {
		PlayLayer::resetLevel();
		g.reset();
	}

	void destroyPlayer(PlayerObject* player, GameObject* obj) {
		auto& c = cfg::get();
		if (c.noclip && obj != m_anticheatSpike) {
			bool isP1 = player == m_player1;
			if (pick(c.noclipPlayers, isP1 ? 0 : 1)) return;
		}
		PlayLayer::destroyPlayer(player, obj);
	}
};

class $modify(CheatRingObject, RingObject) {
	void spawnCircle() {
		if (cfg::get().noOrbEffects) return;
		RingObject::spawnCircle();
	}
};

class $modify(CheatPauseLayer, PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();

		auto win = CCDirector::get()->getWinSize();
		auto spr = ButtonSprite::create("Cheats");
		spr->setScale(0.7f);
		auto btn = CCMenuItemExt::createSpriteExtra(spr, [](CCMenuItemSpriteExtra*) {
			cheatmenu::open();
		});

		auto menu = CCMenu::create();
		menu->setID("cheats-button-menu"_spr);
		menu->setPosition({win.width - 52.f, win.height - 28.f});
		menu->addChild(btn);
		this->addChild(menu, 10);
	}
};
