#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "game.h"

using namespace Gdiplus;

// Simple Button class
class Button {
public:
    float x, y, width, height;
    std::wstring text;
    Color normalColor;
    Color hoverColor;
    Color pressedColor;
    Color disabledColor;
    bool isHovered = false;
    bool isPressed = false;
    bool isEnabled = true;

    Button(float x, float y, float width, float height, const std::wstring& text,
        Color normalColor = Color(255, 70, 70, 70))
        : x(x), y(y), width(width), height(height), text(text),
        normalColor(normalColor),
        hoverColor(Color(255, 90, 90, 90)),
        pressedColor(Color(255, 50, 50, 50)),
        disabledColor(Color(255, 40, 40, 40)) {
    }

    bool Contains(int mouseX, int mouseY) const {
        return mouseX >= x && mouseX <= x + width &&
            mouseY >= y && mouseY <= y + height;
    }

    void Render(Graphics& graphics, Font& font) {
        // Choose color based on state
        Color currentColor = normalColor;
        if (!isEnabled) currentColor = disabledColor;
        else if (isPressed) currentColor = pressedColor;
        else if (isHovered) currentColor = hoverColor;

        // Draw button background
        SolidBrush bgBrush(currentColor);
        graphics.FillRectangle(&bgBrush, x, y, width, height);

        // Draw border
        Color borderColor = isEnabled ? Color(255, 150, 150, 150) : Color(255, 80, 80, 80);
        Pen borderPen(borderColor, 2.0f);
        graphics.DrawRectangle(&borderPen, x, y, width, height);

        // Draw text centered
        Color textColor = isEnabled ? Color(255, 255, 255, 255) : Color(255, 120, 120, 120);
        SolidBrush textBrush(textColor);
        RectF rect(x, y, width, height);
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        graphics.DrawString(text.c_str(), -1, &font, rect, &format, &textBrush);
    }
};

// UI Manager class
class UIManager {
public:
    std::vector<Button> gatherButtons;
    std::vector<Button> buildingButtons;

    std::wstring clickFeedback;
    float feedbackTimer = 0.0f;

    int mouseX = 0;
    int mouseY = 0;
    bool mouseDown = false;

    void Initialize() {
        InitializeGatherButtons();
        InitializeBuildingButtons();
    }

    void InitializeGatherButtons() {
        float buttonX = 800.0f;
        float buttonY = 100.0f;
        float buttonWidth = 180.0f;
        float buttonHeight = 45.0f;
        float buttonSpacing = 55.0f;

        gatherButtons.push_back(Button(buttonX, buttonY, buttonWidth, buttonHeight,
            L"Gather Food (+5)", Color(255, 50, 150, 50)));
        buttonY += buttonSpacing;

        gatherButtons.push_back(Button(buttonX, buttonY, buttonWidth, buttonHeight,
            L"Chop Wood (+3)", Color(255, 100, 50, 0)));
        buttonY += buttonSpacing;

        gatherButtons.push_back(Button(buttonX, buttonY, buttonWidth, buttonHeight,
            L"Mine Stone (+2)", Color(255, 80, 80, 80)));
        buttonY += buttonSpacing;

        gatherButtons.push_back(Button(buttonX, buttonY, buttonWidth, buttonHeight,
            L"Pan Gold (+1)", Color(255, 180, 150, 0)));
    }

    void InitializeBuildingButtons() {
        float buttonX = 400.0f;
        float buttonY = 350.0f;
        float buttonWidth = 160.0f;
        float buttonHeight = 60.0f;
        float buttonSpacingX = 170.0f;
        float buttonSpacingY = 70.0f;

        // Create buttons for each building type (will be filled with actual building names later)
        for (int i = 0; i < 5; i++) {
            float x = buttonX + (i % 3) * buttonSpacingX;
            float y = buttonY + (i / 3) * buttonSpacingY;

            buildingButtons.push_back(Button(x, y, buttonWidth, buttonHeight,
                L"Building", Color(255, 60, 60, 100)));
        }
    }

