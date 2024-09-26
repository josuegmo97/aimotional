#include "camera_plugin_windows.h"

#include <mfapi.h>
#include <mfobjects.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfplay.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")

CameraPluginWindows::CameraPluginWindows() : capture_session_(nullptr), media_source_(nullptr), media_sink_(nullptr) {}

CameraPluginWindows::~CameraPluginWindows() {
    StopRecording(nullptr);
}

void CameraPluginWindows::RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar) {
    auto method_channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
        registrar->messenger(), "camera_channel", &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<CameraPluginWindows>();

    method_channel->SetMethodCallHandler([plugin_pointer = plugin.get()](const auto& call, auto result) {
        if (call.method_name().compare("getCameras") == 0) {
            plugin_pointer->GetAvailableCameras(const_cast<flutter::MethodCall<flutter::EncodableValue>&>(call), std::move(result));
        } else if (call.method_name().compare("startRecording") == 0) {
            const auto* arguments = std::get_if<flutter::EncodableMap>(call.arguments());
            auto path_it = arguments->find(flutter::EncodableValue("path"));
            auto camera_it = arguments->find(flutter::EncodableValue("camera"));

            if (path_it != arguments->end() && camera_it != arguments->end()) {
                std::string file_path = std::get<std::string>(path_it->second);
                std::string camera_name = std::get<std::string>(camera_it->second);

                plugin_pointer->StartRecording(std::wstring(file_path.begin(), file_path.end()), std::wstring(camera_name.begin(), camera_name.end()), std::move(result));
            } else {
                result->Error("INVALID_ARGUMENTS", "Invalid arguments for startRecording");
            }
        } else if (call.method_name().compare("stopRecording") == 0) {
            plugin_pointer->StopRecording(std::move(result));
        } else {
            result->NotImplemented();
        }
    });

    registrar->AddPlugin(std::move(plugin));
}

void CameraPluginWindows::GetAvailableCameras(flutter::MethodCall<flutter::EncodableValue> &call, std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
    // Lógica para obtener cámaras usando Media Foundation
    // ...
}

void CameraPluginWindows::StartRecording(const std::wstring& file_path, const std::wstring& camera_name, std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
    // Lógica para iniciar grabación usando Media Foundation
    // ...
    result->Success();
}

void CameraPluginWindows::StopRecording(std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
    // Lógica para detener grabación usando Media Foundation
    // ...
    result->Success();
}
