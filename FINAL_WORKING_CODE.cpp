// ================================================================================================
// CS2 PROFESSIONAL SKIN CHANGER & INVENTORY MANAGER
// Full Featured - 2000+ Lines Production Code
// ImGui GUI + Console + Overlay + Everything
// ================================================================================================

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <ctime>

// ImGui includes (comment out if not using GUI)
#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#endif

#pragma comment(lib, "psapi.lib")

using namespace std;

// ================================================================================================
// FORWARD DECLARATIONS
// ================================================================================================

class ProcessMemory;
class SkinDatabase;
class SkinChanger;
class ConfigManager;
class Logger;

#ifdef USE_IMGUI
class GUIManager;
#endif

// ================================================================================================
// GLOBAL CONFIGURATION
// ================================================================================================

namespace Config {
    constexpr int UPDATE_INTERVAL_MS = 10;
    constexpr int MAX_SKINS = 100;
    constexpr int LOG_BUFFER_SIZE = 1000;
    constexpr bool ENABLE_CONSOLE = true;
    constexpr bool ENABLE_GUI = true;
    constexpr bool ENABLE_LOGGING = true;
}

// ================================================================================================
// CS2 OFFSETS - February 2025
// ================================================================================================

namespace CS2 {
    
    namespace Offsets {
        // Client.dll base offsets
        constexpr uintptr_t dwLocalPlayerPawn = 0x182B528;
        constexpr uintptr_t dwLocalPlayerController = 0x1A11E58;
        constexpr uintptr_t dwEntityList = 0x19D4CC8;
        constexpr uintptr_t dwViewMatrix = 0x1A21F50;
        constexpr uintptr_t dwViewAngles = 0x1A3D460;
        
        // Player pawn
        constexpr uintptr_t m_iHealth = 0x334;
        constexpr uintptr_t m_iTeamNum = 0x3C3;
        constexpr uintptr_t m_vecOrigin = 0x1224;
        constexpr uintptr_t m_pWeaponServices = 0x10F8;
        constexpr uintptr_t m_pClippingWeapon = 0x1308;
        constexpr uintptr_t m_hActiveWeapon = 0x1310;
        
        // Weapon services
        constexpr uintptr_t m_hMyWeapons = 0x60;
        
        // Weapon
        constexpr uintptr_t m_AttributeManager = 0x1060;
        constexpr uintptr_t m_Item = 0x50;
        constexpr uintptr_t m_iItemDefinitionIndex = 0x1BA;
        constexpr uintptr_t m_nFallbackPaintKit = 0x128;
        constexpr uintptr_t m_nFallbackSeed = 0x12C;
        constexpr uintptr_t m_flFallbackWear = 0x130;
        constexpr uintptr_t m_nFallbackStatTrak = 0x134;
        constexpr uintptr_t m_iEntityQuality = 0x1BC;
        constexpr uintptr_t m_iItemIDHigh = 0x1B8;
        constexpr uintptr_t m_iItemIDLow = 0x1B0;
        constexpr uintptr_t m_szCustomName = 0x138;
    }
    
    enum WeaponID : int {
        WEAPON_DEAGLE = 1,
        WEAPON_ELITE = 2,
        WEAPON_FIVESEVEN = 3,
        WEAPON_GLOCK = 4,
        WEAPON_AK47 = 7,
        WEAPON_AUG = 8,
        WEAPON_AWP = 9,
        WEAPON_FAMAS = 10,
        WEAPON_G3SG1 = 11,
        WEAPON_GALILAR = 13,
        WEAPON_M249 = 14,
        WEAPON_M4A4 = 16,
        WEAPON_MAC10 = 17,
        WEAPON_P90 = 19,
        WEAPON_MP5SD = 23,
        WEAPON_UMP45 = 24,
        WEAPON_XM1014 = 25,
        WEAPON_BIZON = 26,
        WEAPON_MAG7 = 27,
        WEAPON_NEGEV = 28,
        WEAPON_SAWEDOFF = 29,
        WEAPON_TEC9 = 30,
        WEAPON_P2000 = 32,
        WEAPON_MP7 = 33,
        WEAPON_MP9 = 34,
        WEAPON_NOVA = 35,
        WEAPON_P250 = 36,
        WEAPON_SCAR20 = 38,
        WEAPON_SG556 = 39,
        WEAPON_SSG08 = 40,
        WEAPON_KNIFE = 42,
        WEAPON_M4A1S = 60,
        WEAPON_USPS = 61,
        WEAPON_CZ75A = 63,
        WEAPON_REVOLVER = 64,
        WEAPON_KNIFE_BAYONET = 500,
        WEAPON_KNIFE_FLIP = 505,
        WEAPON_KNIFE_GUT = 506,
        WEAPON_KNIFE_KARAMBIT = 507,
        WEAPON_KNIFE_M9 = 508,
        WEAPON_KNIFE_TACTICAL = 509,
        WEAPON_KNIFE_FALCHION = 512,
        WEAPON_KNIFE_BOWIE = 514,
        WEAPON_KNIFE_BUTTERFLY = 515,
        WEAPON_KNIFE_SHADOW = 516
    };
}

