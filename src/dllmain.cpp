#include "stdafx.h"
#include "helper.hpp"

#include <inipp/inipp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <safetyhook.hpp>

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE thisModule; // Fix DLL

// Version
std::string sFixName = "FFXVIFix";
std::string sFixVer = "0.8.3";
std::string sLogFile = sFixName + ".log";

// Logger
std::shared_ptr<spdlog::logger> logger;
std::filesystem::path sExePath;
std::string sExeName;
std::filesystem::path sThisModulePath;

// Ini
inipp::Ini<char> ini;
std::string sConfigFile = sFixName + ".ini";
std::pair DesktopDimensions = { 0,0 };

// Ini variables
bool bFixResolution;
int iWindowedResX;
int iWindowedResY;
bool bFixHUD;
int iHUDSize;
bool bFixMovies;
bool bFixFOV;
float fGameplayCamFOV;
float fGameplayCamHorPos;
float fGameplayCamDistMulti;
bool bUncapFPS;
float fFPSCap;
bool bCutsceneFramegen;
bool bMotionBlurFramegen;
float fJXLQuality = 75.0f;
int iJXLThreads = 1;
bool bDisableDbgCheck;
bool bDisableDOF;
bool bBackgroundAudio;
bool bLockCursor;
bool bResizableWindow;
bool bDisableScreensaver;
int iMaxDynRes;
int iMinDynRes;
float fLODMulti = 1.00f;

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
float fEikonCursorWidthOffset;
float fEikonCursorHeightOffset;
bool bIsMoviePlaying = false;
LPCWSTR sWindowClassName = L"FAITHGame";

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

// Spdlog sink (truncate on startup, single file)
template<typename Mutex>
class size_limited_sink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit size_limited_sink(const std::string& filename, size_t max_size)
        : _filename(filename), _max_size(max_size) {
        truncate_log_file();

        _file.open(_filename, std::ios::app);
        if (!_file.is_open()) {
            throw spdlog::spdlog_ex("Failed to open log file " + filename);
        }
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (std::filesystem::exists(_filename) && std::filesystem::file_size(_filename) >= _max_size) {
            return;
        }

        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        _file.write(formatted.data(), formatted.size());
        _file.flush();
    }

    void flush_() override {
        _file.flush();
    }

private:
    std::ofstream _file;
    std::string _filename;
    size_t _max_size;

    void truncate_log_file() {
        if (std::filesystem::exists(_filename)) {
            std::ofstream ofs(_filename, std::ofstream::out | std::ofstream::trunc);
            ofs.close();
        }
    }
};

