#ifndef FLUTTER_PLUGIN_CAMERA_PLUGIN_WINDOWS_H_
#define FLUTTER_PLUGIN_CAMERA_PLUGIN_WINDOWS_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <memory>
#include <string>

#include <mfapi.h>          // Media Foundation API
#include <mfidl.h>          // Media Foundation Interfaces
#include <mfobjects.h>      // Media Foundation Objects
#include <mfreadwrite.h>    // Media Foundation Read/Write

class CameraPluginWindows {
public:
    CameraPluginWindows();
    ~CameraPluginWindows();

    static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

private:
    void GetAvailableCameras(flutter::MethodCall<flutter::EncodableValue> &call, std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
    void StartRecording(const std::wstring& file_path, const std::wstring& camera_name, std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
    void StopRecording(std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

    // Variables para gestionar la grabación
    IMFMediaSession* capture_session_;
    IMFMediaSource* media_source_;
    IMFMediaSink* media_sink_;
};

#endif  // FLUTTER_PLUGIN_CAMERA_PLUGIN_WINDOWS_H_