    void Update(float deltaTime, GameState& game) {
        // Update button hover states
        for (auto& button : gatherButtons) {
            button.isHovered = button.Contains(mouseX, mouseY);
            if (!mouseDown) button.isPressed = false;
        }

        for (size_t i = 0; i < buildingButtons.size(); i++) {
            buildingButtons[i].isHovered = buildingButtons[i].Contains(mouseX, mouseY);
            if (!mouseDown) buildingButtons[i].isPressed = false;

            // Update enabled state based on affordability
            buildingButtons[i].isEnabled = game.CanAfford(i);

            // Update button text with building info
            if (i < game.buildings.size()) {
                const auto& building = game.buildings[i];
                std::wstringstream ss;
                ss << building.type->name << L" (" << building.count << L")";
                buildingButtons[i].text = ss.str();
            }
        }

        // Update click feedback
        if (feedbackTimer > 0.0f) {
            feedbackTimer -= deltaTime;
            if (feedbackTimer <= 0.0f) {
                clickFeedback = L"";
            }
        }
    }

    void HandleGatherButtonClick(int buttonIndex, GameState& game) {
        switch (buttonIndex) {
        case 0:
            game.GatherResource(ResourceType::Food, 5.0);
            clickFeedback = L"+5 Food!";
            feedbackTimer = 1.0f;
            break;
        case 1:
            game.GatherResource(ResourceType::Wood, 3.0);
            clickFeedback = L"+3 Wood!";
            feedbackTimer = 1.0f;
            break;
        case 2:
            game.GatherResource(ResourceType::Stone, 2.0);
            clickFeedback = L"+2 Stone!";
            feedbackTimer = 1.0f;
            break;
        case 3:
            game.GatherResource(ResourceType::Gold, 1.0);
            clickFeedback = L"+1 Gold!";
            feedbackTimer = 1.0f;
            break;
        }
    }

    void HandleBuildingButtonClick(int buttonIndex, GameState& game) {
        if (game.PurchaseBuilding(buttonIndex)) {
            clickFeedback = L"Built " + game.buildings[buttonIndex].type->name + L"!";
            feedbackTimer = 1.5f;
        }
        else {
            clickFeedback = L"Not enough resources!";
            feedbackTimer = 1.0f;
        }
    }

    void HandleMouseDown(int x, int y, GameState& game) {
        mouseDown = true;

        // Check gather buttons
        for (size_t i = 0; i < gatherButtons.size(); i++) {
            if (gatherButtons[i].Contains(x, y)) {
                gatherButtons[i].isPressed = true;
                HandleGatherButtonClick(i, game);
                return;
            }
        }

        // Check building buttons
        for (size_t i = 0; i < buildingButtons.size(); i++) {
            if (buildingButtons[i].Contains(x, y) && buildingButtons[i].isEnabled) {
                buildingButtons[i].isPressed = true;
                HandleBuildingButtonClick(i, game);
                return;
            }
        }
    }

    void HandleMouseUp() {
        mouseDown = false;
        for (auto& button : gatherButtons) button.isPressed = false;
        for (auto& button : buildingButtons) button.isPressed = false;
    }

    void HandleMouseMove(int x, int y) {
        mouseX = x;
        mouseY = y;
    }

    void Render(HDC hdc, int width, int height, const GameState& game, int fps) {
        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

        // Clear background
        SolidBrush bgBrush(Color(255, 20, 20, 30));
        graphics.FillRectangle(&bgBrush, 0, 0, width, height);

        // Set up fonts
        FontFamily fontFamily(L"Arial");
        Font titleFont(&fontFamily, 24, FontStyleBold, UnitPixel);
        Font resourceFont(&fontFamily, 18, FontStyleRegular, UnitPixel);
        Font smallFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
        Font buttonFont(&fontFamily, 14, FontStyleBold, UnitPixel);
        Font tinyFont(&fontFamily, 11, FontStyleRegular, UnitPixel);

        RenderTitle(graphics, titleFont);
        RenderFPS(graphics, smallFont, fps);
        RenderResources(graphics, resourceFont, game);
        RenderProductionRates(graphics, smallFont, game);
        RenderBuildings(graphics, resourceFont, smallFont, game);
        RenderButtons(graphics, buttonFont, tinyFont, game);
        RenderFeedback(graphics, resourceFont);
    }

private:
    void RenderTitle(Graphics& graphics, Font& font) {
        SolidBrush goldBrush(Color(255, 255, 215, 0));
        PointF titlePos(300.0f, 20.0f);
        graphics.DrawString(L"=== PROCEDURAL CIVILIZATION ===", -1, &font, titlePos, &goldBrush);
    }