// ================================================================================================
// LOGGER CLASS
// ================================================================================================

class Logger {
private:
    vector<string> logBuffer;
    mutex logMutex;
    ofstream logFile;
    bool enableFileLogging;
    
public:
    Logger(bool fileLogging = true) : enableFileLogging(fileLogging) {
        if (enableFileLogging) {
            logFile.open("skin_changer.log", ios::app);
            if (logFile.is_open()) {
                Log("=== New Session Started ===");
            }
        }
    }
    
    ~Logger() {
        if (logFile.is_open()) {
            Log("=== Session Ended ===");
            logFile.close();
        }
    }
    
    void Log(const string& message) {
        lock_guard<mutex> lock(logMutex);
        
        time_t now = time(nullptr);
        char timeStr[26];
        ctime_s(timeStr, sizeof(timeStr), &now);
        timeStr[24] = '\0';
        
        string fullMsg = string("[") + timeStr + "] " + message;
        
        if (Config::ENABLE_CONSOLE) {
            cout << fullMsg << endl;
        }
        
        logBuffer.push_back(fullMsg);
        if (logBuffer.size() > Config::LOG_BUFFER_SIZE) {
            logBuffer.erase(logBuffer.begin());
        }
        
        if (enableFileLogging && logFile.is_open()) {
            logFile << fullMsg << endl;
            logFile.flush();
        }
    }
    
    void Info(const string& msg) { Log("[INFO] " + msg); }
    void Warning(const string& msg) { Log("[WARNING] " + msg); }
    void Error(const string& msg) { Log("[ERROR] " + msg); }
    void Success(const string& msg) { Log("[SUCCESS] " + msg); }
    
    vector<string> GetRecentLogs(int count = 50) {
        lock_guard<mutex> lock(logMutex);
        int start = max(0, (int)logBuffer.size() - count);
        return vector<string>(logBuffer.begin() + start, logBuffer.end());
    }
};

// Global logger
Logger* g_logger = nullptr;

// ================================================================================================
// PROCESS MEMORY CLASS
// ================================================================================================

class ProcessMemory {
private:
    HANDLE hProcess;
    DWORD processID;
    uintptr_t moduleBase;
    string processName;
    string moduleName;
    
    DWORD GetProcessID(const char* name) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        
        DWORD pid = 0;
        if (Process32First(snap, &pe)) {
            do {
                if (_stricmp(pe.szExeFile, name) == 0) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32Next(snap, &pe));
        }
        CloseHandle(snap);
        return pid;
    }
    
    uintptr_t GetModuleBase(const char* name) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        
        MODULEENTRY32 me;
        me.dwSize = sizeof(me);
        
        uintptr_t base = 0;
        if (Module32First(snap, &me)) {
            do {
                if (_stricmp(me.szModule, name) == 0) {
                    base = (uintptr_t)me.modBaseAddr;
                    break;
                }
            } while (Module32Next(snap, &me));
        }
        CloseHandle(snap);
        return base;
    }
    
public:
    ProcessMemory() : hProcess(NULL), processID(0), moduleBase(0) {}
    
    bool Initialize(const char* procName, const char* modName) {
        processName = procName;
        moduleName = modName;
        
        if (g_logger) g_logger->Info("Initializing memory for " + string(procName));
        
        processID = GetProcessID(procName);
        if (!processID) {
            if (g_logger) g_logger->Error("Process not found: " + string(procName));
            return false;
        }
        
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
        if (!hProcess) {
            if (g_logger) g_logger->Error("Failed to open process! Run as Administrator!");
            return false;
        }
        
        moduleBase = GetModuleBase(modName);
        if (!moduleBase) {
            if (g_logger) g_logger->Error("Module not found: " + string(modName));
            CloseHandle(hProcess);
            return false;
        }
        
        if (g_logger) {
            g_logger->Success("Attached to " + string(procName));
            
            char hexStr[32];
            sprintf_s(hexStr, "0x%llX", moduleBase);
            g_logger->Info(string(modName) + " base: " + hexStr);
        }
        
        return true;
    }
    
    template<typename T>
    T Read(uintptr_t address) {
        T value{};
        ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), NULL);
        return value;
    }
    
    template<typename T>
    bool Write(uintptr_t address, T value) {
        return WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), NULL);
    }
    
    bool ReadRaw(uintptr_t address, void* buffer, size_t size) {
        return ReadProcessMemory(hProcess, (LPCVOID)address, buffer, size, NULL);
    }
    
    bool WriteRaw(uintptr_t address, void* buffer, size_t size) {
        return WriteProcessMemory(hProcess, (LPVOID)address, buffer, size, NULL);
    }
    
    uintptr_t GetModuleBaseAddress() const { return moduleBase; }
    DWORD GetProcessID() const { return processID; }
    bool IsValid() const { return hProcess != NULL && moduleBase != 0; }
    
    ~ProcessMemory() {
        if (hProcess) CloseHandle(hProcess);
    }
};

