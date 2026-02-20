#pragma once
#include "SimpleMemory.h"
#include "CS2_Offsets.h"
#include <vector>
#include <map>

// Skin data structure
struct SkinData {
    int weaponID;
    int paintKit;
    float wear;
    int seed;
    int statTrak;
    
    SkinData(int w, int p, float wear_ = 0.01f, int seed_ = 0, int st = -1)
        : weaponID(w), paintKit(p), wear(wear_), seed(seed_), statTrak(st) {}
};

class WorkingSkinChanger {
private:
    ProcessMemory* mem;
    std::map<int, SkinData> skinMap;  // weaponID -> skin
    
public:
    bool enabled = false;
    
    WorkingSkinChanger(ProcessMemory* m) : mem(m) {}
    
    // Add skin for a weapon
    void AddSkin(int weaponID, int paintKit, float wear = 0.01f, int seed = 0, int statTrak = -1) {
        skinMap[weaponID] = SkinData(weaponID, paintKit, wear, seed, statTrak);
        printf("[+] Added skin: Weapon %d -> Paint %d (Wear: %.4f)\n", weaponID, paintKit, wear);
    }
    
    // Remove skin
    void RemoveSkin(int weaponID) {
        skinMap.erase(weaponID);
    }
    
    // Clear all skins
    void Clear() {
        skinMap.clear();
        printf("[*] Cleared all skins\n");
    }
    
    // Apply skins (call this in a loop)
    void Update() {
        if (!enabled || skinMap.empty()) return;
        
        // Get local player pawn
        uintptr_t localPawn = mem->Read<uintptr_t>(mem->moduleBase + CS2::Offsets::dwLocalPlayerPawn);
        if (!localPawn) return;
        
        // Get active weapon
        uintptr_t activeWeapon = mem->Read<uintptr_t>(localPawn + CS2::Pawn::m_pClippingWeapon);
        if (!activeWeapon) return;
        
        // Get weapon ID
        uintptr_t attributeManager = activeWeapon + CS2::Weapon::m_AttributeManager;
        uintptr_t item = attributeManager + CS2::Weapon::m_Item;
        
        short weaponID = mem->Read<short>(item + CS2::Weapon::m_iItemDefinitionIndex);
        
        // Check if we have a skin for this weapon
        auto it = skinMap.find((int)weaponID);
        if (it == skinMap.end()) return;
        
        SkinData& skin = it->second;
        
        // Apply skin
        mem->Write<int>(item + CS2::Weapon::m_nFallbackPaintKit, skin.paintKit);
        mem->Write<float>(item + CS2::Weapon::m_flFallbackWear, skin.wear);
        mem->Write<int>(item + CS2::Weapon::m_nFallbackSeed, skin.seed);
        
        if (skin.statTrak >= 0) {
            mem->Write<int>(item + CS2::Weapon::m_nFallbackStatTrak, skin.statTrak);
        }
    }
    
    // Quick presets
    void LoadPopularPreset() {
        AddSkin(CS2::WEAPON_AK47, CS2::AK_REDLINE, 0.15f, 50, 1337);
        AddSkin(CS2::WEAPON_AWP, CS2::AWP_ASIIMOV, 0.25f, 100, 500);
        AddSkin(CS2::WEAPON_M4A4, CS2::M4A4_ASIIMOV, 0.20f, 75);
        AddSkin(CS2::WEAPON_M4A1S, CS2::AWP_HYPER_BEAST, 0.10f, 25);
        AddSkin(CS2::WEAPON_DEAGLE, CS2::DEAGLE_BLAZE, 0.01f, 10);
        AddSkin(CS2::WEAPON_USPS, CS2::USPS_KILL_CONFIRMED, 0.05f, 30, 250);
        AddSkin(CS2::WEAPON_GLOCK, CS2::GLOCK_FADE, 0.01f, 5);
        AddSkin(CS2::WEAPON_KNIFE_KARAMBIT, CS2::KNIFE_DOPPLER, 0.001f, 100);
        
        printf("[+] Loaded popular preset (8 skins)\n");
    }
    
    void LoadKnifePreset() {
        AddSkin(CS2::WEAPON_KNIFE_KARAMBIT, CS2::KNIFE_FADE, 0.001f, 50);
        AddSkin(CS2::WEAPON_KNIFE_BUTTERFLY, CS2::KNIFE_DOPPLER, 0.001f, 100);
        AddSkin(CS2::WEAPON_KNIFE_M9, CS2::KNIFE_TIGER_TOOTH, 0.001f, 75);
        
        printf("[+] Loaded knife preset (3 knives)\n");
    }
    
    // Get skin count
    int GetSkinCount() const {
        return (int)skinMap.size();
    }
};
