#pragma once

#include <memory>
#include <string>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

/**
 * HelloGame is the example application shipped by cna-template.
 *
 * It is intentionally small: load one texture and animate it. Delete it and
 * replace it with your own game -- see README.md.
 *
 * It is also deliberately renderer-agnostic. CNA has 46 renderers, and they are
 * not interchangeable: some are 2D-only, some open no window at all, some draw
 * nothing on purpose. Everything HelloGame does (Clear, SpriteBatch, Texture2D)
 * is in the subset every renderer implements, and anything beyond that subset is
 * gated on GraphicsDevice::SupportsCapability() rather than on the renderer's
 * name. Follow that pattern as you grow the game and it will keep working
 * wherever you point it.
 */
class HelloGame : public Microsoft::Xna::Framework::Game {
public:
    explicit HelloGame(bool smokeTest = false);

    GetTypeNameHPP()

protected:
    void LoadContent() override;
    void Update(Microsoft::Xna::Framework::GameTime& gameTime) override;
    void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override;

private:
    /** Writes what the selected renderer can actually do to stdout. */
    void ReportRendererCapabilities();
    void Draw2DLogo();
    void Draw3DLogoCube();
    void DrawRendererBanner();

    Microsoft::Xna::Framework::GraphicsDeviceManager graphics_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> spriteBatch_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> cubeEffect_;
    Microsoft::Xna::Framework::Graphics::Texture2D logoTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D solidTexture_;
    Microsoft::Xna::Framework::Vector2 position_;
    Microsoft::Xna::Framework::Vector2 velocity_;
    std::string rendererName_;
    float animationSeconds_;
    bool smokeTest_;
    unsigned int drawnFrames_;
    bool supportsThreeD_;
    bool supportsCombinedDepthStencil_;

    /**
     * False for the renderers that create no window (HEADLESS, SOFTWARE, STUB,
     * PORTABLEGL). There is no GraphicsCapability for this, so it is probed from
     * the GameWindow rather than assumed from the renderer name.
     */
    bool hasWindow_;

    static constexpr float RendererBannerSeconds = 5.0f;
    static constexpr float AnimationSpeed = 2.0f;
    static constexpr float Maximum2DLogoScale = 1.08f;
    static constexpr unsigned int SmokeTestFrames = 3;
};