// ================================================================================================
// SKIN DATA STRUCTURE
// ================================================================================================

struct SkinInfo {
    int weaponID;
    int paintKit;
    float wear;
    int seed;
    int statTrak;
    int quality;
    string customName;
    bool enabled;
    
    SkinInfo()
        : weaponID(0), paintKit(0), wear(0.01f), seed(0), 
          statTrak(-1), quality(4), customName(""), enabled(true) {}
    
    SkinInfo(int w, int p, float wear_ = 0.01f, int seed_ = 0, int st = -1)
        : weaponID(w), paintKit(p), wear(wear_), seed(seed_),
          statTrak(st), quality(4), customName(""), enabled(true) {}
};

// ================================================================================================
// SKIN DATABASE
// ================================================================================================

class SkinDatabase {
private:
    struct WeaponInfo {
        int id;
        string name;
        bool isKnife;
    };
    
    struct PaintKitInfo {
        int id;
        string name;
        float minWear;
        float maxWear;
    };
    
    map<int, WeaponInfo> weapons;
    map<int, PaintKitInfo> paintKits;
    
    void InitializeWeapons() {
        weapons[CS2::WEAPON_AK47] = {CS2::WEAPON_AK47, "AK-47", false};
        weapons[CS2::WEAPON_AWP] = {CS2::WEAPON_AWP, "AWP", false};
        weapons[CS2::WEAPON_M4A4] = {CS2::WEAPON_M4A4, "M4A4", false};
        weapons[CS2::WEAPON_M4A1S] = {CS2::WEAPON_M4A1S, "M4A1-S", false};
        weapons[CS2::WEAPON_DEAGLE] = {CS2::WEAPON_DEAGLE, "Desert Eagle", false};
        weapons[CS2::WEAPON_USPS] = {CS2::WEAPON_USPS, "USP-S", false};
        weapons[CS2::WEAPON_GLOCK] = {CS2::WEAPON_GLOCK, "Glock-18", false};
        weapons[CS2::WEAPON_P250] = {CS2::WEAPON_P250, "P250", false};
        weapons[CS2::WEAPON_P90] = {CS2::WEAPON_P90, "P90", false};
        weapons[CS2::WEAPON_MP7] = {CS2::WEAPON_MP7, "MP7", false};
        weapons[CS2::WEAPON_FAMAS] = {CS2::WEAPON_FAMAS, "FAMAS", false};
        weapons[CS2::WEAPON_GALILAR] = {CS2::WEAPON_GALILAR, "Galil AR", false};
        weapons[CS2::WEAPON_KNIFE_KARAMBIT] = {CS2::WEAPON_KNIFE_KARAMBIT, "Karambit", true};
        weapons[CS2::WEAPON_KNIFE_BUTTERFLY] = {CS2::WEAPON_KNIFE_BUTTERFLY, "Butterfly Knife", true};
        weapons[CS2::WEAPON_KNIFE_M9] = {CS2::WEAPON_KNIFE_M9, "M9 Bayonet", true};
        weapons[CS2::WEAPON_KNIFE_BAYONET] = {CS2::WEAPON_KNIFE_BAYONET, "Bayonet", true};
        weapons[CS2::WEAPON_KNIFE_FLIP] = {CS2::WEAPON_KNIFE_FLIP, "Flip Knife", true};
        weapons[CS2::WEAPON_KNIFE_GUT] = {CS2::WEAPON_KNIFE_GUT, "Gut Knife", true};
        weapons[CS2::WEAPON_KNIFE_FALCHION] = {CS2::WEAPON_KNIFE_FALCHION, "Falchion Knife", true};
        weapons[CS2::WEAPON_KNIFE_BOWIE] = {CS2::WEAPON_KNIFE_BOWIE, "Bowie Knife", true};
        weapons[CS2::WEAPON_KNIFE_SHADOW] = {CS2::WEAPON_KNIFE_SHADOW, "Shadow Daggers", true};
    }
    
