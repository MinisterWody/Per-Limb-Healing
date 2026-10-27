// PerLimbHealing
// Target: Fallout 4 OG/NG/AE desktop runtime families / F4SE 0.6.23 and 0.7.x
// No CRT/STL/CommonLib link dependency. Uses the user-installed Address Library database at runtime.
// Exact patch numbers are diagnostic information only; runtime capability checks, not a patch allowlist,
// determine whether engine integrations can activate safely.
//
// Runtime contract:
// - passive player limb regeneration is optionally suppressed through player-only ConditionRateMult ownership;
// - ordinary player Stimpaks can either retain full Bethesda limb repair or use PLH Health-only suppression;
// - a limb click arms one native Stimpak use, binds only that new StimpakEffect*, reads Bethesda's
//   finalized rate/duration/condition scale, restores the selected limb instantly through
//   ActorValueOwner::RestoreActorValue, then force-dispels only that targeted Health effect;
// - optional performance telemetry is accumulated in memory and emitted only at reporting boundaries;
// - MCM settings are synchronized from Data\MCM using a blocking directory watcher, with no settings-file I/O on healing hot paths;
// - engine addresses are resolved once into EngineAddressTable and validated by each dependent subsystem;
// - the settings watcher starts only after mandatory plugin initialization succeeds and owns its change-notification handle.
//
// Source map (dependency order):
//   1. F4SE / Scaleform ABI and CRT-free platform support
//   2. settings, logging, QPC and process-image helpers
//   3. runtime-family profile, one-pass Address Library resolution and layout facts
//   4. basic player/form/ActorValue access
//   5. Stimpak identity, targeted request state, telemetry and hooks
//   6. ConditionRateMult ownership + F4SE task/message lifecycle
//   7. Scaleform bridge + exported F4SE entry points

using UInt8 = unsigned char;
using UInt16 = unsigned short;
using UInt32 = unsigned int;
using SInt32 = int;
using UInt64 = unsigned long long;
using SInt64 = long long;
using UIntPtr = unsigned long long;

extern "C" int _fltused = 0;

static constexpr UInt32 kRuntime_1_10_163 = 0x010A0A30u;
static constexpr UInt32 kRuntime_1_10_980 = 0x010A3D40u;
static constexpr UInt32 kRuntime_1_10_984 = 0x010A3D80u;
static constexpr UInt32 kRuntime_1_11_137 = 0x010B0890u;
static constexpr UInt32 kRuntime_1_11_191 = 0x010B0BF0u;
static constexpr UInt32 kRuntime_1_11_221 = 0x010B0DD0u;
static constexpr UInt32 kRuntime_1_11_240 = 0x010B0F00u;
static constexpr UInt32 kPluginVersion = 52u;
static constexpr UInt32 kAddressIndependence_AddressLibrary_1_10_980 = 1u << 1;
static constexpr UInt32 kAddressIndependence_AddressLibrary_1_11_137 = 1u << 2;
static constexpr UInt32 kStructureIndependence_1_10_980Layout = 1u << 1;
static constexpr UInt32 kStructureIndependence_1_11_137Layout = 1u << 2;
static constexpr UInt32 kInterface_Messaging = 1;
static constexpr UInt32 kInterface_Scaleform = 2;
static constexpr UInt32 kInterface_Task = 5;

struct GFxMovieView;
struct GFxValue;

struct F4SEInterface
{
    UInt32 f4seVersion;
    UInt32 runtimeVersion;
    UInt32 editorVersion;
    UInt32 isEditor;
    void* (*QueryInterface)(UInt32 id);
    UInt32 (*GetPluginHandle)();
    UInt32 (*GetReleaseIndex)();
    const void* (*GetPluginInfo)(const char* name);
    const char* (*GetSaveFolderName)();
};

struct PluginInfo
{
    UInt32 infoVersion;
    const char* name;
    UInt32 version;
};

struct F4SEScaleformInterface
{
    UInt32 interfaceVersion;
    using RegisterCallback = bool (*)(GFxMovieView* view, GFxValue* pluginObject);
    bool (*Register)(const char* name, RegisterCallback callback);
};

struct F4SEMessagingInterface
{
    struct Message
    {
        const char* sender;
        UInt32 type;
        UInt32 dataLen;
        void* data;
    };
    using EventCallback = void (*)(Message* message);

    UInt32 interfaceVersion;
    bool (*RegisterListener)(UInt32 listener, const char* sender, EventCallback handler);
    bool (*Dispatch)(UInt32 sender, UInt32 messageType, void* data, UInt32 dataLen, const char* receiver);
    void* (*GetEventDispatcher)(UInt32 dispatcherId);
};

struct ITaskDelegate
{
    virtual ~ITaskDelegate() {}
    virtual void Run() = 0;
};

struct F4SETaskInterface
{
    UInt32 interfaceVersion;
    void (*AddTask)(ITaskDelegate* task);
    void (*AddUITask)(ITaskDelegate* task);
    void (*AddTaskPermanent)(ITaskDelegate* task);
};

static constexpr UInt32 kMessage_PreLoadGame = 2;
static constexpr UInt32 kMessage_PostLoadGame = 3;
static constexpr UInt32 kMessage_PreSaveGame = 4;
static constexpr UInt32 kMessage_PostSaveGame = 5;
static constexpr UInt32 kMessage_NewGame = 8;
static constexpr UInt32 kMessage_GameLoaded = 9;
static constexpr UInt32 kMessage_GameDataReady = 10;


struct F4SEPluginVersionData
{
    UInt32 dataVersion;
    UInt32 pluginVersion;
    char name[256];
    char author[256];
    UInt32 addressIndependence;
    UInt32 structureIndependence;
    UInt32 compatibleVersions[16];
    UInt32 seVersionRequired;
    UInt32 reservedNonBreaking;
    UInt32 reservedBreaking;
    UInt8 reserved[512];
};

struct GFxValue
{
    enum Type : UInt32
    {
        kType_Undefined = 0,
        kType_Null = 1,
        kType_Bool = 2,
        kType_Int = 3,
        kType_UInt = 4,
        kType_Number = 5,
        kType_String = 6,
        kType_Object = 8,
        kType_Array = 9,
        kType_DisplayObject = 10,
        kType_Function = 11,
        kTypeFlag_Managed = 1u << 6,
        kMask_Type = 0x8F
    };

    struct ObjectInterface
    {
        void* unk00;
        GFxMovieView* view;
    };

    union Data
    {
        UInt32 u32;
        SInt32 s32;
        double number;
        bool boolean;
        const char* string;
        const char** managedString;
        void* obj;
    };

    ObjectInterface* objectInterface; // 00
    UInt32 type;                      // 08
    UInt32 pad0C;                     // 0C
    Data data;                        // 10
    void* unk18;                      // 18

    UInt32 GetType() const { return type & kMask_Type; }
    bool IsManaged() const { return (type & kTypeFlag_Managed) != 0; }
};

static_assert(sizeof(GFxValue) == 0x20, "GFxValue layout mismatch");

struct GRefCountBase
{
    virtual ~GRefCountBase() {}
    volatile SInt32 refCount;
    UInt32 pad0C;

    GRefCountBase() : refCount(1), pad0C(0) {}
};

struct GFxFunctionHandler : GRefCountBase
{
    // These handlers are placement-constructed in process-lifetime static storage. Scaleform may
    // run a deleting destructor when its refcount reaches zero, so deallocation must be a no-op for
    // this hierarchy only; do not make global delete a no-op.
    static void operator delete(void*) noexcept {}
    static void operator delete(void*, unsigned long long) noexcept {}

    struct Args
    {
        GFxValue* result;       // 00
        GFxMovieView* movie;    // 08
        GFxValue* thisObj;      // 10
        GFxValue* unk18;        // 18
        GFxValue* args;         // 20
        UInt32 numArgs;         // 28
        UInt32 pad2C;           // 2C
        UInt32 optionID;        // 30
        UInt32 pad34;           // 34
    };

    virtual void Invoke(Args* args) = 0;
};

static_assert(sizeof(GFxFunctionHandler::Args) == 0x38, "GFxFunctionHandler::Args layout mismatch");

extern "C" unsigned __int64 __readgsqword(unsigned long);
#pragma intrinsic(__readgsqword)

// Declared before task types; the CRT-free definition lives near the Scaleform bridge.
void* operator new(unsigned long long, void* where) noexcept;

// Win32 APIs are resolved dynamically from the process module list so this
// DLL remains completely import-library/CRT independent in the source snapshot.
using CreateFileAFn = void* (*)(const char*, UInt32, UInt32, void*, UInt32, UInt32, void*);
using CreateDirectoryAFn = int (*)(const char*, void*);
using LoadLibraryExAFn = void* (*)(const char*, void*, UInt32);
using SHGetFolderPathAFn = SInt32 (*)(void*, SInt32, void*, UInt32, char*);
using WriteFileFn = int (*)(void*, const void*, UInt32, UInt32*, void*);
using ReadFileFn = int (*)(void*, void*, UInt32, UInt32*, void*);
using CloseHandleFn = int (*)(void*);
using FlushFileBuffersFn = int (*)(void*);
using GetModuleFileNameAFn = UInt32 (*)(void*, char*, UInt32);
using QueryPerformanceCounterFn = int (*)(SInt64*);
using QueryPerformanceFrequencyFn = int (*)(SInt64*);
using RtlAllocateHeapFn = void* (*)(void*, UInt32, UIntPtr);
using RtlFreeHeapFn = UInt8 (*)(void*, UInt32, void*);
using VirtualProtectFn = int (*)(void*, UIntPtr, UInt32, UInt32*);
struct MemoryBasicInformation
{
    void* baseAddress;
    void* allocationBase;
    UInt32 allocationProtect;
    UInt16 partitionId;
    UInt16 pad16;
    UIntPtr regionSize;
    UInt32 state;
    UInt32 protect;
    UInt32 type;
    UInt32 pad2C;
};
static_assert(sizeof(MemoryBasicInformation) == 0x30, "MEMORY_BASIC_INFORMATION x64 layout mismatch");
using VirtualQueryFn = UIntPtr (*)(const void*, MemoryBasicInformation*, UIntPtr);
using GetPrivateProfileIntAFn = UInt32 (*)(const char*, const char*, SInt32, const char*);
using FindFirstChangeNotificationAFn = void* (*)(const char*, int, UInt32);
using FindNextChangeNotificationFn = int (*)(void*);
using FindCloseChangeNotificationFn = int (*)(void*);
using WaitForSingleObjectFn = UInt32 (*)(void*, UInt32);
using CreateThreadFn = void* (*)(void*, UIntPtr, UInt32 (*)(void*), void*, UInt32, UInt32*);
using SleepFn = void (*)(UInt32);

enum class DebugLogLevel : SInt32
{
    Off = 0,
    Errors = 1,
    Verbose = 2
};

struct PLHSettings
{
    bool passiveLimbRegeneration;
    bool ordinaryStimpaksHealLimbs;
    DebugLogLevel debugLog;
    bool performanceLog;
};

static constexpr PLHSettings kDefaultSettings{ false, false, DebugLogLevel::Errors, false };
static PLHSettings g_settings = kDefaultSettings;

static CreateFileAFn g_CreateFileA = nullptr;
static CreateDirectoryAFn g_CreateDirectoryA = nullptr;
static LoadLibraryExAFn g_LoadLibraryExA = nullptr;
static SHGetFolderPathAFn g_SHGetFolderPathA = nullptr;
static WriteFileFn g_WriteFile = nullptr;
static ReadFileFn g_ReadFile = nullptr;
static CloseHandleFn g_CloseHandle = nullptr;
static FlushFileBuffersFn g_FlushFileBuffers = nullptr;
static GetModuleFileNameAFn g_GetModuleFileNameA = nullptr;
static QueryPerformanceCounterFn g_QueryPerformanceCounter = nullptr;
static QueryPerformanceFrequencyFn g_QueryPerformanceFrequency = nullptr;
static UInt64 g_qpcFrequency = 0;
static RtlAllocateHeapFn g_RtlAllocateHeap = nullptr;
static RtlFreeHeapFn g_RtlFreeHeap = nullptr;
static VirtualProtectFn g_VirtualProtect = nullptr;
static VirtualQueryFn g_VirtualQuery = nullptr;
static GetPrivateProfileIntAFn g_GetPrivateProfileIntA = nullptr;
static FindFirstChangeNotificationAFn g_FindFirstChangeNotificationA = nullptr;
static FindNextChangeNotificationFn g_FindNextChangeNotification = nullptr;
static FindCloseChangeNotificationFn g_FindCloseChangeNotification = nullptr;
static WaitForSingleObjectFn g_WaitForSingleObject = nullptr;
static CreateThreadFn g_CreateThread = nullptr;
static SleepFn g_Sleep = nullptr;
static void* g_processHeap = nullptr;
static bool g_settingsWatcherStarted = false;

static bool StringsEqual(const char* a, const char* b)
{
    if (!a || !b) return false;
    UInt32 i = 0;
    while (a[i] && b[i])
    {
        if (a[i] != b[i]) return false;
        ++i;
    }
    return a[i] == b[i];
}

static UIntPtr GetImageSize(UIntPtr base)
{
    if (!base || *reinterpret_cast<const UInt16*>(base) != 0x5A4D) return 0;
    const UInt32 peOffset = *reinterpret_cast<const UInt32*>(base + 0x3C);
    const UIntPtr nt = base + peOffset;
    if (*reinterpret_cast<const UInt32*>(nt) != 0x00004550) return 0;
    const UIntPtr optional = nt + 0x18;
    if (*reinterpret_cast<const UInt16*>(optional) != 0x20B) return 0;
    return static_cast<UIntPtr>(*reinterpret_cast<const UInt32*>(optional + 0x38));
}

static void* FindExportInImage(UIntPtr base, const char* wanted)
{
    if (!base || !wanted || *reinterpret_cast<const UInt16*>(base) != 0x5A4D) return nullptr;
    const UInt32 peOffset = *reinterpret_cast<const UInt32*>(base + 0x3C);
    const UIntPtr nt = base + peOffset;
    if (*reinterpret_cast<const UInt32*>(nt) != 0x00004550) return nullptr;
    const UIntPtr optional = nt + 0x18;
    if (*reinterpret_cast<const UInt16*>(optional) != 0x20B) return nullptr;

    const UInt32 exportRva = *reinterpret_cast<const UInt32*>(optional + 0x70);
    const UInt32 exportSize = *reinterpret_cast<const UInt32*>(optional + 0x74);
    if (!exportRva || !exportSize) return nullptr;
    const UIntPtr exp = base + exportRva;
    const UInt32 numberOfFunctions = *reinterpret_cast<const UInt32*>(exp + 0x14);
    const UInt32 numberOfNames = *reinterpret_cast<const UInt32*>(exp + 0x18);
    const UInt32 functionsRva = *reinterpret_cast<const UInt32*>(exp + 0x1C);
    const UInt32 namesRva = *reinterpret_cast<const UInt32*>(exp + 0x20);
    const UInt32 ordinalsRva = *reinterpret_cast<const UInt32*>(exp + 0x24);
    if (!numberOfFunctions || !numberOfNames || !functionsRva || !namesRva || !ordinalsRva) return nullptr;

    const auto* names = reinterpret_cast<const UInt32*>(base + namesRva);
    const auto* ordinals = reinterpret_cast<const UInt16*>(base + ordinalsRva);
    const auto* functions = reinterpret_cast<const UInt32*>(base + functionsRva);
    for (UInt32 i = 0; i < numberOfNames; ++i)
    {
        const char* name = reinterpret_cast<const char*>(base + names[i]);
        if (!StringsEqual(name, wanted)) continue;
        const UInt16 ordinal = ordinals[i];
        if (ordinal >= numberOfFunctions) return nullptr;
        const UInt32 fnRva = functions[ordinal];
        // Forwarded exports point back inside the export directory. Skip those;
        // the same API is normally exported directly by KernelBase.dll.
        if (fnRva >= exportRva && fnRva < exportRva + exportSize) return nullptr;
        return reinterpret_cast<void*>(base + fnRva);
    }
    return nullptr;
}

static void* ResolveProcessExport(const char* wanted)
{
    auto* peb = reinterpret_cast<UInt8*>(__readgsqword(0x60));
    if (!peb) return nullptr;
    auto* ldr = *reinterpret_cast<UInt8**>(peb + 0x18);
    if (!ldr) return nullptr;
    auto* head = reinterpret_cast<UInt8*>(ldr + 0x20);
    auto* link = *reinterpret_cast<UInt8**>(head);
    UInt32 guard = 0;
    while (link && link != head && guard++ < 256)
    {
        const UIntPtr base = *reinterpret_cast<UIntPtr*>(link + 0x20);
        if (void* fn = FindExportInImage(base, wanted)) return fn;
        link = *reinterpret_cast<UInt8**>(link);
    }
    return nullptr;
}

static UIntPtr GetOwnModuleBase(const void* address)
{
    auto* peb = reinterpret_cast<UInt8*>(__readgsqword(0x60));
    if (!peb) return 0;
    auto* ldr = *reinterpret_cast<UInt8**>(peb + 0x18);
    if (!ldr) return 0;
    auto* head = reinterpret_cast<UInt8*>(ldr + 0x20);
    auto* link = *reinterpret_cast<UInt8**>(head);
    const UIntPtr probe = reinterpret_cast<UIntPtr>(address);
    UInt32 guard = 0;
    while (link && link != head && guard++ < 256)
    {
        const UIntPtr base = *reinterpret_cast<UIntPtr*>(link + 0x20);
        const UIntPtr size = GetImageSize(base);
        if (size && probe >= base && probe < base + size) return base;
        link = *reinterpret_cast<UInt8**>(link);
    }
    return 0;
}

static bool ResolveWin32APIs()
{
    // Resolve the small Win32 surface directly from loaded modules so the DLL keeps no import table.
    // Optional telemetry must never be able to suppress logging or gameplay initialization.
    g_CreateFileA = reinterpret_cast<CreateFileAFn>(ResolveProcessExport("CreateFileA"));
    g_CreateDirectoryA = reinterpret_cast<CreateDirectoryAFn>(ResolveProcessExport("CreateDirectoryA"));
    g_LoadLibraryExA = reinterpret_cast<LoadLibraryExAFn>(ResolveProcessExport("LoadLibraryExA"));
    g_SHGetFolderPathA = reinterpret_cast<SHGetFolderPathAFn>(ResolveProcessExport("SHGetFolderPathA"));
    if (!g_SHGetFolderPathA && g_LoadLibraryExA)
    {
        const UIntPtr shell32 = reinterpret_cast<UIntPtr>(g_LoadLibraryExA("shell32.dll", nullptr, 0));
        if (shell32)
            g_SHGetFolderPathA = reinterpret_cast<SHGetFolderPathAFn>(FindExportInImage(shell32, "SHGetFolderPathA"));
    }
    g_WriteFile = reinterpret_cast<WriteFileFn>(ResolveProcessExport("WriteFile"));
    g_ReadFile = reinterpret_cast<ReadFileFn>(ResolveProcessExport("ReadFile"));
    g_CloseHandle = reinterpret_cast<CloseHandleFn>(ResolveProcessExport("CloseHandle"));
    g_FlushFileBuffers = reinterpret_cast<FlushFileBuffersFn>(ResolveProcessExport("FlushFileBuffers"));
    g_GetModuleFileNameA = reinterpret_cast<GetModuleFileNameAFn>(ResolveProcessExport("GetModuleFileNameA"));
    g_GetPrivateProfileIntA = reinterpret_cast<GetPrivateProfileIntAFn>(ResolveProcessExport("GetPrivateProfileIntA"));
    g_FindFirstChangeNotificationA = reinterpret_cast<FindFirstChangeNotificationAFn>(ResolveProcessExport("FindFirstChangeNotificationA"));
    g_FindNextChangeNotification = reinterpret_cast<FindNextChangeNotificationFn>(ResolveProcessExport("FindNextChangeNotification"));
    g_FindCloseChangeNotification = reinterpret_cast<FindCloseChangeNotificationFn>(ResolveProcessExport("FindCloseChangeNotification"));
    g_WaitForSingleObject = reinterpret_cast<WaitForSingleObjectFn>(ResolveProcessExport("WaitForSingleObject"));
    g_CreateThread = reinterpret_cast<CreateThreadFn>(ResolveProcessExport("CreateThread"));
    g_Sleep = reinterpret_cast<SleepFn>(ResolveProcessExport("Sleep"));

    // Optional high-resolution telemetry. Logging remains functional if QPC cannot be resolved.
    g_QueryPerformanceCounter = reinterpret_cast<QueryPerformanceCounterFn>(ResolveProcessExport("QueryPerformanceCounter"));
    g_QueryPerformanceFrequency = reinterpret_cast<QueryPerformanceFrequencyFn>(ResolveProcessExport("QueryPerformanceFrequency"));
    g_VirtualProtect = reinterpret_cast<VirtualProtectFn>(ResolveProcessExport("VirtualProtect"));
    g_VirtualQuery = reinterpret_cast<VirtualQueryFn>(ResolveProcessExport("VirtualQuery"));
    if (g_QueryPerformanceCounter && g_QueryPerformanceFrequency)
    {
        SInt64 frequency = 0;
        if (g_QueryPerformanceFrequency(&frequency) && frequency > 0)
            g_qpcFrequency = static_cast<UInt64>(frequency);
    }

    return g_CreateFileA && g_WriteFile && g_ReadFile && g_CloseHandle &&
           g_FlushFileBuffers && g_GetModuleFileNameA && g_GetPrivateProfileIntA;
}

