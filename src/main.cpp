
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>
#include <string>

using namespace geode::prelude;

class $modify(HeartMonitorPlayLayer, PlayLayer) {
    struct Fields {
        CCLabelBMFont* bpmLabel = nullptr;
        int lastBpm = -1;
    };

    bool init(
        GJGameLevel* level,
        bool useReplay,
        bool dontCreateObjects
    ) {
        if (!PlayLayer::init(
            level,
            useReplay,
            dontCreateObjects
        )) {
            return false;
        }

        auto winSize =
            CCDirector::sharedDirector()->getWinSize();

        auto hud = CCNode::create();
        this->addChild(hud, 9999);

        // Build a pixel heart without external image files.
        auto heart = CCNode::create();
        heart->setPosition({
            winSize.width - 116.f,
            winSize.height - 35.f
        });
        hud->addChild(heart);

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
                if (pixels[row][col] != '1') {
                    continue;
                }

                auto pixel = CCLayerColor::create(
                    ccc4(255, 55, 75, 255),
                    4.f,
                    4.f
                );

                if (pixel) {
                    pixel->setPosition({
                        col * 4.5f,
                        (6 - row) * 4.5f
                    });

                    heart->addChild(pixel);
                }
            }
        }

        // Green BPM counter.
        auto label = CCLabelBMFont::create(
            "72 BPM",
            "bigFont.fnt"
        );

        if (label) {
            label->setScale(0.45f);
            label->setColor(ccc3(0, 255, 80));
            label->setAnchorPoint({0.f, 0.5f});
            label->setPosition({
                winSize.width - 76.f,
                winSize.height - 35.f
            });

            hud->addChild(label);
            m_fields->bpmLabel = label;
        }

        return true;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!m_fields->bpmLabel || !m_player1 || !m_level) {
            return;
        }

        const int progress = std::clamp(
            static_cast<int>(getCurrentPercent()),
            0,
            100
        );

        int bpm = 72;

        if (progress >= 90) {
            bpm = 180;
        } else if (progress >= 60) {
            bpm = 145;
        } else if (progress >= 30) {
            bpm = 100;
        }

        if (bpm != m_fields->lastBpm) {
            m_fields->lastBpm = bpm;

            const std::string text =
                std::to_string(bpm) + " BPM";

            m_fields->bpmLabel->setString(text.c_str());
        }
    }
};
