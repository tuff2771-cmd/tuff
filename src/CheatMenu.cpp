#include "CheatMenu.hpp"
#include "Config.hpp"
#include <Geode/Geode.hpp>
#include <Geode/ui/ScrollLayer.hpp>

using namespace geode::prelude;

namespace {

// ---------------------------------------------------------------------------
// Base panel: black window, white border, white text, scrollable row list
// ---------------------------------------------------------------------------
class PanelBase : public FLAlertLayer {
protected:
	float m_panelRowH = 32.f;
	float m_panelRowW = 320.f;
	float m_panelContentH = 0.f;
	int m_panelRow = 0;
	ScrollLayer* m_panelScroll = nullptr;
	CCMenu* m_panelMenu = nullptr;
	std::vector<std::function<void()>> m_refreshers;

	void keyBackClicked() override { this->close(); }

	void close() {
		this->setKeypadEnabled(false);
		this->removeFromParentAndCleanup(true);
	}

	bool initPanel(float width, float viewH, char const* title, char const* subtitle, int rows) {
		if (!FLAlertLayer::init(150)) return false;
		m_noElasticity = true;

		m_panelRowW = width - 40.f;
		float top = subtitle ? 76.f : 50.f;
		float height = viewH + top + 14.f;
		auto win = CCDirector::get()->getWinSize();
		CCPoint o{(win.width - width) / 2.f, (win.height - height) / 2.f};

		m_mainLayer = CCLayer::create();
		this->addChild(m_mainLayer);

		auto border = CCLayerColor::create({255, 255, 255, 255}, width + 4.f, height + 4.f);
		border->setPosition({o.x - 2.f, o.y - 2.f});
		m_mainLayer->addChild(border, 0);

		auto bg = CCLayerColor::create({0, 0, 0, 255}, width, height);
		bg->setPosition(o);
		m_mainLayer->addChild(bg, 1);

		auto t = CCLabelBMFont::create(title, "bigFont.fnt");
		t->setColor({255, 255, 255});
		t->setScale(0.7f);
		t->setPosition({o.x + width / 2.f, o.y + height - 24.f});
		m_mainLayer->addChild(t, 2);

		if (subtitle) {
			auto s = CCLabelBMFont::create(subtitle, "bigFont.fnt");
			s->setColor({255, 255, 255});
			s->setScale(0.55f);
			s->setPosition({o.x + width / 2.f, o.y + height - 52.f});
			m_mainLayer->addChild(s, 2);

			auto line = CCLayerColor::create({255, 255, 255, 255}, width - 40.f, 1.f);
			line->setPosition({o.x + 20.f, o.y + height - 66.f});
			m_mainLayer->addChild(line, 2);
		}

		m_buttonMenu = CCMenu::create();
		m_buttonMenu->setPosition({0.f, 0.f});
		m_mainLayer->addChild(m_buttonMenu, 10);

		auto xLabel = CCLabelBMFont::create("X", "bigFont.fnt");
		xLabel->setColor({255, 255, 255});
		xLabel->setScale(0.7f);
		auto closeBtn = CCMenuItemExt::createSpriteExtra(xLabel, [this](CCMenuItemSpriteExtra*) {
			this->close();
		});
		closeBtn->setPosition({o.x + width - 20.f, o.y + height - 20.f});
		m_buttonMenu->addChild(closeBtn);

		m_panelScroll = ScrollLayer::create({m_panelRowW, viewH});
		m_panelScroll->setPosition({o.x + 20.f, o.y + 14.f});
		m_mainLayer->addChild(m_panelScroll, 5);

		m_panelContentH = std::max(viewH, rows * m_panelRowH);
		auto content = m_panelScroll->m_contentLayer;
		content->setContentSize({m_panelRowW, m_panelContentH});

		m_panelMenu = CCMenu::create();
		m_panelMenu->setPosition({0.f, 0.f});
		content->addChild(m_panelMenu, 1);

		this->setTouchEnabled(true);
		this->setKeypadEnabled(true);
		return true;
	}