static bool ResolveTaskAllocator()
{
    // PEB+0x30 is ProcessHeap on x64 Windows. Use ntdll's direct exports so
    // forwarded Kernel32/KernelBase heap exports cannot break task allocation.
    auto* peb = reinterpret_cast<UInt8*>(__readgsqword(0x60));
    g_processHeap = peb ? *reinterpret_cast<void**>(peb + 0x30) : nullptr;
    g_RtlAllocateHeap = reinterpret_cast<RtlAllocateHeapFn>(ResolveProcessExport("RtlAllocateHeap"));
    g_RtlFreeHeap = reinterpret_cast<RtlFreeHeapFn>(ResolveProcessExport("RtlFreeHeap"));
    return g_processHeap && g_RtlAllocateHeap && g_RtlFreeHeap;
}

static constexpr UInt32 kGenericRead = 0x80000000u;
static constexpr UInt32 kFileAppendData = 0x00000004u;
static constexpr SInt32 kCsidlPersonal = 0x0005;
static constexpr SInt32 kCsidlFlagCreate = 0x8000;
static constexpr UInt32 kShgfpTypeCurrent = 0u;
static constexpr UInt32 kFileShareRead = 0x00000001u;
static constexpr UInt32 kFileShareWrite = 0x00000002u;
static constexpr UInt32 kCreateAlways = 2u;
static constexpr UInt32 kOpenExisting = 3u;
static constexpr UInt32 kOpenAlways = 4u;
static constexpr UInt32 kFileAttributeNormal = 0x00000080u;
static void* const kInvalidHandleValue = reinterpret_cast<void*>(~UIntPtr(0));

static void* g_logFile = kInvalidHandleValue;
static bool g_logInitializedThisProcess = false;
static bool g_forceLogWrite = false;

static bool DebugVerboseEnabled() { return g_settings.debugLog == DebugLogLevel::Verbose; }
static bool DebugErrorsEnabled() { return g_settings.debugLog != DebugLogLevel::Off; }
static bool AnyLogOutputEnabled() { return DebugErrorsEnabled() || g_settings.performanceLog; }

static bool PerfTelemetryAvailable()
{
    return g_QueryPerformanceCounter && g_qpcFrequency != 0;
}

static UInt64 PerfNowTicks()
{
    if (!g_settings.performanceLog || !PerfTelemetryAvailable()) return 0;
    SInt64 value = 0;
    if (!g_QueryPerformanceCounter(&value) || value < 0) return 0;
    return static_cast<UInt64>(value);
}

static UInt32 StringLength(const char* text)
{
    UInt32 length = 0;
    if (text)
        while (text[length] != '\0') ++length;
    return length;
}

static void WriteLogBytes(const char* data, UInt32 length)
{
    if (g_logFile == kInvalidHandleValue || !data || !length || !g_WriteFile) return;
    UInt32 offset = 0;
    while (offset < length)
    {
        UInt32 written = 0;
        if (!g_WriteFile(g_logFile, data + offset, length - offset, &written, nullptr) || written == 0)
            return;
        offset += written;
    }
}

static __declspec(noinline) void LogRaw(const char* text)
{
    if (!g_forceLogWrite && !DebugVerboseEnabled()) return;
    if (g_logFile == kInvalidHandleValue || !text) return;
    const UInt32 length = StringLength(text);
    if (length) WriteLogBytes(text, length);
}

static __declspec(noinline) void LogLine(const char* text)
{
    LogRaw(text);
    LogRaw("\r\n");
}

static __declspec(noinline) void LogInt(const char* prefix, SInt32 value)
{
    if (!g_forceLogWrite && !DebugVerboseEnabled()) return;
    char buffer[128];
    UInt32 pos = 0;
    if (prefix)
    {
        while (pos < 100 && prefix[pos] != '\0')
        {
            buffer[pos] = prefix[pos];
            ++pos;
        }
    }

    UInt32 magnitude;
    if (value < 0)
    {
        buffer[pos++] = '-';
        magnitude = static_cast<UInt32>(-(value + 1)) + 1u;
    }
    else
    {
        magnitude = static_cast<UInt32>(value);
    }

    char digits[16];
    UInt32 digitCount = 0;
    do
    {
        digits[digitCount++] = static_cast<char>('0' + (magnitude % 10u));
        magnitude /= 10u;
    } while (magnitude && digitCount < 16);

    while (digitCount) buffer[pos++] = digits[--digitCount];
    buffer[pos++] = '\r';
    buffer[pos++] = '\n';
    WriteLogBytes(buffer, pos);
}

static __declspec(noinline) void LogHex(const char* prefix, UIntPtr value)
{
    if (!g_forceLogWrite && !DebugVerboseEnabled()) return;
    char buffer[160];
    UInt32 pos = 0;
    if (prefix)
    {
        while (pos < 110 && prefix[pos] != '\0')
        {
            buffer[pos] = prefix[pos];
            ++pos;
        }
    }
    buffer[pos++] = '0';
    buffer[pos++] = 'x';
    bool started = false;
    for (SInt32 shift = 60; shift >= 0; shift -= 4)
    {
        const UInt32 nibble = static_cast<UInt32>((value >> shift) & 0xFu);
        if (!started && nibble == 0 && shift != 0) continue;
        started = true;
        buffer[pos++] = static_cast<char>(nibble < 10 ? ('0' + nibble) : ('A' + nibble - 10));
    }
    buffer[pos++] = '\r';
    buffer[pos++] = '\n';
    WriteLogBytes(buffer, pos);
}

static __declspec(noinline) void LogFloat(const char* prefix, float value)
{
    if (!g_forceLogWrite && !DebugVerboseEnabled()) return;
    char buffer[192];
    UInt32 pos = 0;
    if (prefix)
    {
        while (pos < 120 && prefix[pos] != '\0')
        {
            buffer[pos] = prefix[pos];
            ++pos;
        }
    }

    if (value < 0.0f)
    {
        buffer[pos++] = '-';
        value = -value;
    }
    UInt64 whole = static_cast<UInt64>(value);
    float fractionFloat = (value - static_cast<float>(whole)) * 1000000.0f + 0.5f;
    UInt32 fraction = static_cast<UInt32>(fractionFloat);
    if (fraction >= 1000000u)
    {
        ++whole;
        fraction -= 1000000u;
    }

    char digits[32];
    UInt32 count = 0;
    do
    {
        digits[count++] = static_cast<char>('0' + (whole % 10ull));
        whole /= 10ull;
    } while (whole && count < 32);
    while (count) buffer[pos++] = digits[--count];
    buffer[pos++] = '.';
    UInt32 divisor = 100000u;
    while (divisor)
    {
        buffer[pos++] = static_cast<char>('0' + ((fraction / divisor) % 10u));
        divisor /= 10u;
    }
    buffer[pos++] = '\r';
    buffer[pos++] = '\n';
    WriteLogBytes(buffer, pos);
}

static void ErrorLogLine(const char* text)
{
    if (!DebugErrorsEnabled()) return;
    const bool previous = g_forceLogWrite;
    g_forceLogWrite = true;
    LogLine(text);
    g_forceLogWrite = previous;
}

static void ErrorLogRaw(const char* text)
{
    if (!DebugErrorsEnabled()) return;
    const bool previous = g_forceLogWrite;
    g_forceLogWrite = true;
    LogRaw(text);
    g_forceLogWrite = previous;
}

static void ErrorLogHex(const char* prefix, UIntPtr value)
{
    if (!DebugErrorsEnabled()) return;
    const bool previous = g_forceLogWrite;
    g_forceLogWrite = true;
    LogHex(prefix, value);
    g_forceLogWrite = previous;
}

static void StatusLogLine(const char* text)
{
    if (!DebugErrorsEnabled()) return;
    const bool previous = g_forceLogWrite;
    g_forceLogWrite = true;
    LogLine(text);
    g_forceLogWrite = previous;
}

static void StatusLogRaw(const char* text)
{
    if (!DebugErrorsEnabled()) return;
    const bool previous = g_forceLogWrite;
    g_forceLogWrite = true;
    LogRaw(text);
    g_forceLogWrite = previous;
}

static void StatusLogInt(const char* prefix, SInt32 value)
{
    if (!DebugErrorsEnabled()) return;
    const bool previous = g_forceLogWrite;
    g_forceLogWrite = true;
    LogInt(prefix, value);
    g_forceLogWrite = previous;
}

static void StatusLogHex(const char* prefix, UIntPtr value)
{
    if (!DebugErrorsEnabled()) return;
    const bool previous = g_forceLogWrite;
    g_forceLogWrite = true;
    LogHex(prefix, value);
    g_forceLogWrite = previous;
}

static bool BuildOwnDirectoryFilePath(const char* fileName, char* outPath, UInt32 capacity)
{
    if (!fileName || !outPath || capacity < 8 || !g_GetModuleFileNameA) return false;
    const UIntPtr module = GetOwnModuleBase(reinterpret_cast<const void*>(&BuildOwnDirectoryFilePath));
    if (!module) return false;

    const UInt32 length = g_GetModuleFileNameA(reinterpret_cast<void*>(module), outPath, capacity);
    if (!length || length >= capacity) return false;

    UInt32 cut = length;
    while (cut > 0 && outPath[cut - 1] != '\\' && outPath[cut - 1] != '/') --cut;
    UInt32 i = 0;
    while (fileName[i] && cut + i + 1 < capacity)
    {
        outPath[cut + i] = fileName[i];
        ++i;
    }
    if (fileName[i]) return false;
    outPath[cut + i] = '\0';
    return true;
}


static bool BuildGameDirectoryFilePath(const char* relativePath, char* outPath, UInt32 capacity)
{
    if (!relativePath || !outPath || capacity < 8 || !g_GetModuleFileNameA) return false;
    const UInt32 length = g_GetModuleFileNameA(nullptr, outPath, capacity);
    if (!length || length >= capacity) return false;

    UInt32 cut = length;
    while (cut > 0 && outPath[cut - 1] != '\\' && outPath[cut - 1] != '/') --cut;
    UInt32 i = 0;
    while (relativePath[i] && cut + i + 1 < capacity)
    {
        outPath[cut + i] = relativePath[i];
        ++i;
    }
    if (relativePath[i]) return false;
    outPath[cut + i] = '\0';
    return true;
}


static bool AppendPathComponent(char* path, UInt32 capacity, const char* component)
{
    if (!path || !component || capacity < 4) return false;
    UInt32 length = 0;
    while (length < capacity && path[length]) ++length;
    if (length == 0 || length >= capacity) return false;

    if (path[length - 1] != '\\' && path[length - 1] != '/')
    {
        if (length + 1 >= capacity) return false;
        path[length++] = '\\';
    }

    UInt32 i = 0;
    while (component[i] && length + i + 1 < capacity)
    {
        path[length + i] = component[i];
        ++i;
    }
    if (component[i]) return false;
    path[length + i] = '\0';
    return true;
}

static bool BuildF4SELogFilePath(char* outPath, UInt32 capacity)
{
    if (!outPath || capacity < 32 || !g_SHGetFolderPathA) return false;
    const SInt32 result = g_SHGetFolderPathA(
        nullptr,
        kCsidlPersonal | kCsidlFlagCreate,
        nullptr,
        kShgfpTypeCurrent,
        outPath);
    if (result < 0 || !outPath[0]) return false;

    if (!AppendPathComponent(outPath, capacity, "My Games")) return false;
    if (g_CreateDirectoryA) g_CreateDirectoryA(outPath, nullptr);
    if (!AppendPathComponent(outPath, capacity, "Fallout4")) return false;
    if (g_CreateDirectoryA) g_CreateDirectoryA(outPath, nullptr);
    if (!AppendPathComponent(outPath, capacity, "F4SE")) return false;
    if (g_CreateDirectoryA) g_CreateDirectoryA(outPath, nullptr);
    return AppendPathComponent(outPath, capacity, "PerLimbHealing.log");
}

static DebugLogLevel NormalizeDebugLogLevel(SInt32 value)
{
    if (value <= 0) return DebugLogLevel::Off;
    if (value >= 2) return DebugLogLevel::Verbose;
    return DebugLogLevel::Errors;
}

static PLHSettings ReadSettingsFromDisk()
{
    PLHSettings result = kDefaultSettings;
    if (!g_GetPrivateProfileIntA) return result;

    char defaultsPath[1024];
    if (BuildGameDirectoryFilePath("Data\\MCM\\Config\\PerLimbHealing\\settings.ini", defaultsPath, sizeof(defaultsPath)))
    {
        result.passiveLimbRegeneration = g_GetPrivateProfileIntA("Gameplay", "bPassiveLimbRegeneration", result.passiveLimbRegeneration ? 1 : 0, defaultsPath) != 0;
        result.ordinaryStimpaksHealLimbs = g_GetPrivateProfileIntA("Gameplay", "bOrdinaryStimpaksHealLimbs", result.ordinaryStimpaksHealLimbs ? 1 : 0, defaultsPath) != 0;
        result.debugLog = NormalizeDebugLogLevel(static_cast<SInt32>(g_GetPrivateProfileIntA("Diagnostics", "iDebugLog", static_cast<SInt32>(result.debugLog), defaultsPath)));
        result.performanceLog = g_GetPrivateProfileIntA("Diagnostics", "bPerformanceLog", result.performanceLog ? 1 : 0, defaultsPath) != 0;
    }

    char userPath[1024];
    if (BuildGameDirectoryFilePath("Data\\MCM\\Settings\\PerLimbHealing.ini", userPath, sizeof(userPath)))
    {
        result.passiveLimbRegeneration = g_GetPrivateProfileIntA("Gameplay", "bPassiveLimbRegeneration", result.passiveLimbRegeneration ? 1 : 0, userPath) != 0;
        result.ordinaryStimpaksHealLimbs = g_GetPrivateProfileIntA("Gameplay", "bOrdinaryStimpaksHealLimbs", result.ordinaryStimpaksHealLimbs ? 1 : 0, userPath) != 0;
        result.debugLog = NormalizeDebugLogLevel(static_cast<SInt32>(g_GetPrivateProfileIntA("Diagnostics", "iDebugLog", static_cast<SInt32>(result.debugLog), userPath)));
        result.performanceLog = g_GetPrivateProfileIntA("Diagnostics", "bPerformanceLog", result.performanceLog ? 1 : 0, userPath) != 0;
    }
    return result;
}

static bool SettingsEqual(const PLHSettings& a, const PLHSettings& b)
{
    return a.passiveLimbRegeneration == b.passiveLimbRegeneration &&
           a.ordinaryStimpaksHealLimbs == b.ordinaryStimpaksHealLimbs &&
           a.debugLog == b.debugLog &&
           a.performanceLog == b.performanceLog;
}

static void OpenLogFileUnconditionally()
{
    if (g_logFile != kInvalidHandleValue || !g_CreateFileA) return;
    char path[1024];
    if (!BuildF4SELogFilePath(path, sizeof(path))) return;

    const bool firstOpenThisProcess = !g_logInitializedThisProcess;
    g_logFile = g_CreateFileA(
        path,
        kFileAppendData,
        kFileShareRead | kFileShareWrite,
        nullptr,
        firstOpenThisProcess ? kCreateAlways : kOpenAlways,
        kFileAttributeNormal,
        nullptr);

    if (g_logFile != kInvalidHandleValue)
    {
        g_logInitializedThisProcess = true;
        const bool previous = g_forceLogWrite;
        g_forceLogWrite = true;
        if (firstOpenThisProcess)
        {
            LogLine("=== PerLimbHealing session ===");
            LogLine("Log opened in Documents\\My Games\\Fallout4\\F4SE. Previous-session log contents were replaced.");
            LogLine("Settings source: Data\\MCM\\Config\\PerLimbHealing + Data\\MCM\\Settings\\PerLimbHealing.ini");
        }
        else
        {
            LogLine("");
            LogLine("=== Diagnostics logging resumed ===");
        }
        g_forceLogWrite = previous;
    }
}

static void EnsureLogFileOpen()
{
    if (AnyLogOutputEnabled()) OpenLogFileUnconditionally();
}

static void CloseLogFileIfUnused()
{
    if (AnyLogOutputEnabled() || g_logFile == kInvalidHandleValue || !g_CloseHandle) return;
    g_CloseHandle(g_logFile);
    g_logFile = kInvalidHandleValue;
}

// The first InMemoryOrder module is the process image (Fallout4.exe).
static UIntPtr GetFallout4Base()
{
    auto* peb = reinterpret_cast<UInt8*>(__readgsqword(0x60));
    if (!peb) return 0;
    auto* ldr = *reinterpret_cast<UInt8**>(peb + 0x18);
    if (!ldr) return 0;
    auto* firstInMemoryOrderLink = *reinterpret_cast<UInt8**>(ldr + 0x20);
    if (!firstInMemoryOrderLink) return 0;
    return *reinterpret_cast<UIntPtr*>(firstInMemoryOrderLink + 0x20);
}

static UIntPtr g_fallout4Base = 0;
static UIntPtr g_fallout4ImageSize = 0;

// ---------------------------------------------------------------------------
// Runtime compatibility profile and Address Library engine references
// ---------------------------------------------------------------------------
// All engine identities, structure offsets and vtable slots used by PLH flow through a runtime
// profile. Exact patch numbers are not an allowlist: the runtime version selects a candidate ABI
// family, then Address Library resolution and exact vtable/function identity checks decide whether
// each native integration is safe to activate.

enum class RuntimeFamily : UInt32
{
    Unknown = 0,
    OG = 1,
    NG = 2,
    AE = 3
};

struct RuntimeVersionParts
{
    UInt32 major;
    UInt32 minor;
    UInt32 patch;
    UInt32 build;
};

struct EngineAddressIDs
{
    UInt64 playerSingleton;
    UInt64 formGetByNumericID;
    UInt64 playerVTable;
    UInt64 playerDrinkPotion;
    UInt64 stimpakVTable;
    UInt64 valueModifierVTable;
    UInt64 stimpakStart;
    UInt64 stimpakDestructor;
    UInt64 stimpakValueApply;
    UInt64 valueModifierValueApply;
    UInt64 activeEffectTargetActor;
    UInt64 stimpakSelectedActorValue;
    UInt64 stimpakConditionScale;
    UInt64 activeEffectDispel;
    UInt64 gfxSetMember;
    UInt64 gfxReleaseManaged;
};

struct EngineLayout
{
    UIntPtr playerActorValueOwner;
    UIntPtr activeEffectSpell;
    UIntPtr activeEffectEffectItem;
    UIntPtr activeEffectDuration;
    UIntPtr activeEffectMagnitude;
    UIntPtr stimpakEffectStateByte;
    UIntPtr effectItemBaseEffect;

    UIntPtr actorValueOwnerGetActorValueVSlot;
    UIntPtr actorValueOwnerModActorValueVSlot;
    UIntPtr actorValueOwnerGetModifierVSlot;
    UIntPtr actorValueOwnerRestoreActorValueVSlot;
    UIntPtr playerDrinkPotionVSlot;
    UIntPtr stimpakDestructorVSlot;
    UIntPtr stimpakTargetActorVSlot;
    UIntPtr stimpakStartVSlot;
    UIntPtr stimpakValueApplyVSlot;
    UIntPtr stimpakSelectedActorValueVSlot;
    UIntPtr stimpakConditionScaleVSlot;

