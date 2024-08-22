#include "stdafx.h"
#include "helper.hpp"

#include <inipp/inipp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <safetyhook.hpp>

HMODULE baseModule = GetModuleHandle(NULL);

// Version
std::string sFixName = "FFXVIFix";
std::string sFixVer = "0.7.2";
std::string sLogFile = sFixName + ".log";

// Logger
std::shared_ptr<spdlog::logger> logger;
std::filesystem::path sExePath;
std::string sExeName;

// Ini
inipp::Ini<char> ini;
std::string sConfigFile = sFixName + ".ini";
std::pair DesktopDimensions = { 0,0 };

// Ini variables
bool bFixResolution;
bool bFixHUD;
bool bFixFOV;
float fAdditionalFOV;
bool bUncapFPS;

// Aspect ratio + HUD stuff
float fPi = (float)3.141592653;
float fAspectRatio;
float fNativeAspect = (float)16 / 9;
float fAspectMultiplier;
float fHUDWidth;
float fHUDHeight;
float fHUDWidthOffset;
float fHUDHeightOffset;

// Variables
int iCurrentResX;
int iCurrentResY;

void CalculateAspectRatio(bool bLog)
{
    // Calculate aspect ratio
    fAspectRatio = (float)iCurrentResX / (float)iCurrentResY;
    fAspectMultiplier = fAspectRatio / fNativeAspect;

    // HUD variables
    fHUDWidth = iCurrentResY * fNativeAspect;
    fHUDHeight = (float)iCurrentResY;
    fHUDWidthOffset = (float)(iCurrentResX - fHUDWidth) / 2;
    fHUDHeightOffset = 0;
    if (fAspectRatio < fNativeAspect) {
        fHUDWidth = (float)iCurrentResX;
        fHUDHeight = (float)iCurrentResX / fNativeAspect;
        fHUDWidthOffset = 0;
        fHUDHeightOffset = (float)(iCurrentResY - fHUDHeight) / 2;
    }

    if (bLog) {
        // Log details about current resolution
        spdlog::info("----------");
        spdlog::info("Current Resolution: Resolution: {}x{}", iCurrentResX, iCurrentResY);
        spdlog::info("Current Resolution: fAspectRatio: {}", fAspectRatio);
        spdlog::info("Current Resolution: fAspectMultiplier: {}", fAspectMultiplier);
        spdlog::info("Current Resolution: fHUDWidth: {}", fHUDWidth);
        spdlog::info("Current Resolution: fHUDHeight: {}", fHUDHeight);
        spdlog::info("Current Resolution: fHUDWidthOffset: {}", fHUDWidthOffset);
        spdlog::info("Current Resolution: fHUDHeightOffset: {}", fHUDHeightOffset);
        spdlog::info("----------");
    }   
}

void Logging()
{
    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // spdlog initialisation
    {
        try {
            logger = spdlog::basic_logger_st(sFixName.c_str(), sExePath.string() + sLogFile, true);
            spdlog::set_default_logger(logger);

            spdlog::flush_on(spdlog::level::debug);
            spdlog::info("----------");
            spdlog::info("{} v{} loaded.", sFixName.c_str(), sFixVer.c_str());
            spdlog::info("----------");
            spdlog::info("Path to logfile: {}", sExePath.string() + sLogFile);
            spdlog::info("----------");

            // Log module details
            spdlog::info("Module Name: {0:s}", sExeName.c_str());
            spdlog::info("Module Path: {0:s}", sExePath.string());
            spdlog::info("Module Address: 0x{0:x}", (uintptr_t)baseModule);
            spdlog::info("Module Timestamp: {0:d}", Memory::ModuleTimestamp(baseModule));
            spdlog::info("----------");
        }
        catch (const spdlog::spdlog_ex& ex) {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
            FreeLibraryAndExitThread(baseModule, 1);
        }
    }
}

void Configuration()
{
    // Initialise config
    std::ifstream iniFile(sExePath.string() + sConfigFile);
    if (!iniFile) {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVer.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sExePath.string().c_str() << std::endl;
        FreeLibraryAndExitThread(baseModule, 1);
    }
    else {
        spdlog::info("Path to config file: {}", sExePath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Parse config
    ini.strip_trailing_comments();
    inipp::get_value(ini.sections["Fix Resolution"], "Enabled", bFixResolution);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bFixHUD);
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFixFOV);
    inipp::get_value(ini.sections["Gameplay FOV"], "AdditionalFOV", fAdditionalFOV);
    inipp::get_value(ini.sections["Remove 30FPS Cap"], "Enabled", bUncapFPS);

    spdlog::info("----------");
    spdlog::info("Config Parse: bFixResolution: {}", bFixResolution);
    spdlog::info("Config Parse: bFixHUD: {}", bFixHUD);
    spdlog::info("Config Parse: bFixFOV: {}", bFixFOV);
    if (fAdditionalFOV < (float)-80 || fAdditionalFOV >(float)80) {
        fAdditionalFOV = std::clamp(fAdditionalFOV, (float)-80, (float)80);
        spdlog::warn("Config Parse: fAdditionalFOV value invalid, clamped to {}", fAdditionalFOV);
    }
    spdlog::info("Config Parse: fAdditionalFOV: {}", fAdditionalFOV);
    spdlog::info("Config Parse: bUncapFPS: {}", bUncapFPS);
    spdlog::info("----------");

    // Grab desktop resolution/aspect
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();
    iCurrentResX = DesktopDimensions.first;
    iCurrentResY = DesktopDimensions.second;
    CalculateAspectRatio(false);
}