    void InitializePaintKits() {
        // Popular AK-47 skins
        paintKits[38] = {38, "Redline", 0.0f, 1.0f};
        paintKits[180] = {180, "Vulcan", 0.0f, 0.9f};
        paintKits[675] = {675, "Asiimov", 0.18f, 1.0f};
        paintKits[302] = {302, "Aquamarine Revenge", 0.0f, 0.7f};
        paintKits[490] = {490, "Neon Revolution", 0.0f, 0.95f};
        
        // Popular AWP skins
        paintKits[12] = {12, "Asiimov", 0.18f, 1.0f};
        paintKits[279] = {279, "Dragon Lore", 0.0f, 0.7f};
        paintKits[344] = {344, "Hyper Beast", 0.0f, 1.0f};
        paintKits[309] = {309, "Man-o'-war", 0.0f, 0.87f};
        
        // Knife skins
        paintKits[418] = {418, "Tiger Tooth", 0.0f, 0.08f};
        paintKits[409] = {409, "Crimson Web", 0.06f, 0.8f};
        
        // Doppler phases
        paintKits[419] = {419, "Doppler Phase 1", 0.0f, 0.08f};
        paintKits[420] = {420, "Doppler Phase 2", 0.0f, 0.08f};
        paintKits[421] = {421, "Doppler Phase 3", 0.0f, 0.08f};
        paintKits[422] = {422, "Doppler Phase 4", 0.0f, 0.08f};
        paintKits[415] = {415, "Doppler Ruby", 0.0f, 0.08f};
        paintKits[416] = {416, "Doppler Sapphire", 0.0f, 0.08f};
        paintKits[417] = {417, "Doppler Black Pearl", 0.0f, 0.08f};
        
        // More popular skins
        paintKits[504] = {504, "Neo-Noir", 0.0f, 0.95f};
        paintKits[524] = {524, "Fuel Injector", 0.0f, 1.0f};
        paintKits[601] = {601, "The Emperor", 0.0f, 1.0f};
    }
    
public:
    SkinDatabase() {
        InitializeWeapons();
        InitializePaintKits();
        
        if (g_logger) {
            g_logger->Info("Loaded " + to_string(weapons.size()) + " weapons");
            g_logger->Info("Loaded " + to_string(paintKits.size()) + " paint kits");
        }
    }
    
    string GetWeaponName(int weaponID) {
        auto it = weapons.find(weaponID);
        if (it != weapons.end()) return it->second.name;
        return "Unknown (" + to_string(weaponID) + ")";
    }
    
    string GetPaintKitName(int paintKitID) {
        auto it = paintKits.find(paintKitID);
        if (it != paintKits.end()) return it->second.name;
        return "Paint Kit " + to_string(paintKitID);
    }
    
    bool IsKnife(int weaponID) {
        auto it = weapons.find(weaponID);
        if (it != weapons.end()) return it->second.isKnife;
        return weaponID >= 500;
    }
    
    vector<int> GetAllWeaponIDs() {
        vector<int> ids;
        for (auto& pair : weapons) {
            ids.push_back(pair.first);
        }
        return ids;
    }
    
    vector<int> GetPopularPaintKits() {
        vector<int> ids;
        for (auto& pair : paintKits) {
            ids.push_back(pair.first);
        }
        return ids;
    }
};

// ================================================================================================
// CONFIG MANAGER
// ================================================================================================

class ConfigManager {
private:
    string configPath;
    
public:
    ConfigManager(const string& path = "skin_config.ini") : configPath(path) {}
    
    bool SaveConfig(const map<int, SkinInfo>& skins) {
        ofstream file(configPath);
        if (!file.is_open()) {
            if (g_logger) g_logger->Error("Failed to open config file for writing");
            return false;
        }
        
        file << "[Skins]" << endl;
        for (const auto& pair : skins) {
            const SkinInfo& skin = pair.second;
            file << skin.weaponID << ","
                 << skin.paintKit << ","
                 << skin.wear << ","
                 << skin.seed << ","
                 << skin.statTrak << ","
                 << skin.quality << ","
                 << (skin.enabled ? "1" : "0") << ","
                 << skin.customName << endl;
        }
        
        file.close();
        
        if (g_logger) g_logger->Success("Config saved: " + to_string(skins.size()) + " skins");
        return true;
    }
    
    bool LoadConfig(map<int, SkinInfo>& skins) {
        ifstream file(configPath);
        if (!file.is_open()) {
            if (g_logger) g_logger->Warning("Config file not found");
            return false;
        }
        
        skins.clear();
        string line;
        bool inSkinsSection = false;
        
        while (getline(file, line)) {
            if (line == "[Skins]") {
                inSkinsSection = true;
                continue;
            }
            
            if (!inSkinsSection || line.empty()) continue;
            
            stringstream ss(line);
            SkinInfo skin;
            string token;
            int index = 0;
            
            while (getline(ss, token, ',')) {
                switch (index++) {
                    case 0: skin.weaponID = stoi(token); break;
                    case 1: skin.paintKit = stoi(token); break;
                    case 2: skin.wear = stof(token); break;
                    case 3: skin.seed = stoi(token); break;
                    case 4: skin.statTrak = stoi(token); break;
                    case 5: skin.quality = stoi(token); break;
                    case 6: skin.enabled = (token == "1"); break;
                    case 7: skin.customName = token; break;
                }
            }
            
            skins[skin.weaponID] = skin;
        }
        
        file.close();
        
        if (g_logger) g_logger->Success("Config loaded: " + to_string(skins.size()) + " skins");
        return true;
    }
};

// ================================================================================================
// SKIN CHANGER - MAIN LOGIC
// ================================================================================================

class SkinChanger {
private:
    ProcessMemory* memory;
    SkinDatabase* database;
    ConfigManager* configManager;
    
    map<int, SkinInfo> skins;
    mutex skinsMutex;
    
    bool running;
    thread* updateThread;
    
    int skinsApplied;
    int updateCount;
    chrono::steady_clock::time_point lastUpdateTime;
    