    UIntPtr gfxMovieViewMovieRoot;
    UIntPtr gfxMovieRootCreateFunctionVSlot;
};

struct RuntimeProfile
{
    RuntimeFamily family;
    const char* familyName;
    EngineAddressIDs ids;
    EngineLayout layout;
};

// The supplied OG/NG/AE corpus and three-runtime source evidence support one PLH-relevant
// desktop structure/vtable layout. Runtime-specific relocation IDs remain in separate profiles;
// exact patch numbers never decide whether hooks may install.
static constexpr EngineLayout kRuntimeLayoutDesktop =
{
    0x58ull, 0x48ull, 0x50ull, 0x7Cull, 0x80ull, 0xB8ull, 0x10ull,
    0x01ull, 0x06ull, 0x07ull, 0x08ull,
    0x119ull, 0x00ull, 0x04ull, 0x16ull, 0x21ull, 0x22ull, 0x23ull,
    0x18ull, 0x30ull
};

static constexpr RuntimeProfile kRuntimeProfileOG =
{
    RuntimeFamily::OG,
    "OG",
    {
        412034ull,  // PlayerCharacter::Singleton
        796114ull,  // TESForm::GetFormByNumericID
        1400465ull, // VTABLE::PlayerCharacter
        421894ull,  // PlayerCharacter::DrinkPotion
        1565062ull, // VTABLE::StimpakEffect
        453177ull,  // VTABLE::ValueModifierEffect
        583869ull,  // StimpakEffect::vslot16
        1236593ull, // StimpakEffect::~StimpakEffect
        863299ull,  // StimpakEffect::vslot21
        703726ull,  // ValueModifierEffect::vslot21 base helper
        33056ull,   // ActiveEffect/Stimpak target-Actor virtual helper (vslot04)
        1143126ull, // StimpakEffect selected-ActorValue getter (vslot22)
        167410ull,  // StimpakEffect condition-scale getter (vslot23)
        178839ull,  // ActiveEffect::Dispel(bool)
        1360149ull, // GFx::Value::SetMember
        856221ull   // GFx managed-object release
    },
    kRuntimeLayoutDesktop
};

static constexpr RuntimeProfile kRuntimeProfileNG =
{
    RuntimeFamily::NG,
    "NG",
    {
        2690919ull, // PlayerCharacter::Singleton
        2193092ull, // TESForm::GetFormByNumericID
        1400465ull, // VTABLE::PlayerCharacter
        2233172ull, // PlayerCharacter::DrinkPotion
        1565062ull, // VTABLE::StimpakEffect
        453177ull,  // VTABLE::ValueModifierEffect
        2226807ull, // StimpakEffect::vslot16
        2226810ull, // StimpakEffect::~StimpakEffect
        2226809ull, // StimpakEffect::vslot21
        2226901ull, // ValueModifierEffect::vslot21 base helper
        2225996ull, // ActiveEffect/Stimpak target-Actor virtual helper (vslot04)
        2226804ull, // StimpakEffect selected-ActorValue getter (vslot22)
        2226203ull, // StimpakEffect condition-scale getter (vslot23)
        2226002ull, // ActiveEffect::Dispel
        2286589ull, // GFx::Value::SetMember
        2286229ull  // GFx managed-object release
    },
    kRuntimeLayoutDesktop
};

static constexpr RuntimeProfile kRuntimeProfileAE =
{
    RuntimeFamily::AE,
    "AE",
    {
        4798212ull, // PlayerCharacter::Singleton
        2193092ull, // TESForm::GetFormByNumericID
        1400465ull, // VTABLE::PlayerCharacter
        2233172ull, // PlayerCharacter::DrinkPotion
        1565062ull, // VTABLE::StimpakEffect
        453177ull,  // VTABLE::ValueModifierEffect
        2226807ull, // StimpakEffect::vslot16
        2226810ull, // StimpakEffect::~StimpakEffect
        2226809ull, // StimpakEffect::vslot21
        2226901ull, // ValueModifierEffect::vslot21 base helper
        2225996ull, // ActiveEffect/Stimpak target-Actor virtual helper (vslot04)
        2226804ull, // StimpakEffect selected-ActorValue getter (vslot22)
        2226203ull, // StimpakEffect condition-scale getter (vslot23)
        2226002ull, // ActiveEffect::Dispel
        2286589ull, // GFx::Value::SetMember
        2286229ull  // GFx managed-object release
    },
    kRuntimeLayoutDesktop
};

static UInt32 g_runtimeVersion = 0;
static const RuntimeProfile* g_runtimeProfile = nullptr;

static RuntimeVersionParts DecodeRuntimeVersion(UInt32 packed)
{
    RuntimeVersionParts result{};
    result.major = (packed >> 24) & 0xFFu;
    result.minor = (packed >> 16) & 0xFFu;
    result.patch = (packed >> 4) & 0x0FFFu;
    result.build = packed & 0x0Fu;
    return result;
}

static const RuntimeProfile* SelectRuntimeProfile(UInt32 packed)
{
    const RuntimeVersionParts version = DecodeRuntimeVersion(packed);
    if (version.major == 1 && version.minor == 11) return &kRuntimeProfileAE;
    if (version.major == 1 && version.minor == 10)
    {
        if (version.patch >= 980) return &kRuntimeProfileNG;
        return &kRuntimeProfileOG;
    }
    return nullptr;
}

static bool IsOGBinaryValidatedRuntime(UInt32 packed)
{
    // Informational only. The exact 1.10.163 corpus was validated through Address Library, RTTI,
    // vtables, three-runtime source evidence, and direct Dispel semantic/Papyrus-binding proof.
    return packed == kRuntime_1_10_163;
}

static bool IsOGGameplayValidatedRuntime(UInt32)
{
    // Informational only. No OG runtime has been gameplay-validated by the project yet.
    return false;
}

static bool IsNGBinaryValidatedRuntime(UInt32 packed)
{
    // Informational only: these exact depot executables were statically validated using their
    // Address Library databases, RTTI, vtables and function metadata. This never gates startup.
    return packed == kRuntime_1_10_980 || packed == kRuntime_1_10_984;
}

static bool IsNGGameplayValidatedRuntime(UInt32)
{
    // Informational only. No NG runtime has been gameplay-validated by the project yet.
    return false;
}

static bool IsAEBinaryValidatedRuntime(UInt32 packed)
{
    // Informational only: these exact depot executables were directly compared during project
    // compatibility research. This list never controls whether the AE profile may run.
    return packed == kRuntime_1_11_137 ||
           packed == kRuntime_1_11_191 ||
           packed == kRuntime_1_11_221 ||
           packed == kRuntime_1_11_240;
}

static bool IsAEGameplayValidatedRuntime(UInt32 packed)
{
    // Informational only. Capability checks remain authoritative for every AE runtime.
    return packed == kRuntime_1_11_221;
}

static bool IsCurrentRuntimeBinaryValidated()
{
    if (!g_runtimeProfile) return false;
    switch (g_runtimeProfile->family)
    {
        case RuntimeFamily::OG: return IsOGBinaryValidatedRuntime(g_runtimeVersion);
        case RuntimeFamily::NG: return IsNGBinaryValidatedRuntime(g_runtimeVersion);
        case RuntimeFamily::AE: return IsAEBinaryValidatedRuntime(g_runtimeVersion);
        default: return false;
    }
}

static const EngineLayout& RuntimeLayout()
{
    return g_runtimeProfile->layout;
}

static bool AppendText(char* buffer, UInt32 capacity, UInt32& position, const char* text)
{
    if (!buffer || !text || position >= capacity) return false;
    UInt32 i = 0;
    while (text[i] && position + 1 < capacity) buffer[position++] = text[i++];
    if (text[i]) return false;
    buffer[position] = '\0';
    return true;
}

static bool AppendUInt32Decimal(char* buffer, UInt32 capacity, UInt32& position, UInt32 value)
{
    char digits[16];
    UInt32 count = 0;
    do
    {
        digits[count++] = static_cast<char>('0' + (value % 10u));
        value /= 10u;
    } while (value && count < sizeof(digits));

    if (position + count + 1 > capacity) return false;
    while (count) buffer[position++] = digits[--count];
    buffer[position] = '\0';
    return true;
}

static bool BuildAddressLibraryFileName(UInt32 packed, char* out, UInt32 capacity)
{
    if (!out || capacity < 32) return false;
    const RuntimeVersionParts version = DecodeRuntimeVersion(packed);
    UInt32 pos = 0;
    out[0] = '\0';
    return AppendText(out, capacity, pos, "version-") &&
           AppendUInt32Decimal(out, capacity, pos, version.major) &&
           AppendText(out, capacity, pos, "-") &&
           AppendUInt32Decimal(out, capacity, pos, version.minor) &&
           AppendText(out, capacity, pos, "-") &&
           AppendUInt32Decimal(out, capacity, pos, version.patch) &&
           AppendText(out, capacity, pos, "-") &&
           AppendUInt32Decimal(out, capacity, pos, version.build) &&
           AppendText(out, capacity, pos, ".bin");
}

static constexpr SInt32 kActorValueModifier_Permanent = 0;
static constexpr SInt32 kActorValueModifier_Temporary = 1;
static constexpr SInt32 kActorValueModifier_Damage = 2;
static constexpr float kConditionRateEpsilon = 0.0001f;

struct AddressMapping
{
    UInt64 id;
    UInt64 offset;
};
static_assert(sizeof(AddressMapping) == 16, "AddressMapping layout mismatch");

struct EngineAddressTable
{
    UIntPtr playerSingleton;
    UIntPtr formGetByNumericID;
    UIntPtr playerVTable;
    UIntPtr playerDrinkPotion;
    UIntPtr stimpakVTable;
    UIntPtr valueModifierVTable;
    UIntPtr stimpakStart;
    UIntPtr stimpakDestructor;
    UIntPtr stimpakValueApply;
    UIntPtr valueModifierValueApply;
    UIntPtr activeEffectTargetActor;
    UIntPtr stimpakSelectedActorValue;
    UIntPtr stimpakConditionScale;
    UIntPtr activeEffectDispel;
    UIntPtr gfxSetMember;
    UIntPtr gfxReleaseManaged;
    bool scanCompleted;
    UInt32 foundCount;
    UInt32 usableCount;
    UInt32 requiredCount;
};

static EngineAddressTable g_engineAddresses{};

struct RequiredEngineAddress
{
    UInt64 id;
    UIntPtr* address;
    const char* label;
};

static bool LoadEngineAddressTable()
{
    if (g_engineAddresses.scanCompleted) return true;
    if (!g_runtimeProfile || !g_CreateFileA || !g_ReadFile || !g_CloseHandle ||
        !g_fallout4Base || !g_fallout4ImageSize)
        return false;

    const EngineAddressIDs& ids = g_runtimeProfile->ids;
    RequiredEngineAddress required[] =
    {
        { ids.playerSingleton, &g_engineAddresses.playerSingleton, "PlayerCharacter::Singleton" },
        { ids.formGetByNumericID, &g_engineAddresses.formGetByNumericID, "TESForm::GetFormByNumericID" },
        { ids.playerVTable, &g_engineAddresses.playerVTable, "VTABLE::PlayerCharacter" },
        { ids.playerDrinkPotion, &g_engineAddresses.playerDrinkPotion, "PlayerCharacter::DrinkPotion" },
        { ids.stimpakVTable, &g_engineAddresses.stimpakVTable, "VTABLE::StimpakEffect" },
        { ids.valueModifierVTable, &g_engineAddresses.valueModifierVTable, "VTABLE::ValueModifierEffect" },
        { ids.stimpakStart, &g_engineAddresses.stimpakStart, "StimpakEffect::vslot16" },
        { ids.stimpakDestructor, &g_engineAddresses.stimpakDestructor, "StimpakEffect::~StimpakEffect" },
        { ids.stimpakValueApply, &g_engineAddresses.stimpakValueApply, "StimpakEffect::vslot21" },
        { ids.valueModifierValueApply, &g_engineAddresses.valueModifierValueApply, "ValueModifierEffect::vslot21" },
        { ids.activeEffectTargetActor, &g_engineAddresses.activeEffectTargetActor, "ActiveEffect/Stimpak target-Actor vslot04" },
        { ids.stimpakSelectedActorValue, &g_engineAddresses.stimpakSelectedActorValue, "StimpakEffect selected-ActorValue vslot22" },
        { ids.stimpakConditionScale, &g_engineAddresses.stimpakConditionScale, "StimpakEffect condition-scale vslot23" },
        { ids.activeEffectDispel, &g_engineAddresses.activeEffectDispel, "ActiveEffect::Dispel" },
        { ids.gfxSetMember, &g_engineAddresses.gfxSetMember, "GFx::Value::SetMember" },
        { ids.gfxReleaseManaged, &g_engineAddresses.gfxReleaseManaged, "GFx managed release" }
    };
    constexpr UInt32 requiredCount = static_cast<UInt32>(sizeof(required) / sizeof(required[0]));
    g_engineAddresses.requiredCount = requiredCount;

    char fileName[64];
    if (!BuildAddressLibraryFileName(g_runtimeVersion, fileName, sizeof(fileName)))
    {
        ErrorLogLine("[AddressLibrary] FAILED to construct runtime database filename.");
        return false;
    }
    char path[1024];
    if (!BuildOwnDirectoryFilePath(fileName, path, sizeof(path)))
    {
        ErrorLogLine("[AddressLibrary] FAILED to construct database path.");
        return false;
    }

    LogRaw("[AddressLibrary] runtime database=");
    LogLine(fileName);
    void* file = g_CreateFileA(path, kGenericRead, kFileShareRead | kFileShareWrite,
                               nullptr, kOpenExisting, kFileAttributeNormal, nullptr);
    if (file == kInvalidHandleValue)
    {
        ErrorLogRaw("[AddressLibrary] FAILED to open matching runtime database: ");
        ErrorLogLine(fileName);
        return false;
    }

    UInt64 count = 0;
    UInt32 got = 0;
    if (!g_ReadFile(file, &count, sizeof(count), &got, nullptr) || got != sizeof(count) ||
        count == 0 || count > 10000000ull)
    {
        ErrorLogLine("[AddressLibrary] invalid database header.");
        g_CloseHandle(file);
        return false;
    }

    UInt64 offsets[requiredCount]{};
    bool found[requiredCount]{};
    UInt32 foundCount = 0;
    alignas(16) static AddressMapping chunk[4096];
    UInt64 remaining = count;
    bool readOK = true;

    while (remaining && foundCount < requiredCount)
    {
        const UInt32 entries = static_cast<UInt32>(remaining > 4096ull ? 4096ull : remaining);
        const UInt32 bytesWanted = entries * static_cast<UInt32>(sizeof(AddressMapping));
        got = 0;
        if (!g_ReadFile(file, chunk, bytesWanted, &got, nullptr) || got != bytesWanted)
        {
            readOK = false;
            break;
        }

        for (UInt32 i = 0; i < entries && foundCount < requiredCount; ++i)
        {
            for (UInt32 r = 0; r < requiredCount; ++r)
            {
                if (!found[r] && chunk[i].id == required[r].id)
                {
                    found[r] = true;
                    offsets[r] = chunk[i].offset;
                    ++foundCount;
                    break;
                }
            }
        }
        remaining -= entries;
    }

    g_CloseHandle(file);
    if (!readOK)
    {
        ErrorLogLine("[AddressLibrary] truncated/read-failed database.");
        return false;
    }

    UInt32 usableCount = 0;
    for (UInt32 r = 0; r < requiredCount; ++r)
    {
        if (!found[r])
        {
            ErrorLogRaw("[AddressLibrary] ID not found: ");
            ErrorLogLine(required[r].label);
            continue;
        }
        if (!offsets[r] || offsets[r] >= g_fallout4ImageSize)
        {
            ErrorLogRaw("[AddressLibrary] rejected out-of-image offset for ");
            ErrorLogLine(required[r].label);
            ErrorLogHex("[AddressLibrary] offset=", offsets[r]);
            continue;
        }

        *required[r].address = g_fallout4Base + static_cast<UIntPtr>(offsets[r]);
        ++usableCount;
        LogRaw("[AddressLibrary] resolved ");
        LogLine(required[r].label);
        LogHex("[AddressLibrary] offset=", offsets[r]);
        LogHex("[AddressLibrary] address=", *required[r].address);
    }

    g_engineAddresses.scanCompleted = true;
    g_engineAddresses.foundCount = foundCount;
    g_engineAddresses.usableCount = usableCount;
    g_engineAddresses.requiredCount = requiredCount;
    LogInt("[AddressLibrary] required identities found=", static_cast<SInt32>(foundCount));
    LogInt("[AddressLibrary] required identities usable=", static_cast<SInt32>(usableCount));
    LogInt("[AddressLibrary] required identity count=", static_cast<SInt32>(requiredCount));
    return true;
}

static bool PassiveGateAddressCapabilityReady()
{
    return LoadEngineAddressTable() &&
           g_engineAddresses.playerSingleton &&
           g_engineAddresses.formGetByNumericID;
}

static bool StimpakAddressCapabilityReady()
{
    return LoadEngineAddressTable() &&
           g_engineAddresses.playerSingleton &&
           g_engineAddresses.formGetByNumericID &&
           g_engineAddresses.playerVTable &&
           g_engineAddresses.playerDrinkPotion &&
           g_engineAddresses.stimpakVTable &&
           g_engineAddresses.valueModifierVTable &&
           g_engineAddresses.stimpakStart &&
           g_engineAddresses.stimpakDestructor &&
           g_engineAddresses.stimpakValueApply &&
           g_engineAddresses.valueModifierValueApply &&
           g_engineAddresses.activeEffectTargetActor &&
           g_engineAddresses.stimpakSelectedActorValue &&
           g_engineAddresses.stimpakConditionScale &&
           g_engineAddresses.activeEffectDispel;
}

static bool ScaleformAddressCapabilityReady()
{
    return LoadEngineAddressTable() &&
           g_engineAddresses.gfxSetMember &&
           g_engineAddresses.gfxReleaseManaged;
}

static bool ValidateStimpakHelperVTableContracts(bool logSuccess)
{
    if (!LoadEngineAddressTable() || !g_engineAddresses.stimpakVTable ||
        !g_engineAddresses.activeEffectTargetActor || !g_engineAddresses.stimpakSelectedActorValue ||
        !g_engineAddresses.stimpakConditionScale)
        return false;

    auto** stimpakVTable = reinterpret_cast<void**>(g_engineAddresses.stimpakVTable);
    const UIntPtr observedTargetActor =
        reinterpret_cast<UIntPtr>(stimpakVTable[RuntimeLayout().stimpakTargetActorVSlot]);
    const UIntPtr observedSelectedActorValue =
        reinterpret_cast<UIntPtr>(stimpakVTable[RuntimeLayout().stimpakSelectedActorValueVSlot]);
    const UIntPtr observedConditionScale =
        reinterpret_cast<UIntPtr>(stimpakVTable[RuntimeLayout().stimpakConditionScaleVSlot]);

    bool ok = true;
    if (observedTargetActor != g_engineAddresses.activeEffectTargetActor)
    {
        ErrorLogLine("[Compatibility] Stimpak target-Actor vslot04=FAIL");
        ErrorLogHex("[Compatibility] observed target-Actor helper=", observedTargetActor);
        ErrorLogHex("[Compatibility] expected target-Actor helper=", g_engineAddresses.activeEffectTargetActor);
        ok = false;
    }
    else if (logSuccess)
    {
        LogLine("[Compatibility] Stimpak target-Actor vslot04=PASS");
    }

    if (observedSelectedActorValue != g_engineAddresses.stimpakSelectedActorValue)
    {
        ErrorLogLine("[Compatibility] Stimpak selected-ActorValue vslot22=FAIL");
        ErrorLogHex("[Compatibility] observed selected-ActorValue helper=", observedSelectedActorValue);
        ErrorLogHex("[Compatibility] expected selected-ActorValue helper=", g_engineAddresses.stimpakSelectedActorValue);
        ok = false;
    }
    else if (logSuccess)
    {
        LogLine("[Compatibility] Stimpak selected-ActorValue vslot22=PASS");
    }

    if (observedConditionScale != g_engineAddresses.stimpakConditionScale)
    {
        ErrorLogLine("[Compatibility] Stimpak condition-scale vslot23=FAIL");
        ErrorLogHex("[Compatibility] observed condition-scale helper=", observedConditionScale);
        ErrorLogHex("[Compatibility] expected condition-scale helper=", g_engineAddresses.stimpakConditionScale);
        ok = false;
    }
    else if (logSuccess)
    {
        LogLine("[Compatibility] Stimpak condition-scale vslot23=PASS");
    }

    return ok;
}


