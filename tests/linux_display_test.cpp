// Exercise the built driver without SteamVR, a display or an Arduino.
#include <openvr_driver.h>
#include <dlfcn.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

using namespace vr;

static void check(bool condition, const char *message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

class Settings : public IVRSettings
{
public:
    bool required = false;
    bool missing = false;
    std::map<std::string, int32_t> ints = {
        {"WindowX", 1920}, {"WindowY", 0}, {"windowWidth", 1920},
        {"windowHeight", 1080}, {"RenderWidth", 960}, {"RenderHeight", 1080}
    };
    std::map<std::string, float> floats = {
        {"IPD", 0.065f}, {"displayFrequency", 60}, {"FOV", 57.296f},
        {"ZoomWidth", 1}, {"ZoomHeight", 1}
    };
    const char *GetSettingsErrorNameFromEnum(EVRSettingsError) override { return "test"; }
    void SetBool(const char *, const char *, bool, EVRSettingsError *) override {}
    void SetInt32(const char *, const char *, int32_t, EVRSettingsError *) override {}
    void SetFloat(const char *, const char *, float, EVRSettingsError *) override {}
    void SetString(const char *, const char *, const char *, EVRSettingsError *) override {}
    bool GetBool(const char *section, const char *key, EVRSettingsError *error) override
    {
        if (error) *error = VRSettingsError_None;
        if (std::strcmp(key, "ArduinoRequire") == 0) {
            check(std::strcmp(section, "ArduinoHMD") == 0, "tracker section");
            if (missing && error) *error = VRSettingsError_UnsetSettingHasNoDefault;
            return required;
        }
        return std::strcmp(key, "Stereo") == 0;
    }
    int32_t GetInt32(const char *, const char *key, EVRSettingsError *) override
    {
        check(std::strcmp(key, "ArduinoRequire") != 0, "read boolean with GetBool");
        return ints[key];
    }
    float GetFloat(const char *, const char *key, EVRSettingsError *) override { return floats[key]; }
    void GetString(const char *, const char *, char *out, uint32_t size, EVRSettingsError *) override
    {
        std::snprintf(out, size, "test-hmd");
    }
    void RemoveSection(const char *, EVRSettingsError *) override {}
    void RemoveKeyInSection(const char *, const char *, EVRSettingsError *) override {}
};

class Properties : public IVRProperties
{
public:
    bool neverTracked = false;
    ETrackedPropertyError ReadPropertyBatch(PropertyContainerHandle_t, PropertyRead_t *, uint32_t) override
    { return TrackedProp_UnknownProperty; }
    ETrackedPropertyError WritePropertyBatch(PropertyContainerHandle_t, PropertyWrite_t *items, uint32_t count) override
    {
        for (uint32_t i = 0; i < count; ++i) {
            if (items[i].prop == Prop_NeverTracked_Bool)
                neverTracked = *static_cast<bool *>(items[i].pvBuffer);
            items[i].eError = TrackedProp_Success;
        }
        return TrackedProp_Success;
    }
    const char *GetPropErrorNameFromEnum(ETrackedPropertyError) override { return "test"; }
    PropertyContainerHandle_t TrackedDeviceToPropertyContainer(TrackedDeviceIndex_t) override { return 1; }
};

class Host : public IVRServerDriverHost
{
public:
    ITrackedDeviceServerDriver *device = nullptr;
    unsigned poses = 0;
    bool TrackedDeviceAdded(const char *, ETrackedDeviceClass type, ITrackedDeviceServerDriver *driver) override
    {
        check(type == TrackedDeviceClass_HMD && !device, "single HMD registration");
        device = driver;
        return true;
    }
    void TrackedDevicePoseUpdated(uint32_t id, const DriverPose_t &pose, uint32_t size) override
    {
        check(id == 0 && size == sizeof(DriverPose_t), "pose callback ABI");
        check(pose.poseIsValid && pose.deviceIsConnected, "connected fixed pose");
        check(pose.qRotation.w == 1 && pose.qRotation.x == 0 && pose.qRotation.y == 0 && pose.qRotation.z == 0, "identity orientation");
        check(pose.vecPosition[0] == 0 && pose.vecPosition[1] == 0 && pose.vecPosition[2] == 0, "fixed position");
        check(!pose.shouldApplyHeadModel, "no inferred head motion");
        ++poses;
    }
    void VsyncEvent(double) override {}
    void VendorSpecificEvent(uint32_t, EVREventType, const VREvent_Data_t &, double) override {}
    bool IsExiting() override { return false; }
    bool PollNextEvent(VREvent_t *, uint32_t) override { return false; }
    void GetRawTrackedDevicePoses(float, TrackedDevicePose_t *, uint32_t) override {}
    void RequestRestart(const char *, const char *, const char *, const char *) override {}
    uint32_t GetFrameTimings(Compositor_FrameTiming *, uint32_t) override { return 0; }
    void SetDisplayEyeToHead(uint32_t, const HmdMatrix34_t &, const HmdMatrix34_t &) override {}
    void SetDisplayProjectionRaw(uint32_t, const HmdRect2_t &, const HmdRect2_t &) override {}
    void SetRecommendedRenderTargetSize(uint32_t, uint32_t, uint32_t) override {}
};

class TestLog : public IVRDriverLog
{
public:
    std::string last;
    void Log(const char *message) override { last = message; }
};

class Resources : public IVRResources
{
public:
    uint32_t LoadSharedResource(const char *, char *, uint32_t) override { return 0; }
    uint32_t GetResourceFullPath(const char *, const char *, char *, uint32_t) override { return 0; }
};

class Manager : public IVRDriverManager
{
public:
    uint32_t GetDriverCount() const override { return 1; }
    uint32_t GetDriverName(DriverId_t, char *, uint32_t) override { return 0; }
    DriverHandle_t GetDriverHandle(const char *) override { return 1; }
    bool IsEnabled(DriverId_t) const override { return true; }
};

class Context : public IVRDriverContext
{
public:
    Settings settings;
    Properties properties;
    Host host;
    TestLog log;
    Resources resources;
    Manager manager;
    void *GetGenericInterface(const char *name, EVRInitError *error) override
    {
        if (error) *error = VRInitError_None;
        if (!std::strcmp(name, IVRSettings_Version)) return &settings;
        if (!std::strcmp(name, IVRProperties_Version)) return &properties;
        if (!std::strcmp(name, IVRServerDriverHost_Version)) return &host;
        if (!std::strcmp(name, IVRDriverLog_Version)) return &log;
        if (!std::strcmp(name, IVRResources_Version)) return &resources;
        if (!std::strcmp(name, IVRDriverManager_Version)) return &manager;
        if (error) *error = VRInitError_Init_InterfaceNotFound;
        return nullptr;
    }
    DriverHandle_t GetDriverHandle() override { return 1; }
};

int main(int argc, char **argv)
{
    check(argc == 2, "supply built module path");
    void *module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    check(module != nullptr, "dlopen with all symbols resolved");
    auto factory = reinterpret_cast<void *(*)(const char *, int *)>(dlsym(module, "HmdDriverFactory"));
    check(factory != nullptr, "exported factory");
    int error = 0;
    check(factory("unsupported-test-interface", &error) == nullptr && error == VRInitError_Init_InterfaceNotFound, "unknown interface rejected");
    auto provider = static_cast<IServerTrackedDeviceProvider *>(factory(IServerTrackedDeviceProvider_Version, &error));
    check(provider != nullptr, "provider factory");

    Context context;
    context.settings.required = true;
    check(provider->Init(&context) == VRInitError_Driver_Failed && !context.host.device, "required tracker rejected");
    check(context.log.last.find("ArduinoRequire") != std::string::npos, "actionable tracker error");
    context.settings.required = false;
    context.settings.missing = true;
    check(provider->Init(&context) == VRInitError_Driver_Failed && !context.host.device, "missing tracker setting rejected");
    context.settings.missing = false;

    for (int cycle = 0; cycle < 3; ++cycle) {
        // Test identity optics, then a nonzero radial profile and viewport offset.
        context.settings.floats["DistortionK1"] = cycle ? 0.5f : 0;
        context.settings.floats["DistortionK2"] = cycle ? 0.25f : 0;
        context.settings.ints["DistanceBetweenEyes"] = cycle ? 8 : 0;
        check(provider->Init(&context) == VRInitError_None, "display-only initialization");
        auto device = context.host.device;
        check(device && device->Activate(0) == VRInitError_None, "device activation");
        check(context.properties.neverTracked, "untracked capability");
        auto display = static_cast<IVRDisplayComponent *>(device->GetComponent(IVRDisplayComponent_Version));
        check(display != nullptr && !device->GetComponent("unsupported"), "display component negotiation");
        int32_t wx, wy;
        uint32_t width, height;
        display->GetWindowBounds(&wx, &wy, &width, &height);
        check(wx == 1920 && wy == 0 && width == 1920 && height == 1080, "extended panel bounds");
        display->GetRecommendedRenderTargetSize(&width, &height);
        check(width == 960 && height == 1080, "per-eye render resolution");
        for (EVREye eye : {Eye_Left, Eye_Right}) {
            uint32_t x, y;
            display->GetEyeOutputViewport(eye, &x, &y, &width, &height);
            const uint32_t offset = cycle ? 8 : 0;
            check(x == (eye == Eye_Left ? offset : 960 - offset) && y == 0 && width == 960 && height == 1080, "eye viewport order/offset");
            for (float u : {0.0f, 0.25f, 0.5f, 1.0f}) {
                for (float v : {0.0f, 0.5f, 0.75f, 1.0f}) {
                    const float dx = u - 0.5f, dy = v - 0.5f;
                    const float radiusSquared = dx * dx + dy * dy;
                    const float scale = cycle ? 1 + 0.5f * radiusSquared + 0.25f * radiusSquared * radiusSquared : 1;
                    const auto uv = display->ComputeDistortion(eye, u, v);
                    for (const float *channel : {uv.rfRed, uv.rfGreen, uv.rfBlue}) {
                        check(std::abs(channel[0] - (0.5f + dx * scale)) < 1e-6f &&
                              std::abs(channel[1] - (0.5f + dy * scale)) < 1e-6f, "radial distortion analytic oracle");
                    }
                }
            }
        }
        const unsigned before = context.host.poses;
        for (int i = 0; i < 100; ++i) provider->RunFrame();
        check(context.host.poses == before + 100, "fixed-pose updates continue");
        device->Deactivate();
        provider->RunFrame();
        check(context.host.poses == before + 100, "deactivation stops pose callbacks");
        provider->Cleanup();
        context.host.device = nullptr;
        provider->RunFrame();
        check(context.host.poses == before + 100, "cleanup stops callbacks");
    }
    dlclose(module);
    std::puts("PASS: Linux display-only module, refusal paths, geometry, optics, fixed pose and lifecycle");
}
