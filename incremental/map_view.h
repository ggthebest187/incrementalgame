#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "world_map.h"
#include "game.h"
#include <map>

using namespace Gdiplus;

class MapView {
public:
    WorldMap worldMap;

    // Camera position (in world pixels)
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float zoom = 1.0f;

    // View bounds
    float viewX = 20.0f;
    float viewY = 80.0f;
    float viewWidth = 700.0f;
    float viewHeight = 550.0f;

    // Interaction
    bool isPanning = false;
    int panStartX = 0;
    int panStartY = 0;
    float panStartCamX = 0.0f;
    float panStartCamY = 0.0f;

    int hoveredTileX = -1;
    int hoveredTileY = -1;

    // For showing placement preview
    bool showPlacementPreview = false;
    int placementBuildingIndex = -1;

    MapView(unsigned int seed = 12345) : worldMap(seed) {
        cameraX = 0;
        cameraY = 0;
    }

    bool Contains(int mouseX, int mouseY) const {
        return mouseX >= viewX && mouseX <= viewX + viewWidth &&
            mouseY >= viewY && mouseY <= viewY + viewHeight;
    }

    void StartPan(int mouseX, int mouseY) {
        if (Contains(mouseX, mouseY)) {
            isPanning = true;
            panStartX = mouseX;
            panStartY = mouseY;
            panStartCamX = cameraX;
            panStartCamY = cameraY;
        }
    }

    void Pan(int mouseX, int mouseY) {
        if (isPanning) {
            cameraX = panStartCamX - (mouseX - panStartX);
            cameraY = panStartCamY - (mouseY - panStartY);
        }
    }

    void StopPan() {
        isPanning = false;
    }

    void Zoom(int delta) {
        float oldZoom = zoom;
        zoom += delta * 0.001f;

        if (zoom < 0.5f) zoom = 0.5f;
        if (zoom > 2.0f) zoom = 2.0f;
    }

    void UpdateHover(int mouseX, int mouseY) {
        if (!Contains(mouseX, mouseY)) {
            hoveredTileX = -1;
            hoveredTileY = -1;
            return;
        }

        float worldX = (mouseX - viewX) / zoom + cameraX;
        float worldY = (mouseY - viewY) / zoom + cameraY;

        hoveredTileX = (int)floor(worldX / WorldMap::TILE_SIZE);
        hoveredTileY = (int)floor(worldY / WorldMap::TILE_SIZE);
    }