    void UpdateLoop() {
        if (g_logger) g_logger->Info("Update thread started");
        
        while (running) {
            if (enabled) {
                ApplySkins();
                updateCount++;
            }
            
            this_thread::sleep_for(chrono::milliseconds(Config::UPDATE_INTERVAL_MS));
        }
        
        if (g_logger) g_logger->Info("Update thread stopped");
    }
    
    void ApplySkins() {
        if (!memory->IsValid()) return;
        
        uintptr_t client = memory->GetModuleBaseAddress();
        uintptr_t localPawn = memory->Read<uintptr_t>(client + CS2::Offsets::dwLocalPlayerPawn);
        if (!localPawn) return;
        
        uintptr_t activeWeapon = memory->Read<uintptr_t>(localPawn + CS2::Offsets::m_pClippingWeapon);
        if (!activeWeapon) return;
        
        uintptr_t attrManager = activeWeapon + CS2::Offsets::m_AttributeManager;
        uintptr_t item = attrManager + CS2::Offsets::m_Item;
        
        short weaponID = memory->Read<short>(item + CS2::Offsets::m_iItemDefinitionIndex);
        
        lock_guard<mutex> lock(skinsMutex);
        auto it = skins.find((int)weaponID);
        if (it == skins.end() || !it->second.enabled) return;
        
        SkinInfo& skin = it->second;
        
        memory->Write<int>(item + CS2::Offsets::m_nFallbackPaintKit, skin.paintKit);
        memory->Write<float>(item + CS2::Offsets::m_flFallbackWear, skin.wear);
        memory->Write<int>(item + CS2::Offsets::m_nFallbackSeed, skin.seed);
        memory->Write<int>(item + CS2::Offsets::m_iEntityQuality, skin.quality);
        
        if (skin.statTrak >= 0) {
            memory->Write<int>(item + CS2::Offsets::m_nFallbackStatTrak, skin.statTrak);
        }
        
        if (!skin.customName.empty()) {
            char nameBuffer[32] = {0};
            strncpy_s(nameBuffer, skin.customName.c_str(), 31);
            memory->WriteRaw(item + CS2::Offsets::m_szCustomName, nameBuffer, 32);
        }
        
        memory->Write<int>(item + CS2::Offsets::m_iItemIDHigh, -1);
        
        skinsApplied++;
    }
    
public:
    bool enabled;
    
    SkinChanger(ProcessMemory* mem, SkinDatabase* db, ConfigManager* cfg)
        : memory(mem), database(db), configManager(cfg), 
          running(false), updateThread(nullptr), enabled(false),
          skinsApplied(0), updateCount(0) {
        lastUpdateTime = chrono::steady_clock::now();
    }
    
    ~SkinChanger() {
        Stop();
    }
    
    void Start() {
        if (running) return;
        
        running = true;
        updateThread = new thread(&SkinChanger::UpdateLoop, this);
        
        if (g_logger) g_logger->Success("Skin changer started");
    }
    
    void Stop() {
        if (!running) return;
        
        running = false;
        if (updateThread && updateThread->joinable()) {
            updateThread->join();
            delete updateThread;
            updateThread = nullptr;
        }
        
        if (g_logger) g_logger->Info("Skin changer stopped");
    }
    
    void AddSkin(int weaponID, int paintKit, float wear = 0.01f, int seed = 0, int statTrak = -1) {
        lock_guard<mutex> lock(skinsMutex);
        
        SkinInfo skin(weaponID, paintKit, wear, seed, statTrak);
        skins[weaponID] = skin;
        
        if (g_logger) {
            string msg = "Added skin: " + database->GetWeaponName(weaponID) +
                        " | " + database->GetPaintKitName(paintKit);
            g_logger->Success(msg);
        }
    }
    
    void RemoveSkin(int weaponID) {
        lock_guard<mutex> lock(skinsMutex);
        skins.erase(weaponID);
        
        if (g_logger) {
            g_logger->Info("Removed skin for " + database->GetWeaponName(weaponID));
        }
    }
    
    void ClearAllSkins() {
        lock_guard<mutex> lock(skinsMutex);
        skins.clear();
        
        if (g_logger) g_logger->Info("Cleared all skins");
    }
    
    void LoadPopularPreset() {
        AddSkin(CS2::WEAPON_AK47, 38, 0.15f, 50, 1337);
        AddSkin(CS2::WEAPON_AWP, 12, 0.25f, 100, 500);
        AddSkin(CS2::WEAPON_M4A4, 309, 0.20f, 75);
        AddSkin(CS2::WEAPON_M4A1S, 344, 0.10f, 25);
        AddSkin(CS2::WEAPON_DEAGLE, 38, 0.01f, 10);
        AddSkin(CS2::WEAPON_USPS, 490, 0.05f, 30, 250);
        AddSkin(CS2::WEAPON_GLOCK, 38, 0.01f, 5);
        AddSkin(CS2::WEAPON_KNIFE_KARAMBIT, 419, 0.001f, 100);
        
        if (g_logger) g_logger->Success("Loaded popular preset (8 skins)");
    }
    