	void finishPanel() {
		m_panelScroll->moveToTop();
		handleTouchPriority(this);
	}

	// Starts a new row: adds a dark background and returns the row's center Y
	float beginRow() {
		float y = m_panelContentH - m_panelRowH * (m_panelRow + 0.5f);
		auto bg = CCLayerColor::create({22, 22, 22, 255}, m_panelRowW - 4.f, m_panelRowH - 4.f);
		bg->setPosition({2.f, y - (m_panelRowH - 4.f) / 2.f});
		m_panelScroll->m_contentLayer->addChild(bg, 0);
		++m_panelRow;
		return y;
	}

	void rowLabel(std::string const& text, float y, float maxW) {
		auto l = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
		l->setColor({255, 255, 255});
		l->setAnchorPoint({0.f, 0.5f});
		l->setPosition({12.f, y});
		l->limitLabelWidth(maxW, 0.5f, 0.2f);
		m_panelScroll->m_contentLayer->addChild(l, 2);
	}

public:
	void refreshAll() {
		for (auto& f : m_refreshers) f();
	}

	void addHeader(std::string const& text) {
		float y = beginRow();
		auto l = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
		l->setColor({255, 255, 255});
		l->setScale(0.5f);
		l->setPosition({m_panelRowW / 2.f, y});
		m_panelScroll->m_contentLayer->addChild(l, 2);
	}

	void addToggle(std::string const& text, bool* v, std::function<void()> gear = {}) {
		float y = beginRow();
		rowLabel(text, y, m_panelRowW - (gear ? 105.f : 65.f));

		// NOTE: toggler callbacks run before the state flips, hence the "!"
		auto t = CCMenuItemExt::createTogglerWithStandardSprites(0.55f, [v](CCMenuItemToggler* tg) {
			*v = !tg->isToggled();
			cfg::save();
		});
		t->toggle(*v);
		t->setPosition({m_panelRowW - 26.f, y});
		m_panelMenu->addChild(t);

		if (gear) {
			auto spr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn02_001.png");
			spr->setScale(0.42f);
			auto g = CCMenuItemExt::createSpriteExtra(spr, [gear](CCMenuItemSpriteExtra*) { gear(); });
			g->setPosition({m_panelRowW - 64.f, y});
			m_panelMenu->addChild(g);
		}
	}

	void addChoice(std::string const& text, int* v, std::vector<std::string> opts) {
		float y = beginRow();
		rowLabel(text, y, m_panelRowW - 125.f);

		*v = std::clamp(*v, 0, (int)opts.size() - 1);
		auto spr = ButtonSprite::create(opts[*v].c_str(), 84, true, "bigFont.fnt", "GJ_button_04.png", 26.f, 0.6f);
		auto btn = CCMenuItemExt::createSpriteExtra(spr, [v, opts, spr](CCMenuItemSpriteExtra*) {
			*v = (*v + 1) % (int)opts.size();
			spr->setString(opts[*v].c_str());
			cfg::save();
		});
		btn->setPosition({m_panelRowW - 56.f, y});
		m_panelMenu->addChild(btn);
	}

