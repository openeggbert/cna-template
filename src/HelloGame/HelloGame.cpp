#include "HelloGame/HelloGame.hpp"

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

HelloGame::HelloGame()
    : graphics_(this),
      logoTexture_(),
      position_(300.0f, 220.0f)
{
    graphics_.setPreferredBackBufferWidthProperty(800);
    graphics_.setPreferredBackBufferHeightProperty(600);
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
    const float maxX = static_cast<float>(graphics_.getPreferredBackBufferWidthProperty() - bounds.Width);
    const float maxY = static_cast<float>(graphics_.getPreferredBackBufferHeightProperty() - bounds.Height);

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

    device.Present();
}

GetTypeNameCPP(HelloGame, "HelloGame")
