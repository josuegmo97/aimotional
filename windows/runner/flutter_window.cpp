#include "flutter_window.h"

#include <optional>

#include "flutter/generated_plugin_registrant.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>

FlutterWindow::FlutterWindow(const flutter::DartProject& project)
    : project_(project) {}

FlutterWindow::~FlutterWindow() {}

bool FlutterWindow::OnCreate() {
  if (!Win32Window::OnCreate()) {
    return false;
  }

  RECT frame = GetClientArea();

  // The size here must match the window dimensions to avoid unnecessary surface
  // creation / destruction in the startup path.
  flutter_controller_ = std::make_unique<flutter::FlutterViewController>(
      frame.right - frame.left, frame.bottom - frame.top, project_);
  // Ensure that basic setup of the controller was successful.
  if (!flutter_controller_->engine() || !flutter_controller_->view()) {
    return false;
  }

  // Registrar el canal de cámara
  RegisterCameraMethodChannel(flutter_controller_->engine());
  RegisterPlugins(flutter_controller_->engine());
  SetChildContent(flutter_controller_->view()->GetNativeWindow());

  flutter_controller_->engine()->SetNextFrameCallback([&]() {
    this->Show();
  });

  return true;
}

void FlutterWindow::OnDestroy() {
  if (flutter_controller_) {
    flutter_controller_ = nullptr;
  }

  Win32Window::OnDestroy();
}

LRESULT
FlutterWindow::MessageHandler(HWND hwnd, UINT const message,
                              WPARAM const wparam,
                              LPARAM const lparam) noexcept {
  // Give Flutter, including plugins, an opportunity to handle window messages.
  if (flutter_controller_) {
    std::optional<LRESULT> result =
        flutter_controller_->HandleTopLevelWindowProc(hwnd, message, wparam,
                                                      lparam);
    if (result) {
      return *result;
    }
  }

  switch (message) {
    case WM_FONTCHANGE:
      flutter_controller_->engine()->ReloadSystemFonts();
      break;
  }

  return Win32Window::MessageHandler(hwnd, message, wparam, lparam);
}

void ListVideoDevices(std::vector<std::wstring>& devices) {
  IMFAttributes* pAttributes = NULL;
  IMFActivate** ppDevices = NULL;

  // Initialize Media Foundation.
  MFStartup(MF_VERSION);

  // Create an attribute store to specify enumeration parameters.
  MFCreateAttributes(&pAttributes, 1);
  pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

  UINT32 count = 0;
  MFEnumDeviceSources(pAttributes, &ppDevices, &count);

  for (UINT32 i = 0; i < count; i++) {
    WCHAR* name;
    ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, NULL);
    devices.push_back(std::wstring(name));
    CoTaskMemFree(name);
    ppDevices[i]->Release();
  }

  pAttributes->Release();
  MFShutdown();
}

void RegisterCameraMethodChannel(FlutterEngine* engine) {
  auto camera_channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
    engine->messenger(), "camera_channel",
    &flutter::StandardMethodCodec::GetInstance());

  camera_channel->SetMethodCallHandler(
    [](const flutter::MethodCall<flutter::EncodableValue>& call,
       std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
      if (call.method_name().compare("getCameras") == 0) {
        std::vector<std::wstring> device_names;
        ListVideoDevices(device_names);

        flutter::EncodableList devices;
        for (const auto& name : device_names) {
          devices.push_back(flutter::EncodableValue(name));
        }

        result->Success(devices);
      } else {
        result->NotImplemented();
      }
    });
}