// ---------------------------------------------------------------------------
// Player / ConditionRateMult ownership state
// ---------------------------------------------------------------------------
static F4SETaskInterface* g_taskInterface = nullptr;
using GetFormByNumericIDFn = void* (*)(UInt32);
static void* g_conditionRateMultAV = nullptr;

static void* g_ownedConditionPlayer = nullptr;
static float g_ownedConditionDelta = 0.0f;
static float g_ownedConditionTemporaryBaseline = 0.0f;
static bool g_conditionOwned = false;

// ---------------------------------------------------------------------------
// Forms and biological limb references
// ---------------------------------------------------------------------------
static constexpr UInt32 kFormID_Health = 0x000002D4u;
static constexpr UInt32 kFormID_ConditionRateMult = 0x0000035Au;
static constexpr UInt32 kFormID_PerceptionCondition = 0x0000036Cu;
static constexpr UInt32 kFormID_EnduranceCondition = 0x0000036Du;
static constexpr UInt32 kFormID_LeftAttackCondition = 0x0000036Eu;
static constexpr UInt32 kFormID_RightAttackCondition = 0x0000036Fu;
static constexpr UInt32 kFormID_LeftMobilityCondition = 0x00000370u;
static constexpr UInt32 kFormID_RightMobilityCondition = 0x00000371u;
static constexpr UInt32 kFormID_Stimpak = 0x00023736u;
static constexpr UInt32 kFormID_RestoreHealthStimpak = 0x0021DDB8u;
// Scaleform receives this negative sentinel when a targeted request is already in flight.
static constexpr SInt32 kTargetedRequestBusy = -12;

enum class Limb : SInt32
{
    Head = 0,
    Torso = 1,
    LeftArm = 2,
    RightArm = 3,
    LeftLeg = 4,
    RightLeg = 5,
    Count = 6,
    Invalid = -1
};

static constexpr UInt32 kLimbCount = static_cast<UInt32>(Limb::Count);
static bool IsValidLimbValue(SInt32 value) { return value >= 0 && value < static_cast<SInt32>(Limb::Count); }
static UInt32 LimbIndex(Limb limb) { return static_cast<UInt32>(limb); }

struct LimbDescriptor
{
    const char* name;
};

static constexpr LimbDescriptor kLimbDescriptors[kLimbCount] =
{
    { "Head" },
    { "Torso" },
    { "LeftArm" },
    { "RightArm" },
    { "LeftLeg" },
    { "RightLeg" }
};

static UInt32 LimbActorValueFormID(Limb limb)
{
    switch (limb)
    {
        case Limb::Head: return kFormID_PerceptionCondition;
        case Limb::Torso: return kFormID_EnduranceCondition;
        case Limb::LeftArm: return kFormID_LeftAttackCondition;
        case Limb::RightArm: return kFormID_RightAttackCondition;
        case Limb::LeftLeg: return kFormID_LeftMobilityCondition;
        case Limb::RightLeg: return kFormID_RightMobilityCondition;
        default: return 0;
    }
}

static void* g_healthAV = nullptr;
static void* g_limbAVs[kLimbCount] = {};
static void* g_stimpakForm = nullptr;
static void* g_restoreHealthStimpakEffect = nullptr;

// Deliberately strict pointer provenance check: production ActorValue/Form calls are accepted only
// when the object's vtable lives in Fallout4.exe. External plugins that replace these vtables with
// cloned/plugin-owned tables will be rejected rather than called through blindly.
static bool HasFalloutImageVTable(const void* object)
{
    if (!object || !g_fallout4Base || !g_fallout4ImageSize) return false;
    const UIntPtr vtable = *reinterpret_cast<const UIntPtr*>(object);
    return vtable >= g_fallout4Base && vtable < g_fallout4Base + g_fallout4ImageSize;
}

// ---------------------------------------------------------------------------
// Basic engine/form/ActorValue access
// ---------------------------------------------------------------------------

static bool ConditionEngineRefsReady()
{
    return PassiveGateAddressCapabilityReady();
}

static void LogOperationalCompatibilitySummary(const F4SEInterface* f4se)
{
    if (!DebugErrorsEnabled()) return;

    StatusLogLine("[Startup] ===== OPERATIONAL SUMMARY =====");
    StatusLogInt("[Startup] pluginVersion=", static_cast<SInt32>(kPluginVersion));
    StatusLogHex("[Startup] F4SE packedVersion=", f4se ? static_cast<UIntPtr>(f4se->f4seVersion) : 0);
    const RuntimeVersionParts runtime = DecodeRuntimeVersion(g_runtimeVersion);
    StatusLogInt("[Startup] runtime.major=", static_cast<SInt32>(runtime.major));
    StatusLogInt("[Startup] runtime.minor=", static_cast<SInt32>(runtime.minor));
    StatusLogInt("[Startup] runtime.patch=", static_cast<SInt32>(runtime.patch));
    StatusLogInt("[Startup] runtime.build=", static_cast<SInt32>(runtime.build));
    StatusLogRaw("[Startup] ABI candidate=");
    StatusLogLine(g_runtimeProfile ? g_runtimeProfile->familyName : "none");

    char fileName[64];
    if (BuildAddressLibraryFileName(g_runtimeVersion, fileName, sizeof(fileName)))
    {
        StatusLogRaw("[Startup] AddressLibrary database=");
        StatusLogLine(fileName);
    }

    StatusLogLine(g_engineAddresses.scanCompleted
        ? "[Startup] AddressLibrary scan=COMPLETE"
        : "[Startup] AddressLibrary scan=INCOMPLETE");
    StatusLogInt("[Startup] AddressLibrary found=", static_cast<SInt32>(g_engineAddresses.foundCount));
    StatusLogInt("[Startup] AddressLibrary usable=", static_cast<SInt32>(g_engineAddresses.usableCount));
    StatusLogInt("[Startup] AddressLibrary required=", static_cast<SInt32>(g_engineAddresses.requiredCount));
    const bool passiveAddressCapability = g_engineAddresses.scanCompleted &&
        g_engineAddresses.playerSingleton && g_engineAddresses.formGetByNumericID;
    const bool stimpakAddressCapability = passiveAddressCapability &&
        g_engineAddresses.playerVTable && g_engineAddresses.playerDrinkPotion &&
        g_engineAddresses.stimpakVTable && g_engineAddresses.valueModifierVTable &&
        g_engineAddresses.stimpakStart && g_engineAddresses.stimpakDestructor &&
        g_engineAddresses.stimpakValueApply && g_engineAddresses.valueModifierValueApply &&
        g_engineAddresses.activeEffectTargetActor && g_engineAddresses.stimpakSelectedActorValue &&
        g_engineAddresses.stimpakConditionScale && g_engineAddresses.activeEffectDispel;
    const bool scaleformAddressCapability = g_engineAddresses.scanCompleted &&
        g_engineAddresses.gfxSetMember && g_engineAddresses.gfxReleaseManaged;
    StatusLogLine(passiveAddressCapability
        ? "[Startup] PassiveGate address capability=PASS"
        : "[Startup] PassiveGate address capability=FAIL");
    StatusLogLine(stimpakAddressCapability
        ? "[Startup] Stimpak address capability=PASS"
        : "[Startup] Stimpak address capability=FAIL");
    StatusLogLine(scaleformAddressCapability
        ? "[Startup] Scaleform address capability=PASS"
        : "[Startup] Scaleform address capability=FAIL");
    StatusLogLine(IsCurrentRuntimeBinaryValidated()
        ? "[Startup] exact runtime qualification=BINARY_PROVEN"
        : "[Startup] exact runtime qualification=UNQUALIFIED; capability checks + Scaleform bridge proof govern Stimpak activation");
}

static void* GetCurrentPlayer()
{
    if (!g_engineAddresses.playerSingleton) return nullptr;
    // Address Library points at the global NiPointer<PlayerCharacter> object;
    // NiPointer's first field is the held PlayerCharacter*.
    return *reinterpret_cast<void**>(g_engineAddresses.playerSingleton);
}

static void* ResolveFormByNumericID(UInt32 formID)
{
    if (!g_engineAddresses.formGetByNumericID) return nullptr;
    const auto fn = reinterpret_cast<GetFormByNumericIDFn>(g_engineAddresses.formGetByNumericID);
    void* form = fn(formID);
    return HasFalloutImageVTable(form) ? form : nullptr;
}

static void* ResolveConditionRateMultAV()
{
    if (g_conditionRateMultAV) return g_conditionRateMultAV;
    if (!ConditionEngineRefsReady()) return nullptr;

    g_conditionRateMultAV = ResolveFormByNumericID(kFormID_ConditionRateMult);
    if (!g_conditionRateMultAV)
        ErrorLogLine("[ConditionGate] ConditionRateMult AVIF 0000035A unavailable.");
    else
        LogHex("[ConditionGate] ConditionRateMult AV=", reinterpret_cast<UIntPtr>(g_conditionRateMultAV));
    return g_conditionRateMultAV;
}

static void* GetPlayerActorValueOwner(void* player)
{
    if (!player) return nullptr;
    return reinterpret_cast<void*>(reinterpret_cast<UInt8*>(player) + RuntimeLayout().playerActorValueOwner);
}


static float ActorValueGet(void* owner, void* actorValue)
{
    if (!HasFalloutImageVTable(owner) || !actorValue) return 0.0f;
    using Fn = float (*)(void*, const void*);
    auto** vtbl = *reinterpret_cast<void***>(owner);
    auto fn = reinterpret_cast<Fn>(vtbl[RuntimeLayout().actorValueOwnerGetActorValueVSlot]);
    return fn(owner, actorValue);
}

static void ActorValueModify(void* owner, SInt32 modifier, void* actorValue, float delta)
{
    if (!HasFalloutImageVTable(owner) || !actorValue) return;
    using Fn = void (*)(void*, SInt32, const void*, float);
    auto** vtbl = *reinterpret_cast<void***>(owner);
    auto fn = reinterpret_cast<Fn>(vtbl[RuntimeLayout().actorValueOwnerModActorValueVSlot]);
    fn(owner, modifier, actorValue, delta);
}

static float ActorValueGetModifier(void* owner, SInt32 modifier, void* actorValue)
{
    if (!HasFalloutImageVTable(owner) || !actorValue) return 0.0f;
    using Fn = float (*)(void*, SInt32, const void*);
    auto** vtbl = *reinterpret_cast<void***>(owner);
    auto fn = reinterpret_cast<Fn>(vtbl[RuntimeLayout().actorValueOwnerGetModifierVSlot]);
    return fn(owner, modifier, actorValue);
}

static void ActorValueRestore(void* owner, void* actorValue, float amount)
{
    if (!HasFalloutImageVTable(owner) || !actorValue) return;
    using Fn = void (*)(void*, const void*, float);
    auto** vtbl = *reinterpret_cast<void***>(owner);
    auto fn = reinterpret_cast<Fn>(vtbl[RuntimeLayout().actorValueOwnerRestoreActorValueVSlot]);
    fn(owner, actorValue, amount);
}

static float AbsFloat(float value) { return value < 0.0f ? -value : value; }

static bool GameplayActorValuesReady()
{
    if (!g_healthAV) return false;
    for (UInt32 i = 0; i < kLimbCount; ++i)
        if (!g_limbAVs[i]) return false;
    return true;
}

static bool ResolveGameplayActorValues()
{
    if (GameplayActorValuesReady()) return true;
    if (!ConditionEngineRefsReady()) return false;

    g_healthAV = ResolveFormByNumericID(kFormID_Health);
    for (UInt32 i = 0; i < kLimbCount; ++i)
        g_limbAVs[i] = ResolveFormByNumericID(LimbActorValueFormID(static_cast<Limb>(i)));

    LogHex("[Production] Health AV=", reinterpret_cast<UIntPtr>(g_healthAV));
    for (UInt32 i = 0; i < kLimbCount; ++i)
    {
        LogRaw("[Production] limb AV ");
        LogRaw(kLimbDescriptors[i].name);
        LogRaw("=");
        LogHex("", reinterpret_cast<UIntPtr>(g_limbAVs[i]));
    }

    const bool ok = GameplayActorValuesReady();
    if (ok) LogLine("[Production] biological ActorValue mapping resolved from canonical AVIF FormIDs.");
    else ErrorLogLine("[Production] FAILED to resolve complete biological ActorValue mapping.");
    return ok;
}

static void* ResolveStimpakForm()
{
    if (g_stimpakForm && g_restoreHealthStimpakEffect) return g_stimpakForm;

    g_stimpakForm = ResolveFormByNumericID(kFormID_Stimpak);
    g_restoreHealthStimpakEffect = ResolveFormByNumericID(kFormID_RestoreHealthStimpak);

    if (!g_stimpakForm)
    {
        ErrorLogLine("[Production] Stimpak form 00023736 unavailable.");
        return nullptr;
    }
    if (!g_restoreHealthStimpakEffect)
    {
        ErrorLogLine("[Production] RestoreHealthStimpak effect 0021DDB8 unavailable.");
        g_stimpakForm = nullptr;
        return nullptr;
    }

    LogHex("[Production] Stimpak form resolved=", reinterpret_cast<UIntPtr>(g_stimpakForm));
    LogHex("[Production] RestoreHealthStimpak effect resolved=", reinterpret_cast<UIntPtr>(g_restoreHealthStimpakEffect));
    return g_stimpakForm;
}


// ---------------------------------------------------------------------------
// Production player Stimpak limb-repair suppression
// ---------------------------------------------------------------------------
// Readable AE runtime analysis proves StimpakEffect::vslot 0x21 applies Health through the
// ValueModifierEffect base helper, then applies a condition-scaled amount across conditionActorValues.
// The production hook preserves only the native Health call for exact player Stimpaks; all
// nonmatching effects are forwarded to Bethesda unchanged.
using StimpakEffectValueApplyFn = void (*)(void*, void*, float, void*);
static StimpakEffectValueApplyFn g_stimpakEffectOriginalValueApply = nullptr;
static StimpakEffectValueApplyFn g_valueModifierEffectBaseValueApply = nullptr;
static constexpr UInt32 kPageExecuteReadWrite = 0x40u;

// ---------------------------------------------------------------------------
// Production targeted native-Stimpak path
// ---------------------------------------------------------------------------
// Limb clicks arm a one-use request, then Scaleform invokes Bethesda's real Status-tab
// UseStimpak path.  A successful exact-player Stimpak DrinkPotion confirms the request.
// The next newly-created exact player StimpakEffect is bound by ActiveEffect* to that limb
// after Bethesda's vslot 0x16 has finalized perk-adjusted duration/rate/condition scale.
// Target Actor, Health ActorValue, and condition scale are obtained through validated Bethesda
// virtual helpers; only finalized duration/rate and remaining effect identity fields use raw layout.
// The selected limb receives one ActorValueOwner::RestoreActorValue using the full finalized
// amount, then that targeted Health effect is force-dispelled. A short-lived effect marker
// defensively suppresses any reentrant/late tick until destruction. Ordinary Stimpaks remain
// native Health-only and can overlap targeted effects safely.
using PlayerDrinkPotionFn = bool (*)(void*, void*, UInt32);
using StimpakEffectStartFn = void (*)(void*);
using StimpakEffectDestructorFn = void* (*)(void*, UInt32);
using ActiveEffectDispelFn = void (*)(void*, bool);

static PlayerDrinkPotionFn g_playerOriginalDrinkPotion = nullptr;
static StimpakEffectStartFn g_stimpakEffectOriginalStart = nullptr;
static StimpakEffectDestructorFn g_stimpakEffectOriginalDestructor = nullptr;
static ActiveEffectDispelFn g_activeEffectDispel = nullptr;
static void** g_playerDrinkPotionHookSlot = nullptr;
static void** g_stimpakStartHookSlot = nullptr;
static void** g_stimpakDestructorHookSlot = nullptr;
static UInt32 g_playerDrinkPotionDepth = 0;

enum class StimpakBackendState : UInt32
{
    Uninitialized,
    Ready,
    Failed
};

static StimpakBackendState g_stimpakBackendState = StimpakBackendState::Uninitialized;
static bool g_scaleformRegistrationAccepted = false;
static bool g_scaleformBridgeValidated = false;
static bool g_scaleformBridgeSuccessLogged = false;
static bool g_stimpakBackendDeferredLogged = false;

static bool StimpakBackendActivationAllowed()
{
    if (!g_scaleformRegistrationAccepted) return false;
    if (IsCurrentRuntimeBinaryValidated()) return true;
    return g_scaleformBridgeValidated;
}

// A targeted heal is a short two-ordering handshake between Scaleform, DrinkPotion and
// StimpakEffect::vslot16. Bethesda may construct/start the effect either inside DrinkPotion
// or after DrinkPotion returns, so the request keeps a serial plus an optional early candidate.
// state/limb/serial/serialCounter are accessed only through __atomic builtins. The candidate pair
// is used only by the nested DrinkPotion -> vslot16 -> DrinkPotion-return ordering and is therefore
// intentionally plain storage; do not treat volatile itself as synchronization.
enum TargetedRequestState : SInt32
{
    kTargetRequest_Idle = 0,
    kTargetRequest_Armed = 1,
    kTargetRequest_Confirmed = 2
};

struct TargetedStimpakRequest
{
    volatile SInt32 state;
    volatile SInt32 limb;
    volatile UInt32 serial;
    volatile UInt32 serialCounter;
    void* candidateEffect;
    UInt32 candidateSerial;
};

static TargetedStimpakRequest g_targetRequest{ kTargetRequest_Idle, static_cast<SInt32>(Limb::Invalid), 0u, 0u, nullptr, 0u };

// Immediate Dispel normally prevents any targeted vslot21 tick. A marker is retained until
// Bethesda destroys the ActiveEffect so a reentrant/late callback can still be identified and
// suppressed. Capacity 32 is deliberately generous for the very short targeted-effect lifetime;
// allocation failure safely aborts targeted mutation rather than risking an untracked effect.
struct TargetedEffectMarker
{
    void* activeEffect;
};

static constexpr UInt32 kMaxTargetedEffectMarkers = 32u;
static TargetedEffectMarker g_targetedEffectMarkers[kMaxTargetedEffectMarkers]{};
static UInt32 g_targetedEffectMarkerCount = 0;

// ---------------------------------------------------------------------------
// Performance telemetry
// ---------------------------------------------------------------------------
// All samples remain in memory. No Stimpak hot path writes to the log or forces disk I/O.
// A compact snapshot is emitted on PreSaveGame / PreLoadGame, where synchronous logging cannot
// contaminate the heal frame being measured. Tick totals are kept in raw QPC ticks so the
// per-callback path needs only a few integer additions/comparisons.
struct PerfAccumulator
{
    UInt64 calls;
    UInt64 totalTicks;
    UInt64 worstTicks;
};

struct StimpakPerfSession
{
    UInt32 ordinaryEffects;
    UInt32 targetedEffects;
    UInt32 targetedApplyFailures;
    UInt32 targetedVslot21Suppressed;
    UInt32 reportIndex;

    PerfAccumulator ordinaryDrinkTotal;
    PerfAccumulator ordinaryDrinkNative;
    PerfAccumulator targetedDrinkTotal;
    PerfAccumulator targetedDrinkNative;

    PerfAccumulator ordinaryStartTotal;
    PerfAccumulator ordinaryStartNative;
    PerfAccumulator targetedStartTotal;
    PerfAccumulator targetedStartNative;

    PerfAccumulator ordinaryVslot21Total;
    PerfAccumulator ordinaryVslot21NativePath;
    PerfAccumulator targetedVslot21Total;

    PerfAccumulator targetedApplyRetireTotal;
    PerfAccumulator targetedLimbApply;
    PerfAccumulator targetedDispel;
    PerfAccumulator qpcPairBaseline;
};

static StimpakPerfSession g_stimpakPerf{};

static void PerfAccumulate(PerfAccumulator& a, UInt64 ticks)
{
    if (!g_settings.performanceLog || !PerfTelemetryAvailable()) return;
    ++a.calls;
    a.totalTicks += ticks;
    if (ticks > a.worstTicks) a.worstTicks = ticks;
}

static UInt64 PerfDeltaToUs(UInt64 ticks)
{
    if (!g_qpcFrequency) return 0;
    return (ticks * 1000000ull) / g_qpcFrequency;
}