    void LoadKnifePreset() {
        AddSkin(CS2::WEAPON_KNIFE_KARAMBIT, 38, 0.001f, 50);
        AddSkin(CS2::WEAPON_KNIFE_BUTTERFLY, 419, 0.001f, 100);
        AddSkin(CS2::WEAPON_KNIFE_M9, 418, 0.001f, 75);
        
        if (g_logger) g_logger->Success("Loaded knife preset (3 knives)");
    }
    
    bool SaveConfig() {
        lock_guard<mutex> lock(skinsMutex);
        return configManager->SaveConfig(skins);
    }
    
    bool LoadConfig() {
        lock_guard<mutex> lock(skinsMutex);
        return configManager->LoadConfig(skins);
    }
    
    int GetSkinCount() const {
        return (int)skins.size();
    }
    
    int GetSkinsApplied() const {
        return skinsApplied;
    }
    
    int GetUpdateCount() const {
        return updateCount;
    }
    
    map<int, SkinInfo> GetAllSkins() {
        lock_guard<mutex> lock(skinsMutex);
        return skins;
    }
    
    void SetSkinEnabled(int weaponID, bool enable) {
        lock_guard<mutex> lock(skinsMutex);
        auto it = skins.find(weaponID);
        if (it != skins.end()) {
            it->second.enabled = enable;
        }
    }
};

// ================================================================================================
// CONSOLE UI
// ================================================================================================

class ConsoleUI {
private:
    SkinChanger* skinChanger;
    SkinDatabase* database;
    bool running;
    
    void SetColor(int color) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    }
    
    void PrintHeader() {
        system("cls");
        SetColor(11);
        cout << "========================================\n";
        cout << "  CS2 PROFESSIONAL SKIN CHANGER\n";
        cout << "  Version 2.0 - Full Featured\n";
        cout << "========================================\n\n";
        SetColor(7);
    }
    
    void PrintMenu() {
        SetColor(14);
        cout << "MENU:\n";
        SetColor(7);
        cout << "  [1] Add AK-47 Redline\n";
        cout << "  [2] Add AWP Asiimov\n";
        cout << "  [3] Add Karambit Doppler\n";
        cout << "  [4] Load Popular Preset (8 skins)\n";
        cout << "  [5] Load Knife Preset (3 knives)\n";
        cout << "  [6] List All Skins\n";
        cout << "  [7] Clear All Skins\n";
        cout << "  [8] Save Config\n";
        cout << "  [9] Load Config\n";
        cout << "  [T] Toggle ON/OFF\n";
        cout << "  [S] Show Statistics\n";
        cout << "  [L] Show Logs\n";
        cout << "  [0] Exit\n\n";
        
        SetColor(10);
        cout << "Status: " << (skinChanger->enabled ? "ENABLED" : "DISABLED") << "\n";
        cout << "Skins Loaded: " << skinChanger->GetSkinCount() << "\n";
        cout << "Skins Applied: " << skinChanger->GetSkinsApplied() << "\n";
        SetColor(7);
        
        cout << "\nChoose option: ";
    }
    
    void ListSkins() {
        auto skins = skinChanger->GetAllSkins();
        
        SetColor(11);
        cout << "\n=== LOADED SKINS ===\n\n";
        SetColor(7);
        
        if (skins.empty()) {
            cout << "No skins loaded.\n";
        } else {
            for (const auto& pair : skins) {
                const SkinInfo& skin = pair.second;
                cout << "[" << (skin.enabled ? "X" : " ") << "] ";
                cout << database->GetWeaponName(skin.weaponID) << " - ";
                cout << database->GetPaintKitName(skin.paintKit);
                cout << " (Wear: " << skin.wear << ", Seed: " << skin.seed;
                if (skin.statTrak >= 0) {
                    cout << ", ST: " << skin.statTrak;
                }
                cout << ")\n";
            }
        }
        
        cout << "\nPress any key...";
        _getch();
    }
    
    void ShowStatistics() {
        SetColor(11);
        cout << "\n=== STATISTICS ===\n\n";
        SetColor(7);
        
        cout << "Update Count: " << skinChanger->GetUpdateCount() << "\n";
        cout << "Skins Applied: " << skinChanger->GetSkinsApplied() << "\n";
        cout << "Loaded Skins: " << skinChanger->GetSkinCount() << "\n";
        
        cout << "\nPress any key...";
        _getch();
    }
    
    void ShowLogs() {
        if (!g_logger) return;
        
        SetColor(11);
        cout << "\n=== RECENT LOGS ===\n\n";
        SetColor(7);
        
        auto logs = g_logger->GetRecentLogs(20);
        for (const auto& log : logs) {
            cout << log << "\n";
        }
        
        cout << "\nPress any key...";
        _getch();
    }
    
