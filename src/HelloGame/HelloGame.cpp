#include "HelloGame/HelloGame.hpp"

#include <algorithm>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

HelloGame::HelloGame(const bool smokeTest)
    : graphics_(this),
      logoTexture_(),
      position_(300.0f, 220.0f),
      smokeTest_(smokeTest),
      drawnFrames_(0)
{
    // Deliberately not calling setPreferredBackBufferWidth/HeightProperty()
    // here: doing so made the window visibly resize (and flicker) two or
    // three times during startup as CNA created the window at its own
    // default size and then re-applied ours. Using the manager's default
    // resolution avoids that entirely.
    Game::getWindowProperty().setTitleProperty("cna-template - HelloGame");
}

void HelloGame::LoadContent()
{
    spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
    logoTexture_ = getContentProperty().Load<Texture2D>("logo");
}

void HelloGame::Update(GameTime& gameTime)
{
    const KeyboardState state = Keyboard::GetState();

    if (state.IsKeyDown(Keys::Escape)) {
        Exit();
        return;
    }

    const float deltaTime =
        static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty()) / 1000.0f;
    const float step = MoveSpeed * deltaTime;

    if (state.IsKeyDown(Keys::Left))  position_.X -= step;
    if (state.IsKeyDown(Keys::Right)) position_.X += step;
    if (state.IsKeyDown(Keys::Up))    position_.Y -= step;
    if (state.IsKeyDown(Keys::Down))  position_.Y += step;

    const Rectangle bounds = logoTexture_.getBoundsProperty();
    const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
    const float maxX = std::max(0.0f, static_cast<float>(viewport.getWidthProperty() - bounds.Width));
    const float maxY = std::max(0.0f, static_cast<float>(viewport.getHeightProperty() - bounds.Height));

    if (position_.X < 0.0f) position_.X = 0.0f;
    if (position_.Y < 0.0f) position_.Y = 0.0f;
    if (position_.X > maxX) position_.X = maxX;
    if (position_.Y > maxY) position_.Y = maxY;
}

void HelloGame::Draw(const GameTime& gameTime)
{
    (void)gameTime;

    auto& device = getGraphicsDeviceProperty();

    device.Clear(Color::CornflowerBlue);

    spriteBatch_->Begin();
    spriteBatch_->Draw(logoTexture_, position_, Color::White);
    spriteBatch_->End();

    // Deliberately no device.Present() here. Game::EndDraw() already presents
    // exactly once per frame (via GraphicsDeviceManager::EndDraw()), matching
    // real XNA/FNA, where Game.Draw() never presents. Calling it here too made
    // SDL_RenderPresent() run twice per frame; SDL treats the backbuffer as
    // invalid after a present, so the second one pushed undefined content to
    // the screen and the window visibly flickered on every frame.

    if (smokeTest_ && ++drawnFrames_ >= 3) {
        Exit();
    }
}

GetTypeNameCPP(HelloGame, "HelloGame")
