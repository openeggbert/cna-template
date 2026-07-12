#pragma once

#include <memory>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

/**
 * HelloGame is the example application shipped by cna-template.
 *
 * It is intentionally small: load one texture, draw it, move it around the
 * screen with the arrow keys. Delete/replace it with your own game — see the
 * project README for how the pieces (GraphicsDeviceManager, ContentManager,
 * SpriteBatch) fit together.
 */
class HelloGame : public Microsoft::Xna::Framework::Game {
public:
    HelloGame();

    GetTypeNameHPP()

protected:
    void LoadContent() override;
    void Update(Microsoft::Xna::Framework::GameTime& gameTime) override;
    void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override;

private:
    Microsoft::Xna::Framework::GraphicsDeviceManager graphics_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> spriteBatch_;
    Microsoft::Xna::Framework::Graphics::Texture2D logoTexture_;
    Microsoft::Xna::Framework::Vector2 position_;

    static constexpr float MoveSpeed = 220.0f; // pixels per second
};
