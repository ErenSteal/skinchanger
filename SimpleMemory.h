#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

class ProcessMemory {
public:
    HANDLE hProcess;
    DWORD processID;
    uintptr_t moduleBase;
    
    ProcessMemory() : hProcess(NULL), processID(0), moduleBase(0) {}
    
    bool Init(const char* processName, const char* moduleName) {
        // Get process ID
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return false;
        
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        
        bool found = false;
        if (Process32First(snap, &pe)) {
            do {
                if (_stricmp(pe.szExeFile, processName) == 0) {
                    processID = pe.th32ProcessID;
                    found = true;
                    break;
                }
            } while (Process32Next(snap, &pe));
        }
        CloseHandle(snap);
        
        if (!found) {
            printf("[ERROR] Process '%s' not found!\n", processName);
            return false;
        }
        
        // Open process
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
        if (!hProcess) {
            printf("[ERROR] Failed to open process! Run as Admin!\n");
            return false;
        }
        
        // Get module base
        HANDLE modSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
        if (modSnap == INVALID_HANDLE_VALUE) {
            CloseHandle(hProcess);
            return false;
        }
        
        MODULEENTRY32 me;
        me.dwSize = sizeof(me);
        
        if (Module32First(modSnap, &me)) {
            do {
                if (_stricmp(me.szModule, moduleName) == 0) {
                    moduleBase = (uintptr_t)me.modBaseAddr;
                    break;
                }
            } while (Module32Next(modSnap, &me));
        }
        CloseHandle(modSnap);
        
        if (!moduleBase) {
            printf("[ERROR] Module '%s' not found!\n", moduleName);
            CloseHandle(hProcess);
            return false;
        }
        
        printf("[OK] Attached to %s (PID: %d)\n", processName, processID);
        printf("[OK] %s base: 0x%llX\n", moduleName, moduleBase);
        
        return true;
    }
    
    template<typename T>
    T Read(uintptr_t address) {
        T value{};
        ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), NULL);
        return value;
    }
    
    template<typename T>
    void Write(uintptr_t address, T value) {
        WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), NULL);
    }
    
    void ReadRaw(uintptr_t address, void* buffer, size_t size) {
        ReadProcessMemory(hProcess, (LPCVOID)address, buffer, size, NULL);
    }
    
    void WriteRaw(uintptr_t address, void* buffer, size_t size) {
        WriteProcessMemory(hProcess, (LPVOID)address, buffer, size, NULL);
    }
    
    ~ProcessMemory() {
        if (hProcess) CloseHandle(hProcess);
    }
};
