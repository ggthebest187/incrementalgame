#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "game.h"
#include "upgrade_tree.h"

using namespace Gdiplus;

class UpgradeTreeMenu {
public:
    bool isVisible = false;
    UpgradeTree tree;

    // Camera/panning
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    bool isPanning = false;
    int panStartX = 0;
    int panStartY = 0;

    // Node rendering
    const float NODE_RADIUS = 40.0f;
    const float LINE_THICKNESS = 3.0f;

    int hoveredNode = -1;

    void Initialize() {
        tree.Initialize();
        // Center the view
        offsetX = 100.0f;
        offsetY = 50.0f;
    }

    void Toggle() {
        isVisible = !isVisible;
    }

    void StartPan(int mouseX, int mouseY) {
        isPanning = true;
        panStartX = mouseX;
        panStartY = mouseY;
    }

    void Pan(int mouseX, int mouseY) {
        if (isPanning) {
            offsetX += (mouseX - panStartX);
            offsetY += (mouseY - panStartY);
            panStartX = mouseX;
            panStartY = mouseY;
        }
    }

    void StopPan() {
        isPanning = false;
    }

    int GetNodeAt(int mouseX, int mouseY) const {
        for (size_t i = 0; i < tree.nodes.size(); i++) {
            float nodeX = tree.nodes[i].x + offsetX;
            float nodeY = tree.nodes[i].y + offsetY;

            float dx = mouseX - nodeX;
            float dy = mouseY - nodeY;
            float distSquared = dx * dx + dy * dy;

            if (distSquared <= NODE_RADIUS * NODE_RADIUS) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void Render(HDC hdc, int width, int height, const GameState& game) {
        if (!isVisible) return;

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

        // Semi-transparent background overlay
        SolidBrush overlayBrush(Color(230, 15, 15, 25));
        graphics.FillRectangle(&overlayBrush, 0, 0, width, height);

        // Title
        FontFamily fontFamily(L"Arial");
        Font titleFont(&fontFamily, 32, FontStyleBold, UnitPixel);
        Font nodeFont(&fontFamily, 11, FontStyleBold, UnitPixel);
        Font infoFont(&fontFamily, 14, FontStyleRegular, UnitPixel);

        SolidBrush titleBrush(Color(255, 255, 215, 0));
        PointF titlePos(width / 2.0f - 200.0f, 10.0f);
        graphics.DrawString(L"UPGRADE TREE", -1, &titleFont, titlePos, &titleBrush);

        // Instructions
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        Font smallFont(&fontFamily, 12, FontStyleRegular, UnitPixel);
        PointF instructPos(10.0f, 10.0f);
        graphics.DrawString(L"Right-click & drag to pan | Click nodes to purchase | Press 'U' to close",
            -1, &smallFont, instructPos, &whiteBrush);

        // Draw connections first (so they're behind nodes)
        DrawConnections(graphics, game);

        // Draw nodes
        DrawNodes(graphics, game, nodeFont);

        // Draw info panel for hovered node
        if (hoveredNode >= 0 && hoveredNode < tree.nodes.size()) {
            DrawNodeInfo(graphics, hoveredNode, game, infoFont);
        }
    }

private:
    void DrawConnections(Graphics& graphics, const GameState& game) {
        for (size_t i = 0; i < tree.nodes.size(); i++) {
            const auto& node = tree.nodes[i];

            float x1 = node.x + offsetX;
            float y1 = node.y + offsetY;

            // Draw lines to all unlocked nodes
            for (int unlockIdx : node.unlocks) {
                const auto& unlockNode = tree.nodes[unlockIdx];
                float x2 = unlockNode.x + offsetX;
                float y2 = unlockNode.y + offsetY;

                // Determine line color based on state
                Color lineColor;
                if (game.upgrades[node.upgradeIndex].purchased) {
                    // This node is purchased
                    if (game.upgrades[unlockNode.upgradeIndex].purchased) {
                        lineColor = Color(200, 50, 200, 50);  // Both purchased - green
                    }
                    else if (tree.IsUpgradeAvailable(unlockIdx, game)) {
                        lineColor = Color(200, 100, 200, 255);  // Available - bright blue
                    }
                    else {
                        lineColor = Color(150, 100, 100, 100);  // Locked - gray
                    }
                }
                else {
                    lineColor = Color(100, 80, 80, 80);  // Source not purchased - dark gray
                }

                Pen linePen(lineColor, LINE_THICKNESS);
                graphics.DrawLine(&linePen, x1, y1, x2, y2);
            }
        }
    }

    void DrawNodes(Graphics& graphics, const GameState& game, Font& font) {
        for (size_t i = 0; i < tree.nodes.size(); i++) {
            const auto& node = tree.nodes[i];
            const auto& upgrade = game.upgrades[node.upgradeIndex];

            float x = node.x + offsetX;
            float y = node.y + offsetY;

            // Determine node color
            Color nodeColor;
            Color borderColor;
            Color textColor = Color(255, 255, 255, 255);

            if (upgrade.purchased) {
                // Purchased - green
                nodeColor = Color(255, 40, 150, 40);
                borderColor = Color(255, 80, 255, 80);
            }
            else if (tree.IsUpgradeAvailable(static_cast<int>(i), game)) {
                if (game.CanAffordUpgrade(node.upgradeIndex)) {
                    // Available and affordable - bright gold
                    nodeColor = Color(255, 200, 180, 50);
                    borderColor = Color(255, 255, 215, 0);
                }
                else {
                    // Available but can't afford - blue
                    nodeColor = Color(255, 60, 100, 180);
                    borderColor = Color(255, 100, 150, 255);
                }
            }
            else {
                // Locked - dark gray
                nodeColor = Color(255, 40, 40, 40);
                borderColor = Color(255, 80, 80, 80);
                textColor = Color(255, 120, 120, 120);
            }

            // Highlight if hovered
            if (static_cast<int>(i) == hoveredNode) {
                borderColor = Color(255, 255, 255, 255);
            }

            // Draw node circle
            SolidBrush nodeBrush(nodeColor);
            graphics.FillEllipse(&nodeBrush, x - NODE_RADIUS, y - NODE_RADIUS,
                NODE_RADIUS * 2, NODE_RADIUS * 2);

            // Draw border
            Pen borderPen(borderColor, 3.0f);
            graphics.DrawEllipse(&borderPen, x - NODE_RADIUS, y - NODE_RADIUS,
                NODE_RADIUS * 2, NODE_RADIUS * 2);

            // Draw tier number in small text at top of node
            SolidBrush tierBrush(Color(255, 200, 200, 200));
            Font tinyFont(L"Arial", 9);
            std::wstringstream tierStr;
            tierStr << L"T" << upgrade.tier;
            RectF tierRect(x - NODE_RADIUS, y - NODE_RADIUS + 5, NODE_RADIUS * 2, 15);
            StringFormat tierFormat;
            tierFormat.SetAlignment(StringAlignmentCenter);
            graphics.DrawString(tierStr.str().c_str(), -1, &tinyFont, tierRect, &tierFormat, &tierBrush);

            // Draw abbreviated name
            SolidBrush textBrush(textColor);
            std::wstring shortName = GetShortName(upgrade.name);

            RectF textRect(x - NODE_RADIUS + 2, y - 8, NODE_RADIUS * 2 - 4, NODE_RADIUS);
            StringFormat format;
            format.SetAlignment(StringAlignmentCenter);
            format.SetLineAlignment(StringAlignmentCenter);
            graphics.DrawString(shortName.c_str(), -1, &font, textRect, &format, &textBrush);

            // Draw checkmark if purchased
            if (upgrade.purchased) {
                Font checkFont(L"Arial", 20, FontStyleBold);
                SolidBrush checkBrush(Color(255, 255, 255, 255));
                RectF checkRect(x - NODE_RADIUS, y + 5, NODE_RADIUS * 2, NODE_RADIUS);
                graphics.DrawString(L"✓", -1, &checkFont, checkRect, &format, &checkBrush);
            }
        }
    }

    void DrawNodeInfo(Graphics& graphics, int nodeIdx, const GameState& game, Font& font) {
        const auto& node = tree.nodes[nodeIdx];
        const auto& upgrade = game.upgrades[node.upgradeIndex];

        // Info panel on the right side
        float panelX = 720.0f;
        float panelY = 80.0f;
        float panelWidth = 360.0f;
        float panelHeight = 500.0f;

        // Background
        SolidBrush bgBrush(Color(240, 20, 20, 30));
        graphics.FillRectangle(&bgBrush, panelX, panelY, panelWidth, panelHeight);

        // Border
        Pen borderPen(Color(255, 100, 100, 100), 2.0f);
        graphics.DrawRectangle(&borderPen, panelX, panelY, panelWidth, panelHeight);

        // Content
        SolidBrush textBrush(Color(255, 255, 255, 255));
        float yPos = panelY + 20;

        // Name
        Font nameFont(L"Arial", 18, FontStyleBold);
        RectF nameRect(panelX + 10, yPos, panelWidth - 20, 40);
        graphics.DrawString(upgrade.name.c_str(), -1, &nameFont, nameRect, NULL, &textBrush);
        yPos += 50;

        // Tier
        std::wstringstream tierStr;
        tierStr << L"Tier " << upgrade.tier;
        PointF tierPos(panelX + 10, yPos);
        SolidBrush tierBrush(Color(255, 200, 200, 200));
        graphics.DrawString(tierStr.str().c_str(), -1, &font, tierPos, &tierBrush);
        yPos += 30;

        // Description
        RectF descRect(panelX + 10, yPos, panelWidth - 20, 60);
        Font descFont(L"Arial", 14, FontStyleItalic);
        SolidBrush descBrush(Color(255, 220, 220, 220));
        graphics.DrawString(upgrade.description.c_str(), -1, &descFont, descRect, NULL, &descBrush);
        yPos += 80;

        // Cost
        if (!upgrade.purchased) {
            graphics.DrawString(L"Cost:", -1, &font, PointF(panelX + 10, yPos), &textBrush);
            yPos += 25;

            for (const auto& cost : upgrade.cost) {
                std::wstringstream costStr;
                std::wstring resName;
                Color resColor;

                switch (cost.first) {
                case ResourceType::Food:
                    resName = L"Food";
                    resColor = Color(255, 100, 255, 100);
                    break;
                case ResourceType::Wood:
                    resName = L"Wood";
                    resColor = Color(255, 139, 69, 19);
                    break;
                case ResourceType::Stone:
                    resName = L"Stone";
                    resColor = Color(255, 128, 128, 128);
                    break;
                case ResourceType::Gold:
                    resName = L"Gold";
                    resColor = Color(255, 255, 215, 0);
                    break;
                }

                costStr << L"  " << resName << L": " << (int)cost.second;

                // Check if affordable
                bool canAfford = game.resources.at(cost.first).amount >= cost.second;
                SolidBrush costBrush(canAfford ? resColor : Color(255, 150, 50, 50));

                graphics.DrawString(costStr.str().c_str(), -1, &font,
                    PointF(panelX + 20, yPos), &costBrush);
                yPos += 25;
            }
        }
        else {
            SolidBrush purchasedBrush(Color(255, 80, 255, 80));
            Font purchasedFont(L"Arial", 16, FontStyleBold);
            graphics.DrawString(L"✓ PURCHASED", -1, &purchasedFont,
                PointF(panelX + 10, yPos), &purchasedBrush);
        }
        yPos += 20;

        // Prerequisites
        if (!node.prerequisites.empty()) {
            graphics.DrawString(L"Requires:", -1, &font, PointF(panelX + 10, yPos), &textBrush);
            yPos += 25;

            for (int prereqIdx : node.prerequisites) {
                const auto& prereqNode = tree.nodes[prereqIdx];
                const auto& prereqUpgrade = game.upgrades[prereqNode.upgradeIndex];

                std::wstring prereqText = L"  • " + prereqUpgrade.name;
                if (prereqUpgrade.purchased) {
                    prereqText += L" ✓";
                }

                SolidBrush prereqBrush(prereqUpgrade.purchased ?
                    Color(255, 80, 255, 80) : Color(255, 255, 100, 100));

                RectF prereqRect(panelX + 20, yPos, panelWidth - 40, 25);
                graphics.DrawString(prereqText.c_str(), -1, &font, prereqRect, NULL, &prereqBrush);
                yPos += 25;
            }
        }
        yPos += 10;

        // Unlocks
        if (!node.unlocks.empty()) {
            graphics.DrawString(L"Unlocks:", -1, &font, PointF(panelX + 10, yPos), &textBrush);
            yPos += 25;

            for (int unlockIdx : node.unlocks) {
                if (unlockIdx >= tree.nodes.size()) continue;
                const auto& unlockNode = tree.nodes[unlockIdx];
                const auto& unlockUpgrade = game.upgrades[unlockNode.upgradeIndex];

                std::wstring unlockText = L"  → " + unlockUpgrade.name;

                SolidBrush unlockBrush(Color(255, 180, 180, 255));
                RectF unlockRect(panelX + 20, yPos, panelWidth - 40, 25);
                graphics.DrawString(unlockText.c_str(), -1, &font, unlockRect, NULL, &unlockBrush);
                yPos += 25;
            }
        }
    }

    std::wstring GetShortName(const std::wstring& fullName) {
        // Abbreviate long names to fit in nodes
        if (fullName.length() <= 12) return fullName;

        // Special abbreviations
        if (fullName == L"Agriculture") return L"Farm\nUnlock";
        if (fullName == L"Forestry") return L"Lumber\nUnlock";
        if (fullName == L"Mining") return L"Quarry\nUnlock";
        if (fullName == L"Construction") return L"House\nUnlock";
        if (fullName == L"Better Tools") return L"Better\nTools";
        if (fullName == L"Farming Techniques") return L"Farm\nTech";
        if (fullName == L"Sawmill Technology") return L"Sawmill\nTech";
        if (fullName == L"Explosives") return L"Explo-\nsives";
        if (fullName == L"Deep Mining") return L"Deep\nMining";
        if (fullName == L"Healthcare") return L"Health-\ncare";
        if (fullName == L"Immigration") return L"Immigra-\ntion";
        if (fullName == L"Irrigation") return L"Irriga-\ntion";
        if (fullName == L"Steel Axes") return L"Steel\nAxes";
        if (fullName == L"Industrial Mining") return L"Indust.\nMining";
        if (fullName == L"Gold Rush") return L"Gold\nRush";
        if (fullName == L"Mechanization") return L"Mecha-\nnization";
        if (fullName == L"Refined Tools") return L"Refined\nTools";
        if (fullName == L"Education System") return L"Educa-\ntion";
        if (fullName == L"Automation") return L"Auto-\nmation";
        if (fullName == L"Mass Production") return L"Mass\nProd";
        if (fullName == L"Hyper-Efficiency") return L"Hyper\nEffic.";
        if (fullName == L"Master Craftsman") return L"Master\nCraft";
        if (fullName == L"Foraging Expert") return L"Food\nExpert";
        if (fullName == L"Master Lumberjack") return L"Wood\nExpert";

        // Default: truncate
        return fullName.substr(0, 10) + L"...";
    }
};