static UInt64 PerfAverageNs(const PerfAccumulator& a)
{
    if (!g_qpcFrequency || !a.calls) return 0;
    return (a.totalTicks * 1000000000ull) / (g_qpcFrequency * a.calls);
}

static __declspec(noinline) void LogPerfAccumulator(const char* label, const PerfAccumulator& a)
{
    LogRaw("[Perf] "); LogRaw(label); LogRaw(" calls="); LogInt("", static_cast<SInt32>(a.calls));
    LogRaw("[Perf] "); LogRaw(label); LogRaw(" totalUs="); LogInt("", static_cast<SInt32>(PerfDeltaToUs(a.totalTicks)));
    LogRaw("[Perf] "); LogRaw(label); LogRaw(" avgNs="); LogInt("", static_cast<SInt32>(PerfAverageNs(a)));
    LogRaw("[Perf] "); LogRaw(label); LogRaw(" worstUs="); LogInt("", static_cast<SInt32>(PerfDeltaToUs(a.worstTicks)));
}

static __declspec(noinline) void LogPerformanceSnapshot(const char* reason)
{
    if (!g_settings.performanceLog) return;
    EnsureLogFileOpen();
    if (g_logFile == kInvalidHandleValue) return;
    const bool previousForce = g_forceLogWrite;
    g_forceLogWrite = true;
    ++g_stimpakPerf.reportIndex;
    LogLine("[Perf] ===== PERFORMANCE SNAPSHOT =====");
    LogRaw("[Perf] telemetrySchema=2; pluginVersion=");
    LogInt("", static_cast<SInt32>(kPluginVersion));
    LogRaw("[Perf] reason="); LogLine(reason ? reason : "unknown");
    LogInt("[Perf] setting.passiveLimbRegeneration=", g_settings.passiveLimbRegeneration ? 1 : 0);
    LogInt("[Perf] setting.ordinaryStimpaksHealLimbs=", g_settings.ordinaryStimpaksHealLimbs ? 1 : 0);
    LogInt("[Perf] setting.debugLog=", static_cast<SInt32>(g_settings.debugLog));
    LogInt("[Perf] setting.performanceLog=", g_settings.performanceLog ? 1 : 0);
    LogInt("[Perf] reportIndex=", static_cast<SInt32>(g_stimpakPerf.reportIndex));
    LogInt("[Perf] ordinaryEffects=", static_cast<SInt32>(g_stimpakPerf.ordinaryEffects));
    LogInt("[Perf] targetedEffects=", static_cast<SInt32>(g_stimpakPerf.targetedEffects));
    LogInt("[Perf] targetedApplyFailures=", static_cast<SInt32>(g_stimpakPerf.targetedApplyFailures));
    LogInt("[Perf] targetedVslot21Suppressed=", static_cast<SInt32>(g_stimpakPerf.targetedVslot21Suppressed));
    LogPerfAccumulator("ordinaryDrinkTotal", g_stimpakPerf.ordinaryDrinkTotal);
    LogPerfAccumulator("ordinaryDrinkNative", g_stimpakPerf.ordinaryDrinkNative);
    LogPerfAccumulator("targetedDrinkTotal", g_stimpakPerf.targetedDrinkTotal);
    LogPerfAccumulator("targetedDrinkNative", g_stimpakPerf.targetedDrinkNative);
    LogPerfAccumulator("ordinaryStartTotal", g_stimpakPerf.ordinaryStartTotal);
    LogPerfAccumulator("ordinaryStartNative", g_stimpakPerf.ordinaryStartNative);
    LogPerfAccumulator("targetedStartTotal", g_stimpakPerf.targetedStartTotal);
    LogPerfAccumulator("targetedStartNative", g_stimpakPerf.targetedStartNative);
    LogPerfAccumulator("ordinaryVslot21Total", g_stimpakPerf.ordinaryVslot21Total);
    LogPerfAccumulator("ordinaryVslot21NativePath", g_stimpakPerf.ordinaryVslot21NativePath);
    LogPerfAccumulator("targetedVslot21Total", g_stimpakPerf.targetedVslot21Total);
    LogPerfAccumulator("targetedApplyRetireTotal", g_stimpakPerf.targetedApplyRetireTotal);
    LogPerfAccumulator("targetedLimbApply", g_stimpakPerf.targetedLimbApply);
    LogLine("[Perf] targetedLimbApplyPrimitive=ActorValueOwner::RestoreActorValue(vslot08)");
    LogPerfAccumulator("targetedDispel", g_stimpakPerf.targetedDispel);
    LogPerfAccumulator("qpcPairBaseline", g_stimpakPerf.qpcPairBaseline);

    const UInt64 ordinaryPlhTicks = g_stimpakPerf.ordinaryVslot21Total.totalTicks >= g_stimpakPerf.ordinaryVslot21NativePath.totalTicks
        ? g_stimpakPerf.ordinaryVslot21Total.totalTicks - g_stimpakPerf.ordinaryVslot21NativePath.totalTicks : 0;
    LogInt("[Perf] ordinaryVslot21WrapperMinusNativeTotalUs=", static_cast<SInt32>(PerfDeltaToUs(ordinaryPlhTicks)));
    UInt64 avgOverheadNs = 0;
    if (g_qpcFrequency && g_stimpakPerf.ordinaryVslot21Total.calls)
        avgOverheadNs = (ordinaryPlhTicks * 1000000000ull) / (g_qpcFrequency * g_stimpakPerf.ordinaryVslot21Total.calls);
    LogInt("[Perf] ordinaryVslot21WrapperMinusNativeAvgNs=", static_cast<SInt32>(avgOverheadNs));
    LogLine("[Perf] wrapperMinusNative includes QPC/wrapper measurement overhead; it is not an instruction-exact PLH cost.");
    LogLine("[Perf] hotPathLogging=0; FlushFileBuffersOnHealPath=0; SettingsFileIOOnHealPath=0");
    LogLine("[Perf] ===== END SNAPSHOT =====");
    g_forceLogWrite = previousForce;
}

static void ResetPerformanceTelemetry()
{
    volatile UInt8* bytes = reinterpret_cast<volatile UInt8*>(&g_stimpakPerf);
    for (UIntPtr i = 0; i < sizeof(g_stimpakPerf); ++i) bytes[i] = 0;
}

static void CalibrateQpcTelemetry()
{
    if (!g_settings.performanceLog || !PerfTelemetryAvailable() || g_stimpakPerf.qpcPairBaseline.calls) return;
    for (UInt32 i = 0; i < 256u; ++i)
    {
        const UInt64 a = PerfNowTicks();
        const UInt64 b = PerfNowTicks();
        if (a && b >= a) PerfAccumulate(g_stimpakPerf.qpcPairBaseline, b - a);
    }
}

static bool IsExactStimpakForm(void* form)
{
    return form && g_stimpakForm && form == g_stimpakForm;
}

static bool HasExactStimpakVTable(const void* activeEffect)
{
    if (!activeEffect || !g_engineAddresses.stimpakVTable) return false;
    return *reinterpret_cast<const UIntPtr*>(activeEffect) == g_engineAddresses.stimpakVTable;
}

static void* StimpakGetTargetActor(void* activeEffect)
{
    if (!HasExactStimpakVTable(activeEffect)) return nullptr;
    using Fn = void* (*)(void*);
    auto** vtbl = *reinterpret_cast<void***>(activeEffect);
    auto fn = reinterpret_cast<Fn>(vtbl[RuntimeLayout().stimpakTargetActorVSlot]);
    return fn(activeEffect);
}

static void* StimpakGetSelectedActorValue(void* activeEffect)
{
    if (!HasExactStimpakVTable(activeEffect)) return nullptr;
    using Fn = void* (*)(void*);
    auto** vtbl = *reinterpret_cast<void***>(activeEffect);
    auto fn = reinterpret_cast<Fn>(vtbl[RuntimeLayout().stimpakSelectedActorValueVSlot]);
    return fn(activeEffect);
}

static float StimpakGetConditionScale(void* activeEffect)
{
    if (!HasExactStimpakVTable(activeEffect)) return 0.0f;
    using Fn = float (*)(void*);
    auto** vtbl = *reinterpret_cast<void***>(activeEffect);
    auto fn = reinterpret_cast<Fn>(vtbl[RuntimeLayout().stimpakConditionScaleVSlot]);
    return fn(activeEffect);
}

static bool IsExactRestoreHealthStimpakEffect(void* activeEffect)
{
    if (!activeEffect || !g_stimpakForm || !g_healthAV || !g_restoreHealthStimpakEffect ||
        !HasExactStimpakVTable(activeEffect))
        return false;

    void* spell = *reinterpret_cast<void**>(reinterpret_cast<UInt8*>(activeEffect) + RuntimeLayout().activeEffectSpell);
    void* effectItem = *reinterpret_cast<void**>(reinterpret_cast<UInt8*>(activeEffect) + RuntimeLayout().activeEffectEffectItem);
    void* selectedActorValue = StimpakGetSelectedActorValue(activeEffect);
    void* baseEffect = effectItem ? *reinterpret_cast<void**>(reinterpret_cast<UInt8*>(effectItem) + RuntimeLayout().effectItemBaseEffect) : nullptr;

    return spell == g_stimpakForm &&
           selectedActorValue == g_healthAV &&
           baseEffect == g_restoreHealthStimpakEffect;
}

static bool IsExactPlayerStimpakEffect(void* activeEffect, void** targetActorOut = nullptr)
{
    if (targetActorOut) *targetActorOut = nullptr;
    if (!IsExactRestoreHealthStimpakEffect(activeEffect)) return false;

    void* targetActor = StimpakGetTargetActor(activeEffect);
    if (targetActorOut) *targetActorOut = targetActor;
    void* player = GetCurrentPlayer();
    return targetActor && player && targetActor == player;
}

static TargetedEffectMarker* FindTargetedEffectMarker(void* activeEffect)
{
    if (!activeEffect) return nullptr;
    for (UInt32 i = 0; i < kMaxTargetedEffectMarkers; ++i)
    {
        if (g_targetedEffectMarkers[i].activeEffect == activeEffect)
            return &g_targetedEffectMarkers[i];
    }
    return nullptr;
}

static TargetedEffectMarker* AllocateTargetedEffectMarker(void* activeEffect)
{
    if (!activeEffect) return nullptr;
    if (auto* existing = FindTargetedEffectMarker(activeEffect)) return existing;
    for (UInt32 i = 0; i < kMaxTargetedEffectMarkers; ++i)
    {
        if (!g_targetedEffectMarkers[i].activeEffect)
        {
            g_targetedEffectMarkers[i] = {};
            g_targetedEffectMarkers[i].activeEffect = activeEffect;
            ++g_targetedEffectMarkerCount;
            return &g_targetedEffectMarkers[i];
        }
    }
    return nullptr;
}

static void RemoveTargetedEffectMarker(void* activeEffect)
{
    auto* marker = FindTargetedEffectMarker(activeEffect);
    if (marker)
    {
        *marker = {};
        if (g_targetedEffectMarkerCount) --g_targetedEffectMarkerCount;
    }
}

static bool ArmTargetedStimpakRequest(Limb limb)
{
    const SInt32 limbValue = static_cast<SInt32>(limb);
    if (g_stimpakBackendState != StimpakBackendState::Ready || !IsValidLimbValue(limbValue))
        return false;

    const SInt32 state = __atomic_load_n(&g_targetRequest.state, __ATOMIC_ACQUIRE);
    if (state != kTargetRequest_Idle) return false;

    const UInt32 serial = __atomic_add_fetch(&g_targetRequest.serialCounter, 1u, __ATOMIC_ACQ_REL);
    __atomic_store_n(&g_targetRequest.limb, limbValue, __ATOMIC_RELEASE);
    __atomic_store_n(&g_targetRequest.serial, serial, __ATOMIC_RELEASE);
    __atomic_store_n(&g_targetRequest.state, kTargetRequest_Armed, __ATOMIC_RELEASE);
    g_targetRequest.candidateEffect = nullptr;
    g_targetRequest.candidateSerial = 0;
    return true;
}

static void ResetTargetedStimpakRequest()
{
    __atomic_store_n(&g_targetRequest.state, kTargetRequest_Idle, __ATOMIC_RELEASE);
    __atomic_store_n(&g_targetRequest.limb, static_cast<SInt32>(Limb::Invalid), __ATOMIC_RELEASE);
    __atomic_store_n(&g_targetRequest.serial, 0u, __ATOMIC_RELEASE);
    g_targetRequest.candidateEffect = nullptr;
    g_targetRequest.candidateSerial = 0;
}

static void ResetTransientStimpakState()
{
    ResetTargetedStimpakRequest();
    __atomic_store_n(&g_targetRequest.serialCounter, 0u, __ATOMIC_RELEASE);
    g_playerDrinkPotionDepth = 0;
    for (UInt32 i = 0; i < kMaxTargetedEffectMarkers; ++i)
        g_targetedEffectMarkers[i] = {};
    g_targetedEffectMarkerCount = 0;
}

static bool IsSaneTargetedAmount(float duration, float rate, float scale)
{
    // Broad corruption/layout guards, not gameplay caps. NaN self-comparison avoids a CRT math dependency.
    if (!(duration == duration) || !(rate == rate) || !(scale == scale)) return false;
    if (duration < 0.0f || duration > 3600.0f) return false;
    if (rate < -1000000.0f || rate > 1000000.0f) return false;
    if (scale < -1000.0f || scale > 1000.0f) return false;
    return true;
}

static bool ApplyAndRetireTargetedStimpakEffect(void* activeEffect, Limb limb)
{
    const UInt64 totalStart = PerfNowTicks();
    const SInt32 limbValue = static_cast<SInt32>(limb);
    if (!activeEffect || !IsValidLimbValue(limbValue)) return false;
    void* targetActor = nullptr;
    if (!IsExactPlayerStimpakEffect(activeEffect, &targetActor) || !targetActor) return false;
    if (!ResolveGameplayActorValues() || !g_limbAVs[LimbIndex(limb)] || !g_activeEffectDispel)
        return false;
    void* owner = GetPlayerActorValueOwner(targetActor);
    if (!HasFalloutImageVTable(owner)) return false;

    if (!AllocateTargetedEffectMarker(activeEffect)) return false;

    const float duration = *reinterpret_cast<const float*>(reinterpret_cast<const UInt8*>(activeEffect) + RuntimeLayout().activeEffectDuration);
    const float rate = *reinterpret_cast<const float*>(reinterpret_cast<const UInt8*>(activeEffect) + RuntimeLayout().activeEffectMagnitude);
    const float scale = StimpakGetConditionScale(activeEffect);

    // Exact vanilla RestoreHealthStimpak is a constant-rate effect on this runtime. vslot16 has
    // already applied Bethesda's perk/mod adjustments and converted magnitude into absolute HP/s,
    // so rate * duration * conditionScale is the finalized total used for instant limb repair.
    // This is intentionally not a generic integrator for arbitrary shaped/tapered magic effects.
    const float fullConditionAmount = rate * duration * scale;

    if (!IsSaneTargetedAmount(duration, rate, scale) || fullConditionAmount < 0.0f || fullConditionAmount > 1000000.0f)
    {
        RemoveTargetedEffectMarker(activeEffect);
        return false;
    }

    const UInt64 limbApplyStart = PerfNowTicks();
    ActorValueRestore(owner, g_limbAVs[LimbIndex(limb)], fullConditionAmount);
    const UInt64 limbApplyEnd = PerfNowTicks();
    PerfAccumulate(g_stimpakPerf.targetedLimbApply, limbApplyEnd - limbApplyStart);

    // Once the selected limb receives Bethesda's finalized total, the targeted Health effect
    // has no remaining gameplay role. Force-dispel removes its HUD projection and future Health ticks;
    // the marker remains only as defensive identity until Bethesda destroys the effect.
    const UInt64 dispelStart = PerfNowTicks();
    g_activeEffectDispel(activeEffect, true);
    const UInt64 dispelEnd = PerfNowTicks();
    PerfAccumulate(g_stimpakPerf.targetedDispel, dispelEnd - dispelStart);

    const UInt64 totalEnd = PerfNowTicks();
    PerfAccumulate(g_stimpakPerf.targetedApplyRetireTotal, totalEnd - totalStart);
    return true;
}

static void RecordTargetedApplyFailure()
{
    if (g_settings.performanceLog) ++g_stimpakPerf.targetedApplyFailures;
    ErrorLogLine("[TargetedStimpak] ERROR: targeted effect apply/retire failed; request cleared.");
}

static void FinishTargetedStimpakRequestAfterUse(Limb limb)
{
    const SInt32 state = __atomic_load_n(&g_targetRequest.state, __ATOMIC_ACQUIRE);
    const SInt32 armedLimb = __atomic_load_n(&g_targetRequest.limb, __ATOMIC_ACQUIRE);
    if (state == kTargetRequest_Armed && armedLimb == static_cast<SInt32>(limb))
        ResetTargetedStimpakRequest();
}

static bool Hook_PlayerDrinkPotion(void* player, void* potion, UInt32 stackID)
{
    const UInt64 totalStart = PerfNowTicks();
    ++g_playerDrinkPotionDepth;
    const bool exactPlayerStimpak = player && player == GetCurrentPlayer() && IsExactStimpakForm(potion);
    const SInt32 requestStateAtEnter = __atomic_load_n(&g_targetRequest.state, __ATOMIC_ACQUIRE);
    const SInt32 requestLimbAtEnter = __atomic_load_n(&g_targetRequest.limb, __ATOMIC_ACQUIRE);
    const UInt32 requestSerialAtEnter = __atomic_load_n(&g_targetRequest.serial, __ATOMIC_ACQUIRE);
    const bool targetedRequest = exactPlayerStimpak && requestStateAtEnter == kTargetRequest_Armed;

    const UInt64 nativeStart = PerfNowTicks();
    const bool result = g_playerOriginalDrinkPotion ? g_playerOriginalDrinkPotion(player, potion, stackID) : false;
    const UInt64 nativeEnd = PerfNowTicks();

    if (targetedRequest)
    {
        const UInt32 liveSerial = __atomic_load_n(&g_targetRequest.serial, __ATOMIC_ACQUIRE);
        const SInt32 liveLimb = __atomic_load_n(&g_targetRequest.limb, __ATOMIC_ACQUIRE);
        if (liveSerial == requestSerialAtEnter && liveLimb == requestLimbAtEnter)
        {
            if (result)
            {
                __atomic_store_n(&g_targetRequest.state, kTargetRequest_Confirmed, __ATOMIC_RELEASE);
                if (g_targetRequest.candidateEffect && g_targetRequest.candidateSerial == requestSerialAtEnter)
                {
                    if (!ApplyAndRetireTargetedStimpakEffect(g_targetRequest.candidateEffect, static_cast<Limb>(requestLimbAtEnter)))
                        RecordTargetedApplyFailure();
                    ResetTargetedStimpakRequest();
                }
            }
            else
            {
                ResetTargetedStimpakRequest();
            }
        }
    }

    if (g_playerDrinkPotionDepth) --g_playerDrinkPotionDepth;
    const UInt64 totalEnd = PerfNowTicks();
    if (exactPlayerStimpak)
    {
        PerfAccumulate(targetedRequest ? g_stimpakPerf.targetedDrinkNative : g_stimpakPerf.ordinaryDrinkNative, nativeEnd - nativeStart);
        PerfAccumulate(targetedRequest ? g_stimpakPerf.targetedDrinkTotal : g_stimpakPerf.ordinaryDrinkTotal, totalEnd - totalStart);
    }
    return result;
}

