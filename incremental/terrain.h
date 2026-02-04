#pragma once
#include <vector>
#include <cmath>
#include <random>

// Simple Perlin Noise implementation
class PerlinNoise {
private:
    std::vector<int> permutation;

public:
    PerlinNoise(unsigned int seed = 0) {
        // Initialize permutation table
        permutation.resize(512);

        // Fill with values 0-255
        for (int i = 0; i < 256; i++) {
            permutation[i] = i;
        }

        // Shuffle using seed
        std::default_random_engine engine(seed);
        std::shuffle(permutation.begin(), permutation.begin() + 256, engine);

        // Duplicate for wrapping
        for (int i = 0; i < 256; i++) {
            permutation[256 + i] = permutation[i];
        }
    }

    double noise(double x, double y) {
        // Find unit grid cell containing point
        int X = (int)floor(x) & 255;
        int Y = (int)floor(y) & 255;

        // Get relative xy coordinates within cell
        x -= floor(x);
        y -= floor(y);

        // Compute fade curves
        double u = fade(x);
        double v = fade(y);

        // Hash coordinates of 4 corners
        int A = permutation[X] + Y;
        int AA = permutation[A];
        int AB = permutation[A + 1];
        int B = permutation[X + 1] + Y;
        int BA = permutation[B];
        int BB = permutation[B + 1];

        // Blend results from 4 corners
        double res = lerp(v,
            lerp(u, grad(permutation[AA], x, y), grad(permutation[BA], x - 1, y)),
            lerp(u, grad(permutation[AB], x, y - 1), grad(permutation[BB], x - 1, y - 1))
        );

        return (res + 1.0) / 2.0; // Normalize to 0-1
    }

    // Multi-octave noise for more natural terrain
    double octaveNoise(double x, double y, int octaves, double persistence) {
        double total = 0;
        double frequency = 1;
        double amplitude = 1;
        double maxValue = 0;

        for (int i = 0; i < octaves; i++) {
            total += noise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2;
        }

        return total / maxValue;
    }

private:
    double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }

    double grad(int hash, double x, double y) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};

// Terrain types
enum class TerrainType {
    Water,
    Plains,
    Forest,
    Hills,
    Mountains,
    Desert
};

// Terrain tile
struct TerrainTile {
    TerrainType type;
    double elevation;      // 0.0 to 1.0
    double moisture;       // 0.0 to 1.0
    double temperature;    // 0.0 to 1.0

    // Resource bonuses
    double foodBonus = 1.0;
    double woodBonus = 1.0;
    double stoneBonus = 1.0;
    double goldBonus = 1.0;

    TerrainTile() : type(TerrainType::Plains), elevation(0.5), moisture(0.5), temperature(0.5) {
        CalculateBonuses();
    }

    void CalculateBonuses() {
        // Set bonuses based on terrain type
        switch (type) {
        case TerrainType::Water:
            foodBonus = 0.5;
            woodBonus = 0.0;
            stoneBonus = 0.0;
            goldBonus = 0.0;
            break;

        case TerrainType::Plains:
            foodBonus = 1.5;
            woodBonus = 0.8;
            stoneBonus = 0.5;
            goldBonus = 0.3;
            break;

        case TerrainType::Forest:
            foodBonus = 1.0;
            woodBonus = 2.0;
            stoneBonus = 0.3;
            goldBonus = 0.2;
            break;

        case TerrainType::Hills:
            foodBonus = 0.8;
            woodBonus = 1.0;
            stoneBonus = 1.8;
            goldBonus = 1.2;
            break;

        case TerrainType::Mountains:
            foodBonus = 0.3;
            woodBonus = 0.5;
            stoneBonus = 2.5;
            goldBonus = 2.0;
            break;

        case TerrainType::Desert:
            foodBonus = 0.4;
            woodBonus = 0.2;
            stoneBonus = 1.2;
            goldBonus = 0.8;
            break;
        }
    }
};

// Terrain generator
class TerrainGenerator {
private:
    PerlinNoise elevationNoise;
    PerlinNoise moistureNoise;
    PerlinNoise temperatureNoise;

public:
    TerrainGenerator(unsigned int seed)
        : elevationNoise(seed),
        moistureNoise(seed + 1000),
        temperatureNoise(seed + 2000) {
    }

    TerrainTile GenerateTile(int x, int y) {
        TerrainTile tile;

        // Generate noise values with different frequencies
        double scale = 0.05; // Adjust for terrain feature size

        tile.elevation = elevationNoise.octaveNoise(x * scale, y * scale, 4, 0.5);
        tile.moisture = moistureNoise.octaveNoise(x * scale * 1.5, y * scale * 1.5, 3, 0.6);
        tile.temperature = temperatureNoise.octaveNoise(x * scale * 0.8, y * scale * 0.8, 2, 0.7);

        // Determine terrain type based on elevation and moisture
        tile.type = DetermineTerrainType(tile.elevation, tile.moisture, tile.temperature);
        tile.CalculateBonuses();

        return tile;
    }

private:
    TerrainType DetermineTerrainType(double elevation, double moisture, double temperature) {
        // Water
        if (elevation < 0.35) {
            return TerrainType::Water;
        }

        // Mountains
        if (elevation > 0.75) {
            return TerrainType::Mountains;
        }

        // Hills
        if (elevation > 0.60) {
            return TerrainType::Hills;
        }

        // Desert (low moisture, high temp)
        if (moisture < 0.3 && temperature > 0.6) {
            return TerrainType::Desert;
        }

        // Forest (high moisture)
        if (moisture > 0.55) {
            return TerrainType::Forest;
        }

        // Default to plains
        return TerrainType::Plains;
    }
};