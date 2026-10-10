
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <cmath>
#include <string>

using namespace geode::prelude;

class $modify(HeartMonitorPlayLayer, PlayLayer) {
    struct Fields {
        CCLabelBMFont* bpmLabel = nullptr;
        CCNode* heart = nullptr;
        float elapsed = 0.f;
        float beatPhase = 0.f;
        float currentBpm = 72.f;
        float heartBaseY = 0.f;
        int lastBpm = -1;
    };

    bool init(GJGameLevel* level, bool useReplay,
              bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;

        auto size = CCDirector::sharedDirector()->getWinSize();

        auto hud = CCNode::create();
        this->addChild(hud, 9999);

        auto heart = CCNode::create();
        heart->setPosition({size.width - 116.f, size.height - 35.f});
        hud->addChild(heart);
        m_fields->heart = heart;
        m_fields->heartBaseY = size.height - 35.f;

        static constexpr const char* pixels[] = {
            "01100110",
            "11111111",
            "11111111",
            "11111111",
            "01111110",
            "00111100",
            "00011000"
        };

        for (int r = 0; r < 7; r++) {
            for (int c = 0; c < 8; c++) {
                if (pixels[r][c] != '1') continue;

                auto pixel = CCLayerColor::create(
                    ccc4(255, 45, 65, 255), 4.f, 4.f
                );
                if (!pixel) continue;

                pixel->setPosition({
                    c * 4.5f, (6 - r) * 4.5f
                });
                heart->addChild(pixel);
            }
        }

        auto label = CCLabelBMFont::create("72 BPM", "bigFont.fnt");
        if (label) {
            label->setScale(0.45f);
            label->setColor(ccc3(0, 255, 80));
            label->setAnchorPoint({0.f, 0.5f});
            label->setPosition({
                size.width - 76.f, size.height - 35.f
            });
            hud->addChild(label);
            m_fields->bpmLabel = label;
        }

        return true;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!m_fields->bpmLabel || !m_fields->heart)
            return;

        m_fields->elapsed += dt;
        m_fields->beatPhase += dt;

        int progress = std::clamp(
            static_cast<int>(getCurrentPercent()), 0, 100
        );

        // Simulated BPM: varies continuously with progress.
        float wave = std::sin(m_fields->elapsed * 2.2f) * 8.f;
        float target = 72.f + progress * 0.75f + wave;

        if (progress >= 60)
            target = 140.f + (progress - 60) * 0.45f + wave;

        if (progress >= 90)
            target = 165.f + wave;

        // Ease toward the target instead of jumping between fixed values.
        m_fields->currentBpm +=
            (target - m_fields->currentBpm) *
            std::min(dt * 2.f, 1.f);

        int bpm = static_cast<int>(m_fields->currentBpm);
        bpm = std::clamp(bpm, 60, 190);

        if (bpm != m_fields->lastBpm) {
            m_fields->lastBpm = bpm;
            auto text = std::to_string(bpm) + " BPM";
            m_fields->bpmLabel->setString(text.c_str());
        }

        // Green at low BPM, yellow in the middle, red at high BPM.
        float t = std::clamp((bpm - 70.f) / 110.f, 0.f, 1.f);
        GLubyte red = static_cast<GLubyte>(255.f * t);
        GLubyte green = static_cast<GLubyte>(
            255.f * (1.f - t)
        );
        m_fields->bpmLabel->setColor(ccc3(red, green, 20));

        // Two quick upward pulses per heartbeat, then settle back down.
        float rate = bpm / 60.f;
        float beat = std::fmod(m_fields->beatPhase * rate, 1.f);
        float jump = 0.f;

        if (beat < 0.18f)
            jump = std::sin((beat / 0.18f) * 3.14159f) * 9.f;
        else if (beat > 0.22f && beat < 0.36f)
            jump = std::sin(
                ((beat - 0.22f) / 0.14f) * 3.14159f
            ) * 5.f;

        m_fields->heart->setPosition({
            m_fields->heart->getPositionX(),
            m_fields->heartBaseY + jump
        });
    }
};
