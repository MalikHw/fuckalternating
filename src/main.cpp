#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <eclipse.eclipse-menu/include/components.hpp>
#include <eclipse.eclipse-menu/include/config.hpp>
#include <eclipse.eclipse-menu/include/labels.hpp>
#include <eclipse.eclipse-menu/include/modules.hpp>

using namespace geode::prelude;

$on_mod(Loaded) {
    auto tab = eclipse::MenuTab::find("Force Alternate");
    tab.addToggle("forcealt.enabled", "Force Alternating", [](bool v) {
        Mod::get()->setSettingValue("mod-enabled", v);
        eclipse::config::set("forcealt.enabled", v);
        auto* pl = PlayLayer::get();
        if (pl) {
            eclipse::label::setVariable<std::string>("turn", v ? "P1" : "P(none)");
        }
    }).setDescription("Forces players to strictly alternate inputs.");
    bool cur = Mod::get()->getSettingValue<bool>("mod-enabled");
    eclipse::config::set("forcealt.enabled", cur);
    geode::listenForSettingChanges<bool>("mod-enabled", [](bool v) {
        eclipse::config::set("forcealt.enabled", v);
    });
}

class $modify(TurnPlayLayer, PlayLayer) {
    struct Fields {
        int last = 0;
        bool held = false;
    };
    bool init(GJGameLevel* lvl, bool replay, bool nobj) {
        if (!PlayLayer::init(lvl, replay, nobj)) return false;
        bool enabled = Mod::get()->getSettingValue<bool>("mod-enabled");
        eclipse::label::setVariable<std::string>("turn", enabled ? "P1" : "P(none)");
        return true;
    }
    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->last = 0;
        m_fields->held = false;
        bool enabled = Mod::get()->getSettingValue<bool>("mod-enabled");
        eclipse::label::setVariable<std::string>("turn", enabled ? "P1" : "P(none)");
    }
};
class $modify(TurnInput, GJBaseGameLayer) {
    void handleButton(bool down, int btn, bool p1) {
        auto pl = typeinfo_cast<PlayLayer*>(this);
        if (!pl) { GJBaseGameLayer::handleButton(down, btn, p1); return; }
        if (!Mod::get()->getSettingValue<bool>("mod-enabled") || (m_level->isPlatformer() && btn != (int)PlayerButton::Jump)) {
            GJBaseGameLayer::handleButton(down, btn, p1);
            return;
        }
        auto* f = static_cast<TurnPlayLayer*>(pl)->m_fields.self();
        if (!down) {
            f->held = false;
            GJBaseGameLayer::handleButton(down, btn, p1);
            return;
        }
        int who = p1 ? 1 : 2;
        bool bad = f->held || who == f->last;
        if (bad) {
            eclipse::label::setVariable<std::string>("turn", p1 ? "P2 !!" : "P1 !!");
            if (Mod::get()->getSettingValue<bool>("hard-mode") && m_player1)
                this->destroyPlayer(m_player1, nullptr);
            return;
        }
        f->held = true;
        f->last = who;
        eclipse::label::setVariable<std::string>("turn", p1 ? "P2" : "P1");
        GJBaseGameLayer::handleButton(down, btn, p1);
    }
};