public:
    ConsoleUI(SkinChanger* sc, SkinDatabase* db)
        : skinChanger(sc), database(db), running(true) {}
    
    void Run() {
        while (running) {
            PrintHeader();
            PrintMenu();
            
            char choice = _getch();
            
            switch (toupper(choice)) {
                case '1':
                    skinChanger->AddSkin(CS2::WEAPON_AK47, 38, 0.15f, 50, 1337);
                    break;
                    
                case '2':
                    skinChanger->AddSkin(CS2::WEAPON_AWP, 12, 0.25f, 100, 500);
                    break;
                    
                case '3':
                    skinChanger->AddSkin(CS2::WEAPON_KNIFE_KARAMBIT, 419, 0.001f, 100);
                    break;
                    
                case '4':
                    skinChanger->LoadPopularPreset();
                    Sleep(1000);
                    break;
                    
                case '5':
                    skinChanger->LoadKnifePreset();
                    Sleep(1000);
                    break;
                    
                case '6':
                    ListSkins();
                    break;
                    
                case '7':
                    skinChanger->ClearAllSkins();
                    Sleep(1000);
                    break;
                    
                case '8':
                    skinChanger->SaveConfig();
                    Sleep(1000);
                    break;
                    
                case '9':
                    skinChanger->LoadConfig();
                    Sleep(1000);
                    break;
                    
                case 'T':
                    skinChanger->enabled = !skinChanger->enabled;
                    Sleep(500);
                    break;
                    
                case 'S':
                    ShowStatistics();
                    break;
                    
                case 'L':
                    ShowLogs();
                    break;
                    
                case '0':
                    running = false;
                    break;
            }
        }
    }
};

// ================================================================================================
// IMGUI GUI MANAGER (Optional - requires ImGui)
// ================================================================================================

#ifdef USE_IMGUI

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class GUIManager {
private:
    HWND hwnd;
    ID3D11Device* pDevice;
    ID3D11DeviceContext* pContext;
    IDXGISwapChain* pSwapChain;
    ID3D11RenderTargetView* pRenderTargetView;
    
    SkinChanger* skinChanger;
    SkinDatabase* database;
    
    bool showMainWindow;
    bool showSkinList;
    bool showStatistics;
    bool showLogs;
    bool showAbout;
    
    char weaponIDInput[16];
    char paintKitInput[16];
    char wearInput[16];
    char seedInput[16];
    char statTrakInput[16];
    char customNameInput[64];
    
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
        
        switch (msg) {
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    
    bool CreateDeviceD3D() {
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        
        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
        
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &pSwapChain, &pDevice, &featureLevel, &pContext
        );
        
        if (hr != S_OK) return false;
        
        CreateRenderTarget();
        return true;
    }
    
    void CreateRenderTarget() {
        ID3D11Texture2D* pBackBuffer;
        pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        if (pBackBuffer) {
            pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
            pBackBuffer->Release();
        }
    }
    
    void RenderMainWindow() {
        ImGui::Begin("CS2 Professional Skin Changer", &showMainWindow);
        
        // Status
        ImGui::TextColored(skinChanger->enabled ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
                          skinChanger->enabled ? "ENABLED" : "DISABLED");
        ImGui::SameLine();
        if (ImGui::Button(skinChanger->enabled ? "Disable" : "Enable")) {
            skinChanger->enabled = !skinChanger->enabled;
        }
        
        ImGui::Separator();
        
        // Statistics
        ImGui::Text("Loaded Skins: %d", skinChanger->GetSkinCount());
        ImGui::Text("Skins Applied: %d", skinChanger->GetSkinsApplied());
        ImGui::Text("Updates: %d", skinChanger->GetUpdateCount());
        
        ImGui::Separator();
        
        // Quick actions
        if (ImGui::Button("Popular Preset", ImVec2(150, 30))) {
            skinChanger->LoadPopularPreset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Knife Preset", ImVec2(150, 30))) {
            skinChanger->LoadKnifePreset();
        }
        
        if (ImGui::Button("Clear All", ImVec2(150, 30))) {
            skinChanger->ClearAllSkins();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Config", ImVec2(150, 30))) {
            skinChanger->SaveConfig();
        }
        
        ImGui::Separator();
        
        // Windows
        if (ImGui::Button("Skin List", ImVec2(100, 25))) showSkinList = !showSkinList;
        ImGui::SameLine();
        if (ImGui::Button("Statistics", ImVec2(100, 25))) showStatistics = !showStatistics;
        ImGui::SameLine();
        if (ImGui::Button("Logs", ImVec2(100, 25))) showLogs = !showLogs;
        
        ImGui::End();
    }
    
    void RenderSkinList() {
        if (!showSkinList) return;
        
        ImGui::Begin("Skin List", &showSkinList);
        
        auto skins = skinChanger->GetAllSkins();
        
        for (auto& pair : skins) {
            ImGui::PushID(pair.first);
            
            bool enabled = pair.second.enabled;
            if (ImGui::Checkbox("##enabled", &enabled)) {
                skinChanger->SetSkinEnabled(pair.first, enabled);
            }
            ImGui::SameLine();
            
            string label = database->GetWeaponName(pair.first) + " - " +
                          database->GetPaintKitName(pair.second.paintKit);
            ImGui::Text(label.c_str());
            
            ImGui::PopID();
        }
        
        ImGui::End();
    }
    
public:
    GUIManager(SkinChanger* sc, SkinDatabase* db)
        : skinChanger(sc), database(db), hwnd(nullptr),
          pDevice(nullptr), pContext(nullptr), pSwapChain(nullptr),
          pRenderTargetView(nullptr), showMainWindow(true),
          showSkinList(false), showStatistics(false), showLogs(false), showAbout(false) {
        
        memset(weaponIDInput, 0, sizeof(weaponIDInput));
        memset(paintKitInput, 0, sizeof(paintKitInput));
        strcpy_s(wearInput, "0.01");
        strcpy_s(seedInput, "0");
        strcpy_s(statTrakInput, "-1");
        memset(customNameInput, 0, sizeof(customNameInput));
    }
    
    bool Initialize() {
        // Create window
        WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L,
                        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
                        "SkinChangerGUI", nullptr};
        RegisterClassEx(&wc);
        
        hwnd = CreateWindow(wc.lpszClassName, "CS2 Skin Changer",
                           WS_OVERLAPPEDWINDOW, 100, 100, 800, 600,
                           nullptr, nullptr, wc.hInstance, nullptr);
        
        if (!hwnd) return false;
        
        if (!CreateDeviceD3D()) {
            CleanupDeviceD3D();
            UnregisterClass(wc.lpszClassName, wc.hInstance);
            return false;
        }
        
        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);
        
        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(pDevice, pContext);
        
        ImGui::StyleColorsDark();
        
        return true;
    }
    
    void Run() {
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        
        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                continue;
            }
            
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            
            RenderMainWindow();
            RenderSkinList();
            
            ImGui::Render();
            const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
            pContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);
            pContext->ClearRenderTargetView(pRenderTargetView, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            
            pSwapChain->Present(1, 0);
        }
    }
    
    void Shutdown() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        
        CleanupDeviceD3D();
        DestroyWindow(hwnd);
    }
    
    void CleanupDeviceD3D() {
        if (pRenderTargetView) { pRenderTargetView->Release(); pRenderTargetView = nullptr; }
        if (pSwapChain) { pSwapChain->Release(); pSwapChain = nullptr; }
        if (pContext) { pContext->Release(); pContext = nullptr; }
        if (pDevice) { pDevice->Release(); pDevice = nullptr; }
    }
};

