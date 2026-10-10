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
        if (distance <= straightHalfHeight) return width / 2.f;

        const float curveDistance = distance - straightHalfHeight;
        const float inside = std::max(
            0.f, radius * radius - curveDistance * curveDistance
        );
        return width / 2.f - radius + std::sqrt(inside);
    }

    static void addRoundedPanel(CCNode* parent, float width,
                                float height, float radius,
                                float border) {
        constexpr float rowStep = 2.f;

        for (float y = 0.f; y < height; y += rowStep) {
            const float rowHeight = std::min(rowStep, height - y);
            const float half = roundedHalfWidth(
                width, height, radius, y + rowHeight / 2.f
            );
            addRect(parent, width / 2.f - half, y,
                    half * 2.f, rowHeight, ccc4(255, 255, 255, 255));
        }

        const float innerWidth = width - 2.f * border;
        const float innerHeight = height - 2.f * border;
        const float innerRadius = std::max(1.f, radius - border);

        for (float y = border; y < height - border; y += rowStep) {
            const float rowHeight = std::min(
                rowStep, height - border - y
            );
            const float half = roundedHalfWidth(
                innerWidth, innerHeight, innerRadius,
                y + rowHeight / 2.f - border
            );
            addRect(parent, width / 2.f - half, y,
                    half * 2.f, rowHeight, ccc4(0, 0, 0, 245));
        }
    }

    static void addEcgLogo(CCNode* panel, float panelHeight) {
        auto logo = CCDrawNode::create();
        if (!logo) return;

        // White heart outline, sitting partly outside the panel's top-left.
        const auto white = ccc4f(1.f, 1.f, 1.f, 1.f);
        const float thickness = 1.8f;
        const CCPoint outline[] = {
            {-15.f, 5.f}, {-16.f, 11.f}, {-13.f, 17.f},
            {-8.f, 20.f}, {-3.f, 19.f}, {0.f, 15.f},
            {4.f, 19.f}, {9.f, 20.f}, {14.f, 17.f},
            {16.f, 11.f}, {14.f, 5.f}, {8.f, -1.f},
            {0.f, -7.f}, {-8.f, -1.f}, {-15.f, 5.f}
        };
        for (int i = 0; i < 14; ++i) {
            logo->drawSegment(outline[i], outline[i + 1],
                              thickness, white);
        }

        // ECG pulse line through the heart.
        const CCPoint pulse[] = {
            {-15.f, 5.f}, {-8.f, 5.f}, {-4.f, 1.f},
            {-1.f, 13.f}, {3.f, -2.f}, {6.f, 7.f},
            {10.f, 5.f}, {15.f, 5.f}
        };
        for (int i = 0; i < 7; ++i) {
            logo->drawSegment(pulse[i], pulse[i + 1],
                              1.6f, white);
        }

        logo->setScale(0.52f);
        logo->setPosition({18.f, panelHeight - 1.f});
        panel->addChild(logo, 20);
    }

    bool init(GJGameLevel* level, bool useReplay,
              bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;

        auto size = CCDirector::sharedDirector()->getWinSize();
        auto hud = CCNode::create();
        this->addChild(hud, 9999);

        // Compact pill panel, matching the reference graphic.
        constexpr float panelWidth = 165.f;
        constexpr float panelHeight = 44.f;
        constexpr float panelRadius = 20.f;
        constexpr float panelBorder = 2.f;
        constexpr float rightMargin = 12.f;
        constexpr float topMargin = 12.f;

        auto panel = CCNode::create();
        panel->setPosition({
            size.width - panelWidth - rightMargin,
            size.height - panelHeight - topMargin
        });
        hud->addChild(panel);
        addRoundedPanel(panel, panelWidth, panelHeight,
                        panelRadius, panelBorder);
        addEcgLogo(panel, panelHeight);

        auto heart = CCNode::create();
        m_fields->heartBaseY = 10.5f;
        heart->setPosition({15.f, m_fields->heartBaseY});
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
                    ccc4(255, 35, 55, 255), 3.f, 3.f
                );
                if (!pixel) continue;
                pixel->setPosition({
                    col * 3.f, (6 - row) * 3.f
                });
                heart->addChild(pixel);
            }
        }

        // White stepped pulse mark, matching the reference heart.
        const CCPoint whitePixels[] = {
            {3.f, 12.f}, {6.f, 9.f}, {9.f, 6.f}, {12.f, 3.f}
        };
        for (const auto& pos : whitePixels) {
            auto highlight = CCLayerColor::create(
                ccc4(255, 255, 255, 255), 3.f, 3.f
            );
            if (highlight) {
                highlight->setPosition(pos);
                heart->addChild(highlight, 2);
            }
        }

        auto label = CCLabelBMFont::create("72 BPM", "bigFont.fnt");
        if (label) {
            label->setScale(0.36f);
            label->setColor(ccc3(0, 255, 80));
            label->setAnchorPoint({0.f, 0.5f});
            label->setPosition({48.f, panelHeight / 2.f});
            panel->addChild(label, 10);
            m_fields->bpmLabel = label;
        }

        return true;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (!m_fields->bpmLabel || !m_fields->heart) return;

        m_fields->elapsed += dt;
        m_fields->beatPhase += dt;

        const int progress = std::clamp(
            static_cast<int>(getCurrentPercent()), 0, 100
        );

        const float wave = std::sin(m_fields->elapsed * 2.2f) * 6.f;
        float target = 72.f + progress * 0.75f + wave;
        if (progress >= 60)
            target = 147.f + (progress - 60) * 0.4f + wave;
        if (progress >= 90)
            target = 166.f + wave;

        if (progress < m_fields->lastProgress)
            target = std::min(target, 78.f);
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

        const float t = std::clamp((bpm - 80.f) / 90.f, 0.f, 1.f);
        const GLubyte red = static_cast<GLubyte>(255.f * t);
        const GLubyte green = static_cast<GLubyte>(255.f * (1.f - t));
        m_fields->bpmLabel->setColor(ccc3(red, green, 0));

        const float rate = bpm / 60.f;
        const float beat = std::fmod(m_fields->beatPhase * rate, 1.f);
        float pulse = 0.f;
        float jump = 0.f;

        if (beat < 0.18f) {
            pulse = std::sin((beat / 0.18f) * 3.14159f);
            jump = pulse * 5.f;
        } else if (beat > 0.22f && beat < 0.36f) {
            pulse = std::sin(
                ((beat - 0.22f) / 0.14f) * 3.14159f
            );
            jump = pulse * 3.f;
        }

        m_fields->heart->setScale(1.f + pulse * 0.14f);
        m_fields->heart->setPosition({
            15.f, m_fields->heartBaseY + jump
        });
    }
};
