#pragma once
#include <string>
#include <vector>
#include <map>

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

    ResourceInfo() : amount(0.0), perSecond(0.0) {}
    ResourceInfo(const std::wstring& n, double amt = 0.0, double ps = 0.0)
        : name(n), amount(amt), perSecond(ps) {
    }
};

// Building type
struct BuildingType {
    std::wstring name;
    std::wstring description;
    std::map<ResourceType, double> cost;           // What it costs to build
    std::map<ResourceType, double> production;     // What it produces per second
    int baseCount;                                  // How many you start with

    BuildingType() : baseCount(0) {}
};

// Building instance
struct Building {
    const BuildingType* type;
    int count;

    Building(const BuildingType* t, int c = 0) : type(t), count(c) {}

    // Calculate total cost for next building (with scaling)
    std::map<ResourceType, double> GetNextCost() const {
        std::map<ResourceType, double> nextCost;
        for (const auto& cost : type->cost) {
            // Cost increases by 15% for each building owned
            double scaledCost = cost.second * pow(1.15, count);
            nextCost[cost.first] = scaledCost;
        }
        return nextCost;
    }

    // Calculate total production from all buildings of this type
    std::map<ResourceType, double> GetTotalProduction() const {
        std::map<ResourceType, double> totalProd;
        for (const auto& prod : type->production) {
            totalProd[prod.first] = prod.second * count;
        }
        return totalProd;
    }
};

// Game state
class GameState {
public:
    // Resources
    std::map<ResourceType, ResourceInfo> resources;

    // Buildings
    std::vector<Building> buildings;
    std::vector<BuildingType> buildingTypes;

    // Time tracking
    float gameTime;

    GameState() : gameTime(0.0f) {
        InitializeResources();
        InitializeBuildingTypes();
        InitializeBuildings();
    }

    void InitializeResources() {
        resources[ResourceType::Food] = ResourceInfo(L"Food", 10.0, 1.0);
        resources[ResourceType::Wood] = ResourceInfo(L"Wood", 10.0, 0.5);
        resources[ResourceType::Stone] = ResourceInfo(L"Stone", 5.0, 0.3);
        resources[ResourceType::Gold] = ResourceInfo(L"Gold", 0.0, 0.1);
    }

    void InitializeBuildingTypes() {
        // Farm - produces food, costs wood
        BuildingType farm;
        farm.name = L"Farm";
        farm.description = L"Produces food";
        farm.cost[ResourceType::Wood] = 10.0;
        farm.production[ResourceType::Food] = 2.0;
        farm.baseCount = 0;
        buildingTypes.push_back(farm);

        // Lumber Mill - produces wood, costs food and stone
        BuildingType lumberMill;
        lumberMill.name = L"Lumber Mill";
        lumberMill.description = L"Produces wood";
        lumberMill.cost[ResourceType::Food] = 15.0;
        lumberMill.cost[ResourceType::Stone] = 5.0;
        lumberMill.production[ResourceType::Wood] = 1.5;
        lumberMill.baseCount = 0;
        buildingTypes.push_back(lumberMill);

        // Quarry - produces stone, costs wood and food
        BuildingType quarry;
        quarry.name = L"Quarry";
        quarry.description = L"Produces stone";
        quarry.cost[ResourceType::Wood] = 20.0;
        quarry.cost[ResourceType::Food] = 10.0;
        quarry.production[ResourceType::Stone] = 1.0;
        quarry.baseCount = 0;
        buildingTypes.push_back(quarry);

        // Mine - produces gold, costs wood, stone, and food
        BuildingType mine;
        mine.name = L"Mine";
        mine.description = L"Produces gold";
        mine.cost[ResourceType::Wood] = 50.0;
        mine.cost[ResourceType::Stone] = 30.0;
        mine.cost[ResourceType::Food] = 25.0;
        mine.production[ResourceType::Gold] = 0.5;
        mine.baseCount = 0;
        buildingTypes.push_back(mine);

        // House - increases base production rates
        BuildingType house;
        house.name = L"House";
        house.description = L"Boosts production +10%";
        house.cost[ResourceType::Wood] = 30.0;
        house.cost[ResourceType::Stone] = 15.0;
        house.production[ResourceType::Food] = 0.5;  // Small food production
        house.baseCount = 0;
        buildingTypes.push_back(house);
    }

    void InitializeBuildings() {
        for (const auto& type : buildingTypes) {
            buildings.push_back(Building(&type, type.baseCount));
        }
    }

    // Check if player can afford a building
    bool CanAfford(int buildingIndex) const {
        if (buildingIndex < 0 || buildingIndex >= buildings.size()) return false;

        auto cost = buildings[buildingIndex].GetNextCost();
        for (const auto& costItem : cost) {
            auto it = resources.find(costItem.first);
            if (it == resources.end() || it->second.amount < costItem.second) {
                return false;
            }
        }
        return true;
    }

    // Purchase a building
    bool PurchaseBuilding(int buildingIndex) {
        if (!CanAfford(buildingIndex)) return false;

        // Deduct costs
        auto cost = buildings[buildingIndex].GetNextCost();
        for (const auto& costItem : cost) {
            resources[costItem.first].amount -= costItem.second;
        }

        // Add building
        buildings[buildingIndex].count++;

        // Recalculate production rates
        RecalculateProduction();

        return true;
    }

    // Recalculate all production rates from buildings
    void RecalculateProduction() {
        // Reset to base rates
        resources[ResourceType::Food].perSecond = 1.0;
        resources[ResourceType::Wood].perSecond = 0.5;
        resources[ResourceType::Stone].perSecond = 0.3;
        resources[ResourceType::Gold].perSecond = 0.1;

        // Add production from all buildings
        for (const auto& building : buildings) {
            auto production = building.GetTotalProduction();
            for (const auto& prod : production) {
                resources[prod.first].perSecond += prod.second;
            }
        }
    }

    // Update resources based on production
    void Update(float deltaTime) {
        gameTime += deltaTime;

        for (auto& resource : resources) {
            resource.second.amount += resource.second.perSecond * deltaTime;

            // Clamp negative values
            if (resource.second.amount < 0.0) {
                resource.second.amount = 0.0;
            }
        }
    }

    // Manual resource gathering
    void GatherResource(ResourceType type, double amount) {
        resources[type].amount += amount;
    }
};