void Logging()
{
    // Get this module path
    WCHAR thisModulePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(thisModule, thisModulePath, MAX_PATH);
    sThisModulePath = thisModulePath;
    sThisModulePath = sThisModulePath.remove_filename();

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // spdlog initialisation
    {
        try {
            // Create 10MB truncated logger
            logger = logger = std::make_shared<spdlog::logger>(sLogFile, std::make_shared<size_limited_sink<std::mutex>>(sThisModulePath.string() + sLogFile, 10 * 1024 * 1024));
            spdlog::set_default_logger(logger);

            spdlog::flush_on(spdlog::level::debug);
            spdlog::info("----------");
            spdlog::info("{} v{} loaded.", sFixName.c_str(), sFixVer.c_str());
            spdlog::info("----------");
            spdlog::info("Log file: {}", sThisModulePath.string() + sLogFile);
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
    std::ifstream iniFile(sThisModulePath.string() + sConfigFile);
    if (!iniFile) {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVer.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sThisModulePath.string().c_str() << std::endl;
        FreeLibraryAndExitThread(baseModule, 1);
    }
    else {
        spdlog::info("Config file: {}", sThisModulePath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Parse config
    ini.strip_trailing_comments();
    inipp::get_value(ini.sections["Fix Resolution"], "Enabled", bFixResolution);
    inipp::get_value(ini.sections["Fix Resolution"], "WindowedResX", iWindowedResX);
    inipp::get_value(ini.sections["Fix Resolution"], "WindowedResY", iWindowedResY);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bFixHUD);
    inipp::get_value(ini.sections["Fix HUD"], "HUDSize", iHUDSize);
    inipp::get_value(ini.sections["Fix Movies"], "Enabled", bFixMovies);
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFixFOV);
    inipp::get_value(ini.sections["Gameplay Camera"], "AdditionalFOV", fGameplayCamFOV);
    inipp::get_value(ini.sections["Gameplay Camera"], "HorizontalPos", fGameplayCamHorPos);
    inipp::get_value(ini.sections["Gameplay Camera"], "DistanceMultiplier", fGameplayCamDistMulti);
    inipp::get_value(ini.sections["Remove 30FPS Cap"], "Enabled", bUncapFPS);
    inipp::get_value(ini.sections["Remove 30FPS Cap"], "Framerate", fFPSCap);
    inipp::get_value(ini.sections["Cutscene Frame Generation"], "Enabled", bCutsceneFramegen);
    inipp::get_value(ini.sections["Motion Blur + Frame Generation"], "Enabled", bMotionBlurFramegen);
    inipp::get_value(ini.sections["JPEG XL Tweaks"], "NumThreads", iJXLThreads);
    inipp::get_value(ini.sections["JPEG XL Tweaks"], "Quality", fJXLQuality);
    inipp::get_value(ini.sections["Disable Graphics Debugger Check"], "Enabled", bDisableDbgCheck);
    inipp::get_value(ini.sections["Disable Depth of Field"], "Enabled", bDisableDOF);
    inipp::get_value(ini.sections["Game Window"], "BackgroundAudio", bBackgroundAudio);
    inipp::get_value(ini.sections["Game Window"], "LockCursor", bLockCursor);
    inipp::get_value(ini.sections["Game Window"], "Resizable", bResizableWindow);
    inipp::get_value(ini.sections["Game Window"], "DisableScreensaver", bDisableScreensaver);
    inipp::get_value(ini.sections["Dynamic Resolution"], "MaxResolution", iMaxDynRes);
    inipp::get_value(ini.sections["Dynamic Resolution"], "MinResolution", iMinDynRes);
    inipp::get_value(ini.sections["Level of Detail"], "Multiplier", fLODMulti);

    spdlog::info("----------");
    spdlog::info("Config Parse: bFixResolution: {}", bFixResolution);
    spdlog::info("Config Parse: iWindowedResX: {}", iWindowedResX);
    spdlog::info("Config Parse: iWindowedResY: {}", iWindowedResY);
    spdlog::info("Config Parse: bFixHUD: {}", bFixHUD);
    spdlog::info("Config Parse: iHUDSize: {}", iHUDSize);
    spdlog::info("Config Parse: bFixMovies: {}", bFixMovies);
    spdlog::info("Config Parse: bFixFOV: {}", bFixFOV);
    if (fGameplayCamFOV < -40.00f || fGameplayCamFOV > 140.00f) {
        fGameplayCamFOV = std::clamp(fGameplayCamFOV, -40.00f, 140.00f);
        spdlog::warn("Config Parse: fGameplayCamFOV value invalid, clamped to {}", fGameplayCamFOV);
    }
    spdlog::info("Config Parse: fGameplayCamFOV: {}", fGameplayCamFOV);
    if (fGameplayCamHorPos < -5.00f || fGameplayCamHorPos > 5.00f) {
        fGameplayCamHorPos = std::clamp(fGameplayCamHorPos, -5.00f, 5.00f);
        spdlog::warn("Config Parse: fGameplayCamHorPos value invalid, clamped to {}", fGameplayCamHorPos);
    }
    spdlog::info("Config Parse: fGameplayCamHorPos: {}", fGameplayCamHorPos);
    if (fGameplayCamDistMulti < 0.10f || fGameplayCamDistMulti > 10.00f) {
        fGameplayCamDistMulti = std::clamp(fGameplayCamDistMulti, 0.10f, 10.00f);
        spdlog::warn("Config Parse: fGameplayCamDistMulti value invalid, clamped to {}", fGameplayCamDistMulti);
    }
    spdlog::info("Config Parse: fGameplayCamDistMulti: {}", fGameplayCamDistMulti);
    spdlog::info("Config Parse: bUncapFPS: {}", bUncapFPS);
    if (fFPSCap < 10.00f) { // Don't go lower than 10fps if someone messes up.
        fFPSCap = 10.00f;
        spdlog::warn("Config Parse: fFPSCap value invalid, set to {}", fFPSCap);
    }
    spdlog::info("Config Parse: fFPSCap: {}", fFPSCap);
    spdlog::info("Config Parse: bCutsceneFramegen: {}", bCutsceneFramegen);
    spdlog::info("Config Parse: bMotionBlurFramegen: {}", bMotionBlurFramegen);
    if (iJXLThreads > (int)std::thread::hardware_concurrency() || iJXLThreads < 1 ) {
        iJXLThreads = 1;
        spdlog::warn("Config Parse: iJXLThreads value invalid, set to {}", iJXLThreads);
    }
    spdlog::info("Config Parse: iJXLThreads: {}", iJXLThreads);
    if (fJXLQuality < (float)1 || fJXLQuality >(float)100) {
        fJXLQuality = std::clamp(fJXLQuality, (float)1, (float)100);
        spdlog::warn("Config Parse: fJXLQuality value invalid, clamped to {}", fJXLQuality);
    }
    spdlog::info("Config Parse: fJXLQuality: {}", fJXLQuality);
    spdlog::info("Config Parse: bDisableDbgCheck: {}", bDisableDbgCheck);
    spdlog::info("Config Parse: bDisableDOF: {}", bDisableDOF);
    spdlog::info("Config Parse: bBackgroundAudio: {}", bBackgroundAudio);
    spdlog::info("Config Parse: bLockCursor: {}", bLockCursor);
    spdlog::info("Config Parse: bResizableWindow: {}", bResizableWindow);
    spdlog::info("Config Parse: bDisableScreensaver: {}", bDisableScreensaver);
    if (iMaxDynRes < 50 || iMaxDynRes > 100) {
        iMaxDynRes = std::clamp(iMaxDynRes, 50, 100);
        spdlog::warn("Config Parse: iMaxDynRes value invalid, clamped to {}", iMaxDynRes);
    }
    spdlog::info("Config Parse: iMaxDynRes: {}", iMaxDynRes);
    if (iMinDynRes < 50 || iMinDynRes > 100) {
        iMinDynRes = std::clamp(iMinDynRes, 50, 100);
        spdlog::warn("Config Parse: iMinDynRes value invalid, clamped to {}", iMinDynRes);
    }
    spdlog::info("Config Parse: iMinDynRes: {}", iMinDynRes);
    if ((float)fLODMulti < 0.10f || (float)fLODMulti > 10.00f) {
        fLODMulti = std::clamp((float)fLODMulti, 0.10f, 10.00f);
        spdlog::warn("Config Parse: fLODMulti value invalid, clamped to {}", fLODMulti);
    }
    spdlog::info("Config Parse: fLODMulti: {}", fLODMulti);
    spdlog::info("----------");

    // Grab desktop resolution/aspect
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();
    iCurrentResX = DesktopDimensions.first;
    iCurrentResY = DesktopDimensions.second;
    CalculateAspectRatio(true);
}

void Resolution()
{
    if (bFixResolution) {
        // Startup resolution
        uint8_t* StartupResolutionScanResult = Memory::PatternScan(baseModule, "45 ?? ?? 0F 84 ?? ?? ?? ?? 8B ?? C5 ?? ?? ?? C4 ?? ?? ?? ?? 41 ?? ??");
        if (StartupResolutionScanResult) {
            spdlog::info("Startup Resolution: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)StartupResolutionScanResult - (uintptr_t)baseModule);
            Memory::PatchBytes((uintptr_t)StartupResolutionScanResult + 0x4, "\x85", 1);
            spdlog::info("Startup Resolution: Patched instruction.");
        }
        else if (!StartupResolutionScanResult) {
            spdlog::error("Startup Resolution: Pattern scan failed.");
        }

        // Fix borderless/fullscreen resolution
        uint8_t* ResolutionFixScanResult = Memory::PatternScan(baseModule, "C4 ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? ?? 8D ?? ?? ?? ?? ?? 48 ?? ?? 05 48 8D ?? ?? ?? ?? ?? ?? ?? ??");
        if (ResolutionFixScanResult) {
            spdlog::info("Resolution Fix: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)ResolutionFixScanResult - (uintptr_t)baseModule);

            static SafetyHookMid ResolutionFixMidHook{};
            ResolutionFixMidHook = safetyhook::create_mid(ResolutionFixScanResult + 0x5,
                [](SafetyHookContext& ctx) {
                    ctx.rdi = ctx.r8;
                    ctx.rsi = ctx.r9;
                });
        }
        else if (!ResolutionFixScanResult) {
            spdlog::error("Resolution Fix: Pattern scan failed.");
        }

        // Windowed Resolution
        uint8_t* WindowedResolutionsScanResult = Memory::PatternScan(baseModule, "8B ?? ?? 3B ?? ?? ?? ?? ?? 77 ?? 8B ?? ?? 3B ?? ?? ?? ?? ?? 77 ?? 89 ?? ??");
        if (WindowedResolutionsScanResult) {
            spdlog::info("Windowed Resolutions: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)WindowedResolutionsScanResult - (uintptr_t)baseModule);
            static SafetyHookMid WindowedResolutionsMidHook{};
            WindowedResolutionsMidHook = safetyhook::create_mid(WindowedResolutionsScanResult,
                [](SafetyHookContext& ctx) {
                    // Change first resolution option (seems to be 8K?)
                    if (ctx.rax + 0x4 && ctx.rbx == 0) {
                        if (iWindowedResX == 0 || iWindowedResY == 0) {
                            // Add desktop resolution
                            *reinterpret_cast<int*>(ctx.rax + 0x4) = DesktopDimensions.first;
                            *reinterpret_cast<int*>(ctx.rax + 0x8) = DesktopDimensions.second;
                        }
                        else {
                            // Add custom windowed resolution
                            *reinterpret_cast<int*>(ctx.rax + 0x4) = iWindowedResX;
                            *reinterpret_cast<int*>(ctx.rax + 0x8) = iWindowedResY;
                        }
           
                    }
                });  
        }
        else if (!WindowedResolutionsScanResult) {
            spdlog::error("Windowed Resolution: Pattern scan failed.");
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
    uint8_t* FSRFramegenAspectScanResult = Memory::PatternScan(baseModule, "74 ?? 41 8B ?? ?? C5 FA ?? ?? ?? ?? ?? ?? C5 ?? ?? ?? C4 ?? ?? ?? ?? 41 ?? ?? ??");
    if (FSRFramegenAspectScanResult) {
        spdlog::info("FSR Framegen Aspect: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FSRFramegenAspectScanResult - (uintptr_t)baseModule);

        static SafetyHookMid FSRFramegenAspectMidHook{};
        FSRFramegenAspectMidHook = safetyhook::create_mid(FSRFramegenAspectScanResult + 0xE,
            [](SafetyHookContext& ctx) {
                ctx.xmm0.f32[0] = fAspectRatio;
            });
    }
    else if (!FSRFramegenAspectScanResult) {
        spdlog::error("FSR Framegen Aspect: Pattern scan failed.");
    }

    // Vignette
    uint8_t* VignetteStrengthScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? C5 ?? ?? ?? ?? 41 ?? ?? ?? 00 00 80 3F");
    if (VignetteStrengthScanResult) {
        spdlog::info("Vignette Strength: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)VignetteStrengthScanResult - (uintptr_t)baseModule);

        static SafetyHookMid VignetteStrengthMidHook{};
        VignetteStrengthMidHook = safetyhook::create_mid(VignetteStrengthScanResult + 0x12,
            [](SafetyHookContext& ctx) {
                if (fAspectRatio > fNativeAspect) {
                    if (ctx.r15 + 0x6C) {
                        *reinterpret_cast<float*>(ctx.r15 + 0x6C) = 1.00f / fAspectMultiplier;
                    }
                }
            });
    }
    else if (!VignetteStrengthScanResult) {
        spdlog::error("Vignette Strength: Pattern scan failed.");
    }
}

void HUD()
{
    if (bFixHUD && bFixResolution) {
        // HUD size
        uint8_t* HUDSizeScanResult = Memory::PatternScan(baseModule, "D1 ?? 89 ?? ?? ?? 48 8B ?? ?? 48 85 ?? 74 ?? 48 8B ?? 48 8B ?? FF 50 ?? 48 8B ?? ??");
        if (HUDSizeScanResult) {
            spdlog::info("HUD: HUD Size: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)HUDSizeScanResult - (uintptr_t)baseModule);

            static SafetyHookMid HUDSizeMidHook{};
            HUDSizeMidHook = safetyhook::create_mid(HUDSizeScanResult + 0x6,
                [](SafetyHookContext& ctx) {
                    // Make the hud size the same as the current resolution
                    ctx.r12 = ctx.rdi;
                    ctx.r15 = ctx.rbp;

                    // Pillarboxing/letterboxing
                    ctx.r13 = 0;
                    ctx.rax = 0; // -> [rsp+40]

                    if (ctx.rsp + 0x40) {
                        *reinterpret_cast<int*>(ctx.rsp + 0x40) = 0;
                    }
                });
        }
        else if (!HUDSizeScanResult) {
            spdlog::error("HUD: HUD Size: Pattern scan failed.");
        }

        // HUD pillarboxing
        uint8_t* HUDPillarboxingScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? ?? ?? ?? 48 85 ?? 74 ?? C5 ?? ?? ?? C5 ?? ?? ?? ?? ?? 44 ?? ?? 76 ??");
        if (HUDPillarboxingScanResult) {
            spdlog::info("HUD: HUD Pillarboxing: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)HUDPillarboxingScanResult - (uintptr_t)baseModule);

            static SafetyHookMid HUDPillarboxingMidHook{};
            HUDPillarboxingMidHook = safetyhook::create_mid(HUDPillarboxingScanResult,
                [](SafetyHookContext& ctx) {
                    ctx.xmm5.f32[0] = fAspectRatio;
                });
        }
        else if (!HUDPillarboxingScanResult) {
            spdlog::error("HUD: HUD Pillarboxing: Pattern scan failed.");
        }

        // Gameplay HUD Width
        uint8_t* GameplayHUDWidthScanResult = Memory::PatternScan(baseModule, "C5 F2 ?? ?? ?? ?? ?? ?? 8B ?? 99 2B ?? D1 ?? C5 ?? ?? ?? C5 ?? ?? ??");
        if (GameplayHUDWidthScanResult) {
            spdlog::info("HUD: Gameplay HUD Width: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameplayHUDWidthScanResult - (uintptr_t)baseModule);

            static SafetyHookMid GameplayHUDWidthMidHook{};
            GameplayHUDWidthMidHook = safetyhook::create_mid(GameplayHUDWidthScanResult,
                [](SafetyHookContext& ctx) {
                    if (ctx.xmm2.f32[0] > fNativeAspect) {
                        switch (iHUDSize) {
                        case 0:                       
                            break;                      // Automatic
                        case 1:
                            ctx.xmm1.f32[0] = 1440.00f; // 4:3 (1.333~)
                            break;
                        case 2:
                            ctx.xmm1.f32[0] = 1728.00f; // 16:10 (1.600~)
                            break;
                        case 3:
                            ctx.xmm1.f32[0] = 1920.00f; // 16:9 (1.778~)
                            break;
                        case 4:
                            ctx.xmm1.f32[0] = 2520.00f; // 21:9 (2.333~)
                            break;
                        default:                              
                            break;                      // Automatic
                        }
                        fEikonCursorWidthOffset = (ctx.xmm1.f32[0] - 1920.00f) / 2.00f;
                    }
                });
        }
        else if (!GameplayHUDWidthScanResult) {
            spdlog::error("HUD: Gameplay HUD Width: Pattern scan failed.");
        }

        // Gameplay HUD Height
        uint8_t* GameplayHUDHeightScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? ?? ?? ?? 2B ?? D1 ?? C5 ?? ?? ?? C5 ?? ?? ?? C5 ?? ?? ?? E8 ?? ?? ?? ??");
        if (GameplayHUDHeightScanResult) {
            spdlog::info("HUD: Gameplay HUD Height: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameplayHUDHeightScanResult - (uintptr_t)baseModule);

            static SafetyHookMid GameplayHUDHeightMidHook{};
            GameplayHUDHeightMidHook = safetyhook::create_mid(GameplayHUDHeightScanResult,
                [](SafetyHookContext& ctx) {
                    if (ctx.xmm2.f32[0] < fNativeAspect) {
                        switch (iHUDSize) {
                        case 0:
                            break;                      // Automatic
                        case 1:
                            ctx.xmm0.f32[0] = 1440.00f; // 4:3 (1.333~)
                            break;
                        case 2:
                            ctx.xmm0.f32[0] = 1200.00f; // 16:10 (1.600~)
                            break;
                        case 3:
                            ctx.xmm0.f32[0] = 1080.00f; // 16:9 (1.778~)
                            break;
                        case 4:
                            ctx.xmm0.f32[0] = 823.00f; // 21:9 (2.333~)
                            break;
                        default:
                            break;                      // Automatic
                        }
                        fEikonCursorHeightOffset = (ctx.xmm0.f32[0] - 1080.00f) / 2.00f;
                    }
                });
        }
        else if (!GameplayHUDHeightScanResult) {
            spdlog::error("HUD: Gameplay HUD Height: Pattern scan failed.");
        }

        // Eikon Cursor
        uint8_t* EikonCursorScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? C5 ?? ?? ?? ?? C5 ?? ?? ?? 8B ?? ?? 99 2B ?? D1 ?? C5 ?? ?? ??");
        if (EikonCursorScanResult) {
            spdlog::info("HUD: Eikon Cursor: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)EikonCursorScanResult - (uintptr_t)baseModule);

            static SafetyHookMid EikonCursorWidthOffsetMidHook{};
            EikonCursorWidthOffsetMidHook = safetyhook::create_mid(EikonCursorScanResult + 0x22,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio > fNativeAspect) {
                        ctx.xmm0.f32[0] += fEikonCursorWidthOffset;
                    }
                });

            static SafetyHookMid EikonCursorHeightOffsetMidHook{};
            EikonCursorHeightOffsetMidHook = safetyhook::create_mid(EikonCursorScanResult,
                [](SafetyHookContext& ctx) {
                    if (fAspectRatio < fNativeAspect) {
                        ctx.xmm0.f32[0] += fEikonCursorHeightOffset;
                    }
                });
        }
        else if (!EikonCursorScanResult) {
            spdlog::error("HUD: Eikon Cursor: Pattern scan failed.");
        }

        // Photo mode blur background 
        uint8_t* PhotoModeBgBlurScanResult = Memory::PatternScan(baseModule, "48 8B ?? ?? 48 89 ?? ?? ?? 75 ?? 80 ?? ?? ?? ?? ?? 00 75 ??");
        if (PhotoModeBgBlurScanResult) {
            spdlog::info("HUD: Photo Mode Blur: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)PhotoModeBgBlurScanResult - (uintptr_t)baseModule);

            static SafetyHookMid PhotoModeBgBlurMidHook{};
            PhotoModeBgBlurMidHook = safetyhook::create_mid(PhotoModeBgBlurScanResult,
                [](SafetyHookContext& ctx) {
                    if (ctx.rcx + 0x40) {
                        // Check size, should be 660x1080
                        if ((*reinterpret_cast<int*>(ctx.rcx + 0x40) >= 655 && *reinterpret_cast<int*>(ctx.rcx + 0x40) <= 665) && (*reinterpret_cast<int*>(ctx.rcx + 0x44) >= 1075 && *reinterpret_cast<int*>(ctx.rcx + 0x44) <= 1085)) {
                            // Check horizontal position
                            if ((*reinterpret_cast<float*>(ctx.rcx + 0xB0) >= -675.00f && *reinterpret_cast<float*>(ctx.rcx + 0xB0) <= -665.00f) || (*reinterpret_cast<float*>(ctx.rcx + 0xB0) >= 1925.00f && *reinterpret_cast<float*>(ctx.rcx + 0xB0) <= 1935.00f)) {
                                // Write 0 to width
                                *reinterpret_cast<int*>(ctx.rcx + 0x40) = 0;
                            }
                        }
                    }
                });
        }
        else if (!PhotoModeBgBlurScanResult) {
            spdlog::error("HUD: Photo Mode Blur: Pattern scan failed.");
        }

        // Fades
        uint8_t* FadeToBlackScanResult = Memory::PatternScan(baseModule, "89 ?? ?? 44 ?? ?? ?? C6 ?? ?? ?? ?? ?? 01 74 ?? B8 ?? ?? ?? ?? 89 ?? ?? 89 ?? ?? ?? ?? ?? 80 ?? ?? ?? ?? ?? 00");
        if (FadeToBlackScanResult) {
            spdlog::info("HUD: Fades: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FadeToBlackScanResult - (uintptr_t)baseModule);

            static SafetyHookMid FadeToBlackMidHook{};
            FadeToBlackMidHook = safetyhook::create_mid(FadeToBlackScanResult,
                [](SafetyHookContext& ctx) {
                    // Fade to black is 1940x1100. TODO: Add another check here?
                    if (ctx.rdx == (int)1940 && ctx.r8 == (int)1100) {
                        if (ctx.rcx + 0x38 && ctx.rcx + 0x3C) {
                            if (fAspectRatio > fNativeAspect) {
                                float fWidth = ceilf(1080.00f * fAspectRatio);
                                float fWidthOffset = ceilf((fWidth - 1920.00f) / 2);

                                ctx.rdx = (int)fWidth;
                                *reinterpret_cast<int*>(ctx.rcx + 0x38) = (int)-fWidthOffset;
                            }
                            else if (fAspectRatio < fNativeAspect) {
                                float fHeight = ceilf(1920.00f / fAspectRatio);
                                float fHeightOffset = ceilf((fHeight - 1080.00f) / 2);

                                ctx.r8 = (int)fHeight;
                                *reinterpret_cast<int*>(ctx.rcx + 0x3C) = (int)-fHeightOffset;
                            }
                        }
                    }
                });
        }
        else if (!FadeToBlackScanResult) {
            spdlog::error("HUD: Fades: Pattern scan failed.");
        }
    }

    if (bFixMovies) {
        // Get movie status 
        uint8_t* MovieStatusScanResult = Memory::PatternScan(baseModule, "0F 84 ?? ?? ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? C4 ?? ?? ?? ?? 48 8B ?? 80 ?? ?? ?? ?? ?? 02");
        if (MovieStatusScanResult) {
            spdlog::info("HUD: Movies: Status: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MovieStatusScanResult - (uintptr_t)baseModule);

            static SafetyHookMid MovieStatusMidHook{};
            MovieStatusMidHook = safetyhook::create_mid(MovieStatusScanResult,
                [](SafetyHookContext& ctx) {
                    // Check zero flag 
                    if ((ctx.rflags & (1 << 6)) == 0)
                    {
                        bIsMoviePlaying = true;
                    }
                    else {
                        bIsMoviePlaying = false;
                    }
                });
        }
        else if (!MovieStatusScanResult) {
            spdlog::error("HUD: Movies: Status: Pattern scan failed.");
        }

        // Movie size
        uint8_t* MovieSize1ScanResult = Memory::PatternScan(baseModule, "44 89 ?? ?? ?? 44 89 ?? ?? ?? C5 ?? ?? ?? ?? 41 ?? ?? ?? 77 ??");
        uint8_t* MovieSize2ScanResult = Memory::PatternScan(baseModule, "49 ?? ?? 4C 89 ?? ?? 4C 89 ?? ?? E8 ?? ?? ?? ?? 8B ?? ?? ?? 48 8D ?? ??");
        if (MovieSize1ScanResult && MovieSize2ScanResult) {
            spdlog::info("HUD: Movies: Size: 1: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MovieSize1ScanResult - (uintptr_t)baseModule);
            spdlog::info("HUD: Movies: Size: 2: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MovieSize2ScanResult - (uintptr_t)baseModule);

            static bool bUpdate = false;

            static SafetyHookMid MovieSize1MidHook{};
            MovieSize1MidHook = safetyhook::create_mid(MovieSize1ScanResult,
                [](SafetyHookContext& ctx) {
                    bUpdate = false;

                    // Force an update which forces the second hook to execute
                    if (bIsMoviePlaying) {
                        if (fAspectRatio > fNativeAspect) {
                            ctx.r10 = (int)std::round(fHUDWidth);
                        }
                        else if (fAspectRatio < fNativeAspect) {
                            ctx.r11 = (int)std::round(fHUDHeight);
                        }
                        bUpdate = true;
                    }
                });

            static SafetyHookMid MovieSize2MidHook{};
            MovieSize2MidHook = safetyhook::create_mid(MovieSize2ScanResult,
                [](SafetyHookContext& ctx) {
                    if (bIsMoviePlaying && bUpdate) {
                        if (fAspectRatio > fNativeAspect) {
                            ctx.r9 = (int)std::round(fHUDWidth);
                        }
                        else if (fAspectRatio < fNativeAspect) {
                            ctx.r8 = (int)std::round(fHUDHeight);
                        }
                    }
                });
        }
        else if (!MovieSize1ScanResult || !MovieSize2ScanResult) {
            spdlog::error("HUD: Movies: Size: Pattern scan failed.");
        }

        // Movie offset
        uint8_t* MovieOffsetScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? ?? C5 ?? ?? ?? ?? ?? 49 ?? ?? ?? 4D ?? ?? ?? ?? ?? ?? ?? 4C ?? ?? 4D ?? ??");
        if (MovieOffsetScanResult) {
            spdlog::info("HUD: Movies: Offset: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)MovieOffsetScanResult - (uintptr_t)baseModule);

            static SafetyHookMid MovieOffsetMidHook{};
            MovieOffsetMidHook = safetyhook::create_mid(MovieOffsetScanResult,
                [](SafetyHookContext& ctx) {
                    if (bIsMoviePlaying) {
                        if (fAspectRatio > fNativeAspect) {
                            if (ctx.xmm11.f32[0] == std::round(fHUDWidth)) {
                                ctx.xmm0.f32[0] = fHUDWidthOffset;
                            }
                        }
                        else if (fAspectRatio < fNativeAspect) {
                            if (ctx.xmm12.f32[0] == std::round(fHUDHeight)) {
                                ctx.xmm1.f32[0] = fHUDHeightOffset;
                            }
                        }
                    }
                });
        }
        else if (!MovieOffsetScanResult) {
            spdlog::error("HUD: Movies: Offset: Pattern scan failed.");
        }
    }
}

void FOV()
{
    if (bFixFOV) { 
        // Fix <16:9 FOV
        uint8_t* FOVScanResult = Memory::PatternScan(baseModule, "89 ?? ?? ?? ?? ?? 48 8B ?? ?? 48 8B ?? ?? C5 ?? ?? ?? ?? ?? ?? ?? 48 8B ?? 48 8B ?? FF 90 ?? ?? ?? ??");
        if (FOVScanResult) {
            spdlog::info("FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FOVScanResult - (uintptr_t)baseModule);

            static SafetyHookMid FOVMidHook{};
            FOVMidHook = safetyhook::create_mid(FOVScanResult,
                [](SafetyHookContext& ctx) {
                    // Fix cropped FOV when at <16:9
                    if (fAspectRatio < fNativeAspect) {
                        float fov = *reinterpret_cast<float*>(&ctx.rax);
                        fov = 2.0f * atanf(tanf(fov / 2.0f) * (fNativeAspect / fAspectRatio));
                        ctx.rax = *(uint32_t*)&fov;
                    }     
                });
        }
        else if (!FOVScanResult) {
            spdlog::error("FOV: Pattern scan failed.");
        }
    }

    if (fGameplayCamFOV != 0.00f) {
        // Gameplay FOV
        uint8_t* GameplayFOVScanResult = Memory::PatternScan(baseModule, "48 8D ?? ?? ?? ?? ?? C3 C5 FA ?? ?? ?? ?? ?? 00 C5 FA ?? ?? ?? ?? ?? ?? C5 F2 ?? ?? ?? ?? ?? ?? C3");
        if (GameplayFOVScanResult) {
            spdlog::info("Gameplay Camera: FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameplayFOVScanResult - (uintptr_t)baseModule);

            static SafetyHookMid GameplayFOVMidHook{};
            GameplayFOVMidHook = safetyhook::create_mid(GameplayFOVScanResult + 0x10,
                [](SafetyHookContext& ctx) {
                    ctx.xmm0.f32[0] += fGameplayCamFOV;
                });
        }
        else if (!GameplayFOVScanResult) {
            spdlog::error("Gameplay Camera: FOV: Pattern scan failed.");
        }
    }

    if (fGameplayCamHorPos != 0.95f) {
        // Gameplay Camera Horizontal Position
        uint8_t* GameplayCameraHorPosScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? ?? ?? ?? 8B ?? 41 ?? ?? 48 8D ?? ?? E8 ?? ?? ?? ??");
        if (GameplayCameraHorPosScanResult) {
            spdlog::info("Gameplay Camera: Horizontal Position: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameplayCameraHorPosScanResult - (uintptr_t)baseModule);

            static SafetyHookMid GameplayCameraHorPosMidHook{};
            GameplayCameraHorPosMidHook = safetyhook::create_mid(GameplayCameraHorPosScanResult + 0x8,
                [](SafetyHookContext& ctx) {
                    ctx.xmm1.f32[0] = fGameplayCamHorPos;
                });
        }
        else if (!GameplayCameraHorPosScanResult) {
            spdlog::error("Gameplay Camera: Horizontal Position: Pattern scan failed.");
        }
    }

    if (fGameplayCamDistMulti != 1.00f) {
        // Gameplay Camera Distance
        uint8_t* GameplayCameraDistScanResult = Memory::PatternScan(baseModule, "C5 ?? ?? ?? ?? ?? ?? ?? 48 8D ?? ?? ?? ?? ?? 4C 8D ?? ?? ?? ?? ?? 48 8B ?? C4 ?? ?? ?? ?? E8 ?? ?? ?? ??");
        if (GameplayCameraDistScanResult) {
            spdlog::info("Gameplay Camera: Distance: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GameplayCameraDistScanResult - (uintptr_t)baseModule);

            static SafetyHookMid GameplayCameraDistMidHook{};
            GameplayCameraDistMidHook = safetyhook::create_mid(GameplayCameraDistScanResult,
                [](SafetyHookContext& ctx) {
                    ctx.xmm3.f32[0] *= fGameplayCamDistMulti;
                });
        }
        else if (!GameplayCameraDistScanResult) {
            spdlog::error("Gameplay Camera: Distance: Pattern scan failed.");
        }
    }  
}

void Framerate()
{
    if (!bUncapFPS && fFPSCap != 29.97f) {
        // Adjust cutscene 30fps cap
        uint8_t* CutsceneFramerateCapScanResult = Memory::PatternScan(baseModule, "C7 44 ?? ?? 01 00 00 00 C7 44 ?? ?? ?? ?? 00 00 89 ?? ?? ?? C7 44 ?? ?? ?? ?? 00 00");
        if (CutsceneFramerateCapScanResult) {
            spdlog::info("FPS: Cutscene Framerate Cap: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)CutsceneFramerateCapScanResult - (uintptr_t)baseModule);
            int iFPSCap = static_cast<int>(fFPSCap * 100.00f);
            Memory::Write((uintptr_t)CutsceneFramerateCapScanResult + 0xC, (int)iFPSCap);
            spdlog::info("FPS: Cutscene Framerate Cap: Patched instruction and set framerate cap to {:d}.", iFPSCap);
        }
        else if (!CutsceneFramerateCapScanResult) {
            spdlog::error("FPS: Cutscene Framerate Cap: Pattern scan failed.");
        }
    }

    if (bUncapFPS) {  
        // Remove 30fps framerate cap
        uint8_t* FramerateCapScanResult = Memory::PatternScan(baseModule, "75 ?? 85 ?? 74 ?? 40 ?? 01 41 ?? ?? ?? ?? ?? ?? ??");
        if (FramerateCapScanResult) {
            spdlog::info("FPS: Disable Cutscene Framerate Cap: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FramerateCapScanResult - (uintptr_t)baseModule);
            Memory::PatchBytes((uintptr_t)FramerateCapScanResult + 0x8, "\x00", 1);
            spdlog::info("FPS: Disable Cutscene Framerate Cap: Patched instruction.");
        }
        else if (!FramerateCapScanResult) {
            spdlog::error("FPS: Disable Cutscene Framerate Cap: Pattern scan failed.");
        }
    }

    if (bCutsceneFramegen) {
        // Enable frame generation during real-time cutscenes
        uint8_t* CutsceneFramegenScanResult = Memory::PatternScan(baseModule, "41 ?? ?? 74 ?? 33 ?? 48 ?? ?? E8 ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? D1 ??");
        if (CutsceneFramegenScanResult) {
            spdlog::info("FPS: Cutscene Frame Generation: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)CutsceneFramegenScanResult - (uintptr_t)baseModule);
            Memory::PatchBytes((uintptr_t)CutsceneFramegenScanResult + 0x3, "\xEB", 1);
            spdlog::info("FPS: Cutscene Frame Generation: Patched instruction.");
        }
        else if (!CutsceneFramegenScanResult) {
            spdlog::error("FPS: Cutscene Frame Generation: Pattern scan failed.");
        }
    }
}

void Misc()
{
    if (bMotionBlurFramegen) {
        // Motion blur + frame generation
        uint8_t* FrameGenMotionBlurLockoutScanResult = Memory::PatternScan(baseModule, "0F 85 ?? ?? ?? ?? 44 ?? ?? ?? ?? ?? ?? 44 88 ?? ?? ?? C6 ?? ?? ?? ?? 44 88 ?? ?? ?? E9 ?? ?? ?? ??");
        uint8_t* FrameGenMotionBlurLogicScanResult = Memory::PatternScan(baseModule, "74 ?? C4 ?? ?? ?? ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? C4 ?? ?? ?? ?? ?? ?? ?? ?? EB ?? 41 ?? ?? ?? ?? ?? ?? 74 ??");
        if (FrameGenMotionBlurLockoutScanResult && FrameGenMotionBlurLogicScanResult) {
            spdlog::info("Frame Generation Motion Blur: Menu Lock: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FrameGenMotionBlurLockoutScanResult - (uintptr_t)baseModule);
            spdlog::info("Frame Generation Motion Blur: Logic: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)FrameGenMotionBlurLogicScanResult - (uintptr_t)baseModule);

            // Stop the game menu from hiding motion blur option when frame generation is enabled.
            Memory::PatchBytes((uintptr_t)FrameGenMotionBlurLockoutScanResult, "\x90\x90\x90\x90\x90\x90", 6);
            // Stop the game from setting the motion blur float to 0 when frame generation is enabled.
            Memory::PatchBytes((uintptr_t)FrameGenMotionBlurLogicScanResult, "\xEB", 1);

            spdlog::info("Frame Generation Motion Blur: Patched instructions.");
        }
        else if (!FrameGenMotionBlurLockoutScanResult || !FrameGenMotionBlurLogicScanResult) {
            spdlog::error("Frame Generation Motion Blur: Pattern scan failed.");
        }
    }

    if (bDisableDbgCheck) {
        // Disable graphics debugger check
        uint8_t* GraphicsDbgCheckScanResult = Memory::PatternScan(baseModule, "74 ?? E8 ?? ?? ?? ?? 84 ?? 0F 85 ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 85 ?? 74 ??");
        if (GraphicsDbgCheckScanResult) {
            spdlog::info("Graphics Debugger Check: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GraphicsDbgCheckScanResult - (uintptr_t)baseModule);
            Memory::PatchBytes((uintptr_t)GraphicsDbgCheckScanResult, "\xEB", 1);
            spdlog::info("Graphics Debugger Check: Patched instruction.");
        }
        else if (!GraphicsDbgCheckScanResult) {
            spdlog::error("Graphics Debugger Check: Pattern scan failed.");
        }
    }

    if (bDisableDOF) {
        // Disable depth of field
        uint8_t* DepthofFieldScanResult = Memory::PatternScan(baseModule, "74 ?? 80 ?? ?? 08 73 ?? 44 ?? ?? ?? 41 ?? ?? C3");
        uint8_t* NearDepthofFieldScanResult = Memory::PatternScan(baseModule, "75 ?? 48 ?? ?? ?? 44 ?? ?? ?? ?? ?? ?? 75 ?? 0F ?? ?? ?? 48 ?? ?? ?? ?? ?? ??");
        if (DepthofFieldScanResult && NearDepthofFieldScanResult) {
            spdlog::info("Disable Depth of Field: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)DepthofFieldScanResult - (uintptr_t)baseModule);
            Memory::PatchBytes((uintptr_t)DepthofFieldScanResult, "\xEB", 1);
            spdlog::info("Disable Depth of Field: Patched instruction.");

            spdlog::info("Disable Depth of Field: Near: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)NearDepthofFieldScanResult - (uintptr_t)baseModule);
            Memory::PatchBytes((uintptr_t)NearDepthofFieldScanResult, "\xEB", 1);
            spdlog::info("Disable Depth of Field: Near: Patched instruction.");
        }
        else if (!DepthofFieldScanResult || !NearDepthofFieldScanResult) {
            spdlog::error("Disable Depth of Field: Pattern scan failed.");
        }
    }

    if (iMaxDynRes != 95 || iMinDynRes != 50) {
        // Dynamic resolution upper/lower bounds
        uint8_t* DynamicResBoundsScanResult = Memory::PatternScan(baseModule, "0F ?? ?? ?? 3A ?? 0F ?? ?? 0F ?? ?? 0F ?? ?? ?? 3B ?? 0F ?? ?? 3B ?? 0F ?? ?? ??");
        if (DynamicResBoundsScanResult) {
            spdlog::info("Dynamic Resolution: Bounds: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)DynamicResBoundsScanResult - (uintptr_t)baseModule);

            static SafetyHookMid DynamicResBoundsMidHook{};
            DynamicResBoundsMidHook = safetyhook::create_mid(DynamicResBoundsScanResult,
                [](SafetyHookContext& ctx) {
                    if (ctx.rdi + 0x22) {
                        *reinterpret_cast<BYTE*>(ctx.rdi + 0x22) = static_cast<BYTE>(iMaxDynRes); // Max scale
                        *reinterpret_cast<BYTE*>(ctx.rdi + 0x20) = static_cast<BYTE>(iMinDynRes); // Min scale
                    }
                });
        }
        else if (!DynamicResBoundsScanResult) {
            spdlog::error("Dynamic Resolution: Bounds: Pattern scan failed.");
        }
    }

    if (fLODMulti != 1.00f) {
        // LOD distance
        uint8_t* LevelOfDetailScanResult = Memory::PatternScan(baseModule, "44 ?? ?? ?? ?? ?? ?? 75 ?? C5 ?? ?? ?? ?? ?? ?? ?? C5 ?? ?? E8 ?? ?? ?? ??");
        if (LevelOfDetailScanResult) {
            spdlog::info("LOD Distance: Framerate: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)LevelOfDetailScanResult - (uintptr_t)baseModule);

            static SafetyHookMid LevelOfDetailMidHook{};
            LevelOfDetailMidHook = safetyhook::create_mid(LevelOfDetailScanResult,
                [](SafetyHookContext& ctx) {
                    ctx.xmm6.f32[0] *= fLODMulti;
                });
        }
        else if (!LevelOfDetailScanResult) {
            spdlog::error("LOD Distance: Framerate: Pattern scan failed.");
        }
    }
}

// JXL Hooks
SafetyHookInline JxlEncoderDistanceFromQuality_sh{};
float JxlEncoderDistanceFromQuality_hk(float quality)
{
    quality = fJXLQuality;
    spdlog::info("JXL Tweaks: JxlEncoderDistanceFromQuality: Quality level = {}", quality);
    return JxlEncoderDistanceFromQuality_sh.fastcall<float>(quality);
}

SafetyHookInline JxlThreadParallelRunnerDefaultNumWorkerThreads_sh{};
size_t JxlThreadParallelRunnerDefaultNumWorkerThreads_hk(void)
{
    spdlog::info("JXL Tweaks: JxlThreadParallelRunnerDefaultNumWorkerThreads: NumThreads = {}", iJXLThreads);
    return iJXLThreads;
}

void JXL()
{
    // JXL Tweaks
    while (!GetModuleHandle(L"jxl.dll") || !GetModuleHandle(L"jxl_threads.dll")) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    HMODULE jxlLib = GetModuleHandle(L"jxl.dll");
    HMODULE jxlThreadsLib = GetModuleHandle(L"jxl_threads.dll");

    if (!jxlLib || !jxlThreadsLib) {
        spdlog::info("JXL Tweaks: Failed to get module handle for JXL libraries.");
        return;
    }

    FARPROC JxlEncoderDistanceFromQuality_fn = GetProcAddress(jxlLib, "JxlEncoderDistanceFromQuality");
    FARPROC JxlThreadParallelRunnerDefaultNumWorkerThreads_fn = GetProcAddress(jxlThreadsLib, "JxlThreadParallelRunnerDefaultNumWorkerThreads");

    if (!JxlEncoderDistanceFromQuality_fn || !JxlThreadParallelRunnerDefaultNumWorkerThreads_fn) {
        spdlog::info("JXL Tweaks: Failed to get function addresses.");
        return;
    }

    spdlog::info("JXL Tweaks: JxlEncoderDistanceFromQuality address = {:x}", (uintptr_t)JxlEncoderDistanceFromQuality_fn);
    spdlog::info("JXL Tweaks: JxlThreadParallelRunnerDefaultNumWorkerThreads address = {:x}", (uintptr_t)JxlThreadParallelRunnerDefaultNumWorkerThreads_fn);

    JxlEncoderDistanceFromQuality_sh = safetyhook::create_inline(JxlEncoderDistanceFromQuality_fn, reinterpret_cast<void*>(JxlEncoderDistanceFromQuality_hk));
    JxlThreadParallelRunnerDefaultNumWorkerThreads_sh = safetyhook::create_inline(JxlThreadParallelRunnerDefaultNumWorkerThreads_fn, reinterpret_cast<void*>(JxlThreadParallelRunnerDefaultNumWorkerThreads_hk));
    spdlog::info("JXL Tweaks: Hooked functions.");
}

HHOOK   hkCallWndProc  = NULL;
WNDPROC OldWndProc     = NULL;
HWND    hWndGame       = NULL;
BOOL    bWindowFocused = FALSE;
BOOL    bSizeMove      = FALSE;

SafetyHookInline ShowWindow_sh{};
BOOL
WINAPI
ShowWindow_hk(
  _In_ HWND hWnd,
  _In_ int  nCmdShow)
{
    // Never let the game hide the window.
    //
    // That will cause spurious window activation after resuming from
    // display sleep/screensaver
    if (hWnd == hWndGame && hWnd != 0) {
        if (nCmdShow == SW_HIDE) {
            return TRUE;
        }
    }

    return ShowWindow_sh.stdcall<BOOL>(hWnd, nCmdShow);
}

SafetyHookInline ClipCursor_sh{};
BOOL ClipCursor_hk(const RECT *lpRect)
{
    RECT bounds = {};

    if (bWindowFocused == FALSE || bSizeMove || GetForegroundWindow () != hWndGame) {
        lpRect = nullptr;
    }
    
    else if (bLockCursor && bWindowFocused) {
        GetWindowRect(hWndGame, &bounds);
        InflateRect(&bounds, -7, -7); // Keep the cursor within the grabby bits of the window frame

        lpRect = &bounds;
    }

    return ClipCursor_sh.stdcall<BOOL>(lpRect);
}
LRESULT __stdcall NewWndProc(HWND window, UINT message_type, WPARAM w_param, LPARAM l_param)
{
    if (window != hWndGame)
        return CallWindowProc(OldWndProc, window, message_type, w_param, l_param);

    // Game does not process Win32 keyboard or mouse messages, and this causes
    //   windows to beep on system keys and other events. Just block all of these
    if ((message_type >= WM_KEYFIRST   && message_type <= WM_KEYLAST) ||
         message_type >= WM_MOUSEFIRST && message_type <= WM_MOUSELAST)
    {
      DefWindowProcW(window, message_type, w_param, l_param);
      return 0;
    }

    BOOL style_changed    = FALSE;
    BOOL need_clip_cursor = FALSE;

    HWND hWndForeground =
          GetForegroundWindow ();

    if (hWndForeground == hWndGame || GetTopWindow (NULL) == hWndGame) {
        if (!std::exchange(bWindowFocused, true)) {
            // Get styles
            LONG lStyle   = GetWindowLong(window, GWL_STYLE);
            LONG lExStyle = GetWindowLong(window, GWL_EXSTYLE);

            // Add re-sizable style and enable maximize button
            if (bResizableWindow) {
                // Check for borderless/fullscreen styles
                if ((lStyle & WS_THICKFRAME) != WS_THICKFRAME && (lStyle & WS_POPUP) == 0 && (lExStyle & WS_EX_TOPMOST) == 0) {
                    // Add resizable + maximize styles
                    lStyle |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
                    SetWindowLong(window, GWL_STYLE, lStyle);

                    style_changed = TRUE;
                }
            }

            need_clip_cursor = TRUE;
            CallWindowProc(OldWndProc, hWndGame, WM_ACTIVATE, WA_CLICKACTIVE, 0);
        }
    }
    else if (! bBackgroundAudio) {
        if (std::exchange(bWindowFocused, false)) {
            need_clip_cursor = TRUE;
            CallWindowProc(OldWndProc, hWndGame, WM_ACTIVATE, WA_INACTIVE, 0);
        }
    }

    switch (message_type) {
    case WM_SYSCOMMAND:
        switch (LOWORD (w_param & 0xFFF0))
        {
            case SC_SCREENSAVE:
            case SC_MONITORPOWER: // Many users are experiencing display shutoff during lengthy cutscenes (!!)
            {
                if (bDisableScreensaver && bWindowFocused)
                {
                    if (l_param != -1) // -1 == Monitor Power On, we do not want to block that!
                        return TRUE;
                }
            }
        }
        break;

    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
        bSizeMove        = (message_type == WM_ENTERSIZEMOVE);
        need_clip_cursor = TRUE;
        break;

    case WM_KILLFOCUS:
    case WM_SETFOCUS:
    {
        need_clip_cursor = TRUE;
    } break;

    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
    case WM_NCACTIVATE:
        return DefWindowProcW(window, message_type, w_param, l_param);
    }

    if (need_clip_cursor)
        ClipCursor_hk(NULL); // The hook will calculate the actual rectangle to use

    if (style_changed) // Force window to update
      SetWindowPos(window, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE   |
                                                   SWP_NOSIZE | SWP_NOZORDER | SWP_NOREPOSITION);

    return CallWindowProc(OldWndProc, window, message_type, w_param, l_param);
};

void SubclassGameWindow (HWND hWnd)
{
    FARPROC ClipCursor_fn = GetProcAddress(GetModuleHandleW (L"user32.dll"), "ClipCursor");
    ClipCursor_sh = safetyhook::create_inline(ClipCursor_fn, reinterpret_cast<void*>(ClipCursor_hk));

    FARPROC ShowWindow_fn = GetProcAddress(GetModuleHandleW (L"user32.dll"), "ShowWindow");
    ShowWindow_sh = safetyhook::create_inline(ShowWindow_fn, reinterpret_cast<void*>(ShowWindow_hk));

    // Set new wnd proc
    OldWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)NewWndProc);
    spdlog::info("Window Focus: Subclassed Game Window");

    hWndGame = hWnd;
}

LRESULT CALLBACK CallWndProcHook (
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam )
{
    LRESULT ret =
        CallNextHookEx (hkCallWndProc, nCode, wParam, lParam);

    if (nCode < 0)
        return ret;

    wchar_t wnd_class_name [32];
    HWND    hWndMsg = ((CWPSTRUCT *)lParam)->hwnd;

    if (RealGetWindowClassW (hWndMsg, wnd_class_name, 31) > 0) {
        if (! wcscmp (wnd_class_name, sWindowClassName)) {
            UnhookWindowsHookEx (hkCallWndProc);
            SubclassGameWindow  (hWndMsg);

            auto lExStyle = GetWindowLongPtrW(hWndGame, GWL_EXSTYLE);

            // The game does not have AppWindow style, so it does not
            // correctly register itself to appear in the taskbar and
            // activate the way applications are supposed to...
            if ((lExStyle & WS_EX_APPWINDOW) == 0) {
                lExStyle |= WS_EX_APPWINDOW;
                SetWindowLongPtrW(hWndGame, GWL_EXSTYLE, lExStyle);
            }
        }
    }

    return ret;
}

#include <tlhelp32.h>
DWORD GetMainThreadId (void)
{
    THREADENTRY32 te32 { .dwSize = sizeof (THREADENTRY32) };

    const DWORD dwProcId   = GetCurrentProcessId ();
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0);

    if (Thread32First (hThreadSnapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == dwProcId) {
                return te32.th32ThreadID;
            }
        } while (Thread32Next (hThreadSnapshot, &te32));
    }

    return 0;
}

void WindowFocus()
{
    if (bBackgroundAudio || bLockCursor || bResizableWindow || bDisableScreensaver) {
        // Hook wndproc and then subclass the window when we find the game's main window
        hkCallWndProc =
          SetWindowsHookExW (WH_CALLWNDPROC, CallWndProcHook, 0, GetMainThreadId ());

        if (hkCallWndProc != 0)
        {
            // Loop and periodically wake the game's window thread to deal with
            // stupid handling of window activation by the game
            MSG msg = {};
            do
            {
                Sleep (666UL);

                if (hWndGame != 0)
                    SendMessage (hWndGame, WM_NULL, 0, 0);

                PeekMessage (&msg, 0, 0, 0, 0);
            } while (msg.message != WM_QUIT);
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
    Misc();
    JXL();
    WindowFocus();
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
        thisModule = hModule;
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, CREATE_SUSPENDED, 0);
        if (mainHandle)
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_TIME_CRITICAL);
            ResumeThread(mainHandle);
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