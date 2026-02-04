#pragma once
#include "terrain.h"
#include <vector>
#include <map>

// World map with chunked loading
class WorldMap {
public:
    static const int CHUNK_SIZE = 16;
    static const int TILE_SIZE = 32;  // pixels per tile

private:
    struct Chunk {
        int chunkX, chunkY;
        std::vector<std::vector<TerrainTile>> tiles;

        Chunk() : chunkX(0), chunkY(0) {
            tiles.resize(CHUNK_SIZE, std::vector<TerrainTile>(CHUNK_SIZE));
        }

        Chunk(int cx, int cy) : chunkX(cx), chunkY(cy) {
            tiles.resize(CHUNK_SIZE, std::vector<TerrainTile>(CHUNK_SIZE));
        }
    };

    std::map<std::pair<int, int>, Chunk> chunks;
    TerrainGenerator generator;
    unsigned int worldSeed;

public:
    WorldMap(unsigned int seed = 12345) : generator(seed), worldSeed(seed) {}

    // Get or generate chunk
    Chunk& GetChunk(int chunkX, int chunkY) {
        auto key = std::make_pair(chunkX, chunkY);

        // If chunk doesn't exist, generate it
        if (chunks.find(key) == chunks.end()) {
            Chunk newChunk(chunkX, chunkY);

            // Generate all tiles in chunk
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    int worldX = chunkX * CHUNK_SIZE + x;
                    int worldY = chunkY * CHUNK_SIZE + y;
                    newChunk.tiles[y][x] = generator.GenerateTile(worldX, worldY);
                }
            }

            chunks[key] = newChunk;
        }

        return chunks[key];
    }

    // Get tile at world coordinates (safe version)
    TerrainTile GetTile(int worldX, int worldY) {
        int chunkX = worldX / CHUNK_SIZE;
        int chunkY = worldY / CHUNK_SIZE;

        if (worldX < 0) chunkX = (worldX - CHUNK_SIZE + 1) / CHUNK_SIZE;
        if (worldY < 0) chunkY = (worldY - CHUNK_SIZE + 1) / CHUNK_SIZE;

        int localX = worldX - chunkX * CHUNK_SIZE;
        int localY = worldY - chunkY * CHUNK_SIZE;

        // Ensure local coordinates are in valid range
        while (localX < 0) localX += CHUNK_SIZE;
        while (localY < 0) localY += CHUNK_SIZE;
        while (localX >= CHUNK_SIZE) localX -= CHUNK_SIZE;
        while (localY >= CHUNK_SIZE) localY -= CHUNK_SIZE;

        try {
            Chunk& chunk = GetChunk(chunkX, chunkY);
            if (localY >= 0 && localY < CHUNK_SIZE && localX >= 0 && localX < CHUNK_SIZE) {
                return chunk.tiles[localY][localX];
            }
        }
        catch (...) {
            // Return default tile if anything goes wrong
        }

        // Return default tile
        TerrainTile defaultTile;
        defaultTile.type = TerrainType::Plains;
        defaultTile.elevation = 0.5;
        defaultTile.moisture = 0.5;
        defaultTile.temperature = 0.5;
        defaultTile.CalculateBonuses();
        return defaultTile;
    }

    // Get chunks in view range
    std::vector<std::pair<int, int>> GetVisibleChunks(int centerChunkX, int centerChunkY, int range = 2) {
        std::vector<std::pair<int, int>> visible;

        for (int dy = -range; dy <= range; dy++) {
            for (int dx = -range; dx <= range; dx++) {
                visible.push_back(std::make_pair(centerChunkX + dx, centerChunkY + dy));
            }
        }

        return visible;
    }

    // Clear far chunks to save memory (optional)
    void UnloadDistantChunks(int centerChunkX, int centerChunkY, int maxDistance = 5) {
        std::vector<std::pair<int, int>> toRemove;

        for (const auto& pair : chunks) {
            int dx = abs(pair.first.first - centerChunkX);
            int dy = abs(pair.first.second - centerChunkY);

            if (dx > maxDistance || dy > maxDistance) {
                toRemove.push_back(pair.first);
            }
        }

        for (const auto& key : toRemove) {
            chunks.erase(key);
        }
    }

    // Get world seed
    unsigned int GetSeed() const { return worldSeed; }

    // Regenerate with new seed
    void Regenerate(unsigned int newSeed) {
        worldSeed = newSeed;
        generator = TerrainGenerator(newSeed);
        chunks.clear();
    }
};