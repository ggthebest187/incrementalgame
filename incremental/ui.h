#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "game.h"
#include "upgrade_tree_menu.h"
#include "map_view.h"

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
        Color currentColor = normalColor;
        if (!isEnabled) currentColor = disabledColor;
        else if (isPressed) currentColor = pressedColor;
        else if (isHovered) currentColor = hoverColor;

        SolidBrush bgBrush(currentColor);
        graphics.FillRectangle(&bgBrush, x, y, width, height);

        Color borderColor = isEnabled ? Color(255, 150, 150, 150) : Color(255, 80, 80, 80);
        Pen borderPen(borderColor, 2.0f);
        graphics.DrawRectangle(&borderPen, x, y, width, height);

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
    Button upgradeTreeButton;
    Button mapButton;

    UpgradeTreeMenu upgradeTreeMenu;
    MapView mapView;
    bool showMap = false;

    std::wstring clickFeedback;
    float feedbackTimer = 0.0f;

    int mouseX = 0;
    int mouseY = 0;
    bool mouseDown = false;
    bool rightMouseDown = false;

    UIManager() : upgradeTreeButton(450.0f, 20.0f, 200.0f, 40.0f, L"UPGRADE TREE (U)",
        Color(255, 100, 50, 150)),
        mapButton(660.0f, 20.0f, 150.0f, 40.0f, L"MAP (M)",
            Color(255, 50, 150, 100)) {
    }

    void Initialize(const GameState& game) {
        InitializeGatherButtons();
        InitializeBuildingButtons();
        upgradeTreeMenu.Initialize();
    }

    void InitializeGatherButtons() {
        float buttonX = 800.0f;
        float buttonY = 100.0f;
        float buttonWidth = 180.0f;
        float buttonHeight = 45.0f;
        float buttonSpacing = 55.0f;

        gatherButtons.push_back(Button(buttonX, buttonY, buttonWidth, buttonHeight,
            L"Gather Food", Color(255, 50, 150, 50)));
        buttonY += buttonSpacing;

        gatherButtons.push_back(Button(buttonX, buttonY, buttonWidth, buttonHeight,
            L"Chop Wood", Color(255, 100, 50, 0)));
        buttonY += buttonSpacing;

        gatherButtons.push_back(Button(buttonX, buttonY, buttonWidth, buttonHeight,
            L"Mine Stone", Color(255, 80, 80, 80)));
        buttonY += buttonSpacing;

        gatherButtons.push_back(Button(buttonX, buttonY, buttonWidth, buttonHeight,
            L"Pan Gold", Color(255, 180, 150, 0)));
    }

    void InitializeBuildingButtons() {
        float buttonX = 270.0f;
        float buttonY = 400.0f;
        float buttonWidth = 140.0f;
        float buttonHeight = 55.0f;
        float buttonSpacingX = 150.0f;
        float buttonSpacingY = 65.0f;

        for (int i = 0; i < 5; i++) {
            float x = buttonX + (i % 3) * buttonSpacingX;
            float y = buttonY + (i / 3) * buttonSpacingY;

            buildingButtons.push_back(Button(x, y, buttonWidth, buttonHeight,
                L"Building", Color(255, 60, 60, 100)));
        }
    }

    void Update(float deltaTime, GameState& game) {
        // Update map hover
        if (showMap) {
            mapView.UpdateHover(mouseX, mouseY);
        }

        // Don't update main UI if tree is visible
        if (upgradeTreeMenu.isVisible) {
            upgradeTreeMenu.hoveredNode = upgradeTreeMenu.GetNodeAt(mouseX, mouseY);
            return;
        }

        // Update map button
        mapButton.isHovered = mapButton.Contains(mouseX, mouseY);
        if (!mouseDown) mapButton.isPressed = false;

        // Update gather button text with click power
        for (size_t i = 0; i < gatherButtons.size(); i++) {
            gatherButtons[i].isHovered = gatherButtons[i].Contains(mouseX, mouseY);
            if (!mouseDown) gatherButtons[i].isPressed = false;

            std::wstringstream ss;
            switch (i) {
            case 0:
                ss << L"Gather Food (+" << std::fixed << std::setprecision(2) << game.resources[ResourceType::Food].clickPower << L")";
                break;
            case 1:
                ss << L"Chop Wood (+" << std::fixed << std::setprecision(2) << game.resources[ResourceType::Wood].clickPower << L")";
                break;
            case 2:
                ss << L"Mine Stone (+" << std::fixed << std::setprecision(2) << game.resources[ResourceType::Stone].clickPower << L")";
                break;
            case 3:
                ss << L"Pan Gold (+" << std::fixed << std::setprecision(2) << game.resources[ResourceType::Gold].clickPower << L")";
                break;
            }
            gatherButtons[i].text = ss.str();
        }

        for (size_t i = 0; i < buildingButtons.size(); i++) {
            buildingButtons[i].isHovered = buildingButtons[i].Contains(mouseX, mouseY);
            if (!mouseDown) buildingButtons[i].isPressed = false;

            buildingButtons[i].isEnabled = game.CanAfford(static_cast<int>(i));

            if (i < game.buildings.size()) {
                const auto& building = game.buildings[i];
                std::wstringstream ss;

                if (game.IsBuildingUnlocked(static_cast<int>(i))) {
                    ss << building.type->name << L" (" << building.count << L")";
                }
                else {
                    ss << L"[LOCKED]";
                    buildingButtons[i].isEnabled = false;
                }

                buildingButtons[i].text = ss.str();
            }
        }

        // Update tree button
        upgradeTreeButton.isHovered = upgradeTreeButton.Contains(mouseX, mouseY);
        if (!mouseDown) upgradeTreeButton.isPressed = false;

        if (feedbackTimer > 0.0f) {
            feedbackTimer -= deltaTime;
            if (feedbackTimer <= 0.0f) {
                clickFeedback = L"";
            }
        }
    }

    void HandleGatherButtonClick(int buttonIndex, GameState& game) {
        ResourceType type;
        switch (buttonIndex) {
        case 0: type = ResourceType::Food; break;
        case 1: type = ResourceType::Wood; break;
        case 2: type = ResourceType::Stone; break;
        case 3: type = ResourceType::Gold; break;
        default: return;
        }

        game.GatherResource(type);

        std::wstringstream ss;
        ss << L"+" << std::fixed << std::setprecision(2) << game.resources[type].clickPower << L" " << game.resources[type].name << L"!";
        clickFeedback = ss.str();
        feedbackTimer = 1.0f;
    }

    void HandleBuildingButtonClick(int buttonIndex, GameState& game) {
        if (game.PurchaseBuilding(buttonIndex)) {
            // Entered placement mode - open map automatically
            showMap = true;
            mapView.showPlacementPreview = true;
            mapView.placementBuildingIndex = buttonIndex;

            clickFeedback = L"Click on map to place " + game.buildings[buttonIndex].type->name + L"!";
            feedbackTimer = 3.0f;
        }
        else {
            clickFeedback = L"Cannot afford!";
            feedbackTimer = 1.0f;
        }
    }

    void HandleMouseDown(int x, int y, GameState& game) {
        mouseDown = true;

        // Check if tree is visible
        if (upgradeTreeMenu.isVisible) {
            int nodeIdx = upgradeTreeMenu.GetNodeAt(x, y);
            if (nodeIdx >= 0) {
                // Clicked on a node
                const auto& node = upgradeTreeMenu.tree.nodes[nodeIdx];
                if (game.PurchaseUpgrade(node.upgradeIndex)) {
                    clickFeedback = L"Purchased: " + game.upgrades[node.upgradeIndex].name + L"!";
                    feedbackTimer = 2.0f;
                }
            }
            return;
        }

        // Check if in placement mode and clicking on map
        if (showMap && game.placementMode && mapView.Contains(x, y)) {
            // Place building at hovered tile
            if (mapView.hoveredTileX != -1 && mapView.hoveredTileY != -1) {
                TerrainTile tile;
                try {
                    tile = mapView.worldMap.GetTile(mapView.hoveredTileX, mapView.hoveredTileY);
                    double bonus = game.GetTileBonusForBuilding(game.selectedBuildingType, tile);

                    if (game.PlaceBuilding(game.selectedBuildingType,
                        mapView.hoveredTileX, mapView.hoveredTileY, bonus)) {
                        std::wstringstream ss;
                        ss << L"Built " << game.buildings[game.selectedBuildingType].type->name
                            << L" (x" << std::fixed << std::setprecision(2) << bonus << L" bonus)!";
                        clickFeedback = ss.str();
                        feedbackTimer = 2.5f;

                        // Exit placement mode
                        mapView.showPlacementPreview = false;
                        mapView.placementBuildingIndex = -1;
                    }
                }
                catch (...) {
                    clickFeedback = L"Cannot place here!";
                    feedbackTimer = 1.0f;
                }
            }
            return;
        }

        // Check map button
        if (mapButton.Contains(x, y)) {
            mapButton.isPressed = true;

            // If in placement mode and closing map, cancel placement
            if (showMap && game.placementMode) {
                game.CancelPlacement();
                mapView.showPlacementPreview = false;
                mapView.placementBuildingIndex = -1;
                clickFeedback = L"Placement cancelled";
                feedbackTimer = 1.5f;
            }

            showMap = !showMap;
            return;
        }

        // Check tree button
        if (upgradeTreeButton.Contains(x, y)) {
            upgradeTreeButton.isPressed = true;
            upgradeTreeMenu.Toggle();
            return;
        }

        // Check gather buttons
        for (size_t i = 0; i < gatherButtons.size(); i++) {
            if (gatherButtons[i].Contains(x, y)) {
                gatherButtons[i].isPressed = true;
                HandleGatherButtonClick(static_cast<int>(i), game);
                return;
            }
        }

        // Check building buttons
        for (size_t i = 0; i < buildingButtons.size(); i++) {
            if (buildingButtons[i].Contains(x, y) && buildingButtons[i].isEnabled) {
                buildingButtons[i].isPressed = true;
                HandleBuildingButtonClick(static_cast<int>(i), game);
                return;
            }
        }
    }

    void HandleRightMouseDown(int x, int y) {
        rightMouseDown = true;
        if (upgradeTreeMenu.isVisible) {
            upgradeTreeMenu.StartPan(x, y);
        }
        else if (showMap) {
            mapView.StartPan(x, y);
        }
    }

    void HandleMouseUp() {
        mouseDown = false;
        for (auto& button : gatherButtons) button.isPressed = false;
        for (auto& button : buildingButtons) button.isPressed = false;
        upgradeTreeButton.isPressed = false;
        mapButton.isPressed = false;
    }

    void HandleRightMouseUp() {
        rightMouseDown = false;
        upgradeTreeMenu.StopPan();
        mapView.StopPan();
    }

    void HandleMouseMove(int x, int y) {
        mouseX = x;
        mouseY = y;
        upgradeTreeMenu.Pan(x, y);
        mapView.Pan(x, y);
    }

    void HandleMouseWheel(int delta) {
        if (showMap && mapView.Contains(mouseX, mouseY)) {
            mapView.Zoom(delta);
        }
    }

    void HandleKeyPress(WPARAM key, GameState& game) {
        if (key == 'U' || key == 'u') {
            upgradeTreeMenu.Toggle();
        }
        else if (key == 'M' || key == 'm') {
            // If in placement mode, cancel it
            if (game.placementMode) {
                game.CancelPlacement();
                mapView.showPlacementPreview = false;
                mapView.placementBuildingIndex = -1;
                clickFeedback = L"Placement cancelled";
                feedbackTimer = 1.5f;
            }
            showMap = !showMap;
        }
        else if (key == VK_ESCAPE) {
            // Cancel placement mode
            if (game.placementMode) {
                game.CancelPlacement();
                mapView.showPlacementPreview = false;
                mapView.placementBuildingIndex = -1;
                showMap = false;
                clickFeedback = L"Placement cancelled";
                feedbackTimer = 1.5f;
            }
        }
    }

    void Render(HDC hdc, int width, int height, const GameState& game, int fps) {
        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

        SolidBrush bgBrush(Color(255, 20, 20, 30));
        graphics.FillRectangle(&bgBrush, 0, 0, width, height);

        FontFamily fontFamily(L"Arial");
        Font titleFont(&fontFamily, 22, FontStyleBold, UnitPixel);
        Font resourceFont(&fontFamily, 16, FontStyleRegular, UnitPixel);
        Font smallFont(&fontFamily, 13, FontStyleRegular, UnitPixel);
        Font buttonFont(&fontFamily, 13, FontStyleBold, UnitPixel);
        Font tinyFont(&fontFamily, 10, FontStyleRegular, UnitPixel);

        // Render main game UI (only if not showing tree or map full-screen)
        if (!upgradeTreeMenu.isVisible) {
            RenderTitle(graphics, titleFont);
            RenderFPS(graphics, smallFont, fps);
            RenderResources(graphics, resourceFont, game);
            RenderPopulation(graphics, smallFont, game);
            RenderProductionRates(graphics, smallFont, game);
            RenderBuildings(graphics, resourceFont, smallFont, game);
            RenderButtons(graphics, buttonFont, tinyFont, game);
            RenderFeedback(graphics, resourceFont);

            // Render map LAST (on top) if visible
            if (showMap) {
                mapView.Render(graphics, game);
            }
        }

        // Always render tree menu if visible (overlays everything)
        upgradeTreeMenu.Render(hdc, width, height, game);
    }