static void Hook_StimpakStart(void* activeEffect)
{
    const UInt64 totalStart = PerfNowTicks();
    const UInt64 nativeStart = PerfNowTicks();
    if (g_stimpakEffectOriginalStart) g_stimpakEffectOriginalStart(activeEffect);
    const UInt64 nativeEnd = PerfNowTicks();

    const bool exactPlayerStimpak = IsExactPlayerStimpakEffect(activeEffect);
    bool targeted = false;
    if (exactPlayerStimpak)
    {
        const SInt32 requestState = __atomic_load_n(&g_targetRequest.state, __ATOMIC_ACQUIRE);
        const SInt32 requestLimb = __atomic_load_n(&g_targetRequest.limb, __ATOMIC_ACQUIRE);
        const UInt32 requestSerial = __atomic_load_n(&g_targetRequest.serial, __ATOMIC_ACQUIRE);
        if (requestState == kTargetRequest_Confirmed)
        {
            targeted = true;
            if (!ApplyAndRetireTargetedStimpakEffect(activeEffect, static_cast<Limb>(requestLimb)))
                RecordTargetedApplyFailure();
            ResetTargetedStimpakRequest();
        }
        else if (requestState == kTargetRequest_Armed && g_playerDrinkPotionDepth > 0)
        {
            targeted = true;
            g_targetRequest.candidateEffect = activeEffect;
            g_targetRequest.candidateSerial = requestSerial;
        }
    }

    const UInt64 totalEnd = PerfNowTicks();
    if (exactPlayerStimpak)
    {
        if (targeted)
        {
            if (g_settings.performanceLog) ++g_stimpakPerf.targetedEffects;
            PerfAccumulate(g_stimpakPerf.targetedStartNative, nativeEnd - nativeStart);
            PerfAccumulate(g_stimpakPerf.targetedStartTotal, totalEnd - totalStart);
        }
        else
        {
            if (g_settings.performanceLog) ++g_stimpakPerf.ordinaryEffects;
            PerfAccumulate(g_stimpakPerf.ordinaryStartNative, nativeEnd - nativeStart);
            PerfAccumulate(g_stimpakPerf.ordinaryStartTotal, totalEnd - totalStart);
        }
    }
}

static void* Hook_StimpakDestructor(void* activeEffect, UInt32 flags)
{
    if (g_targetedEffectMarkerCount) RemoveTargetedEffectMarker(activeEffect);
    return g_stimpakEffectOriginalDestructor ? g_stimpakEffectOriginalDestructor(activeEffect, flags) : activeEffect;
}

static bool PatchVTableSlot(void** slot, void* replacement, void** originalOut, const char* label)
{
    if (!slot || !replacement || !originalOut || !g_VirtualProtect) return false;
    UInt32 oldProtect = 0;
    if (!g_VirtualProtect(slot, sizeof(void*), kPageExecuteReadWrite, &oldProtect))
    {
        ErrorLogRaw("[StimpakBackend] hook install FAILED VirtualProtect: "); ErrorLogLine(label);
        return false;
    }

    void* original = *slot;
    *slot = replacement;
    UInt32 ignored = 0;
    if (!g_VirtualProtect(slot, sizeof(void*), oldProtect, &ignored))
    {
        // The page is still writable at this point, so restore the original slot before failing.
        *slot = original;
        UInt32 secondIgnored = 0;
        g_VirtualProtect(slot, sizeof(void*), oldProtect, &secondIgnored);
        ErrorLogRaw("[StimpakBackend] hook install FAILED restoring page protection: "); ErrorLogLine(label);
        return false;
    }

    *originalOut = original;
    return true;
}

static bool RestoreVTableSlot(void** slot, void* original, const char* label)
{
    if (!slot || !original || !g_VirtualProtect) return false;
    UInt32 oldProtect = 0;
    if (!g_VirtualProtect(slot, sizeof(void*), kPageExecuteReadWrite, &oldProtect))
    {
        ErrorLogRaw("[StimpakBackend] hook rollback FAILED VirtualProtect: "); ErrorLogLine(label);
        return false;
    }
    *slot = original;
    UInt32 ignored = 0;
    if (!g_VirtualProtect(slot, sizeof(void*), oldProtect, &ignored))
    {
        ErrorLogRaw("[StimpakBackend] hook rollback restored slot but FAILED restoring page protection: "); ErrorLogLine(label);
        return false;
    }
    return true;
}

static bool RollbackStimpakLifecycleHooks()
{
    bool ok = true;

    if (g_stimpakDestructorHookSlot && g_stimpakEffectOriginalDestructor)
    {
        const bool restored = RestoreVTableSlot(g_stimpakDestructorHookSlot, reinterpret_cast<void*>(g_stimpakEffectOriginalDestructor), "StimpakEffect destructor");
        ok = restored && ok;
        if (restored)
        {
            g_stimpakDestructorHookSlot = nullptr;
            g_stimpakEffectOriginalDestructor = nullptr;
        }
    }
    if (g_stimpakStartHookSlot && g_stimpakEffectOriginalStart)
    {
        const bool restored = RestoreVTableSlot(g_stimpakStartHookSlot, reinterpret_cast<void*>(g_stimpakEffectOriginalStart), "StimpakEffect vslot16");
        ok = restored && ok;
        if (restored)
        {
            g_stimpakStartHookSlot = nullptr;
            g_stimpakEffectOriginalStart = nullptr;
        }
    }
    if (g_playerDrinkPotionHookSlot && g_playerOriginalDrinkPotion)
    {
        const bool restored = RestoreVTableSlot(g_playerDrinkPotionHookSlot, reinterpret_cast<void*>(g_playerOriginalDrinkPotion), "PlayerCharacter::DrinkPotion");
        ok = restored && ok;
        if (restored)
        {
            g_playerDrinkPotionHookSlot = nullptr;
            g_playerOriginalDrinkPotion = nullptr;
        }
    }

    // If any restore failed, retain every original pointer needed by a residual hook so that hook
    // can still forward safely. The backend enters a sticky Failed state and targeted arming is
    // refused; we never discard a trampoline/original pointer while its vtable slot may still point at PLH.
    if (!g_stimpakStartHookSlot && !g_stimpakDestructorHookSlot && !g_playerDrinkPotionHookSlot)
    {
        g_activeEffectDispel = nullptr;
    }
    return ok;
}

static bool InstallStimpakLifecycleHooks(const char* reason)
{
    if (!LoadEngineAddressTable() || !g_engineAddresses.playerVTable || !g_engineAddresses.playerDrinkPotion ||
        !g_engineAddresses.stimpakVTable || !g_engineAddresses.stimpakStart || !g_engineAddresses.stimpakDestructor ||
        !g_engineAddresses.activeEffectTargetActor || !g_engineAddresses.stimpakSelectedActorValue ||
        !g_engineAddresses.stimpakConditionScale || !g_engineAddresses.activeEffectDispel)
    {
        ErrorLogLine("[StimpakBackend] lifecycle install FAILED: required Address Library identity missing.");
        return false;
    }

    if (!ValidateStimpakHelperVTableContracts(false))
    {
        ErrorLogLine("[StimpakBackend] lifecycle install FAILED: Stimpak helper virtual contract mismatch.");
        return false;
    }

    auto** playerVTable = reinterpret_cast<void**>(g_engineAddresses.playerVTable);
    auto** stimpakVTable = reinterpret_cast<void**>(g_engineAddresses.stimpakVTable);
    if (reinterpret_cast<UIntPtr>(playerVTable[RuntimeLayout().playerDrinkPotionVSlot]) != g_engineAddresses.playerDrinkPotion ||
        reinterpret_cast<UIntPtr>(stimpakVTable[RuntimeLayout().stimpakStartVSlot]) != g_engineAddresses.stimpakStart ||
        reinterpret_cast<UIntPtr>(stimpakVTable[RuntimeLayout().stimpakDestructorVSlot]) != g_engineAddresses.stimpakDestructor)
    {
        ErrorLogLine("[StimpakBackend] lifecycle install FAILED: exact vtable identity mismatch.");
        return false;
    }

    g_activeEffectDispel = reinterpret_cast<ActiveEffectDispelFn>(g_engineAddresses.activeEffectDispel);

    void* original = nullptr;
    g_playerDrinkPotionHookSlot = &playerVTable[RuntimeLayout().playerDrinkPotionVSlot];
    if (!PatchVTableSlot(g_playerDrinkPotionHookSlot, reinterpret_cast<void*>(&Hook_PlayerDrinkPotion), &original, "PlayerCharacter::DrinkPotion"))
    {
        g_playerDrinkPotionHookSlot = nullptr;
        return false;
    }
    g_playerOriginalDrinkPotion = reinterpret_cast<PlayerDrinkPotionFn>(original);

    original = nullptr;
    g_stimpakStartHookSlot = &stimpakVTable[RuntimeLayout().stimpakStartVSlot];
    if (!PatchVTableSlot(g_stimpakStartHookSlot, reinterpret_cast<void*>(&Hook_StimpakStart), &original, "StimpakEffect vslot16"))
    {
        RollbackStimpakLifecycleHooks();
        return false;
    }
    g_stimpakEffectOriginalStart = reinterpret_cast<StimpakEffectStartFn>(original);

    original = nullptr;
    g_stimpakDestructorHookSlot = &stimpakVTable[RuntimeLayout().stimpakDestructorVSlot];
    if (!PatchVTableSlot(g_stimpakDestructorHookSlot, reinterpret_cast<void*>(&Hook_StimpakDestructor), &original, "StimpakEffect destructor"))
    {
        RollbackStimpakLifecycleHooks();
        return false;
    }
    g_stimpakEffectOriginalDestructor = reinterpret_cast<StimpakEffectDestructorFn>(original);

    CalibrateQpcTelemetry();
    LogLine("[StimpakBackend] lifecycle hooks installed: DrinkPotion + Stimpak vslot16 + destructor.");
    LogRaw("[StimpakBackend] reason="); LogLine(reason ? reason : "unknown");
    return true;
}


static void Hook_StimpakValueApply(void* activeEffect, void* targetActor, float amount, void* incomingActorValueOverride)
{
    const UInt64 totalStart = PerfNowTicks();
    auto original = g_stimpakEffectOriginalValueApply;
    auto baseApply = g_valueModifierEffectBaseValueApply;
    if (!original || !baseApply)
    {
        if (original) original(activeEffect, targetActor, amount, incomingActorValueOverride);
        return;
    }

    void* player = GetCurrentPlayer();
    if (!targetActor || !player || targetActor != player || !activeEffect)
    {
        original(activeEffect, targetActor, amount, incomingActorValueOverride);
        return;
    }

    const bool exactPlayerStimpak = IsExactRestoreHealthStimpakEffect(activeEffect);
    const UInt8 stimpakStateByte = *reinterpret_cast<const UInt8*>(reinterpret_cast<const UInt8*>(activeEffect) + RuntimeLayout().stimpakEffectStateByte);
    if (!exactPlayerStimpak || stimpakStateByte != 0)
    {
        original(activeEffect, targetActor, amount, incomingActorValueOverride);
        return;
    }

    if (g_targetedEffectMarkerCount)
    {
        if (FindTargetedEffectMarker(activeEffect))
        {
            if (g_settings.performanceLog) ++g_stimpakPerf.targetedVslot21Suppressed;
            const UInt64 totalEnd = PerfNowTicks();
            PerfAccumulate(g_stimpakPerf.targetedVslot21Total, totalEnd - totalStart);
            return;
        }
    }

    const UInt64 nativeStart = PerfNowTicks();
    if (g_settings.ordinaryStimpaksHealLimbs)
        original(activeEffect, targetActor, amount, incomingActorValueOverride);
    else
        baseApply(activeEffect, targetActor, amount, nullptr);
    const UInt64 nativeEnd = PerfNowTicks();
    const UInt64 totalEnd = PerfNowTicks();
    PerfAccumulate(g_stimpakPerf.ordinaryVslot21NativePath, nativeEnd - nativeStart);
    PerfAccumulate(g_stimpakPerf.ordinaryVslot21Total, totalEnd - totalStart);
}


static bool InstallStimpakSuppressionHook(const char* reason)
{
    if (!g_VirtualProtect)
    {
        ErrorLogLine("[StimpakSuppression] install FAILED: VirtualProtect unavailable.");
        return false;
    }

    if (!LoadEngineAddressTable() || !g_engineAddresses.stimpakVTable || !g_engineAddresses.valueModifierVTable ||
        !g_engineAddresses.stimpakValueApply || !g_engineAddresses.valueModifierValueApply)
    {
        ErrorLogLine("[StimpakSuppression] install FAILED: Address Library resolution failed.");
        return false;
    }

    auto** stimpakVTable = reinterpret_cast<void**>(g_engineAddresses.stimpakVTable);
    auto** valueModifierVTable = reinterpret_cast<void**>(g_engineAddresses.valueModifierVTable);
    void** slot = &stimpakVTable[RuntimeLayout().stimpakValueApplyVSlot];
    if (reinterpret_cast<UIntPtr>(*slot) != g_engineAddresses.stimpakValueApply)
    {
        ErrorLogLine("[StimpakSuppression] install FAILED: StimpakEffect vslot 0x21 identity mismatch.");
        ErrorLogHex("[StimpakSuppression] observedStimpakSlot=", reinterpret_cast<UIntPtr>(*slot));
        ErrorLogHex("[StimpakSuppression] expectedStimpakSlot=", g_engineAddresses.stimpakValueApply);
        return false;
    }
    if (reinterpret_cast<UIntPtr>(valueModifierVTable[RuntimeLayout().stimpakValueApplyVSlot]) != g_engineAddresses.valueModifierValueApply)
    {
        ErrorLogLine("[StimpakSuppression] install FAILED: ValueModifierEffect vslot 0x21 identity mismatch.");
        ErrorLogHex("[StimpakSuppression] observedBaseSlot=", reinterpret_cast<UIntPtr>(valueModifierVTable[RuntimeLayout().stimpakValueApplyVSlot]));
        ErrorLogHex("[StimpakSuppression] expectedBaseSlot=", g_engineAddresses.valueModifierValueApply);
        return false;
    }

    void* original = nullptr;
    if (!PatchVTableSlot(slot, reinterpret_cast<void*>(&Hook_StimpakValueApply), &original, "StimpakEffect vslot21"))
        return false;

    g_stimpakEffectOriginalValueApply = reinterpret_cast<StimpakEffectValueApplyFn>(original);
    g_valueModifierEffectBaseValueApply = reinterpret_cast<StimpakEffectValueApplyFn>(g_engineAddresses.valueModifierValueApply);

    LogLine("[StimpakSuppression] hook installed: runtime setting selects Health-only suppression or full Bethesda ordinary Stimpak behavior.");
    LogRaw("[StimpakSuppression] reason="); LogLine(reason ? reason : "unknown");
    LogLine("[StimpakSuppression] scope validated: player + Stimpak 00023736 + RestoreHealthStimpak 0021DDB8 + Health AV; all nonmatching calls remain vanilla.");
    return true;
}

static bool InstallStimpakBackend(const char* reason)
{
    if (g_stimpakBackendState == StimpakBackendState::Ready) return true;
    if (g_stimpakBackendState == StimpakBackendState::Failed) return false;

    // Install lifecycle hooks first. If the final suppression hook cannot be installed, roll the
    // lifecycle hooks back so PLH never continues in a partially-installed Stimpak configuration.
    if (!InstallStimpakLifecycleHooks(reason))
    {
        g_stimpakBackendState = StimpakBackendState::Failed;
        ErrorLogLine("[StimpakBackend] FAILED: targeted lifecycle hooks unavailable.");
        return false;
    }
    if (!InstallStimpakSuppressionHook(reason))
    {
        const bool rollbackOK = RollbackStimpakLifecycleHooks();
        g_stimpakBackendState = StimpakBackendState::Failed;
        ErrorLogLine(rollbackOK ? "[StimpakBackend] FAILED: suppression hook unavailable; lifecycle hooks rolled back."
                                : "[StimpakBackend] FAILED: suppression hook unavailable; lifecycle rollback also failed.");
        return false;
    }

    g_stimpakBackendState = StimpakBackendState::Ready;
    LogLine("[StimpakBackend] READY: configurable ordinary Stimpak policy + targeted native Stimpak path installed.");
    StatusLogLine("[Startup] Stimpak backend=READY");
    return true;
}


// ---------------------------------------------------------------------------
// ConditionRateMult ownership and save/load lifecycle
// ---------------------------------------------------------------------------
static void LogConditionSnapshot(const char* prefix, void* player, void* owner, void* av)
{
    if (!DebugVerboseEnabled()) return;
    LogRaw("[ConditionGate] snapshot: ");
    LogLine(prefix ? prefix : "unknown");
    LogHex("[ConditionGate] player=", reinterpret_cast<UIntPtr>(player));
    LogHex("[ConditionGate] owner=", reinterpret_cast<UIntPtr>(owner));
    LogFloat("[ConditionGate] current=", ActorValueGet(owner, av));
    LogFloat("[ConditionGate] permanent=", ActorValueGetModifier(owner, kActorValueModifier_Permanent, av));
    LogFloat("[ConditionGate] temporary=", ActorValueGetModifier(owner, kActorValueModifier_Temporary, av));
    LogFloat("[ConditionGate] damage=", ActorValueGetModifier(owner, kActorValueModifier_Damage, av));
}

static void ClearConditionOwnershipBookkeeping()
{
    g_conditionOwned = false;
    g_ownedConditionPlayer = nullptr;
    g_ownedConditionDelta = 0.0f;
    g_ownedConditionTemporaryBaseline = 0.0f;
}

static bool OwnedConditionContributionIsProvablyMissing(void* owner, void* av)
{
    if (!g_conditionOwned || !owner || !av || AbsFloat(g_ownedConditionDelta) <= kConditionRateEpsilon)
        return false;

    const float temporary = ActorValueGetModifier(owner, kActorValueModifier_Temporary, av);
    const float expectedWithOwnedDelta = g_ownedConditionTemporaryBaseline + g_ownedConditionDelta;

    // The ActorValueOwner exposes aggregate modifier buckets rather than source-tagged entries.
    // We therefore only declare PLH's contribution missing when the temporary bucket has returned
    // exactly to the baseline observed immediately before PLH applied its own delta. A load can
    // restore that engine bucket while PLH still remembers prior ownership, so this exact shape is
    // treated as stale bookkeeping before any new delta is applied or removed.
    return AbsFloat(temporary - g_ownedConditionTemporaryBaseline) <= kConditionRateEpsilon &&
           AbsFloat(temporary - expectedWithOwnedDelta) > kConditionRateEpsilon;
}

static bool RemoveOwnedConditionSuppression(const char* reason)
{
    LogRaw("[ConditionGate] remove requested: ");
    LogLine(reason ? reason : "unknown");

    if (!g_conditionOwned)
    {
        LogLine("[ConditionGate] remove: no PLH-owned modifier active.");
        ClearConditionOwnershipBookkeeping();
        return true;
    }

    void* player = GetCurrentPlayer();
    if (!player || player != g_ownedConditionPlayer)
    {
        ErrorLogLine("[ConditionGate] remove FAILED: current player does not match owned player; clearing bookkeeping only.");
        ClearConditionOwnershipBookkeeping();
        return false;
    }

    void* av = ResolveConditionRateMultAV();
    void* owner = GetPlayerActorValueOwner(player);
    if (!av || !HasFalloutImageVTable(owner))
    {
        ErrorLogLine("[ConditionGate] remove FAILED: owner/AV unavailable; clearing bookkeeping only.");
        ClearConditionOwnershipBookkeeping();
        return false;
    }

    if (OwnedConditionContributionIsProvablyMissing(owner, av))
    {
        LogLine("[ConditionGate] stale ownership reconciled before remove: PLH temporary contribution is already absent; no inverse applied.");
        LogFloat("[ConditionGate] tracked temporary baseline=", g_ownedConditionTemporaryBaseline);
        LogFloat("[ConditionGate] tracked owned delta=", g_ownedConditionDelta);
        LogConditionSnapshot("stale-before-remove", player, owner, av);
        ClearConditionOwnershipBookkeeping();
        return true;
    }

    LogConditionSnapshot("before remove", player, owner, av);
    const float inverse = -g_ownedConditionDelta;
    LogFloat("[ConditionGate] inverse delta=", inverse);
    ActorValueModify(owner, kActorValueModifier_Temporary, av, inverse);
    LogConditionSnapshot("after remove", player, owner, av);

    ClearConditionOwnershipBookkeeping();
    LogLine("[ConditionGate] PLH-owned modifier removed.");
    return true;
}

enum class ConditionEnsureReason : UInt32
{
    PostLoadSuccess,
    PostLoadResume,
    PostSave,
    NewGame,
    SettingsChanged,
    ScaleformBridgeReady
};

static const char* ConditionEnsureReasonText(ConditionEnsureReason reason)
{
    switch (reason)
    {
        case ConditionEnsureReason::PostLoadSuccess: return "PostLoadGame success";
        case ConditionEnsureReason::PostLoadResume: return "PostLoadGame failed/resume";
        case ConditionEnsureReason::PostSave: return "PostSaveGame";
        case ConditionEnsureReason::NewGame: return "NewGame";
        case ConditionEnsureReason::SettingsChanged: return "MCM settings changed";
        case ConditionEnsureReason::ScaleformBridgeReady: return "Scaleform bridge ready";
        default: return "unknown";
    }
}