void Resolution()
{
    if (bFixResolution) {
        // Fix borderles/fullscreen resolution
        uint8_t* ResolutionFixScanResult = Memory::PatternScan(baseModule, "45 ?? ?? 74 ?? 41 ?? ?? C5 ?? ?? ?? C4 ?? ?? ?? ?? 41 ?? ?? C5 ?? ?? ??");
        if (ResolutionFixScanResult) {
            spdlog::info("Resolution Fix: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)ResolutionFixScanResult - (uintptr_t)baseModule);
            // Stop resolution for being scaled to 16:9 in borderless/fullscreen.
            Memory::PatchBytes((uintptr_t)ResolutionFixScanResult + 0x3, "\xEB", 1);
            spdlog::info("Resolution Fix: Patched instruction.");
        }
        else if (!ResolutionFixScanResult) {
            spdlog::error("Resolution Fix: Pattern scan failed.");
        }
    }

    // Current Resolution
    uint8_t* CurrentResolutionScanResult = Memory::PatternScan(baseModule, "48 89 ?? ?? 8B ?? ?? ?? ?? ?? C5 ?? ?? ?? C4 ?? ?? ?? ?? 8B ?? ?? ?? ?? ??");
    if (CurrentResolutionScanResult) {
        spdlog::info("Current Resolution: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)CurrentResolutionScanResult - (uintptr_t)baseModule);
        static SafetyHookMid CurrentResolutionMidHook{};
        CurrentResolutionMidHook = safetyhook::create_mid(CurrentResolutionScanResult,
            [](SafetyHookContext& ctx) {
                // Get current resolution
                int iResX = static_cast<int>(ctx.rax & 0xFFFFFFFF);
                int iResY = static_cast<int>((ctx.rax >> 32) & 0xFFFFFFFF);

                // Log resolution
                if (iResX != iCurrentResX || iResY != iCurrentResY) {
                    iCurrentResX = iResX;
                    iCurrentResY = iResY;
                    CalculateAspectRatio(true);
                }
            });
    }
    else if (!CurrentResolutionScanResult) {
        spdlog::error("Current Resolution: Pattern scan failed.");
    }

    // FSR Framegen Fix
    uint8_t* FSRFramegenAspectScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? ?? ?? C4 ?? ?? ?? ?? 41 ?? ?? ?? C5 ?? ?? ??");
    if (FSRFramegenAspectScanResult) {
        spdlog::info("FSR Framegen Aspect: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FSRFramegenAspectScanResult - (uintptr_t)baseModule);
        static SafetyHookMid FSRFramegenAspectMidHook{};
        FSRFramegenAspectMidHook = safetyhook::create_mid(FSRFramegenAspectScanResult + 0x8,
            [](SafetyHookContext& ctx) {
                ctx.xmm0.f32[0] = fAspectRatio;
            });
    }
    else if (!FSRFramegenAspectScanResult) {
        spdlog::error("FSR Framegen Aspect: Pattern scan failed.");
    }
}

void HUD()
{
    if (bFixHUD) {
        // HUD size
        uint8_t* HUDSizeScanResult = Memory::PatternScan(baseModule, "45 ?? ?? 44 ?? ?? 41 ?? ?? 48 8B ?? ?? 48 ?? ?? 74 ?? 48 ?? ?? 48 ?? ??");
        if (HUDSizeScanResult) {
            spdlog::info("HUD Size: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)HUDSizeScanResult - (uintptr_t)baseModule);
            static SafetyHookMid HUDSizeMidHook{};
            HUDSizeMidHook = safetyhook::create_mid(HUDSizeScanResult + 0x9,
                [](SafetyHookContext& ctx) {
                    if (bFixHUD) {
                        // Make the hud size the same as the current resolution
                        ctx.rsi = ctx.r13;
                        ctx.rbp = ctx.r12;

                        // Pillarboxing/letterboxing
                        ctx.r14 = 0;
                        ctx.r15 = 0;
                    }
                });
        }
        else if (!HUDSizeScanResult) {
            spdlog::error("HUD Size: Pattern scan failed.");
        }

        // HUD pillarboxing
        uint8_t* HUDPillarboxingScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? ?? ?? ?? 48 85 ?? 74 ?? C5 ?? ?? ?? C5 ?? ?? ?? ?? ?? 44 ?? ?? 76 ??");
        if (HUDPillarboxingScanResult) {
            spdlog::info("HUD Pillarboxing: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)HUDPillarboxingScanResult - (uintptr_t)baseModule);
            static SafetyHookMid HUDPillarboxingMidHook{};
            HUDPillarboxingMidHook = safetyhook::create_mid(HUDPillarboxingScanResult,
                [](SafetyHookContext& ctx) {
                    ctx.xmm5.f32[0] = fAspectRatio;
                });
        }
        else if (!HUDPillarboxingScanResult) {
            spdlog::error("HUD Pillarboxing: Pattern scan failed.");
        }

        // Movies
        uint8_t* MoviesScanResult = Memory::PatternScan(baseModule, "C4 ?? ?? ?? ?? ?? C5 ?? ?? ?? ?? C5 ?? ?? ?? 48 ?? ?? ?? 4C ?? ?? ?? C5 ?? ?? ?? 48 ?? ?? ??");
        if (MoviesScanResult) {
            spdlog::info("Movies: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MoviesScanResult - (uintptr_t)baseModule);
            static SafetyHookMid MoviesMidHook{};
            MoviesMidHook = safetyhook::create_mid(MoviesScanResult + 0x6,
                [](SafetyHookContext& ctx) {
                    float Width = ctx.xmm0.f32[0];
                    float Height = ctx.xmm2.f32[0];

                    if (fAspectRatio > fNativeAspect) {
                        float HUDWidth = ctx.xmm2.f32[0] * fNativeAspect;
                        float WidthOffset = (Width - HUDWidth) / 2.00f;
                        ctx.xmm0.f32[0] = HUDWidth + WidthOffset;
                        ctx.xmm1.f32[0] = WidthOffset;
                    }
                    else if (fAspectRatio < fNativeAspect) {
                        float HUDHeight = ctx.xmm0.f32[0] / fNativeAspect;
                        float HeightOffset = (Height - HUDHeight) / 2.00f;
                        ctx.xmm2.f32[0] = HUDHeight + HeightOffset;
                        ctx.xmm3.f32[0] = HeightOffset;
                    }
                });
        }
        else if (!MoviesScanResult) {
            spdlog::error("Movies: Pattern scan failed.");
        }
    }
}

void FOV()
{
    if (bFixFOV) { 
        // Fix <16:9 FOV
        uint8_t* FOVScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? 89 ?? ?? ?? ?? ?? 45 ?? ?? 48 8B ?? ?? ?? ?? ?? 48 85 ?? 74 ??");
        if (FOVScanResult) {
            spdlog::info("FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FOVScanResult - (uintptr_t)baseModule);
            static SafetyHookMid FOVMidHook{};
            FOVMidHook = safetyhook::create_mid(FOVScanResult,
                [](SafetyHookContext& ctx) {
                    // Fix cropped FOV when at <16:9
                    if (fAspectRatio < fNativeAspect) {
                        ctx.xmm11.f32[0] = 2.0f * atanf(tanf(ctx.xmm11.f32[0] / 2.0f) * (fNativeAspect / fAspectRatio));
                    }
                });
        }
        else if (!FOVScanResult) {
            spdlog::error("FOV: Pattern scan failed.");
        }
    }

    if (fAdditionalFOV != 0.00f) {
        // Gameplay FOV
        uint8_t* GameplayFOVScanResult = Memory::PatternScan(baseModule, "48 8D ?? ?? ?? ?? ?? C3 C5 FA ?? ?? ?? ?? ?? 00 C5 FA ?? ?? ?? ?? ?? ?? C5 F2 ?? ?? ?? ?? ?? ?? C3");
        if (GameplayFOVScanResult) {
            spdlog::info("Gameplay FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameplayFOVScanResult - (uintptr_t)baseModule);
            static SafetyHookMid GameplayFOVMidHook{};
            GameplayFOVMidHook = safetyhook::create_mid(GameplayFOVScanResult + 0x10,
                [](SafetyHookContext& ctx) {
                    ctx.xmm0.f32[0] += fAdditionalFOV;
                });
        }
        else if (!GameplayFOVScanResult) {
            spdlog::error("Gameplay FOV: Pattern scan failed.");
        }
    }
}

void Framerate()
{
    if (bUncapFPS) {  
        // Remove 30fps framerate cap
        uint8_t* FramerateCapScanResult = Memory::PatternScan(baseModule, "75 ?? 85 ?? 74 ?? 40 ?? 01 41 ?? ?? ?? ?? ?? ?? ??");
        if (FramerateCapScanResult) {
            spdlog::info("Framerate Cap: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FramerateCapScanResult - (uintptr_t)baseModule);
            Memory::PatchBytes((uintptr_t)FramerateCapScanResult + 0x8, "\x00", 1);
            spdlog::info("Framerate Cap: Patched instruction.");
        }
        else if (!FramerateCapScanResult) {
            spdlog::error("Framerate Cap: Pattern scan failed.");
        }
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    Configuration();
    Resolution();
    HUD();
    FOV();
    Framerate();
    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle)
        {
            CloseHandle(mainHandle);
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}