private:
    void RenderTitle(Graphics& graphics, Font& font) {
        SolidBrush goldBrush(Color(255, 255, 215, 0));
        PointF titlePos(20.0f, 15.0f);
        graphics.DrawString(L"=== PROCEDURAL CIVILIZATION ===", -1, &font, titlePos, &goldBrush);
    }

    void RenderFPS(Graphics& graphics, Font& font, int fps) {
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        std::wstringstream fpsStream;
        fpsStream << L"FPS: " << fps;
        PointF fpsPos(10.0f, 650.0f);
        graphics.DrawString(fpsStream.str().c_str(), -1, &font, fpsPos, &whiteBrush);
    }

    void RenderResources(Graphics& graphics, Font& font, const GameState& game) {
        SolidBrush foodBrush(Color(255, 100, 255, 100));
        SolidBrush woodBrush(Color(255, 139, 69, 19));
        SolidBrush stoneBrush(Color(255, 128, 128, 128));
        SolidBrush goldBrush(Color(255, 255, 215, 0));

        float yPos = 70.0f;
        float xPos = 20.0f;

        auto renderResource = [&](ResourceType type, SolidBrush& brush) {
            auto it = game.resources.find(type);
            if (it != game.resources.end()) {
                std::wstringstream ss;
                ss << it->second.name << L": " << std::fixed << std::setprecision(1) << it->second.amount;
                PointF pos(xPos, yPos);
                graphics.DrawString(ss.str().c_str(), -1, &font, pos, &brush);
                yPos += 30.0f;
            }
            };

        renderResource(ResourceType::Food, foodBrush);
        renderResource(ResourceType::Wood, woodBrush);
        renderResource(ResourceType::Stone, stoneBrush);
        renderResource(ResourceType::Gold, goldBrush);
    }

    void RenderPopulation(Graphics& graphics, Font& font, const GameState& game) {
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        std::wstringstream popStream;
        popStream << L"Population: " << game.population.total << L"/" << game.population.maxPopulation;
        popStream << L" (Idle: " << game.population.idle << L")";
        PointF popPos(20.0f, 200.0f);
        graphics.DrawString(popStream.str().c_str(), -1, &font, popPos, &whiteBrush);
    }

    void RenderProductionRates(Graphics& graphics, Font& font, const GameState& game) {
        SolidBrush grayBrush(Color(255, 200, 200, 200));

        std::wstringstream prodStream;
        prodStream << L"Production/sec:";

        for (const auto& res : game.resources) {
            prodStream << L"\n  " << res.second.name << L": +"
                << std::fixed << std::setprecision(2) << res.second.perSecond;
        }

        RectF prodRect(20.0f, 230.0f, 200.0f, 120.0f);
        graphics.DrawString(prodStream.str().c_str(), -1, &font, prodRect, NULL, &grayBrush);
    }

    void RenderBuildings(Graphics& graphics, Font& headerFont, Font& smallFont, const GameState& game) {
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        PointF buildingHeaderPos(270.0f, 360.0f);
        graphics.DrawString(L"=== BUILDINGS ===", -1, &headerFont, buildingHeaderPos, &whiteBrush);
    }

    void RenderButtons(Graphics& graphics, Font& buttonFont, Font& tinyFont, const GameState& game) {
        // Upgrade tree button
        upgradeTreeButton.Render(graphics, buttonFont);

        // Map button
        mapButton.Render(graphics, buttonFont);

        for (auto& button : gatherButtons) {
            button.Render(graphics, buttonFont);
        }

        for (size_t i = 0; i < buildingButtons.size(); i++) {
            buildingButtons[i].Render(graphics, buttonFont);

            if (i < game.buildings.size() && game.IsBuildingUnlocked(static_cast<int>(i))) {
                auto cost = game.buildings[i].GetNextCost();
                std::wstringstream costStream;
                costStream << L"Cost: ";
                bool first = true;
                for (auto& c : cost) {
                    if (!first) costStream << L", ";
                    first = false;

                    std::wstring resName;
                    switch (c.first) {
                    case ResourceType::Food: resName = L"F"; break;
                    case ResourceType::Wood: resName = L"W"; break;
                    case ResourceType::Stone: resName = L"S"; break;
                    case ResourceType::Gold: resName = L"G"; break;
                    }

                    double displayCost = c.second * (1.0 - game.costReduction);
                    costStream << resName << L":" << (int)displayCost;
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
            PointF feedbackPos(270.0f, 310.0f);
            graphics.DrawString(clickFeedback.c_str(), -1, &font, feedbackPos, &feedbackBrush);
        }
    }
};