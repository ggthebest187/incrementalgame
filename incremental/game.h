#pragma once
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include "terrain.h"

// Resource types
enum class ResourceType {
    Food,
    Wood,
    Stone,
    Gold
};

// Resource info
struct ResourceInfo {
    std::wstring name;
    double amount;
    double perSecond;
    double clickPower;  // How much you get per click

    ResourceInfo() : amount(0.0), perSecond(0.0), clickPower(0.1) {}
    ResourceInfo(const std::wstring& n, double amt = 0.0, double ps = 0.0, double cp = 0.1)
        : name(n), amount(amt), perSecond(ps), clickPower(cp) {
    }
};

// Population info
struct Population {
    int total = 3;           // Total population
    int workers = 0;         // Workers assigned to buildings
    int idle = 3;            // Idle population
    double growthRate = 0.0; // Population per second
    int maxPopulation = 10;  // Cap on population

    void Update(float deltaTime) {
        if (total < maxPopulation) {
            double growth = growthRate * deltaTime;
            if (growth >= 1.0) {
                int newPop = (int)growth;
                total += newPop;
                idle += newPop;
                if (total > maxPopulation) {
                    int excess = total - maxPopulation;
                    total = maxPopulation;
                    idle -= excess;
                    if (idle < 0) idle = 0;
                }
            }
        }
    }
};

// Upgrade type
struct Upgrade {
    std::wstring name;
    std::wstring description;
    std::map<ResourceType, double> cost;
    bool purchased = false;
    int tier = 1;  // For organizing upgrades

    // Upgrade effects
    enum class UpgradeType {
        FoodProductionMultiplier,
        WoodProductionMultiplier,
        StoneProductionMultiplier,
        GoldProductionMultiplier,
        AllProductionMultiplier,
        FoodClickPower,
        WoodClickPower,
        StoneClickPower,
        GoldClickPower,
        AllClickPower,
        PopulationGrowth,
        PopulationCap,
        UnlockFarm,
        UnlockLumberMill,
        UnlockQuarry,
        UnlockMine,
        UnlockHouse,
        CostReduction,
        WorkerEfficiency,
        AutoGather
    };

    UpgradeType type;
    double effectValue;  // The multiplier or value of the effect

    Upgrade() : tier(1), type(UpgradeType::AllProductionMultiplier), effectValue(1.0) {}
};

// Building type
struct BuildingType {
    std::wstring name;
    std::wstring description;
    std::map<ResourceType, double> cost;
    std::map<ResourceType, double> production;
    int baseCount;

    BuildingType() : baseCount(0) {}
};

// Building instance
struct Building {
    const BuildingType* type;
    int count;

    // Placement info
    struct Placement {
        int tileX = -1;
        int tileY = -1;
        double bonus = 1.0;  // Tile bonus multiplier
        bool isPlaced = false;
    };

    std::vector<Placement> placements;  // Each building can be placed

    Building(const BuildingType* t, int c = 0) : type(t), count(c) {}

    std::map<ResourceType, double> GetNextCost() const {
        std::map<ResourceType, double> nextCost;
        for (const auto& cost : type->cost) {
            double scaledCost = cost.second * pow(1.15, count);
            nextCost[cost.first] = scaledCost;
        }
        return nextCost;
    }

    std::map<ResourceType, double> GetTotalProduction() const {
        std::map<ResourceType, double> totalProd;

        // If no placements, use base production
        if (placements.empty()) {
            for (const auto& prod : type->production) {
                totalProd[prod.first] = prod.second * count;
            }
        }
        else {
            // Sum production from all placed buildings with their bonuses
            for (const auto& placement : placements) {
                if (placement.isPlaced) {
                    for (const auto& prod : type->production) {
                        totalProd[prod.first] += prod.second * placement.bonus;
                    }
                }
            }
        }

        return totalProd;
    }
};

// Game state
class GameState {
public:
    std::map<ResourceType, ResourceInfo> resources;
    Population population;
    std::vector<Building> buildings;
    std::vector<BuildingType> buildingTypes;
    std::vector<Upgrade> upgrades;
    std::map<int, bool> buildingUnlocked;

    // Building placement
    bool placementMode = false;
    int selectedBuildingType = -1;

    double productionMultiplier = 1.0;
    double clickPowerMultiplier = 1.0;
    double costReduction = 0.0;

    float gameTime;