static bool EnsurePlayerConditionSuppression(ConditionEnsureReason reason, bool& playerReady)
{
    playerReady = false;
    LogRaw("[ConditionGate] ensure requested: ");
    LogLine(ConditionEnsureReasonText(reason));

    if (!ConditionEngineRefsReady())
    {
        ErrorLogLine("[ConditionGate] ensure FAILED: engine refs unavailable.");
        return false;
    }

    void* player = GetCurrentPlayer();
    if (!player)
    {
        LogLine("[ConditionGate] player not ready.");
        return false;
    }
    playerReady = true;

    void* av = ResolveConditionRateMultAV();
    void* owner = GetPlayerActorValueOwner(player);
    if (!av || !HasFalloutImageVTable(owner))
    {
        ErrorLogLine("[ConditionGate] ensure FAILED: ConditionRateMult or ActorValueOwner invalid.");
        return false;
    }

    if (g_conditionOwned && g_ownedConditionPlayer == player)
    {
        if (OwnedConditionContributionIsProvablyMissing(owner, av))
        {
            LogLine("[ConditionGate] stale ownership detected: engine temporary bucket returned to PLH pre-apply baseline.");
            LogFloat("[ConditionGate] tracked temporary baseline=", g_ownedConditionTemporaryBaseline);
            LogFloat("[ConditionGate] tracked owned delta=", g_ownedConditionDelta);
            LogConditionSnapshot("stale-owned-contribution-missing", player, owner, av);
            ClearConditionOwnershipBookkeeping();
            LogLine("[ConditionGate] stale ownership cleared; continuing through normal ensure path.");
        }
        else
        {
            const float current = ActorValueGet(owner, av);
            if (AbsFloat(current) <= kConditionRateEpsilon)
            {
                LogLine("[ConditionGate] already applied and effective value is zero.");
                return true;
            }

            LogLine("[ConditionGate] owned contribution not provably missing but effective value changed; preserving ownership and logging only.");
            LogConditionSnapshot("owned-but-nonzero-ambiguous", player, owner, av);
            return true;
        }
    }

    if (g_conditionOwned && g_ownedConditionPlayer != player)
    {
        LogLine("[ConditionGate] stale owned-player bookkeeping detected; clearing before apply.");
        ClearConditionOwnershipBookkeeping();
    }

    LogConditionSnapshot("before ensure", player, owner, av);
    const float current = ActorValueGet(owner, av);
    if (current <= kConditionRateEpsilon)
    {
        LogLine("[ConditionGate] effective ConditionRateMult already suppressed externally; PLH applies no modifier.");
        ClearConditionOwnershipBookkeeping();
        return true;
    }

    const float temporaryBefore = ActorValueGetModifier(owner, kActorValueModifier_Temporary, av);
    const float delta = -current;
    LogFloat("[ConditionGate] temporary baseline before apply=", temporaryBefore);
    LogFloat("[ConditionGate] applying temporary delta=", delta);
    ActorValueModify(owner, kActorValueModifier_Temporary, av, delta);
    const float after = ActorValueGet(owner, av);
    LogConditionSnapshot("after ensure", player, owner, av);

    if (AbsFloat(after) > kConditionRateEpsilon)
    {
        ErrorLogLine("[ConditionGate] verify FAILED; reverting attempted delta.");
        ActorValueModify(owner, kActorValueModifier_Temporary, av, -delta);
        LogConditionSnapshot("after failed-verify revert", player, owner, av);
        return false;
    }

    g_conditionOwned = true;
    g_ownedConditionPlayer = player;
    g_ownedConditionDelta = delta;
    g_ownedConditionTemporaryBaseline = temporaryBefore;
    LogFloat("[ConditionGate] tracked temporary baseline=", g_ownedConditionTemporaryBaseline);
    LogFloat("[ConditionGate] tracked owned delta=", g_ownedConditionDelta);
    LogLine("[ConditionGate] SUCCESS: player-only ConditionRateMult suppressed to zero.");
    return true;
}

// ---------------------------------------------------------------------------
// Main-thread task scheduling and F4SE lifecycle messages
// ---------------------------------------------------------------------------
static void* AllocateTaskMemory(UIntPtr size)
{
    if (!g_processHeap || !g_RtlAllocateHeap || !size) return nullptr;
    return g_RtlAllocateHeap(g_processHeap, 0, size);
}

static void FreeTaskMemory(void* ptr)
{
    if (ptr && g_processHeap && g_RtlFreeHeap) g_RtlFreeHeap(g_processHeap, 0, ptr);
}

static bool QueueConditionEnsure(ConditionEnsureReason reason, UInt32 attempt);

static const char* OnOffText(bool value) { return value ? "On" : "Off"; }

static const char* DebugLogLevelText(DebugLogLevel value)
{
    switch (value)
    {
        case DebugLogLevel::Off: return "Off";
        case DebugLogLevel::Errors: return "Errors";
        case DebugLogLevel::Verbose: return "Verbose";
        default: return "Unknown";
    }
}

static void LogSettingBoolChange(const char* name, bool before, bool after)
{
    if (!DebugVerboseEnabled() || before == after) return;
    LogRaw("[Settings] "); LogRaw(name); LogRaw(": "); LogRaw(OnOffText(before)); LogRaw(" -> "); LogLine(OnOffText(after));
}

static void LogSettingDebugChange(DebugLogLevel before, DebugLogLevel after)
{
    if (!DebugVerboseEnabled() || before == after) return;
    LogRaw("[Settings] DebugLog: "); LogRaw(DebugLogLevelText(before)); LogRaw(" -> "); LogLine(DebugLogLevelText(after));
}

static void LogChangedSettingsVerbose(const PLHSettings& before, const PLHSettings& after)
{
    LogSettingBoolChange("PassiveLimbRegeneration", before.passiveLimbRegeneration, after.passiveLimbRegeneration);
    LogSettingBoolChange("OrdinaryStimpaksHealLimbs", before.ordinaryStimpaksHealLimbs, after.ordinaryStimpaksHealLimbs);
    LogSettingBoolChange("PerformanceLog", before.performanceLog, after.performanceLog);
    LogSettingDebugChange(before.debugLog, after.debugLog);
}

static void ApplySettingsSnapshotMainThread(const PLHSettings& next)
{
    const PLHSettings previous = g_settings;
    if (SettingsEqual(previous, next)) return;

    const bool wasVerbose = previous.debugLog == DebugLogLevel::Verbose;
    const bool willBeVerbose = next.debugLog == DebugLogLevel::Verbose;

    // If logging is being enabled, open the file before the transition is applied so the
    // first Verbose transition itself can be recorded.
    if (next.debugLog != DebugLogLevel::Off || next.performanceLog) OpenLogFileUnconditionally();

    if (previous.performanceLog && !next.performanceLog)
        LogPerformanceSnapshot("PerformanceLog disabled");

    // When lowering Verbose, record every setting change while Verbose is still active.
    if (wasVerbose) LogChangedSettingsVerbose(previous, next);

    g_settings = next;
    EnsureLogFileOpen();

    // When raising to Verbose, record the transition after Verbose becomes active.
    if (!wasVerbose && willBeVerbose) LogChangedSettingsVerbose(previous, next);

    if (previous.performanceLog != next.performanceLog)
    {
        ResetPerformanceTelemetry();
        if (next.performanceLog) CalibrateQpcTelemetry();
    }

    if (previous.passiveLimbRegeneration != next.passiveLimbRegeneration)
    {
        if (next.passiveLimbRegeneration)
        {
            if (g_conditionOwned)
                RemoveOwnedConditionSuppression("MCM setting: passive limb regeneration enabled");
        }
        else
        {
            QueueConditionEnsure(ConditionEnsureReason::SettingsChanged, 0);
        }
    }

    CloseLogFileIfUnused();
}

struct SettingsApplyTask final : ITaskDelegate
{
    PLHSettings snapshot;

    explicit SettingsApplyTask(const PLHSettings& value) : snapshot(value) {}
    ~SettingsApplyTask() override {}
    static void operator delete(void* ptr) noexcept { FreeTaskMemory(ptr); }
    static void operator delete(void* ptr, unsigned long long) noexcept { FreeTaskMemory(ptr); }

    void Run() override { ApplySettingsSnapshotMainThread(snapshot); }
};

static bool QueueSettingsApply(const PLHSettings& snapshot)
{
    if (!g_taskInterface || !g_taskInterface->AddTask) return false;
    void* memory = AllocateTaskMemory(sizeof(SettingsApplyTask));
    if (!memory) return false;
    auto* task = new (memory) SettingsApplyTask(snapshot);
    g_taskInterface->AddTask(task);
    return true;
}

static constexpr UInt32 kWaitObject0 = 0u;
static constexpr UInt32 kInfiniteWait = 0xFFFFFFFFu;
static constexpr UInt32 kFileNotifyChangeFileName = 0x00000001u;
static constexpr UInt32 kFileNotifyChangeSize = 0x00000008u;
static constexpr UInt32 kFileNotifyChangeLastWrite = 0x00000010u;

static UInt32 SettingsWatcherThread(void* context)
{
    void* changeHandle = context;
    if (!changeHandle || changeHandle == kInvalidHandleValue || !g_WaitForSingleObject ||
        !g_FindNextChangeNotification || !g_FindCloseChangeNotification)
        return 0;

    for (;;)
    {
        if (g_WaitForSingleObject(changeHandle, kInfiniteWait) != kWaitObject0) break;
        // MCM may write/replace an INI in more than one filesystem operation. A short delay keeps
        // the worker from reading an intermediate file while remaining far outside gameplay paths.
        if (g_Sleep) g_Sleep(50u);
        QueueSettingsApply(ReadSettingsFromDisk());
        if (!g_FindNextChangeNotification(changeHandle)) break;
    }
    g_FindCloseChangeNotification(changeHandle);
    return 0;
}

static bool StartSettingsWatcher()
{
    if (g_settingsWatcherStarted) return true;
    if (!g_FindFirstChangeNotificationA || !g_FindNextChangeNotification || !g_FindCloseChangeNotification ||
        !g_WaitForSingleObject || !g_CreateThread)
    {
        ErrorLogLine("[Settings] live synchronization unavailable: required Win32 change-notification APIs missing.");
        return false;
    }

    char mcmPath[1024];
    if (!BuildGameDirectoryFilePath("Data\\MCM", mcmPath, sizeof(mcmPath)))
    {
        ErrorLogLine("[Settings] live synchronization unavailable: failed to construct Data\\MCM path.");
        return false;
    }

    void* changeHandle = g_FindFirstChangeNotificationA(
        mcmPath,
        1,
        kFileNotifyChangeFileName | kFileNotifyChangeSize | kFileNotifyChangeLastWrite);
    if (!changeHandle || changeHandle == kInvalidHandleValue)
    {
        ErrorLogLine("[Settings] live synchronization unavailable: Data\\MCM change notification could not be created.");
        return false;
    }

    UInt32 threadID = 0;
    void* thread = g_CreateThread(nullptr, 0, &SettingsWatcherThread, changeHandle, 0, &threadID);
    if (!thread || thread == kInvalidHandleValue)
    {
        g_FindCloseChangeNotification(changeHandle);
        ErrorLogLine("[Settings] live synchronization unavailable: watcher thread creation failed.");
        return false;
    }
    if (g_CloseHandle) g_CloseHandle(thread);
    g_settingsWatcherStarted = true;
    LogLine("[Settings] MCM directory watcher active; setting-file I/O is isolated from gameplay/healing paths.");
    return true;
}

struct ConditionEnsureTask final : ITaskDelegate
{
    ConditionEnsureReason reason;
    UInt32 attempt;

    ConditionEnsureTask(ConditionEnsureReason a_reason, UInt32 a_attempt) : reason(a_reason), attempt(a_attempt) {}
    ~ConditionEnsureTask() override {}

    static void operator delete(void* ptr) noexcept { FreeTaskMemory(ptr); }
    static void operator delete(void* ptr, unsigned long long) noexcept { FreeTaskMemory(ptr); }

    void Run() override
    {
        bool playerReady = false;
        bool ok = false;
        if (!g_settings.passiveLimbRegeneration)
        {
            ok = EnsurePlayerConditionSuppression(reason, playerReady);
        }
        else
        {
            void* player = GetCurrentPlayer();
            playerReady = player != nullptr;
            if (playerReady)
            {
                if (g_conditionOwned) RemoveOwnedConditionSuppression("passive regeneration enabled");
                else ClearConditionOwnershipBookkeeping();
                ok = true;
            }
        }
        if (ok)
        {
            const bool refsOK = ResolveGameplayActorValues() && ResolveStimpakForm();
            if (!refsOK)
            {
                ErrorLogLine("[Production] player ready but Stimpak/ActorValue references are unavailable.");
                return;
            }
            if (!StimpakBackendActivationAllowed())
            {
                if (!g_stimpakBackendDeferredLogged)
                {
                    StatusLogLine("[Startup] Stimpak backend=DEFERRED; unqualified runtime requires one proven Scaleform bridge before hook activation.");
                    g_stimpakBackendDeferredLogged = true;
                }
                return;
            }
            const char* reasonText = ConditionEnsureReasonText(reason);
            if (!InstallStimpakBackend(reasonText))
                ErrorLogLine("[Production] Stimpak backend unavailable; refusing partial hook state.");
            return;
        }

        if (!playerReady && attempt < 600)
        {
            if (attempt == 0 || (attempt % 60u) == 59u)
                LogInt("[ConditionGate] retry attempt=", static_cast<SInt32>(attempt + 1));
            QueueConditionEnsure(reason, attempt + 1);
        }
        else if (!playerReady)
        {
            ErrorLogLine("[ConditionGate] retry limit reached; suppression not applied.");
        }
    }
};

static bool QueueConditionEnsure(ConditionEnsureReason reason, UInt32 attempt)
{
    if (!g_taskInterface || !g_taskInterface->AddTask)
    {
        ErrorLogLine("[ConditionGate] queue FAILED: F4SE task interface unavailable.");
        return false;
    }

    void* memory = AllocateTaskMemory(sizeof(ConditionEnsureTask));
    if (!memory)
    {
        ErrorLogLine("[ConditionGate] queue FAILED: RtlAllocateHeap/process heap unavailable.");
        return false;
    }

    auto* task = new (memory) ConditionEnsureTask(reason, attempt);
    g_taskInterface->AddTask(task);
    if (attempt == 0)
    {
        LogRaw("[ConditionGate] queued main-thread ensure: ");
        LogLine(ConditionEnsureReasonText(reason));
    }
    return true;
}

