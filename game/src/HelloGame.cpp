#include "HelloGame/HelloGame.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>

#include "CNA/GraphicsCapability.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionTexture.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace
{
    using GlyphRows = std::array<std::uint8_t, 7>;

    [[nodiscard]] constexpr GlyphRows GetGlyphRows(const char character)
    {
        switch (character) {
            case 'A': return {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11};
            case 'B': return {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e};
            case 'C': return {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e};
            case 'D': return {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e};
            case 'E': return {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f};
            case 'F': return {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10};
            case 'G': return {0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0e};
            case 'H': return {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11};
            case 'I': return {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1f};
            case 'J': return {0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0c};
            case 'K': return {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
            case 'L': return {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f};
            case 'M': return {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11};
            case 'N': return {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
            case 'O': return {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e};
            case 'P': return {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10};
            case 'Q': return {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d};
            case 'R': return {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11};
            case 'S': return {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e};
            case 'T': return {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
            case 'U': return {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e};
            case 'V': return {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04};
            case 'W': return {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a};
            case 'X': return {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11};
            case 'Y': return {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04};
            case 'Z': return {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f};
            case '0': return {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e};
            case '1': return {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e};
            case '2': return {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f};
            case '3': return {0x1e, 0x01, 0x01, 0x0e, 0x01, 0x01, 0x1e};
            case '4': return {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02};
            case '5': return {0x1f, 0x10, 0x10, 0x1e, 0x01, 0x01, 0x1e};
            case '6': return {0x0e, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x0e};
            case '7': return {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
            case '8': return {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e};
            case '9': return {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x0e};
            case '_': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f};
            case '-': return {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00};
            case ' ': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            default:  return {0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04};
        }
    }

    [[nodiscard]] std::array<VertexPositionTexture, 36> CreateLogoCubeVertices()
    {
        std::array<VertexPositionTexture, 36> vertices{};
        std::size_t next = 0;

        const auto addFace = [&vertices, &next](const Vector3& topLeft,
                                                const Vector3& topRight,
                                                const Vector3& bottomRight,
                                                const Vector3& bottomLeft) {
            vertices[next++] = {topLeft,     Vector2(0.0f, 0.0f)};
            vertices[next++] = {topRight,    Vector2(1.0f, 0.0f)};
            vertices[next++] = {bottomRight, Vector2(1.0f, 1.0f)};
            vertices[next++] = {topLeft,     Vector2(0.0f, 0.0f)};
            vertices[next++] = {bottomRight, Vector2(1.0f, 1.0f)};
            vertices[next++] = {bottomLeft,  Vector2(0.0f, 1.0f)};
        };

        addFace({-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f},
                { 1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f});
        addFace({ 1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f},
                {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f});
        addFace({ 1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f, -1.0f},
                { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f});
        addFace({-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f,  1.0f},
                {-1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f, -1.0f});
        addFace({-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f},
                { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f});
        addFace({-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
                { 1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f});

        return vertices;
    }
}

HelloGame::HelloGame(const bool smokeTest)
    : graphics_(this),
      spriteBatch_(),
      cubeEffect_(),
      logoTexture_(),
      solidTexture_(),
      position_(400.0f, 240.0f),
      velocity_(104.0f, 74.0f),
      rendererName_(),
      animationSeconds_(0.0f),
      smokeTest_(smokeTest),
      drawnFrames_(0),
      supportsThreeD_(false),
      supportsDepth_(false),
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
    std::cout << "cna-template: renderer " << rendererName_ << "\n"
              << "  window          : " << (hasWindow_ ? "yes" : "no (windowless renderer)") << "\n"
              << "  3D pipeline     : " << (supportsThreeD_ ? "yes" : "no (2D only)") << "\n"
              << "  depth/stencil   : " << (supportsDepth_ ? "yes" : "no") << "\n"
              << "  custom shaders  : "
              << (device.SupportsCapability(CNA::GraphicsCapability::CustomEffects) ? "yes" : "no")
              << "\n"
              << "  max texture size: " << device.GetMaxTextureDimension() << "\n";
    std::cout.flush();
}

void HelloGame::LoadContent()
{
    // A renderer with no SDL window still has a working GraphicsDevice,
    // SpriteBatch and ContentManager -- it just has nowhere to present. CNA
    // exposes no capability for "has a window", so probe the window itself.
    hasWindow_ = Game::getWindowProperty().GetNativeSdlWindowEXT() != nullptr;

    auto& device = getGraphicsDeviceProperty();
    spriteBatch_ = std::make_unique<SpriteBatch>(device);
    logoTexture_ = getContentProperty().Load<Texture2D>("logo");

    solidTexture_ = Texture2D(device, 1, 1);
    const Color white = Color::White;
    solidTexture_.SetData(&white, 1);

    rendererName_ = device.GetGraphicsRendererName();
    Game::getWindowProperty().setTitleProperty(
        "cna-template - HelloGame (" + rendererName_ + ")");
    supportsThreeD_ = device.SupportsCapability(CNA::GraphicsCapability::ThreeD);
    supportsDepth_ = device.SupportsCapability(CNA::GraphicsCapability::DepthStencilBuffer);

    if (supportsThreeD_) {
        cubeEffect_ = std::make_unique<BasicEffect>(device);
        cubeEffect_->setTextureEnabledProperty(true);
        cubeEffect_->setTextureProperty(&logoTexture_);
        cubeEffect_->setLightingEnabledProperty(false);
        cubeEffect_->VertexColorEnabled = false;
    }

    const auto& viewport = device.getViewportProperty();
    position_ = Vector2(static_cast<float>(viewport.getWidthProperty()) * 0.5f,
                        static_cast<float>(viewport.getHeightProperty()) * 0.5f);

    ReportRendererCapabilities();
}

void HelloGame::Update(GameTime& gameTime)
{
    if (hasWindow_ && Keyboard::GetState().IsKeyDown(Keys::Escape)) {
        Exit();
        return;
    }

    const float deltaTime =
        static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty()) / 1000.0f;
    animationSeconds_ += deltaTime;

    if (!supportsThreeD_) {
        // Limit only the movement step after a debugger pause or window drag;
        // the five-second banner still follows real game time above.
        const float movementDelta = std::min(deltaTime, 0.1f);
        position_.X += velocity_.X * movementDelta;
        position_.Y += velocity_.Y * movementDelta;

        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        const float logoSize = static_cast<float>(
            std::max(logoTexture_.getWidthProperty(), logoTexture_.getHeightProperty()));
        const float halfExtent = logoSize * 0.5f * Maximum2DLogoScale * 1.15f;
        const float minX = std::min(halfExtent, static_cast<float>(viewport.getWidthProperty()) * 0.5f);
        const float minY = std::min(halfExtent, static_cast<float>(viewport.getHeightProperty()) * 0.5f);
        const float maxX = std::max(minX, static_cast<float>(viewport.getWidthProperty()) - minX);
        const float maxY = std::max(minY, static_cast<float>(viewport.getHeightProperty()) - minY);

        if (position_.X < minX) {
            position_.X = minX;
            velocity_.X = std::abs(velocity_.X);
        } else if (position_.X > maxX) {
            position_.X = maxX;
            velocity_.X = -std::abs(velocity_.X);
        }

        if (position_.Y < minY) {
            position_.Y = minY;
            velocity_.Y = std::abs(velocity_.Y);
        } else if (position_.Y > maxY) {
            position_.Y = maxY;
            velocity_.Y = -std::abs(velocity_.Y);
        }
    }
}

void HelloGame::Draw2DLogo()
{
    const float motionSeconds = animationSeconds_ * AnimationSpeed;
    const float scale = 0.96f + 0.12f * std::sin(motionSeconds * 0.65f);
    const float rotation = 0.11f * std::sin(motionSeconds * 0.55f);
    const Vector2 origin(static_cast<float>(logoTexture_.getWidthProperty()) * 0.5f,
                         static_cast<float>(logoTexture_.getHeightProperty()) * 0.5f);

    spriteBatch_->Begin();
    spriteBatch_->Draw(logoTexture_, position_, std::nullopt, Color::White,
                       rotation, origin, scale, SpriteEffects::None, 0.0f);
    spriteBatch_->End();
}

void HelloGame::Draw3DLogoCube()
{
    static const std::array<VertexPositionTexture, 36> cubeVertices = CreateLogoCubeVertices();

    auto& device = getGraphicsDeviceProperty();
    const auto& viewport = device.getViewportProperty();
    const float aspectRatio = static_cast<float>(viewport.getWidthProperty()) /
                              static_cast<float>(std::max(1, viewport.getHeightProperty()));
    const float motionSeconds = animationSeconds_ * AnimationSpeed;
    const float scale = 0.88f + 0.10f * std::sin(motionSeconds * 0.48f);
    const float moveX = 1.15f * std::sin(motionSeconds * 0.24f);
    const float moveY = 0.70f * std::sin(motionSeconds * 0.19f + 1.1f);

    cubeEffect_->setWorldProperty(
        Matrix::CreateScale(scale) *
        Matrix::CreateRotationX(motionSeconds * 0.22f) *
        Matrix::CreateRotationY(motionSeconds * 0.31f) *
        Matrix::CreateRotationZ(motionSeconds * 0.13f) *
        Matrix::CreateTranslation(moveX, moveY, 0.0f));
    cubeEffect_->setViewProperty(
        Matrix::CreateLookAt(Vector3(0.0f, 0.0f, 6.0f), Vector3::Zero, Vector3::Up));
    cubeEffect_->setProjectionProperty(
        Matrix::CreatePerspectiveFieldOfView(0.78539816339f, aspectRatio, 0.1f, 100.0f));

    device.setBlendStateProperty(BlendState::Opaque);
    device.setDepthStencilStateProperty(supportsDepth_ ? DepthStencilState::Default
                                                       : DepthStencilState::None);
    device.setRasterizerStateProperty(RasterizerState::CullNone);

    for (auto& pass : cubeEffect_->getCurrentTechniqueProperty()->getPassesProperty()) {
        pass.Apply();
        device.DrawUserPrimitives(PrimitiveType::TriangleList,
                                  cubeVertices.data(), 0,
                                  static_cast<int>(cubeVertices.size()) / 3);
    }
}

void HelloGame::DrawRendererBanner()
{
    const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
    const int viewportWidth = viewport.getWidthProperty();
    const int glyphColumns = std::max(1, static_cast<int>(rendererName_.size()) * 6 - 1);
    const int pixelSize = std::clamp((viewportWidth - 48) / glyphColumns, 1, 8);
    const int textWidth = glyphColumns * pixelSize;
    const int textHeight = 7 * pixelSize;
    const int textX = (viewportWidth - textWidth) / 2;
    constexpr int textY = 28;
    constexpr int padding = 14;
    const Color translucentWhite = Color::FromNonPremultiplied(255, 255, 255, 96);

    spriteBatch_->Begin();
    spriteBatch_->Draw(solidTexture_,
                       Rectangle(textX - padding, textY - padding,
                                 textWidth + padding * 2, textHeight + padding * 2),
                       translucentWhite);

    for (std::size_t characterIndex = 0; characterIndex < rendererName_.size(); ++characterIndex) {
        const GlyphRows rows = GetGlyphRows(rendererName_[characterIndex]);
        const int characterX = textX + static_cast<int>(characterIndex) * 6 * pixelSize;

        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((rows[static_cast<std::size_t>(row)] & (1U << (4 - column))) != 0) {
                    spriteBatch_->Draw(solidTexture_,
                                       Rectangle(characterX + column * pixelSize,
                                                 textY + row * pixelSize,
                                                 pixelSize, pixelSize),
                                       Color(24, 36, 55, 255));
                }
            }
        }
    }

    spriteBatch_->End();
}

void HelloGame::Draw(const GameTime& gameTime)
{
    (void)gameTime;

    auto& device = getGraphicsDeviceProperty();
    if (supportsThreeD_ && supportsDepth_) {
        device.Clear(Color::CornflowerBlue, 1.0f);
    } else {
        device.Clear(Color::CornflowerBlue);
    }

    if (supportsThreeD_) {
        Draw3DLogoCube();
    } else {
        Draw2DLogo();
    }

    if (animationSeconds_ < RendererBannerSeconds) {
        DrawRendererBanner();
    }

    // Deliberately no device.Present() here. Game::EndDraw() already presents
    // exactly once per frame (via GraphicsDeviceManager::EndDraw()), matching
    // real XNA/FNA, where Game.Draw() never presents.
    if (smokeTest_ && ++drawnFrames_ >= SmokeTestFrames) {
        std::cout << "cna-template: smoke test drew " << drawnFrames_ << " frames; exiting\n";
        Exit();
    }
}

GetTypeNameCPP(HelloGame, "HelloGame")