    GameState() : gameTime(0.0f) {
        InitializeResources();
        InitializeBuildingTypes();
        InitializeBuildings();
        InitializeUpgrades();
        InitializeUnlocks();
    }

    void InitializeResources() {
        resources[ResourceType::Food] = ResourceInfo(L"Food", 0.0, 0.0, 0.1);
        resources[ResourceType::Wood] = ResourceInfo(L"Wood", 0.0, 0.0, 0.05);
        resources[ResourceType::Stone] = ResourceInfo(L"Stone", 0.0, 0.0, 0.03);
        resources[ResourceType::Gold] = ResourceInfo(L"Gold", 0.0, 0.0, 0.01);
    }

    void InitializeUnlocks() {
        for (size_t i = 0; i < buildingTypes.size(); i++) {
            buildingUnlocked[static_cast<int>(i)] = false;
        }
    }

    void InitializeBuildingTypes() {
        BuildingType farm;
        farm.name = L"Farm";
        farm.description = L"Produces food";
        farm.cost[ResourceType::Wood] = 10.0;
        farm.production[ResourceType::Food] = 2.0;
        farm.baseCount = 0;
        buildingTypes.push_back(farm);

        BuildingType lumberMill;
        lumberMill.name = L"Lumber Mill";
        lumberMill.description = L"Produces wood";
        lumberMill.cost[ResourceType::Food] = 15.0;
        lumberMill.cost[ResourceType::Stone] = 5.0;
        lumberMill.production[ResourceType::Wood] = 1.5;
        lumberMill.baseCount = 0;
        buildingTypes.push_back(lumberMill);

        BuildingType quarry;
        quarry.name = L"Quarry";
        quarry.description = L"Produces stone";
        quarry.cost[ResourceType::Wood] = 20.0;
        quarry.cost[ResourceType::Food] = 10.0;
        quarry.production[ResourceType::Stone] = 1.0;
        quarry.baseCount = 0;
        buildingTypes.push_back(quarry);

        BuildingType mine;
        mine.name = L"Mine";
        mine.description = L"Produces gold";
        mine.cost[ResourceType::Wood] = 50.0;
        mine.cost[ResourceType::Stone] = 30.0;
        mine.cost[ResourceType::Food] = 25.0;
        mine.production[ResourceType::Gold] = 0.5;
        mine.baseCount = 0;
        buildingTypes.push_back(mine);

        BuildingType house;
        house.name = L"House";
        house.description = L"Boosts production";
        house.cost[ResourceType::Wood] = 30.0;
        house.cost[ResourceType::Stone] = 15.0;
        house.production[ResourceType::Food] = 0.5;
        house.baseCount = 0;
        buildingTypes.push_back(house);
    }

    void InitializeBuildings() {
        for (const auto& type : buildingTypes) {
            buildings.push_back(Building(&type, type.baseCount));
        }
    }

    void InitializeUpgrades();  // Defined below due to length

    bool CanAffordUpgrade(int upgradeIndex) const {
        if (upgradeIndex < 0 || upgradeIndex >= upgrades.size()) return false;
        if (upgrades[upgradeIndex].purchased) return false;

        for (const auto& costItem : upgrades[upgradeIndex].cost) {
            auto it = resources.find(costItem.first);
            if (it == resources.end() || it->second.amount < costItem.second) {
                return false;
            }
        }
        return true;
    }

    bool PurchaseUpgrade(int upgradeIndex) {
        if (!CanAffordUpgrade(upgradeIndex)) return false;

        for (const auto& costItem : upgrades[upgradeIndex].cost) {
            resources[costItem.first].amount -= costItem.second;
        }

        upgrades[upgradeIndex].purchased = true;
        ApplyUpgradeEffects(upgrades[upgradeIndex]);

        return true;
    }

    void ApplyUpgradeEffects(const Upgrade& upgrade) {
        switch (upgrade.type) {
        case Upgrade::UpgradeType::UnlockFarm:
            buildingUnlocked[0] = true;
            break;
        case Upgrade::UpgradeType::UnlockLumberMill:
            buildingUnlocked[1] = true;
            break;
        case Upgrade::UpgradeType::UnlockQuarry:
            buildingUnlocked[2] = true;
            break;
        case Upgrade::UpgradeType::UnlockMine:
            buildingUnlocked[3] = true;
            break;
        case Upgrade::UpgradeType::UnlockHouse:
            buildingUnlocked[4] = true;
            break;
        case Upgrade::UpgradeType::PopulationCap:
            population.maxPopulation += (int)upgrade.effectValue;
            break;
        case Upgrade::UpgradeType::PopulationGrowth:
            population.growthRate += upgrade.effectValue;
            break;
        case Upgrade::UpgradeType::CostReduction:
            costReduction += upgrade.effectValue;
            if (costReduction > 0.9) costReduction = 0.9;
            break;
        default:
            break;
        }

        RecalculateMultipliers();
        RecalculateProduction();
    }

