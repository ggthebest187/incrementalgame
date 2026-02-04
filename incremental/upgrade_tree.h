#pragma once
#include "game.h"
#include <vector>

// Upgrade Tree Node
struct UpgradeNode {
    int upgradeIndex;  // Index into game.upgrades
    float x, y;        // Position in the tree
    std::vector<int> prerequisites;  // Indices of required upgrades
    std::vector<int> unlocks;        // Indices of upgrades this unlocks

    UpgradeNode(int idx, float xPos, float yPos)
        : upgradeIndex(idx), x(xPos), y(yPos) {
    }
};

// Upgrade Tree Manager
class UpgradeTree {
public:
    std::vector<UpgradeNode> nodes;

    void Initialize() {
        nodes.clear();

        // TIER 1 - Starting nodes (no prerequisites)
        // Row 1
        nodes.push_back(UpgradeNode(0, 100, 100));   // Agriculture (unlock farms)
        nodes.push_back(UpgradeNode(1, 250, 100));   // Forestry (unlock lumber mills)
        nodes.push_back(UpgradeNode(2, 400, 100));   // Mining (unlock quarries)
        nodes.push_back(UpgradeNode(4, 550, 100));   // Better Tools (2x click)

        // TIER 2 - Requires Tier 1
        // Row 2 - Production branches
        nodes.push_back(UpgradeNode(5, 100, 220));   // Farming Techniques (+50% food)
        nodes[4].prerequisites.push_back(0);  // Requires Agriculture
        nodes[0].unlocks.push_back(4);

        nodes.push_back(UpgradeNode(6, 250, 220));   // Sawmill Tech (+50% wood)
        nodes[5].prerequisites.push_back(1);  // Requires Forestry
        nodes[1].unlocks.push_back(5);

        nodes.push_back(UpgradeNode(7, 400, 220));   // Explosives (+50% stone)
        nodes[6].prerequisites.push_back(2);  // Requires Mining
        nodes[2].unlocks.push_back(6);

        // Click power branch
        nodes.push_back(UpgradeNode(21, 550, 220));  // Foraging Expert (5x food clicks)
        nodes[7].prerequisites.push_back(3);  // Requires Better Tools
        nodes[3].unlocks.push_back(7);

        nodes.push_back(UpgradeNode(22, 700, 220));  // Master Lumberjack (5x wood clicks)
        nodes[8].prerequisites.push_back(3);  // Requires Better Tools
        nodes[3].unlocks.push_back(8);

        // Population branch
        nodes.push_back(UpgradeNode(9, 850, 100));   // Healthcare (+5 pop)
        nodes.push_back(UpgradeNode(10, 850, 220));  // Immigration (pop growth)
        nodes[10].prerequisites.push_back(9);  // Immigration requires Healthcare
        nodes[9].unlocks.push_back(10);

        // TIER 2 - Special unlocks
        nodes.push_back(UpgradeNode(3, 175, 340));   // Construction (unlock houses)
        nodes[11].prerequisites.push_back(0);  // Requires Agriculture
        nodes[11].prerequisites.push_back(1);  // Requires Forestry
        nodes[0].unlocks.push_back(11);
        nodes[1].unlocks.push_back(11);

        nodes.push_back(UpgradeNode(8, 325, 340));   // Deep Mining (unlock mines)
        nodes[12].prerequisites.push_back(2);  // Requires Mining
        nodes[12].prerequisites.push_back(1);  // Requires Forestry
        nodes[2].unlocks.push_back(12);
        nodes[1].unlocks.push_back(12);

        // TIER 3 - Advanced upgrades
        // Row 3 - 2x production multipliers
        nodes.push_back(UpgradeNode(11, 100, 460));  // Irrigation (2x food)
        nodes[13].prerequisites.push_back(4);  // Requires Farming Techniques
        nodes[4].unlocks.push_back(13);

        nodes.push_back(UpgradeNode(12, 250, 460));  // Steel Axes (2x wood)
        nodes[14].prerequisites.push_back(5);  // Requires Sawmill Tech
        nodes[5].unlocks.push_back(14);

        nodes.push_back(UpgradeNode(13, 400, 460));  // Industrial Mining (2x stone)
        nodes[15].prerequisites.push_back(6);  // Requires Explosives
        nodes[6].unlocks.push_back(15);

        nodes.push_back(UpgradeNode(14, 475, 580));  // Gold Rush (2x gold)
        nodes[16].prerequisites.push_back(12); // Requires Deep Mining
        nodes[12].unlocks.push_back(16);

        // Mechanization - requires multiple tier 2 upgrades
        nodes.push_back(UpgradeNode(15, 250, 580));  // Mechanization (+25% all)
        nodes[17].prerequisites.push_back(4);  // Requires Farming Techniques
        nodes[17].prerequisites.push_back(5);  // Requires Sawmill Tech
        nodes[17].prerequisites.push_back(6);  // Requires Explosives
        nodes[4].unlocks.push_back(17);
        nodes[5].unlocks.push_back(17);
        nodes[6].unlocks.push_back(17);

        // Advanced tools
        nodes.push_back(UpgradeNode(16, 625, 340));  // Refined Tools (3x click)
        nodes[18].prerequisites.push_back(7);  // Requires Foraging Expert
        nodes[18].prerequisites.push_back(8);  // Requires Lumberjack
        nodes[7].unlocks.push_back(18);
        nodes[8].unlocks.push_back(18);

        // TIER 4 - End game
        nodes.push_back(UpgradeNode(17, 850, 340));  // Education (+10 pop)
        nodes[19].prerequisites.push_back(10); // Requires Immigration
        nodes[10].unlocks.push_back(19);

        nodes.push_back(UpgradeNode(18, 700, 460));  // Automation (20% cost reduction)
        nodes[20].prerequisites.push_back(17); // Requires Mechanization
        nodes[17].unlocks.push_back(20);

        nodes.push_back(UpgradeNode(19, 550, 700));  // Mass Production (+50% all)
        nodes[21].prerequisites.push_back(17); // Requires Mechanization
        nodes[21].prerequisites.push_back(20); // Requires Automation
        nodes[17].unlocks.push_back(21);
        nodes[20].unlocks.push_back(21);

        nodes.push_back(UpgradeNode(20, 700, 700));  // Hyper-Efficiency (2x all)
        nodes[22].prerequisites.push_back(21); // Requires Mass Production
        nodes[21].unlocks.push_back(22);

        nodes.push_back(UpgradeNode(23, 625, 580));  // Master Craftsman (10x click)
        nodes[23].prerequisites.push_back(18); // Requires Refined Tools
        nodes[18].unlocks.push_back(23);
    }

    bool IsUpgradeAvailable(int nodeIndex, const GameState& game) const {
        if (nodeIndex < 0 || nodeIndex >= nodes.size()) return false;

        const auto& node = nodes[nodeIndex];
        const auto& upgrade = game.upgrades[node.upgradeIndex];

        // Already purchased
        if (upgrade.purchased) return false;

        // Check if all prerequisites are met
        for (int prereqIdx : node.prerequisites) {
            const auto& prereqNode = nodes[prereqIdx];
            if (!game.upgrades[prereqNode.upgradeIndex].purchased) {
                return false;
            }
        }

        return true;
    }

    bool IsUpgradeLocked(int nodeIndex, const GameState& game) const {
        if (nodeIndex < 0 || nodeIndex >= nodes.size()) return true;

        const auto& node = nodes[nodeIndex];
        const auto& upgrade = game.upgrades[node.upgradeIndex];

        if (upgrade.purchased) return false;

        return !IsUpgradeAvailable(nodeIndex, game);
    }
};