    void RenderFPS(Graphics& graphics, Font& font, int fps) {
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        std::wstringstream fpsStream;
        fpsStream << L"FPS: " << fps;
        PointF fpsPos(10.0f, 10.0f);
        graphics.DrawString(fpsStream.str().c_str(), -1, &font, fpsPos, &whiteBrush);
    }

    void RenderResources(Graphics& graphics, Font& font, const GameState& game) {
        SolidBrush foodBrush(Color(255, 100, 255, 100));
        SolidBrush woodBrush(Color(255, 139, 69, 19));
        SolidBrush stoneBrush(Color(255, 128, 128, 128));
        SolidBrush goldBrush(Color(255, 255, 215, 0));

        float yPos = 80.0f;
        float xPos = 30.0f;

        auto renderResource = [&](ResourceType type, SolidBrush& brush) {
            auto it = game.resources.find(type);
            if (it != game.resources.end()) {
                std::wstringstream ss;
                ss << it->second.name << L": " << std::fixed << std::setprecision(1) << it->second.amount;
                PointF pos(xPos, yPos);
                graphics.DrawString(ss.str().c_str(), -1, &font, pos, &brush);
                yPos += 35.0f;
            }
            };

        renderResource(ResourceType::Food, foodBrush);
        renderResource(ResourceType::Wood, woodBrush);
        renderResource(ResourceType::Stone, stoneBrush);
        renderResource(ResourceType::Gold, goldBrush);
    }

    void RenderProductionRates(Graphics& graphics, Font& font, const GameState& game) {
        SolidBrush grayBrush(Color(255, 200, 200, 200));

        std::wstringstream prodStream;
        prodStream << L"Production/sec:";

        for (const auto& res : game.resources) {
            prodStream << L"\n  " << res.second.name << L": +"
                << std::fixed << std::setprecision(1) << res.second.perSecond;
        }

        RectF prodRect(30.0f, 230.0f, 200.0f, 120.0f);
        graphics.DrawString(prodStream.str().c_str(), -1, &font, prodRect, NULL, &grayBrush);
    }

    void RenderBuildings(Graphics& graphics, Font& headerFont, Font& smallFont, const GameState& game) {
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        SolidBrush grayBrush(Color(255, 180, 180, 180));

        PointF buildingHeaderPos(400.0f, 300.0f);
        graphics.DrawString(L"=== BUILDINGS ===", -1, &headerFont, buildingHeaderPos, &whiteBrush);
    }

    void RenderButtons(Graphics& graphics, Font& buttonFont, Font& tinyFont, const GameState& game) {
        // Render gather buttons
        for (auto& button : gatherButtons) {
            button.Render(graphics, buttonFont);
        }

        // Render building buttons with cost info
        for (size_t i = 0; i < buildingButtons.size(); i++) {
            buildingButtons[i].Render(graphics, buttonFont);

            // Draw cost below button
            if (i < game.buildings.size()) {
                auto cost = game.buildings[i].GetNextCost();
                std::wstringstream costStream;
                costStream << L"Cost: ";
                bool first = true;
                for (const auto& c : cost) {
                    if (!first) costStream << L", ";
                    first = false;

                    std::wstring resName;
                    switch (c.first) {
                    case ResourceType::Food: resName = L"F"; break;
                    case ResourceType::Wood: resName = L"W"; break;
                    case ResourceType::Stone: resName = L"S"; break;
                    case ResourceType::Gold: resName = L"G"; break;
                    }
                    costStream << resName << L":" << (int)c.second;
                }

                SolidBrush costBrush(Color(255, 150, 150, 150));
                PointF costPos(buildingButtons[i].x + 5, buildingButtons[i].y + buildingButtons[i].height + 2);
                graphics.DrawString(costStream.str().c_str(), -1, &tinyFont, costPos, &costBrush);
            }
        }
    }

    void RenderFeedback(Graphics& graphics, Font& font) {
        if (!clickFeedback.empty() && feedbackTimer > 0.0f) {
            SolidBrush feedbackBrush(Color(255, 255, 255, 100));
            PointF feedbackPos(400.0f, 250.0f);
            graphics.DrawString(clickFeedback.c_str(), -1, &font, feedbackPos, &feedbackBrush);
        }
    }
};