    void RecalculateMultipliers() {
        productionMultiplier = 1.0;
        clickPowerMultiplier = 1.0;

        std::map<ResourceType, double> resourceProdMult;
        std::map<ResourceType, double> resourceClickMult;

        for (ResourceType rt : {ResourceType::Food, ResourceType::Wood, ResourceType::Stone, ResourceType::Gold}) {
            resourceProdMult[rt] = 1.0;
            resourceClickMult[rt] = 1.0;
        }

        for (const auto& upgrade : upgrades) {
            if (!upgrade.purchased) continue;

            switch (upgrade.type) {
            case Upgrade::UpgradeType::AllProductionMultiplier:
                productionMultiplier *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::AllClickPower:
                clickPowerMultiplier *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::FoodProductionMultiplier:
                resourceProdMult[ResourceType::Food] *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::WoodProductionMultiplier:
                resourceProdMult[ResourceType::Wood] *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::StoneProductionMultiplier:
                resourceProdMult[ResourceType::Stone] *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::GoldProductionMultiplier:
                resourceProdMult[ResourceType::Gold] *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::FoodClickPower:
                resourceClickMult[ResourceType::Food] *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::WoodClickPower:
                resourceClickMult[ResourceType::Wood] *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::StoneClickPower:
                resourceClickMult[ResourceType::Stone] *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::GoldClickPower:
                resourceClickMult[ResourceType::Gold] *= upgrade.effectValue;
                break;
            default:
                break;
            }
        }

        for (auto& res : resources) {
            double baseClick = 0.1;
            if (res.first == ResourceType::Wood) baseClick = 0.05;
            else if (res.first == ResourceType::Stone) baseClick = 0.03;
            else if (res.first == ResourceType::Gold) baseClick = 0.01;

            res.second.clickPower = baseClick * clickPowerMultiplier * resourceClickMult[res.first];
        }

        RecalculateProduction();
    }

    bool IsBuildingUnlocked(int buildingIndex) const {
        auto it = buildingUnlocked.find(buildingIndex);
        if (it == buildingUnlocked.end()) return false;
        return it->second;
    }

    bool CanAfford(int buildingIndex) const {
        if (buildingIndex < 0 || buildingIndex >= buildings.size()) return false;
        if (!IsBuildingUnlocked(buildingIndex)) return false;

        auto cost = buildings[buildingIndex].GetNextCost();
        for (auto& costItem : cost) {
            double reducedCost = costItem.second * (1.0 - costReduction);
            auto it = resources.find(costItem.first);
            if (it == resources.end() || it->second.amount < reducedCost) {
                return false;
            }
        }
        return true;
    }

    bool PurchaseBuilding(int buildingIndex) {
        if (!CanAfford(buildingIndex)) return false;

        // Enter placement mode - don't deduct resources yet
        placementMode = true;
        selectedBuildingType = buildingIndex;

        return true;
    }

    bool PlaceBuilding(int buildingIndex, int tileX, int tileY, double tileBonus) {
        if (buildingIndex < 0 || buildingIndex >= buildings.size()) return false;
        if (!CanAfford(buildingIndex)) return false;

        // Deduct costs
        auto cost = buildings[buildingIndex].GetNextCost();
        for (auto& costItem : cost) {
            double reducedCost = costItem.second * (1.0 - costReduction);
            resources[costItem.first].amount -= reducedCost;
        }

        // Add building with placement
        buildings[buildingIndex].count++;
        Building::Placement placement;
        placement.tileX = tileX;
        placement.tileY = tileY;
        placement.bonus = tileBonus;
        placement.isPlaced = true;
        buildings[buildingIndex].placements.push_back(placement);

        // Exit placement mode
        placementMode = false;
        selectedBuildingType = -1;

        RecalculateProduction();

        return true;
    }

    void CancelPlacement() {
        placementMode = false;
        selectedBuildingType = -1;
    }

