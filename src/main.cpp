include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

// HeartMonitor starter scaffold.
// This currently logs progress milestones; it does NOT yet draw the HUD widget.
// Next step: implement a Cocos2d overlay and test against the target Android SDK.
class $modify(HeartMonitorPlayLayer, PlayLayer) {
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!m_level || !m_player1) {
            return;
        }

        const int progress = static_cast<int>(m_level->m_levelLength > 0
            ? (m_player1->getPositionX() / m_level->m_levelLength) * 100.0f
            : 0.0f);

        static int lastMilestone = -1;
        int milestone = 0;
        if (progress >= 90) milestone = 3;
        else if (progress >= 60) milestone = 2;
        else if (progress >= 30) milestone = 1;

        if (milestone != lastMilestone) {
            lastMilestone = milestone;
            int bpm = milestone == 0 ? 75 : milestone == 1 ? 100 : milestone == 2 ? 145 : 180;
            log::info("HeartMonitor simulated BPM: {} (progress {}%)", bpm, progress);
        }
    }
};