    void Render(Graphics& graphics, const GameState& game) {
        // CRITICAL: Disable anti-aliasing for massive FPS boost
        graphics.SetSmoothingMode(SmoothingModeNone);
        graphics.SetTextRenderingHint(TextRenderingHintSystemDefault);

        // Set clipping region
        Region clipRegion(RectF(viewX, viewY, viewWidth, viewHeight));
        graphics.SetClip(&clipRegion);

        // Background
        SolidBrush bgBrush(Color(255, 15, 15, 20));
        graphics.FillRectangle(&bgBrush, (REAL)viewX, (REAL)viewY, (REAL)viewWidth, (REAL)viewHeight);

        // Calculate EXACTLY which tiles are visible
        float worldStartX = cameraX;
        float worldStartY = cameraY;
        float worldEndX = cameraX + viewWidth / zoom;
        float worldEndY = cameraY + viewHeight / zoom;

        int startTileX = (int)floor(worldStartX / WorldMap::TILE_SIZE);
        int startTileY = (int)floor(worldStartY / WorldMap::TILE_SIZE);
        int endTileX = (int)ceil(worldEndX / WorldMap::TILE_SIZE);
        int endTileY = (int)ceil(worldEndY / WorldMap::TILE_SIZE);

        // HARD LIMIT to prevent rendering too many tiles
        int tilesWide = endTileX - startTileX;
        int tilesHigh = endTileY - startTileY;

        if (tilesWide > 40) {
            endTileX = startTileX + 40;
            tilesWide = 40;
        }
        if (tilesHigh > 30) {
            endTileY = startTileY + 30;
            tilesHigh = 30;
        }

        // Render tiles
        int tilesRendered = 0;
        for (int ty = startTileY; ty < endTileY; ty++) {
            for (int tx = startTileX; tx < endTileX; tx++) {
                RenderTile(graphics, tx, ty);
                tilesRendered++;
            }
        }

        // Draw placement overlay if in placement mode
        if (showPlacementPreview && placementBuildingIndex >= 0) {
            DrawPlacementOverlay(graphics, game, startTileX, startTileY, endTileX, endTileY);
        }

        // Draw hovered tile highlight
        if (hoveredTileX != -1 && hoveredTileY != -1) {
            DrawTileHighlight(graphics, hoveredTileX, hoveredTileY, game);
        }

        graphics.ResetClip();

        // Re-enable anti-aliasing for UI
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

        // Border
        Pen borderPen(Color(255, 100, 100, 100), 2.0f);
        graphics.DrawRectangle(&borderPen, (REAL)viewX, (REAL)viewY, (REAL)viewWidth, (REAL)viewHeight);

        // Info overlay (includes tile count debug info)
        DrawInfoOverlay(graphics, game, tilesRendered);
    }

private:
    void RenderTile(Graphics& graphics, int tileX, int tileY) {
        // Screen position
        float screenX = viewX + (tileX * WorldMap::TILE_SIZE - cameraX) * zoom;
        float screenY = viewY + (tileY * WorldMap::TILE_SIZE - cameraY) * zoom;
        float size = WorldMap::TILE_SIZE * zoom;

        // Cull if off-screen
        if (screenX + size < viewX - 5 || screenX > viewX + viewWidth + 5 ||
            screenY + size < viewY - 5 || screenY > viewY + viewHeight + 5) {
            return;
        }

        // SAFE tile access with default fallback
        TerrainTile tile;
        tile.type = TerrainType::Plains;
        tile.elevation = 0.5;

        try {
            tile = worldMap.GetTile(tileX, tileY);
        }
        catch (...) {
            // If tile access fails, just use default
        }

        // Get color directly (simpler and safer)
        Color tileColor = GetTerrainColor(tile);

        // Add elevation shading
        int brightness = (int)((tile.elevation - 0.5) * 40);
        int r = Clamp(tileColor.GetR() + brightness, 0, 255);
        int g = Clamp(tileColor.GetG() + brightness, 0, 255);
        int b = Clamp(tileColor.GetB() + brightness, 0, 255);

        // Create brush and render
        SolidBrush tileBrush(Color(255, r, g, b));
        graphics.FillRectangle(&tileBrush, (REAL)screenX, (REAL)screenY, (REAL)size, (REAL)size);
    }

    Color GetTerrainColor(const TerrainTile& tile) {
        switch (tile.type) {
        case TerrainType::Water:
            return Color(255, 50, 100, 200);
        case TerrainType::Plains:
            return Color(255, 120, 180, 80);
        case TerrainType::Forest:
            return Color(255, 40, 120, 40);
        case TerrainType::Hills:
            return Color(255, 140, 120, 80);
        case TerrainType::Mountains:
            return Color(255, 100, 100, 100);
        case TerrainType::Desert:
            return Color(255, 210, 180, 100);
        default:
            return Color(255, 128, 128, 128);
        }
    }

