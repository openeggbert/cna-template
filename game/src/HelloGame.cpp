#include "HelloGame/HelloGame.hpp"

#include <algorithm>
#include <iostream>

#include "CNA/GraphicsCapability.hpp"
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
      drawnFrames_(0),
      hasWindow_(false)
{
    // Deliberately not calling setPreferredBackBufferWidth/HeightProperty()
    // here: doing so made the window visibly resize (and flicker) two or three
    // times during startup as CNA created the window at its own default size and
    // then re-applied ours. The manager's default resolution avoids that.
    Game::getWindowProperty().setTitleProperty("cna-template - HelloGame");
}

void HelloGame::ReportRendererCapabilities()
{
    const auto& device = getGraphicsDeviceProperty();

    // GetGraphicsRendererName() is for display only -- never branch on it.
    // Behaviour belongs on SupportsCapability(), which reports what the renderer
    // can really do rather than what its name suggests.
    std::cout << "cna-template: renderer " << device.GetGraphicsRendererName() << "\n"
              << "  window          : " << (hasWindow_ ? "yes" : "no (windowless renderer)") << "\n"
              << "  3D pipeline     : "
              << (device.SupportsCapability(CNA::GraphicsCapability::ThreeD) ? "yes" : "no (2D only)")
              << "\n"
              << "  depth/stencil   : "
              << (device.SupportsCapability(CNA::GraphicsCapability::DepthStencilBuffer) ? "yes" : "no")
              << "\n"
              << "  custom shaders  : "
              << (device.SupportsCapability(CNA::GraphicsCapability::CustomEffects) ? "yes" : "no")
              << "\n"
              << "  max texture size: " << device.GetMaxTextureDimension() << "\n";
    std::cout.flush();
}

void HelloGame::LoadContent()
{
    // A renderer with no SDL window (HEADLESS, SOFTWARE, STUB, PORTABLEGL) still
    // has a working GraphicsDevice, SpriteBatch and ContentManager -- it just has
    // nowhere to present. CNA exposes no capability for "has a window", so probe
    // the window itself.
    hasWindow_ = Game::getWindowProperty().GetNativeSdlWindowEXT() != nullptr;

    spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
    logoTexture_ = getContentProperty().Load<Texture2D>("logo");

    ReportRendererCapabilities();
}

void HelloGame::Update(GameTime& gameTime)
{
    // Keyboard state comes from SDL's event pump, which only runs when there is a
    // window. Skipping it on the windowless renderers keeps this loop meaningful
    // instead of reading a permanently empty state.
    if (hasWindow_) {
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
    }

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

    // Clear(Color) and the SpriteBatch path below are the subset every one of
    // CNA's renderers implements, so this Draw() needs no capability gating.
    // Anything 3D (VertexBuffer, DrawUserPrimitives, depth state) would, because
    // the 2D-only renderers throw on those calls.
    device.Clear(Color::CornflowerBlue);

    spriteBatch_->Begin();
    spriteBatch_->Draw(logoTexture_, position_, Color::White);
    spriteBatch_->End();

    // Deliberately no device.Present() here. Game::EndDraw() already presents
    // exactly once per frame (via GraphicsDeviceManager::EndDraw()), matching
    // real XNA/FNA, where Game.Draw() never presents. Calling it here too made
    // SDL_RenderPresent() run twice per frame; SDL treats the backbuffer as
    // invalid after a present, so the second one pushed undefined content to the
    // screen and the window visibly flickered on every frame.

    if (smokeTest_ && ++drawnFrames_ >= SmokeTestFrames) {
        std::cout << "cna-template: smoke test drew " << drawnFrames_ << " frames; exiting\n";
        Exit();
    }
}

GetTypeNameCPP(HelloGame, "HelloGame")
