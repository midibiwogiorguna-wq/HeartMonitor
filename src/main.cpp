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
        int lastProgress = 0;
    };

    static void addRect(CCNode* parent, float x, float y,
                        float width, float height, ccColor4B color) {
        auto rect = CCLayerColor::create(color, width, height);
        if (!rect) return;

        rect->setAnchorPoint({0.f, 0.f});
        rect->setPosition({x, y});
        parent->addChild(rect);
    }

    static float roundedHalfWidth(float width, float height,
                                  float radius, float y) {
        const float distance = std::fabs(y - height / 2.f);
        const float straightHalfHeight = height / 2.f - radius;

        if (distance <= straightHalfHeight)
            return width / 2.f;

        const float curveDistance = distance - straightHalfHeight;
        const float inside = std::max(
            0.f, radius * radius - curveDistance * curveDistance
        );

        return width / 2.f - radius + std::sqrt(inside);
    }

    static void addRoundedPanel(CCNode* parent, float width,
                                float height, float radius,
                                float border) {
        // Draw a white rounded rectangle from thin horizontal strips.
        constexpr float rowStep = 2.f;

        for (float y = 0.f; y < height; y += rowStep) {
            const float rowHeight = std::min(rowStep, height - y);
            const float centerY = y + rowHeight / 2.f;
            const float half = roundedHalfWidth(
                width, height, radius, centerY
            );

            addRect(parent, width / 2.f - half, y,
                    half * 2.f, rowHeight, ccc4(255, 255, 255, 255));
        }

        // Cover the center with black, leaving the white outline visible.
        const float innerWidth = width - 2.f * border;
        const float innerHeight = height - 2.f * border;
        const float innerRadius = std::max(1.f, radius - border);

        for (float y = border; y < height - border; y += rowStep) {
            const float rowHeight = std::min(
                rowStep, height - border - y
            );
            const float centerY = y + rowHeight / 2.f;
            const float half = roundedHalfWidth(
                innerWidth, innerHeight, innerRadius,
                centerY - border
            );

            addRect(parent, width / 2.f - half, y,
                    half * 2.f, rowHeight, ccc4(0, 0, 0, 235));
        }
    }

    bool init(GJGameLevel* level, bool useReplay,
              bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;

        auto size = CCDirector::sharedDirector()->getWinSize();
        auto hud = CCNode::create();
        this->addChild(hud, 9999);

        // Compact black HUD with a white rounded outline.
        constexpr float panelWidth = 280.f;
        constexpr float panelHeight = 62.f;
        constexpr float panelRadius = 25.f;
        constexpr float panelBorder = 3.f;
        constexpr float rightMargin = 12.f;
        constexpr float topMargin = 12.f;

        auto panel = CCNode::create();
        panel->setPosition({
            size.width - panelWidth - rightMargin,
            size.height - panelHeight - topMargin
        });
        hud->addChild(panel);

        addRoundedPanel(
            panel, panelWidth, panelHeight, panelRadius, panelBorder
        );

        // Pixel heart. Its bottom position is chosen so its center lines up
        // with the BPM text, and the animation moves it from this baseline.
        auto heart = CCNode::create();
        m_fields->heartBaseY = 14.f;
        heart->setPosition({20.f, m_fields->heartBaseY});
        panel->addChild(heart, 10);
        m_fields->heart = heart;

        static constexpr const char* pixels[] = {
            "01100110",
            "11111111",
            "11111111",
            "11111111",
            "01111110",
            "00111100",
            "00011000"
        };

        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 8; col++) {
                if (pixels[row][col] != '1') continue;

                auto pixel = CCLayerColor::create(
                    ccc4(255, 45, 65, 255), 4.f, 4.f
                );
                if (!pixel) continue;

                pixel->setPosition({
                    col * 4.5f, (6 - row) * 4.5f
                });
                heart->addChild(pixel);
            }
        }

        auto label = CCLabelBMFont::create("72 BPM", "bigFont.fnt");
        if (label) {
            label->setScale(0.45f);
            label->setColor(ccc3(0, 255, 80));
            label->setAnchorPoint({0.f, 0.5f});
            label->setPosition({78.f, panelHeight / 2.f});
            panel->addChild(label, 10);
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

        const int progress = std::clamp(
            static_cast<int>(getCurrentPercent()), 0, 100
        );

        // Simulated BPM varies smoothly instead of using fixed checkpoints.
        // At 60%+ it stays around 140 or higher; near the end it rises again.
        const float wave = std::sin(m_fields->elapsed * 2.2f) * 6.f;
        float target = 72.f + progress * 0.75f + wave;

        if (progress >= 60)
            target = 147.f + (progress - 60) * 0.4f + wave;

        if (progress >= 90)
            target = 166.f + wave;

        // Smoothly lower BPM when progress resets after a death/retry.
        if (progress < m_fields->lastProgress) {
            target = std::min(target, 78.f);
        }
        m_fields->lastProgress = progress;

        m_fields->currentBpm +=
            (target - m_fields->currentBpm) *
            std::min(dt * 2.5f, 1.f);

        const int bpm = std::clamp(
            static_cast<int>(m_fields->currentBpm), 60, 190
        );

        if (bpm != m_fields->lastBpm) {
            m_fields->lastBpm = bpm;
            const std::string text = std::to_string(bpm) + " BPM";
            m_fields->bpmLabel->setString(text.c_str());
        }

        // Green at low BPM, transitioning through yellow/orange to red.
        const float t = std::clamp((bpm - 80.f) / 90.f, 0.f, 1.f);
        const GLubyte red = static_cast<GLubyte>(255.f * t);
        const GLubyte green = static_cast<GLubyte>(255.f * (1.f - t));
        m_fields->bpmLabel->setColor(ccc3(red, green, 0));

        // A quick main beat and a smaller second beat; faster BPM pulses faster.
        const float rate = bpm / 60.f;
        const float beat = std::fmod(m_fields->beatPhase * rate, 1.f);
        float jump = 0.f;
        float pulse = 0.f;

        if (beat < 0.18f) {
            pulse = std::sin((beat / 0.18f) * 3.14159f);
            jump = pulse * 7.f;
        } else if (beat > 0.22f && beat < 0.36f) {
            pulse = std::sin(
                ((beat - 0.22f) / 0.14f) * 3.14159f
            );
            jump = pulse * 4.f;
        }

        m_fields->heart->setScale(1.f + pulse * 0.12f);
        m_fields->heart->setPosition({
            20.f, m_fields->heartBaseY + jump
        });
    }
};