    void DrawTileHighlight(Graphics& graphics, int tileX, int tileY, const GameState& game) {
        float screenX = viewX + (tileX * WorldMap::TILE_SIZE - cameraX) * zoom;
        float screenY = viewY + (tileY * WorldMap::TILE_SIZE - cameraY) * zoom;
        float size = WorldMap::TILE_SIZE * zoom;

        if (screenX + size < viewX || screenX > viewX + viewWidth ||
            screenY + size < viewY || screenY > viewY + viewHeight) {
            return;
        }

        // Color based on placement mode
        Color highlightColor = Color(220, 255, 255, 0);  // Yellow default

        if (showPlacementPreview && placementBuildingIndex >= 0) {
            TerrainTile tile;
            try {
                tile = worldMap.GetTile(tileX, tileY);
                double bonus = game.GetTileBonusForBuilding(placementBuildingIndex, tile);

                // Color code by bonus quality
                if (bonus >= 2.0) {
                    highlightColor = Color(220, 0, 255, 0);  // Excellent - bright green
                }
                else if (bonus >= 1.5) {
                    highlightColor = Color(220, 100, 255, 100);  // Good - green
                }
                else if (bonus >= 1.0) {
                    highlightColor = Color(220, 255, 255, 0);  // OK - yellow
                }
                else {
                    highlightColor = Color(220, 255, 50, 50);  // Bad - red
                }
            }
            catch (...) {}
        }

        Pen highlightPen(highlightColor, 3.0f);
        graphics.DrawRectangle(&highlightPen, (REAL)screenX, (REAL)screenY, (REAL)size, (REAL)size);

        // Show bonus text if in placement mode
        if (showPlacementPreview && placementBuildingIndex >= 0 && size > 15) {
            TerrainTile tile;
            try {
                tile = worldMap.GetTile(tileX, tileY);
                double bonus = game.GetTileBonusForBuilding(placementBuildingIndex, tile);

                FontFamily fontFamily(L"Arial");
                Font bonusFont(&fontFamily, 10, FontStyleBold, UnitPixel);

                std::wstringstream ss;
                ss << std::fixed << std::setprecision(1) << bonus << L"x";

                SolidBrush textBrush(Color(255, 255, 255, 255));
                SolidBrush bgBrush(Color(180, 0, 0, 0));

                // Background for text
                graphics.FillRectangle(&bgBrush, (int)(screenX + 2), (int)(screenY + 2), 30, 14);

                PointF textPos(screenX + 4, screenY + 2);
                graphics.DrawString(ss.str().c_str(), -1, &bonusFont, textPos, &textBrush);
            }
            catch (...) {}
        }
    }

    void DrawPlacementOverlay(Graphics& graphics, const GameState& game,
        int startTileX, int startTileY, int endTileX, int endTileY) {
        // Draw color-coded overlay on all visible tiles
        for (int ty = startTileY; ty < endTileY; ty++) {
            for (int tx = startTileX; tx < endTileX; tx++) {
                float screenX = viewX + (tx * WorldMap::TILE_SIZE - cameraX) * zoom;
                float screenY = viewY + (ty * WorldMap::TILE_SIZE - cameraY) * zoom;
                float size = WorldMap::TILE_SIZE * zoom;

                if (screenX + size < viewX - 5 || screenX > viewX + viewWidth + 5 ||
                    screenY + size < viewY - 5 || screenY > viewY + viewHeight + 5) {
                    continue;
                }

                TerrainTile tile;
                try {
                    tile = worldMap.GetTile(tx, ty);
                }
                catch (...) {
                    continue;
                }

                double bonus = game.GetTileBonusForBuilding(placementBuildingIndex, tile);

                // Semi-transparent overlay color
                Color overlayColor;
                if (bonus >= 2.0) {
                    overlayColor = Color(60, 0, 255, 0);  // Excellent
                }
                else if (bonus >= 1.5) {
                    overlayColor = Color(40, 100, 255, 100);  // Good
                }
                else if (bonus >= 1.2) {
                    overlayColor = Color(30, 255, 255, 0);  // OK
                }
                else if (bonus >= 1.0) {
                    overlayColor = Color(20, 255, 200, 100);  // Meh
                }
                else {
                    overlayColor = Color(60, 255, 0, 0);  // Bad
                }

                SolidBrush overlayBrush(overlayColor);
                graphics.FillRectangle(&overlayBrush, (REAL)screenX, (REAL)screenY, (REAL)size, (REAL)size);
            }
        }
    }