	template <class T>
	void addStepper(std::string const& text, T* v, T mn, T mx, T step, int decimals) {
		float y = beginRow();
		rowLabel(text, y, m_panelRowW - 150.f);

		auto val = CCLabelBMFont::create("", "bigFont.fnt");
		val->setColor({255, 255, 255});
		val->setScale(0.45f);
		val->setPosition({m_panelRowW - 64.f, y});
		m_panelScroll->m_contentLayer->addChild(val, 2);

		auto refresh = [v, val, decimals]() {
			val->setString(fmt::format("{:.{}f}", static_cast<double>(*v), decimals).c_str());
		};
		refresh();
		m_refreshers.push_back(refresh);

		auto arrow = [&](bool right, float x) {
			auto s = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
			s->setScale(0.45f);
			s->setFlipX(right);
			auto b = CCMenuItemExt::createSpriteExtra(s, [=](CCMenuItemSpriteExtra*) {
				double nv = static_cast<double>(*v) + (right ? 1.0 : -1.0) * static_cast<double>(step);
				nv = std::clamp(nv, static_cast<double>(mn), static_cast<double>(mx));
				nv = std::round(nv / static_cast<double>(step)) * static_cast<double>(step);
				*v = static_cast<T>(nv);
				cfg::save();
				refresh();
			});
			b->setPosition({x, y});
			m_panelMenu->addChild(b);
		};
		arrow(false, m_panelRowW - 112.f);
		arrow(true, m_panelRowW - 18.f);
	}

	void addButton(std::string const& text, std::string const& btnText, std::function<void()> cb) {
		float y = beginRow();
		rowLabel(text, y, m_panelRowW - 100.f);
		auto spr = ButtonSprite::create(btnText.c_str(), 60, true, "bigFont.fnt", "GJ_button_04.png", 26.f, 0.6f);
		auto btn = CCMenuItemExt::createSpriteExtra(spr, [cb](CCMenuItemSpriteExtra*) { cb(); });
		btn->setPosition({m_panelRowW - 42.f, y});
		m_panelMenu->addChild(btn);
	}
};

// ---------------------------------------------------------------------------
// Options window opened by the gear buttons
// ---------------------------------------------------------------------------
class SubMenu : public PanelBase {
public:
	static SubMenu* create(char const* title, int rows, std::function<void(SubMenu*)> build) {
		auto ret = new SubMenu();
		if (ret->setup(title, rows, std::move(build))) {
			ret->autorelease();
			return ret;
		}
		delete ret;
		return nullptr;
	}

	bool setup(char const* title, int rows, std::function<void(SubMenu*)> build) {
		float viewH = std::min(rows * m_panelRowH, 200.f);
		if (!initPanel(340.f, viewH, title, nullptr, rows)) return false;
		build(this);
		finishPanel();
		return true;
	}
};

void openSub(char const* title, int rows, std::function<void(SubMenu*)> build) {
	if (auto m = SubMenu::create(title, rows, std::move(build))) m->show();
}

std::vector<std::string> const kPlayers = {"Player 1", "Player 2", "Both"};

// ---------------------------------------------------------------------------
// Main window
// ---------------------------------------------------------------------------
class CheatMenu : public PanelBase {
public:
	static CheatMenu* create() {
		auto ret = new CheatMenu();
		if (ret->setup()) {
			ret->autorelease();
			return ret;
		}
		delete ret;
		return nullptr;
	}