    // Get the best resource bonus from a tile for a building type
    double GetTileBonusForBuilding(int buildingIndex, const TerrainTile& tile) const {
        if (buildingIndex < 0 || buildingIndex >= buildings.size()) return 1.0;

        const auto& building = buildings[buildingIndex];
        if (!building.type) return 1.0;

        // Find which resource this building produces most
        double maxBonus = 1.0;

        for (const auto& prod : building.type->production) {
            double bonus = 1.0;

            switch (prod.first) {
            case ResourceType::Food:
                bonus = tile.foodBonus;
                break;
            case ResourceType::Wood:
                bonus = tile.woodBonus;
                break;
            case ResourceType::Stone:
                bonus = tile.stoneBonus;
                break;
            case ResourceType::Gold:
                bonus = tile.goldBonus;
                break;
            }

            if (bonus > maxBonus) maxBonus = bonus;
        }

        return maxBonus;
    }

    void RecalculateProduction() {
        resources[ResourceType::Food].perSecond = 0.0;
        resources[ResourceType::Wood].perSecond = 0.0;
        resources[ResourceType::Stone].perSecond = 0.0;
        resources[ResourceType::Gold].perSecond = 0.0;

        for (const auto& building : buildings) {
            auto production = building.GetTotalProduction();
            for (const auto& prod : production) {
                resources[prod.first].perSecond += prod.second;
            }
        }

        std::map<ResourceType, double> resourceMult;
        for (ResourceType rt : {ResourceType::Food, ResourceType::Wood, ResourceType::Stone, ResourceType::Gold}) {
            resourceMult[rt] = 1.0;
        }

        for (const auto& upgrade : upgrades) {
            if (!upgrade.purchased) continue;

            switch (upgrade.type) {
            case Upgrade::UpgradeType::FoodProductionMultiplier:
                resourceMult[ResourceType::Food] *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::WoodProductionMultiplier:
                resourceMult[ResourceType::Wood] *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::StoneProductionMultiplier:
                resourceMult[ResourceType::Stone] *= upgrade.effectValue;
                break;
            case Upgrade::UpgradeType::GoldProductionMultiplier:
                resourceMult[ResourceType::Gold] *= upgrade.effectValue;
                break;
            default:
                break;
            }
        }

        for (auto& res : resources) {
            res.second.perSecond *= productionMultiplier * resourceMult[res.first];
        }
    }

    void Update(float deltaTime) {
        gameTime += deltaTime;
        population.Update(deltaTime);

        for (auto& resource : resources) {
            resource.second.amount += resource.second.perSecond * deltaTime;
            if (resource.second.amount < 0.0) {
                resource.second.amount = 0.0;
            }
        }
    }

    void GatherResource(ResourceType type) {
        resources[type].amount += resources[type].clickPower;
    }
};

