#include "Config.hpp"

using namespace geode::prelude;

namespace cfg {
	namespace {
		template <class T>
		void sync(std::string const& key, T& value, bool store) {
			if (store) Mod::get()->setSavedValue<T>(key, value);
			else value = Mod::get()->getSavedValue<T>(key, value);
		}

		template <class T>
		void sync2(std::string const& key, T (&arr)[2], bool store) {
			sync(key + "-p1", arr[0], store);
			sync(key + "-p2", arr[1], store);
		}

		Config& raw() {
			static Config c;
			return c;
		}

		void run(Config& c, bool store) {
			sync("greenDash", c.greenDash, store);
			sync("greenDashPlayers", c.greenDashPlayers, store);
			sync("noOrbEffects", c.noOrbEffects, store);
			sync("blackOrb", c.blackOrb, store);
			sync("blackOrbUfoEasy", c.blackOrbUfoEasy, store);
			sync("jumpPads", c.jumpPads, store);
			sync("gravityPads", c.gravityPads, store);
			sync("maintainGravity", c.maintainGravity, store);
			sync("maintainPlayers", c.maintainPlayers, store);

			sync("straightFly", c.straightFly, store);
			sync("flyPlayers", c.flyPlayers, store);
			sync2("flyThr", c.flyThr, store);

			sync("straightUfo", c.straightUfo, store);
			sync("ufoPlayers", c.ufoPlayers, store);
			sync2("ufoThr", c.ufoThr, store);
			sync2("ufoY", c.ufoY, store);

			sync2("autoClick", c.autoClick, store);
			sync2("acEvery", c.acEvery, store);
			sync2("acHold", c.acHold, store);
			sync2("acSwift", c.acSwift, store);

			sync("noclip", c.noclip, store);
			sync("noclipPlayers", c.noclipPlayers, store);
			sync("autoSwift", c.autoSwift, store);

			sync("link", c.link, store);
			sync("linkMode", c.linkMode, store);
			sync("linkDriver", c.linkDriver, store);

			sync("wave", c.wave, store);
			sync2("waveOn", c.waveOn, store);
			sync2("waveThr", c.waveThr, store);
			sync2("waveCenter", c.waveCenter, store);
			sync2("waveBlue", c.waveBlue, store);
		}
	}

	Config& get() {
		static bool loaded = false;
		auto& c = raw();
		if (!loaded) {
			loaded = true;
			run(c, false);
		}
		return c;
	}

	void save() {
		run(get(), true);
	}
}