	bool setup() {
		constexpr int kRows = 15;
		if (!initPanel(360.f, 190.f, "Cool Utils", "Gameplay", kRows)) return false;
		auto& c = cfg::get();

		addToggle("Click Green Dash Orbs", &c.greenDash, [] {
			openSub("Green Dash Orbs", 1, [](SubMenu* m) {
				m->addChoice("Players", &cfg::get().greenDashPlayers, kPlayers);
			});
		});

		addToggle("No Orb Effects", &c.noOrbEffects);
		addToggle("Click Black Orbs", &c.blackOrb);
		addToggle("Easy Click Black Orb UFO", &c.blackOrbUfoEasy);
		addToggle("Click Jump Pads", &c.jumpPads);
		addToggle("Click Gravity Pads", &c.gravityPads);

		addToggle("Maintain Gravity", &c.maintainGravity, [] {
			openSub("Maintain Gravity", 1, [](SubMenu* m) {
				m->addChoice("Players", &cfg::get().maintainPlayers, kPlayers);
			});
		});

		addToggle("Auto Straight Fly", &c.straightFly, [] {
			openSub("Auto Straight Fly", 3, [](SubMenu* m) {
				auto& c = cfg::get();
				m->addChoice("Players", &c.flyPlayers, kPlayers);
				m->addStepper<float>("Player 1 Threshold", &c.flyThr[0], 0.f, 5.f, 0.05f, 2);
				m->addStepper<float>("Player 2 Threshold", &c.flyThr[1], 0.f, 5.f, 0.05f, 2);
			});
		});

		addToggle("Auto Straight UFO", &c.straightUfo, [] {
			openSub("Auto Straight UFO", 7, [](SubMenu* m) {
				auto& c = cfg::get();
				m->addChoice("Players", &c.ufoPlayers, kPlayers);
				for (int i = 0; i < 2; ++i) {
					std::string p = i == 0 ? "P1" : "P2";
					m->addStepper<float>(p + " Threshold", &c.ufoThr[i], 0.f, 60.f, 1.f, 0);
					m->addStepper<float>(p + " Y Position Target", &c.ufoY[i], -300.f, 3000.f, 5.f, 0);
					m->addButton(p + " Y = current Y", "Set", [i, m] {
						if (auto pl = PlayLayer::get()) {
							auto player = i == 0 ? pl->m_player1 : pl->m_player2;
							if (player) {
								cfg::get().ufoY[i] = player->getPositionY();
								cfg::save();
								m->refreshAll();
							}
						}
					});
				}
			});
		});

		for (int i = 0; i < 2; ++i) {
			addToggle(i == 0 ? "AutoClicker Player 1" : "AutoClicker Player 2", &c.autoClick[i], [i] {
				openSub(i == 0 ? "AutoClicker Player 1" : "AutoClicker Player 2", 3, [i](SubMenu* m) {
					auto& c = cfg::get();
					m->addStepper<int>("Click every (frames)", &c.acEvery[i], 1, 300, 1, 0);
					m->addStepper<int>("Hold for (frames)", &c.acHold[i], 1, 60, 1, 0);
					m->addToggle("Swift", &c.acSwift[i]);
				});
			});
		}

		addToggle("Noclip", &c.noclip, [] {
			openSub("Noclip", 1, [](SubMenu* m) {
				m->addChoice("Players", &cfg::get().noclipPlayers, kPlayers);
			});
		});

		addToggle("Auto Swift", &c.autoSwift);

		addToggle("2P Control Link", &c.link, [] {
			openSub("2P Control Link", 2, [](SubMenu* m) {
				auto& c = cfg::get();
				m->addChoice("Mode", &c.linkMode, {"Copy", "Invert"});
				m->addChoice("Driving side", &c.linkDriver, {"Player 1", "Player 2"});
			});
		});

		addToggle("Moving Wave Gap Spam", &c.wave, [] {
			openSub("Moving Wave Gap Spam", 12, [](SubMenu* m) {
				auto& c = cfg::get();
				for (int i = 0; i < 2; ++i) {
					m->addHeader(i == 0 ? "Player 1" : "Player 2");
					m->addToggle("Enabled", &c.waveOn[i]);
					m->addStepper<float>("Threshold", &c.waveThr[i], 0.f, 30.f, 0.5f, 1);
					m->addStepper<float>("Recenter (fallback Y)", &c.waveCenter[i], -300.f, 3000.f, 5.f, 0);
					m->addButton("Recenter = current Y", "Set", [i, m] {
						if (auto pl = PlayLayer::get()) {
							auto player = i == 0 ? pl->m_player1 : pl->m_player2;
							if (player) {
								cfg::get().waveCenter[i] = player->getPositionY();
								cfg::save();
								m->refreshAll();
							}
						}
					});
					m->addToggle("Blue Orb", &c.waveBlue[i]);
				}
			});
		});

		finishPanel();
		return true;
	}
};

} // namespace

namespace cheatmenu {
	void open() {
		if (auto m = CheatMenu::create()) m->show();
	}
}