    void DrawInfoOverlay(Graphics& graphics, const GameState& game, int tilesRendered) {
        FontFamily fontFamily(L"Arial");
        Font smallFont(&fontFamily, 11, FontStyleRegular, UnitPixel);
        Font tinyFont(&fontFamily, 9, FontStyleRegular, UnitPixel);

        // Camera info with debug
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        std::wstringstream ss;
        ss << L"Zoom: " << (int)(zoom * 100) << L"% | Tiles: " << tilesRendered
            << L" | Cam: " << (int)cameraX << L"," << (int)cameraY;
        PointF infoPos(viewX + 10, viewY + viewHeight - 20);
        graphics.DrawString(ss.str().c_str(), -1, &tinyFont, infoPos, &whiteBrush);

        // Controls
        ss.str(L"");
        ss << L"Right-drag to pan | Scroll to zoom";
        PointF controlPos(viewX + 10, viewY + viewHeight - 35);
        graphics.DrawString(ss.str().c_str(), -1, &tinyFont, controlPos, &whiteBrush);

        // Tile info panel - only if hovering
        if (hoveredTileX == -1 || hoveredTileY == -1) return;

        TerrainTile tile;
        tile.type = TerrainType::Plains;
        tile.elevation = 0.5;
        tile.CalculateBonuses();

        try {
            tile = worldMap.GetTile(hoveredTileX, hoveredTileY);
        }
        catch (...) {
            return;
        }

        float panelX = viewX + viewWidth - 200;
        float panelY = viewY + 10;
        float panelWidth = 190;
        float panelHeight = 120;

        // Panel background
        SolidBrush panelBrush(Color(230, 20, 20, 30));
        graphics.FillRectangle(&panelBrush, (REAL)panelX, (REAL)panelY, (REAL)panelWidth, (REAL)panelHeight);

        Pen panelBorder(Color(255, 100, 100, 100), 1.0f);
        graphics.DrawRectangle(&panelBorder, (REAL)panelX, (REAL)panelY, (REAL)panelWidth, (REAL)panelHeight);

        // Tile info
        float yPos = panelY + 10;
        SolidBrush infoBrush(Color(255, 220, 220, 220));

        std::wstringstream info;
        info << L"Tile: " << hoveredTileX << L", " << hoveredTileY;
        graphics.DrawString(info.str().c_str(), -1, &smallFont,
            PointF(panelX + 10, yPos), &infoBrush);
        yPos += 18;

        std::wstring terrainName = GetTerrainName(tile.type);
        info.str(L"");
        info << L"Type: " << terrainName;
        graphics.DrawString(info.str().c_str(), -1, &smallFont,
            PointF(panelX + 10, yPos), &infoBrush);
        yPos += 20;

        // Resource bonuses
        SolidBrush bonusBrush(Color(255, 180, 255, 180));
        graphics.DrawString(L"Bonuses:", -1, &smallFont,
            PointF(panelX + 10, yPos), &infoBrush);
        yPos += 15;

        info.str(L"");
        info << L"  Food: " << std::fixed << std::setprecision(1) << tile.foodBonus << L"x";
        graphics.DrawString(info.str().c_str(), -1, &tinyFont,
            PointF(panelX + 15, yPos), &bonusBrush);
        yPos += 13;

        info.str(L"");
        info << L"  Wood: " << std::fixed << std::setprecision(1) << tile.woodBonus << L"x";
        graphics.DrawString(info.str().c_str(), -1, &tinyFont,
            PointF(panelX + 15, yPos), &bonusBrush);
        yPos += 13;

        info.str(L"");
        info << L"  Stone: " << std::fixed << std::setprecision(1) << tile.stoneBonus << L"x";
        graphics.DrawString(info.str().c_str(), -1, &tinyFont,
            PointF(panelX + 15, yPos), &bonusBrush);
        yPos += 13;

        info.str(L"");
        info << L"  Gold: " << std::fixed << std::setprecision(1) << tile.goldBonus << L"x";
        graphics.DrawString(info.str().c_str(), -1, &tinyFont,
            PointF(panelX + 15, yPos), &bonusBrush);
    }

    std::wstring GetTerrainName(TerrainType type) {
        switch (type) {
        case TerrainType::Water: return L"Water";
        case TerrainType::Plains: return L"Plains";
        case TerrainType::Forest: return L"Forest";
        case TerrainType::Hills: return L"Hills";
        case TerrainType::Mountains: return L"Mountains";
        case TerrainType::Desert: return L"Desert";
        default: return L"Unknown";
        }
    }

    int Clamp(int value, int min, int max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
};