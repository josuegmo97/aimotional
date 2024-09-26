#include "flutter_window.h"
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>
#include <windows.h>
#include <dshow.h>
#include <memory>
#include <string>
#include <vector>
#include "flutter/generated_plugin_registrant.h"
#pragma comment(lib, "Strmiids.lib")



// Variables para la captura de video
IMediaControl* mediaControl = nullptr;
IGraphBuilder* graphBuilder = nullptr;
ICaptureGraphBuilder2* captureGraphBuilder = nullptr;
IBaseFilter* videoCaptureFilter = nullptr;
IMediaEvent* mediaEvent = nullptr;
bool isRecording = false;

// Helper para convertir std::string a std::wstring
std::wstring stringToWideString(const std::string& str) {
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
  std::wstring wstr(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
  return wstr;
}

// Función auxiliar para convertir std::wstring a std::string
std::string WideStringToString(const std::wstring& wstr) {
  if (wstr.empty()) {
    return std::string();
  }
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
  std::string str(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
  return str;
}

// Método para inicializar la captura de video
void InitializeVideoCapture() {
  CoInitialize(nullptr);
  CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&graphBuilder);
  CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&captureGraphBuilder);
  captureGraphBuilder->SetFiltergraph(graphBuilder);
  CoCreateInstance(CLSID_VideoInputDeviceCategory, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&videoCaptureFilter);
  graphBuilder->AddFilter(videoCaptureFilter, L"Video Capture");
  graphBuilder->QueryInterface(IID_IMediaControl, (void**)&mediaControl);
  graphBuilder->QueryInterface(IID_IMediaEvent, (void**)&mediaEvent);
}

// Método para empezar la grabación de video
void StartRecording(const std::string& filePath) {
  if (!isRecording) {
    mediaControl->Run();
    isRecording = true;
  }
}

// Método para detener la grabación de video
void StopRecording() {
  if (isRecording) {
    mediaControl->Stop();
    isRecording = false;
  }
}


// Función para obtener la lista de cámaras conectadas
std::vector<std::string> GetAvailableCameras() {
  std::vector<std::string> cameras;

  // Inicializamos COM
  CoInitialize(nullptr);

  // Creamos enumerador de dispositivos
  ICreateDevEnum* pDevEnum = nullptr;
  IEnumMoniker* pEnum = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);
  if (SUCCEEDED(hr)) {
    // Enumeramos los dispositivos de video
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
    if (hr == S_OK) {
      IMoniker* pMoniker = nullptr;
      while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
        IPropertyBag* pPropBag;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
        if (SUCCEEDED(hr)) {
          VARIANT varName;
          VariantInit(&varName);
          // Obtenemos el nombre del dispositivo
          hr = pPropBag->Read(L"FriendlyName", &varName, 0);
          if (SUCCEEDED(hr)) {
            // Convertimos el nombre a std::string usando WideCharToMultiByte
            std::wstring ws(varName.bstrVal, SysStringLen(varName.bstrVal));
            std::string cameraName = WideStringToString(ws); // Usar la función correcta
            cameras.push_back(cameraName);
          }
          VariantClear(&varName);
          pPropBag->Release();
        }
        pMoniker->Release();
      }
      pEnum->Release();
    }
    pDevEnum->Release();
  }
  
  // Liberamos COM
  CoUninitialize();

  return cameras;
}


FlutterWindow::FlutterWindow(const flutter::DartProject& project)
    : project_(project) {}

FlutterWindow::~FlutterWindow() {}

bool FlutterWindow::OnCreate() {
  if (!Win32Window::OnCreate()) {
    return false;
  }

  RECT frame = GetClientArea();

  flutter_controller_ = std::make_unique<flutter::FlutterViewController>(
      frame.right - frame.left, frame.bottom - frame.top, project_);

  if (!flutter_controller_->engine() || !flutter_controller_->view()) {
    return false;
  }
  RegisterPlugins(flutter_controller_->engine());
  SetChildContent(flutter_controller_->view()->GetNativeWindow());

  flutter_controller_->engine()->SetNextFrameCallback([&]() {
    this->Show();
  });

  flutter_controller_->ForceRedraw();

  InitializeVideoCapture();

  auto camera_channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
      flutter_controller_->engine()->messenger(), "camera_channel",
      &flutter::StandardMethodCodec::GetInstance());

  camera_channel->SetMethodCallHandler(
      [](const flutter::MethodCall<flutter::EncodableValue>& call,
         std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
          if (call.method_name().compare("getCameras") == 0) {
            // Obtenemos la lista de cámaras disponibles
          std::vector<std::string> availableCameras = GetAvailableCameras();

          // Convertimos la lista de std::string a flutter::EncodableValue
          std::vector<flutter::EncodableValue> cameras;
          for (const std::string& camera : availableCameras) {
            cameras.push_back(flutter::EncodableValue(camera));
          }

          // Devolvemos la lista de cámaras
          result->Success(flutter::EncodableValue(cameras));

        } else if (call.method_name().compare("startRecording") == 0) {
          const auto* arguments = std::get_if<flutter::EncodableMap>(call.arguments());
          std::string filePath = std::get<std::string>(arguments->at(flutter::EncodableValue("path")));
          StartRecording(filePath);
          result->Success();
        } else if (call.method_name().compare("stopRecording") == 0) {
          StopRecording();
          result->Success();
        } else {
          result->NotImplemented();
        }
      });

  return true;
}



void FlutterWindow::OnDestroy() {
  if (flutter_controller_) {
    flutter_controller_ = nullptr;
  }

  if (mediaControl) {
    mediaControl->Release();
  }
  if (graphBuilder) {
    graphBuilder->Release();
  }
  if (captureGraphBuilder) {
    captureGraphBuilder->Release();
  }
  if (videoCaptureFilter) {
    videoCaptureFilter->Release();
  }
  CoUninitialize();

  Win32Window::OnDestroy();
}

LRESULT FlutterWindow::MessageHandler(HWND hwnd, UINT const message,
                                      WPARAM const wparam,
                                      LPARAM const lparam) noexcept {
  if (flutter_controller_) {
    std::optional<LRESULT> result =
        flutter_controller_->HandleTopLevelWindowProc(hwnd, message, wparam, lparam);
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