static void OnF4SEMessage(F4SEMessagingInterface::Message* message)
{
    if (!message) return;

    switch (message->type)
    {
        case kMessage_GameDataReady:
        {
            const bool ready = message->data != nullptr;
            LogLine(ready ? "F4SE message: GameDataReady=true" : "F4SE message: GameDataReady=false");
            break;
        }
        case kMessage_PreLoadGame:
            LogPerformanceSnapshot("PreLoadGame");
            ResetTransientStimpakState();
            if (g_logFile != kInvalidHandleValue && g_FlushFileBuffers) g_FlushFileBuffers(g_logFile);
            LogLine("F4SE message: PreLoadGame");
            // Do not touch ActorValues here. F4SE load messages can occur during
            // engine load work; the old player is about to be replaced anyway. Targeted
            // Stimpak request/effect-marker state is process-transient and is reset here.
            // On successful PostLoadGame, the queued main-thread ensure detects
            // the new player pointer and discards stale ownership bookkeeping.
            // On failed load, the old player and its active modifier remain valid.
            LogLine("[ConditionGate] PreLoadGame: ownership retained; no ActorValue mutation.");
            break;
        case kMessage_PostLoadGame:
        {
            const bool loaded = message->data != nullptr;
            LogLine(loaded ? "F4SE message: PostLoadGame success" : "F4SE message: PostLoadGame failed");
            QueueConditionEnsure(loaded ? ConditionEnsureReason::PostLoadSuccess : ConditionEnsureReason::PostLoadResume, 0);
            break;
        }
        case kMessage_PreSaveGame:
            LogPerformanceSnapshot("PreSaveGame");
            if (g_logFile != kInvalidHandleValue && g_FlushFileBuffers) g_FlushFileBuffers(g_logFile);
            LogLine("F4SE message: PreSaveGame");
            // Synchronous removal is deliberate: the message is dispatched right
            // before Fallout saves, so queuing would be too late for save cleanliness.
            RemoveOwnedConditionSuppression("PreSaveGame");
            break;
        case kMessage_PostSaveGame:
            LogLine("F4SE message: PostSaveGame");
            QueueConditionEnsure(ConditionEnsureReason::PostSave, 0);
            break;
        case kMessage_NewGame:
            ResetTransientStimpakState();
            LogLine("F4SE message: NewGame (queue player-ready retry)");
            QueueConditionEnsure(ConditionEnsureReason::NewGame, 0);
            break;
        case kMessage_GameLoaded:
            LogLine("F4SE message: GameLoaded (no gameplay-setting writes; waiting for save/new-game player)");
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// Scaleform bridge
// ---------------------------------------------------------------------------
static void SetResultInt(GFxValue* value, SInt32 result)
{
    if (!value) return;
    // Scaleform initializes function results as undefined. This bridge only
    // returns a primitive int, so no managed object cleanup is required here.
    value->objectInterface = nullptr;
    value->type = GFxValue::kType_Int;
    value->pad0C = 0;
    value->data.s32 = result;
    value->unk18 = nullptr;
}

static SInt32 ReadInt(const GFxValue& value, bool& valid)
{
    valid = true;
    switch (value.GetType())
    {
        case GFxValue::kType_Int:    return value.data.s32;
        case GFxValue::kType_UInt:   return static_cast<SInt32>(value.data.u32);
        case GFxValue::kType_Number: return static_cast<SInt32>(value.data.number);
        case GFxValue::kType_Bool:   return value.data.boolean ? 1 : 0;
        default:                     valid = false; return -1;
    }
}

struct AcknowledgeLimbHandler final : GFxFunctionHandler
{
    void Invoke(Args* call) override
    {
        SInt32 result = static_cast<SInt32>(Limb::Invalid);
        if (call && call->result && call->args && call->numArgs >= 1)
        {
            bool valid = false;
            const SInt32 limb = ReadInt(call->args[0], valid);
            if (valid && IsValidLimbValue(limb))
                result = ArmTargetedStimpakRequest(static_cast<Limb>(limb)) ? limb : kTargetedRequestBusy;
        }
        SetResultInt(call ? call->result : nullptr, result);
    }
};

struct FinishTargetedStimpakHandler final : GFxFunctionHandler
{
    void Invoke(Args* call) override
    {
        SInt32 limb = static_cast<SInt32>(Limb::Invalid);
        bool valid = false;
        if (call && call->args && call->numArgs >= 1)
            limb = ReadInt(call->args[0], valid);
        if (valid && IsValidLimbValue(limb))
            FinishTargetedStimpakRequestAfterUse(static_cast<Limb>(limb));
        SetResultInt(call ? call->result : nullptr, valid ? limb : static_cast<SInt32>(Limb::Invalid));
    }
};

alignas(16) static UInt8 g_acknowledgeLimbHandlerStorage[sizeof(AcknowledgeLimbHandler)];
static AcknowledgeLimbHandler* g_acknowledgeLimbHandler = nullptr;
alignas(16) static UInt8 g_finishTargetedStimpakHandlerStorage[sizeof(FinishTargetedStimpakHandler)];
static FinishTargetedStimpakHandler* g_finishTargetedStimpakHandler = nullptr;

// CRT-free placement new for the two Scaleform handlers constructed in static storage.
void* operator new(unsigned long long, void* where) noexcept { return where; }

static constexpr UInt32 kMemCommit = 0x00001000u;
static constexpr UInt32 kPageNoAccess = 0x00000001u;
static constexpr UInt32 kPageGuard = 0x00000100u;
static constexpr UInt32 kQueryPageExecute = 0x00000010u;
static constexpr UInt32 kQueryPageExecuteRead = 0x00000020u;
static constexpr UInt32 kQueryPageExecuteReadWrite = 0x00000040u;
static constexpr UInt32 kQueryPageExecuteWriteCopy = 0x00000080u;

static bool IsReadableProcessRange(const void* address, UIntPtr bytes)
{
    if (!address || !bytes || !g_VirtualQuery) return false;
    MemoryBasicInformation mbi{};
    if (g_VirtualQuery(address, &mbi, sizeof(mbi)) != sizeof(mbi)) return false;
    if (mbi.state != kMemCommit || (mbi.protect & (kPageNoAccess | kPageGuard)) != 0) return false;

    const UIntPtr start = reinterpret_cast<UIntPtr>(address);
    const UIntPtr base = reinterpret_cast<UIntPtr>(mbi.baseAddress);
    if (start < base || mbi.regionSize == 0 || base > ~UIntPtr(0) - mbi.regionSize) return false;
    const UIntPtr regionEnd = base + mbi.regionSize;
    if (start > ~UIntPtr(0) - bytes) return false;
    return start + bytes <= regionEnd;
}

static bool IsExecutableProcessAddress(const void* address)
{
    if (!address || !g_VirtualQuery) return false;
    MemoryBasicInformation mbi{};
    if (g_VirtualQuery(address, &mbi, sizeof(mbi)) != sizeof(mbi)) return false;
    if (mbi.state != kMemCommit || (mbi.protect & kPageGuard) != 0) return false;
    const UInt32 protection = mbi.protect & 0xFFu;
    return protection == kQueryPageExecute || protection == kQueryPageExecuteRead ||
           protection == kQueryPageExecuteReadWrite || protection == kQueryPageExecuteWriteCopy;
}

static bool IsFalloutImageRange(UIntPtr address, UIntPtr bytes)
{
    if (!address || !bytes || !g_fallout4Base || !g_fallout4ImageSize) return false;
    if (g_fallout4Base > ~UIntPtr(0) - g_fallout4ImageSize) return false;
    const UIntPtr imageEnd = g_fallout4Base + g_fallout4ImageSize;
    if (address < g_fallout4Base || address > ~UIntPtr(0) - bytes) return false;
    return address + bytes <= imageEnd;
}

static bool GetScaleformMovieRoot(GFxMovieView* view, void*& movieRoot)
{
    movieRoot = nullptr;
    if (!view)
    {
        ErrorLogLine("[Scaleform] bridge FAILED: null GFxMovieView.");
        return false;
    }

    const UIntPtr viewAddress = reinterpret_cast<UIntPtr>(view);
    const UIntPtr offset = RuntimeLayout().gfxMovieViewMovieRoot;
    if (viewAddress > ~UIntPtr(0) - offset)
    {
        ErrorLogLine("[Scaleform] bridge FAILED: GFxMovieView movieRoot field overflow.");
        return false;
    }
    const UIntPtr fieldAddress = viewAddress + offset;
    if (!IsReadableProcessRange(reinterpret_cast<const void*>(fieldAddress), sizeof(void*)))
    {
        ErrorLogLine("[Scaleform] bridge FAILED: GFxMovieView movieRoot field is not readable.");
        return false;
    }

    movieRoot = *reinterpret_cast<void**>(fieldAddress);
    if (!movieRoot || !IsReadableProcessRange(movieRoot, sizeof(void*)))
    {
        ErrorLogLine("[Scaleform] bridge FAILED: movieRoot is null or unreadable.");
        movieRoot = nullptr;
        return false;
    }
    return true;
}

static bool CreateScaleformFunction(void* movieRoot, GFxValue* outValue, GFxFunctionHandler* handler)
{
    if (!movieRoot || !outValue || !handler)
    {
        ErrorLogLine("[Scaleform] CreateFunction FAILED: invalid arguments.");
        return false;
    }

    const UIntPtr vtable = *reinterpret_cast<const UIntPtr*>(movieRoot);
    const UIntPtr slot = RuntimeLayout().gfxMovieRootCreateFunctionVSlot;
    if (!vtable || slot > (~UIntPtr(0) - vtable) / sizeof(void*))
    {
        ErrorLogLine("[Scaleform] CreateFunction FAILED: invalid movieRoot vtable/slot.");
        return false;
    }
    const UIntPtr slotAddress = vtable + slot * sizeof(void*);
    if (!IsFalloutImageRange(vtable, sizeof(void*)) ||
        !IsFalloutImageRange(slotAddress, sizeof(void*)) ||
        !IsReadableProcessRange(reinterpret_cast<const void*>(slotAddress), sizeof(void*)))
    {
        ErrorLogLine("[Scaleform] CreateFunction FAILED: movieRoot vtable/slot is outside Fallout4.exe.");
        ErrorLogHex("[Scaleform] observed movieRoot vtable=", vtable);
        return false;
    }

    const UIntPtr functionAddress = *reinterpret_cast<const UIntPtr*>(slotAddress);
    if (!IsFalloutImageRange(functionAddress, 1) ||
        !IsExecutableProcessAddress(reinterpret_cast<const void*>(functionAddress)))
    {
        ErrorLogLine("[Scaleform] CreateFunction FAILED: vslot 0x30 target is not executable Fallout4.exe code.");
        ErrorLogHex("[Scaleform] observed CreateFunction target=", functionAddress);
        return false;
    }

    using CreateFunctionFn = void (*)(void*, GFxValue*, GFxFunctionHandler*, void*);
    auto fn = reinterpret_cast<CreateFunctionFn>(functionAddress);
    fn(movieRoot, outValue, handler, nullptr);
    return true;
}

static bool SetScaleformMember(GFxValue* object, const char* name, const GFxValue* value)
{
    if (!object || !object->objectInterface || !LoadEngineAddressTable() || !g_engineAddresses.gfxSetMember) return false;
    using SetMemberFn = bool (*)(GFxValue::ObjectInterface*, void*, const char*, const GFxValue*, bool);
    auto fn = reinterpret_cast<SetMemberFn>(g_engineAddresses.gfxSetMember);
    const bool isDisplayObject = object->GetType() == GFxValue::kType_DisplayObject;
    return fn(object->objectInterface, object->data.obj, name, value, isDisplayObject);
}

static void ReleaseManagedValue(GFxValue* value)
{
    if (!value || !value->IsManaged() || !value->objectInterface ||
        !LoadEngineAddressTable() || !g_engineAddresses.gfxReleaseManaged) return;
    using ReleaseManagedFn = void (*)(GFxValue::ObjectInterface*, GFxValue*, void*);
    auto fn = reinterpret_cast<ReleaseManagedFn>(g_engineAddresses.gfxReleaseManaged);
    fn(value->objectInterface, value, value->data.obj);
    value->objectInterface = nullptr;
    value->type = GFxValue::kType_Undefined;
    value->data.obj = nullptr;
    value->unk18 = nullptr;
}

static bool RegisterScaleform(GFxMovieView* view, GFxValue* pluginObject)
{
    if (!pluginObject || !IsReadableProcessRange(pluginObject, sizeof(GFxValue)))
    {
        ErrorLogLine("[Scaleform] bridge FAILED: plugin object is null or unreadable.");
        return false;
    }

    void* movieRoot = nullptr;
    if (!GetScaleformMovieRoot(view, movieRoot)) return false;

    if (!g_acknowledgeLimbHandler)
    {
        g_acknowledgeLimbHandler = new (g_acknowledgeLimbHandlerStorage) AcknowledgeLimbHandler();
    }
    if (!g_finishTargetedStimpakHandler)
    {
        g_finishTargetedStimpakHandler = new (g_finishTargetedStimpakHandlerStorage) FinishTargetedStimpakHandler();
    }

    GFxValue finishValue{};
    if (!CreateScaleformFunction(movieRoot, &finishValue, g_finishTargetedStimpakHandler)) return false;
    GFxValue functionValue{};
    if (!CreateScaleformFunction(movieRoot, &functionValue, g_acknowledgeLimbHandler))
    {
        ReleaseManagedValue(&finishValue);
        return false;
    }

    // Register Finish first. If the second SetMember fails, the plugin object is left with only a
    // harmless cleanup function and cannot arm a targeted request without its matching finish path.
    const bool okFinish = SetScaleformMember(pluginObject, "FinishTargetedStimpak", &finishValue);
    if (!okFinish) ErrorLogLine("[Scaleform] FinishTargetedStimpak member registration FAILED.");
    ReleaseManagedValue(&finishValue);
    if (!okFinish)
    {
        ReleaseManagedValue(&functionValue);
        return false;
    }

    const bool okArm = SetScaleformMember(pluginObject, "AcknowledgeLimb", &functionValue);
    if (!okArm) ErrorLogLine("[Scaleform] AcknowledgeLimb member registration FAILED.");
    ReleaseManagedValue(&functionValue);
    if (!okArm) return false;

    const bool firstValidatedBridge = !g_scaleformBridgeValidated;
    g_scaleformBridgeValidated = true;
    if (!g_scaleformBridgeSuccessLogged)
    {
        StatusLogLine("[Startup] Scaleform bridge=PASS (AcknowledgeLimb + FinishTargetedStimpak)");
        g_scaleformBridgeSuccessLogged = true;
    }

    if (firstValidatedBridge && !IsCurrentRuntimeBinaryValidated() &&
        g_stimpakBackendState == StimpakBackendState::Uninitialized)
    {
        QueueConditionEnsure(ConditionEnsureReason::ScaleformBridgeReady, 0);
    }
    return true;
}

// ---------------------------------------------------------------------------
// F4SE exports / plugin bootstrap
// ---------------------------------------------------------------------------
extern "C"
{
__declspec(dllexport) bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info)
{
    if (info)
    {
        info->infoVersion = 1;
        info->name = "PerLimbHealing";
        info->version = kPluginVersion;
    }
    if (!f4se || !info || f4se->isEditor) return false;
    return SelectRuntimeProfile(f4se->runtimeVersion) != nullptr;
}

__declspec(dllexport) F4SEPluginVersionData F4SEPlugin_Version =
{
    1,                          // dataVersion
    kPluginVersion,             // pluginVersion
    "PerLimbHealing",
    "PerLimbHealing Project",
    kAddressIndependence_AddressLibrary_1_10_980 | kAddressIndependence_AddressLibrary_1_11_137,
    kStructureIndependence_1_10_980Layout | kStructureIndependence_1_11_137Layout,
    { 0 },                      // no exact patch allowlist; runtime capability checks are authoritative
    0,                          // no minimum F4SE packed version beyond runtime compatibility
    0,                          // reservedNonBreaking
    0,                          // reservedBreaking
    { 0 }
};

__declspec(dllexport) bool F4SEPlugin_Load(const F4SEInterface* f4se)
{
    if (!ResolveWin32APIs()) return false;
    g_settings = ReadSettingsFromDisk();
    EnsureLogFileOpen();
    LogLine("F4SEPlugin_Load entered.");
    if (DebugVerboseEnabled())
    {
        LogRaw("[Settings] initial PassiveLimbRegeneration="); LogLine(OnOffText(g_settings.passiveLimbRegeneration));
        LogRaw("[Settings] initial OrdinaryStimpaksHealLimbs="); LogLine(OnOffText(g_settings.ordinaryStimpaksHealLimbs));
        LogRaw("[Settings] initial DebugLog="); LogLine(DebugLogLevelText(g_settings.debugLog));
        LogRaw("[Settings] initial PerformanceLog="); LogLine(OnOffText(g_settings.performanceLog));
    }

    if (!f4se)
    {
        ErrorLogLine("LOAD FAILED: null F4SE interface.");
        return false;
    }
    if (!g_VirtualQuery)
    {
        ErrorLogLine("LOAD FAILED: VirtualQuery unavailable; Scaleform ABI cannot be validated safely.");
        return false;
    }
    if (f4se->isEditor)
    {
        ErrorLogLine("LOAD FAILED: editor runtime is not supported.");
        return false;
    }
    g_runtimeVersion = f4se->runtimeVersion;
    g_runtimeProfile = SelectRuntimeProfile(g_runtimeVersion);
    const RuntimeVersionParts runtime = DecodeRuntimeVersion(g_runtimeVersion);
    LogInt("[Compatibility] runtime.major=", static_cast<SInt32>(runtime.major));
    LogInt("[Compatibility] runtime.minor=", static_cast<SInt32>(runtime.minor));
    LogInt("[Compatibility] runtime.patch=", static_cast<SInt32>(runtime.patch));
    LogInt("[Compatibility] runtime.build=", static_cast<SInt32>(runtime.build));
    if (!g_runtimeProfile)
    {
        ErrorLogLine("[Compatibility] no ABI-family candidate is implemented for this runtime yet.");
        return false;
    }
    LogRaw("[Compatibility] candidate ABI family=");
    LogLine(g_runtimeProfile->familyName);
    if (g_runtimeProfile->family == RuntimeFamily::OG)
    {
        LogLine("[Compatibility] OG ABI family static validation=proven on supplied 1.10.163 corpus; Dispel identity proven by function semantics + Bethesda Papyrus binding");
        LogLine(IsOGBinaryValidatedRuntime(g_runtimeVersion)
            ? "[Compatibility] exact runtime binary-validated=yes"
            : "[Compatibility] exact runtime binary-validated=no; capability checks are authoritative");
        LogLine(IsOGGameplayValidatedRuntime(g_runtimeVersion)
            ? "[Compatibility] exact runtime gameplay-validated=yes"
            : "[Compatibility] exact runtime gameplay-validated=no; capability checks are authoritative");
    }
    else if (g_runtimeProfile->family == RuntimeFamily::NG)
    {
        LogLine("[Compatibility] NG ABI family static validation=proven across supplied 1.10.980/1.10.984 relocation/vtable/RTTI corpus");
        LogLine(IsNGBinaryValidatedRuntime(g_runtimeVersion)
            ? "[Compatibility] exact runtime binary-validated=yes"
            : "[Compatibility] exact runtime binary-validated=no; capability checks are authoritative");
        LogLine(IsNGGameplayValidatedRuntime(g_runtimeVersion)
            ? "[Compatibility] exact runtime gameplay-validated=yes"
            : "[Compatibility] exact runtime gameplay-validated=no; capability checks are authoritative");
    }
    else if (g_runtimeProfile->family == RuntimeFamily::AE)
    {
        LogLine("[Compatibility] AE ABI family binary validation=proven across supplied 1.11.137/1.11.191/1.11.221/1.11.240 corpus");
        LogLine(IsAEBinaryValidatedRuntime(g_runtimeVersion)
            ? "[Compatibility] exact runtime binary-validated=yes"
            : "[Compatibility] exact runtime binary-validated=no; capability checks are authoritative");
        LogLine(IsAEGameplayValidatedRuntime(g_runtimeVersion)
            ? "[Compatibility] exact runtime gameplay-validated=yes"
            : "[Compatibility] exact runtime gameplay-validated=no; capability checks are authoritative");
    }

    if (!f4se->QueryInterface)
    {
        ErrorLogLine("LOAD FAILED: QueryInterface unavailable.");
        return false;
    }

    g_fallout4Base = GetFallout4Base();
    g_fallout4ImageSize = GetImageSize(g_fallout4Base);
    if (!g_fallout4Base || !g_fallout4ImageSize)
    {
        ErrorLogLine("LOAD FAILED: Fallout4.exe image base/size unavailable.");
        return false;
    }
    LogLine("Fallout4.exe base address and image size resolved.");

    if (!ResolveTaskAllocator())
    {
        ErrorLogLine("LOAD FAILED: task allocator unavailable (PEB ProcessHeap / ntdll RtlAllocateHeap/RtlFreeHeap).");
        return false;
    }
    LogHex("[TaskAllocator] ProcessHeap=", reinterpret_cast<UIntPtr>(g_processHeap));
    LogHex("[TaskAllocator] RtlAllocateHeap=", reinterpret_cast<UIntPtr>(g_RtlAllocateHeap));
    LogHex("[TaskAllocator] RtlFreeHeap=", reinterpret_cast<UIntPtr>(g_RtlFreeHeap));

    if (!f4se->GetPluginHandle)
    {
        ErrorLogLine("LOAD FAILED: GetPluginHandle unavailable.");
        return false;
    }
    const UInt32 pluginHandle = f4se->GetPluginHandle();
    LogInt("Plugin handle=", static_cast<SInt32>(pluginHandle));

    auto* messaging = reinterpret_cast<F4SEMessagingInterface*>(f4se->QueryInterface(kInterface_Messaging));
    if (!messaging || messaging->interfaceVersion < 1 || !messaging->RegisterListener)
    {
        ErrorLogLine("LOAD FAILED: Messaging interface unavailable/too old.");
        return false;
    }
    LogInt("Messaging interface version=", static_cast<SInt32>(messaging->interfaceVersion));
    const bool listening = messaging->RegisterListener(pluginHandle, "F4SE", &OnF4SEMessage);
    if (listening) LogLine("F4SE messaging listener registered.");
    else ErrorLogLine("LOAD FAILED: F4SE messaging listener registration rejected.");
    if (!listening) return false;

    g_taskInterface = reinterpret_cast<F4SETaskInterface*>(f4se->QueryInterface(kInterface_Task));
    if (!g_taskInterface || g_taskInterface->interfaceVersion < 2 || !g_taskInterface->AddTask)
    {
        ErrorLogLine("LOAD FAILED: F4SE task interface unavailable/too old.");
        return false;
    }
    LogInt("Task interface version=", static_cast<SInt32>(g_taskInterface->interfaceVersion));

    const bool addressScanCompleted = LoadEngineAddressTable();
    LogOperationalCompatibilitySummary(f4se);
    if (!addressScanCompleted || !g_engineAddresses.playerSingleton || !g_engineAddresses.formGetByNumericID)
    {
        ErrorLogLine("[Compatibility] PassiveGate address capability=FAIL");
        ErrorLogLine("LOAD FAILED: required ConditionRateMult engine references could not be resolved.");
        return false;
    }
    LogLine("[Compatibility] PassiveGate address capability=PASS");
    if (!ScaleformAddressCapabilityReady())
    {
        ErrorLogLine("[Compatibility] Scaleform address capability=FAIL");
        ErrorLogLine("LOAD FAILED: required Scaleform engine identities could not be resolved.");
        return false;
    }
    const bool stimpakHelperContractsOK = ValidateStimpakHelperVTableContracts(true);
    LogLine(stimpakHelperContractsOK
        ? "[Compatibility] Stimpak virtual helper validation=PASS"
        : "[Compatibility] Stimpak virtual helper validation=FAIL");
    StatusLogLine(stimpakHelperContractsOK
        ? "[Startup] Stimpak virtual-contract capability=PASS"
        : "[Startup] Stimpak virtual-contract capability=FAIL");
    if (!StimpakAddressCapabilityReady())
        ErrorLogLine("[Production] Stimpak backend DISABLED: required Stimpak Address Library identities are incomplete; passive gate remains active.");
    else if (!stimpakHelperContractsOK)
        ErrorLogLine("[Production] Stimpak backend DISABLED: virtual helper contract validation failed; passive gate remains active.");
    else
        LogLine("[Production] engine address/form/helper references available; Stimpak hooks will install when the player is ready.");
    LogLine("Runtime policy: healing hot paths contain no settings-file or log I/O.");

    auto* scaleform = reinterpret_cast<F4SEScaleformInterface*>(f4se->QueryInterface(kInterface_Scaleform));
    if (!scaleform)
    {
        ErrorLogLine("LOAD FAILED: Scaleform interface unavailable.");
        return false;
    }
    LogInt("Scaleform interface version=", static_cast<SInt32>(scaleform->interfaceVersion));
    if (scaleform->interfaceVersion < 1 || !scaleform->Register)
    {
        ErrorLogLine("LOAD FAILED: Scaleform interface too old or Register unavailable.");
        return false;
    }

    const bool registered = scaleform->Register("PerLimbHealing", &RegisterScaleform);
    if (!registered)
    {
        ErrorLogLine("LOAD FAILED: F4SE rejected Scaleform plugin registration.");
        return false;
    }
    g_scaleformRegistrationAccepted = true;
    LogLine("Scaleform plugin registration accepted by F4SE.");
    StatusLogLine("[Startup] Scaleform F4SE registration=PASS");
    if (!IsCurrentRuntimeBinaryValidated())
        StatusLogLine("[Startup] Stimpak activation policy=DEFER until first hardened Scaleform bridge succeeds on this unqualified runtime");
    else
        StatusLogLine("[Startup] Stimpak activation policy=ALLOW from qualified runtime evidence + runtime capability checks");

    // Live MCM synchronization is optional. Start its process-lifetime worker only after every
    // mandatory initialization step has committed successfully, so failed loads leave no watcher behind.
    StartSettingsWatcher();
    return true;
}
}