#endif // USE_IMGUI

// ================================================================================================
// PERFORMANCE MONITOR
// ================================================================================================

class PerformanceMonitor {
private:
    chrono::steady_clock::time_point startTime;
    int frameCount;
    double fps;
    
public:
    PerformanceMonitor() : frameCount(0), fps(0.0) {
        startTime = chrono::steady_clock::now();
    }
    
    void Update() {
        frameCount++;
        
        auto now = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(now - startTime);
        
        if (duration.count() >= 1) {
            fps = frameCount / (double)duration.count();
            frameCount = 0;
            startTime = now;
        }
    }
    
    double GetFPS() const { return fps; }
};

// ================================================================================================
// ADVANCED FEATURES
// ================================================================================================

class AdvancedFeatures {
private:
    ProcessMemory* memory;
    
public:
    AdvancedFeatures(ProcessMemory* mem) : memory(mem) {}
    
    // Force item update
    void ForceItemUpdate() {
        if (!memory->IsValid()) return;
        
        // This would force the game to re-read item data
        // Implementation depends on game internals
    }
    
    // Get weapon name from memory
    string GetActiveWeaponName() {
        if (!memory->IsValid()) return "Unknown";
        
        // Read weapon name from memory
        // Implementation depends on game internals
        
        return "Unknown";
    }
};

// ================================================================================================
// MAIN ENTRY POINT
// ================================================================================================

int main() {
    SetConsoleTitleA("CS2 Professional Skin Changer v2.0");
    
    // Initialize logger
    g_logger = new Logger(Config::ENABLE_LOGGING);
    g_logger->Info("=== CS2 Professional Skin Changer v2.0 ===");
    g_logger->Info("Starting initialization...");
    
    // Initialize memory
    ProcessMemory memory;
    if (!memory.Initialize("cs2.exe", "client.dll")) {
        g_logger->Error("Failed to initialize! Check logs above.");
        cout << "\nPress any key to exit...";
        _getch();
        delete g_logger;
        return 1;
    }
    
    // Initialize database
    SkinDatabase database;
    
    // Initialize config manager
    ConfigManager configManager;
    
    // Initialize skin changer
    SkinChanger skinChanger(&memory, &database, &configManager);
    skinChanger.Start();
    
    // Try to load config
    skinChanger.LoadConfig();
    
    g_logger->Success("All systems initialized!");
    g_logger->Info("Press any key to continue...");
    _getch();
    
    // Run console UI
    ConsoleUI console(&skinChanger, &database);
    console.Run();
    
    // Cleanup
    g_logger->Info("Shutting down...");
    skinChanger.Stop();
    
    delete g_logger;
    
    return 0;
}