// Initialize upgrades (defined here due to length)
inline void GameState::InitializeUpgrades() {
    // TIER 1 upgrades continue in next message due to character limits

    Upgrade unlockFarm;
    unlockFarm.name = L"Agriculture";
    unlockFarm.description = L"Unlock Farms";
    unlockFarm.cost[ResourceType::Food] = 5.0;
    unlockFarm.tier = 1;
    unlockFarm.type = Upgrade::UpgradeType::UnlockFarm;
    upgrades.push_back(unlockFarm);

    Upgrade unlockLumber;
    unlockLumber.name = L"Forestry";
    unlockLumber.description = L"Unlock Lumber Mills";
    unlockLumber.cost[ResourceType::Wood] = 10.0;
    unlockLumber.tier = 1;
    unlockLumber.type = Upgrade::UpgradeType::UnlockLumberMill;
    upgrades.push_back(unlockLumber);

    Upgrade unlockQuarry;
    unlockQuarry.name = L"Mining";
    unlockQuarry.description = L"Unlock Quarries";
    unlockQuarry.cost[ResourceType::Stone] = 8.0;
    unlockQuarry.tier = 1;
    unlockQuarry.type = Upgrade::UpgradeType::UnlockQuarry;
    upgrades.push_back(unlockQuarry);

    Upgrade unlockHouse;
    unlockHouse.name = L"Construction";
    unlockHouse.description = L"Unlock Houses";
    unlockHouse.cost[ResourceType::Wood] = 15.0;
    unlockHouse.cost[ResourceType::Stone] = 5.0;
    unlockHouse.tier = 1;
    unlockHouse.type = Upgrade::UpgradeType::UnlockHouse;
    upgrades.push_back(unlockHouse);

    Upgrade betterTools;
    betterTools.name = L"Better Tools";
    betterTools.description = L"2x click power";
    betterTools.cost[ResourceType::Wood] = 3.0;
    betterTools.tier = 1;
    betterTools.type = Upgrade::UpgradeType::AllClickPower;
    betterTools.effectValue = 2.0;
    upgrades.push_back(betterTools);

    // TIER 2
    Upgrade farmingTech;
    farmingTech.name = L"Farming Techniques";
    farmingTech.description = L"+50% food production";
    farmingTech.cost[ResourceType::Food] = 25.0;
    farmingTech.tier = 2;
    farmingTech.type = Upgrade::UpgradeType::FoodProductionMultiplier;
    farmingTech.effectValue = 1.5;
    upgrades.push_back(farmingTech);

    Upgrade sawmill;
    sawmill.name = L"Sawmill Technology";
    sawmill.description = L"+50% wood production";
    sawmill.cost[ResourceType::Wood] = 30.0;
    sawmill.tier = 2;
    sawmill.type = Upgrade::UpgradeType::WoodProductionMultiplier;
    sawmill.effectValue = 1.5;
    upgrades.push_back(sawmill);

    Upgrade explosives;
    explosives.name = L"Explosives";
    explosives.description = L"+50% stone production";
    explosives.cost[ResourceType::Stone] = 20.0;
    explosives.tier = 2;
    explosives.type = Upgrade::UpgradeType::StoneProductionMultiplier;
    explosives.effectValue = 1.5;
    upgrades.push_back(explosives);

    Upgrade unlockMine;
    unlockMine.name = L"Deep Mining";
    unlockMine.description = L"Unlock Mines for gold";
    unlockMine.cost[ResourceType::Stone] = 40.0;
    unlockMine.cost[ResourceType::Wood] = 30.0;
    unlockMine.tier = 2;
    unlockMine.type = Upgrade::UpgradeType::UnlockMine;
    upgrades.push_back(unlockMine);

    Upgrade healthcare;
    healthcare.name = L"Healthcare";
    healthcare.description = L"+5 max population";
    healthcare.cost[ResourceType::Food] = 50.0;
    healthcare.tier = 2;
    healthcare.type = Upgrade::UpgradeType::PopulationCap;
    healthcare.effectValue = 5.0;
    upgrades.push_back(healthcare);

    Upgrade immigration;
    immigration.name = L"Immigration";
    immigration.description = L"Population grows over time";
    immigration.cost[ResourceType::Food] = 30.0;
    immigration.cost[ResourceType::Wood] = 20.0;
    immigration.tier = 2;
    immigration.type = Upgrade::UpgradeType::PopulationGrowth;
    immigration.effectValue = 0.1;
    upgrades.push_back(immigration);

    // TIER 3
    Upgrade irrigation;
    irrigation.name = L"Irrigation";
    irrigation.description = L"2x food production";
    irrigation.cost[ResourceType::Food] = 100.0;
    irrigation.cost[ResourceType::Wood] = 50.0;
    irrigation.tier = 3;
    irrigation.type = Upgrade::UpgradeType::FoodProductionMultiplier;
    irrigation.effectValue = 2.0;
    upgrades.push_back(irrigation);

    Upgrade steelAxes;
    steelAxes.name = L"Steel Axes";
    steelAxes.description = L"2x wood production";
    steelAxes.cost[ResourceType::Wood] = 120.0;
    steelAxes.cost[ResourceType::Stone] = 60.0;
    steelAxes.tier = 3;
    steelAxes.type = Upgrade::UpgradeType::WoodProductionMultiplier;
    steelAxes.effectValue = 2.0;
    upgrades.push_back(steelAxes);

    Upgrade industrialMining;
    industrialMining.name = L"Industrial Mining";
    industrialMining.description = L"2x stone production";
    industrialMining.cost[ResourceType::Stone] = 150.0;
    industrialMining.cost[ResourceType::Gold] = 10.0;
    industrialMining.tier = 3;
    industrialMining.type = Upgrade::UpgradeType::StoneProductionMultiplier;
    industrialMining.effectValue = 2.0;
    upgrades.push_back(industrialMining);

    Upgrade goldRush;
    goldRush.name = L"Gold Rush";
    goldRush.description = L"+100% gold production";
    goldRush.cost[ResourceType::Gold] = 15.0;
    goldRush.cost[ResourceType::Stone] = 100.0;
    goldRush.tier = 3;
    goldRush.type = Upgrade::UpgradeType::GoldProductionMultiplier;
    goldRush.effectValue = 2.0;
    upgrades.push_back(goldRush);

    Upgrade mechanization;
    mechanization.name = L"Mechanization";
    mechanization.description = L"+25% ALL production";
    mechanization.cost[ResourceType::Gold] = 25.0;
    mechanization.cost[ResourceType::Stone] = 150.0;
    mechanization.tier = 3;
    mechanization.type = Upgrade::UpgradeType::AllProductionMultiplier;
    mechanization.effectValue = 1.25;
    upgrades.push_back(mechanization);

    Upgrade refinedTools;
    refinedTools.name = L"Refined Tools";
    refinedTools.description = L"3x click power";
    refinedTools.cost[ResourceType::Stone] = 80.0;
    refinedTools.cost[ResourceType::Wood] = 60.0;
    refinedTools.tier = 3;
    refinedTools.type = Upgrade::UpgradeType::AllClickPower;
    refinedTools.effectValue = 3.0;
    upgrades.push_back(refinedTools);

    // TIER 4
    Upgrade education;
    education.name = L"Education System";
    education.description = L"+10 max population";
    education.cost[ResourceType::Gold] = 50.0;
    education.cost[ResourceType::Food] = 200.0;
    education.tier = 4;
    education.type = Upgrade::UpgradeType::PopulationCap;
    education.effectValue = 10.0;
    upgrades.push_back(education);

    Upgrade automation;
    automation.name = L"Automation";
    automation.description = L"Buildings cost 20% less";
    automation.cost[ResourceType::Gold] = 100.0;
    automation.cost[ResourceType::Stone] = 200.0;
    automation.tier = 4;
    automation.type = Upgrade::UpgradeType::CostReduction;
    automation.effectValue = 0.2;
    upgrades.push_back(automation);

    Upgrade massProduction;
    massProduction.name = L"Mass Production";
    massProduction.description = L"+50% ALL production";
    massProduction.cost[ResourceType::Gold] = 150.0;
    massProduction.cost[ResourceType::Food] = 300.0;
    massProduction.cost[ResourceType::Wood] = 250.0;
    massProduction.tier = 4;
    massProduction.type = Upgrade::UpgradeType::AllProductionMultiplier;
    massProduction.effectValue = 1.5;
    upgrades.push_back(massProduction);

    Upgrade hyperEfficiency;
    hyperEfficiency.name = L"Hyper-Efficiency";
    hyperEfficiency.description = L"2x ALL production";
    hyperEfficiency.cost[ResourceType::Gold] = 500.0;
    hyperEfficiency.cost[ResourceType::Food] = 1000.0;
    hyperEfficiency.cost[ResourceType::Wood] = 800.0;
    hyperEfficiency.cost[ResourceType::Stone] = 600.0;
    hyperEfficiency.tier = 4;
    hyperEfficiency.type = Upgrade::UpgradeType::AllProductionMultiplier;
    hyperEfficiency.effectValue = 2.0;
    upgrades.push_back(hyperEfficiency);

    Upgrade masterCraftsman;
    masterCraftsman.name = L"Master Craftsman";
    masterCraftsman.description = L"10x click power";
    masterCraftsman.cost[ResourceType::Gold] = 200.0;
    masterCraftsman.cost[ResourceType::Stone] = 300.0;
    masterCraftsman.tier = 4;
    masterCraftsman.type = Upgrade::UpgradeType::AllClickPower;
    masterCraftsman.effectValue = 10.0;
    upgrades.push_back(masterCraftsman);

    Upgrade foodExpert;
    foodExpert.name = L"Foraging Expert";
    foodExpert.description = L"5x food clicks";
    foodExpert.cost[ResourceType::Food] = 150.0;
    foodExpert.tier = 3;
    foodExpert.type = Upgrade::UpgradeType::FoodClickPower;
    foodExpert.effectValue = 5.0;
    upgrades.push_back(foodExpert);

    Upgrade lumberjack;
    lumberjack.name = L"Master Lumberjack";
    lumberjack.description = L"5x wood clicks";
    lumberjack.cost[ResourceType::Wood] = 180.0;
    lumberjack.tier = 3;
    lumberjack.type = Upgrade::UpgradeType::WoodClickPower;
    lumberjack.effectValue = 5.0;
    upgrades.push_back(lumberjack);
}