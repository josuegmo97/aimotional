#include <iostream>
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



// Variables globales para la captura de video
IMediaControl* mediaControl = nullptr;
IGraphBuilder* graphBuilder = nullptr;
ICaptureGraphBuilder2* captureGraphBuilder = nullptr;
IBaseFilter* videoCaptureFilter = nullptr;
IBaseFilter* muxFilter = nullptr; // <- Declarada correctamente
IBaseFilter* fileWriterFilter = nullptr; // <- Declarada correctamente
IBaseFilter* compressorFilter = nullptr; // Filtro de compresión de video
IMediaEvent* mediaEvent = nullptr;
IFileSinkFilter* fileSink = nullptr;
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

// Función para enumerar y seleccionar la cámara correcta por su nombre
// Función para seleccionar la cámara por nombre
IBaseFilter* GetCameraByName(const std::string& cameraName) {
    ICreateDevEnum* pDevEnum = nullptr;
    IEnumMoniker* pEnum = nullptr;
    IBaseFilter* selectedCamera = nullptr;
    HRESULT hr;

    // Crear enumerador de dispositivos
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);
    if (FAILED(hr)) {
        std::cerr << "Error al crear enumerador de dispositivos: " << hr << std::endl;
        return nullptr;
    }

    // Enumerar dispositivos de video
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
    if (hr == S_OK) {
        IMoniker* pMoniker = nullptr;
        while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
            IPropertyBag* pPropBag;
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
            if (SUCCEEDED(hr)) {
                VARIANT varName;
                VariantInit(&varName);

                // Obtener el nombre de la cámara
                hr = pPropBag->Read(L"FriendlyName", &varName, 0);
                if (SUCCEEDED(hr)) {
                    std::wstring ws(varName.bstrVal, SysStringLen(varName.bstrVal));

                    // Convertir std::wstring a std::string
                    std::string deviceName = WideStringToString(ws);

                    std::cout << "Dispositivo encontrado: " << deviceName << std::endl;

                    // Verificar si es el dispositivo que estamos buscando
                    if (deviceName == cameraName) {
                        // Crear el filtro para esta cámara
                        hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&selectedCamera);
                        if (FAILED(hr)) {
                            std::cerr << "Error al crear filtro para la cámara: " << hr << std::endl;
                        } else {
                            std::cout << "Cámara seleccionada: " << deviceName << std::endl;
                        }
                    }
                }
                VariantClear(&varName);
                pPropBag->Release();
            }
            pMoniker->Release();

            if (selectedCamera != nullptr) {
                break; // Si ya encontramos la cámara, no necesitamos seguir buscando
            }
        }
        pEnum->Release();
    }
    pDevEnum->Release();
    return selectedCamera;
}

// Método para inicializar la captura de video
void InitializeVideoCapture() {
   std::cout << "Inicializando captura de video" << std::endl;
    CoInitialize(nullptr);
    CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&graphBuilder);
    CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&captureGraphBuilder);
    captureGraphBuilder->SetFiltergraph(graphBuilder);
    CoCreateInstance(CLSID_VideoInputDeviceCategory, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&videoCaptureFilter);
    graphBuilder->AddFilter(videoCaptureFilter, L"Video Capture");

    // Crear y agregar el filtro de compresión de video (opcional)
    CoCreateInstance(CLSID_VideoCompressorCategory, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&compressorFilter);
    graphBuilder->AddFilter(compressorFilter, L"Video Compressor");

    // Crear y agregar el filtro de multiplexación (muxFilter)
    CoCreateInstance(CLSID_AviDest, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&muxFilter);
    graphBuilder->AddFilter(muxFilter, L"AVI Mux");

    // Crear y agregar el filtro de escritura a archivo (fileWriterFilter)
    CoCreateInstance(CLSID_FileWriter, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&fileWriterFilter);
    graphBuilder->AddFilter(fileWriterFilter, L"File Writer");

    // Obtenemos la interfaz IFileSinkFilter para establecer el archivo de salida
    fileWriterFilter->QueryInterface(IID_IFileSinkFilter, (void**)&fileSink);

    // Obtenemos el control del flujo
    graphBuilder->QueryInterface(IID_IMediaControl, (void**)&mediaControl);
    graphBuilder->QueryInterface(IID_IMediaEvent, (void**)&mediaEvent);
}
// Método para empezar la grabación de video, recibe el nombre de la cámara y el filePath
void StartRecording(const std::string& filePath, const std::string& cameraName) {
    std::cout << "Iniciando grabación en: " << filePath << " con la cámara: " << cameraName << std::endl;

    if (!isRecording) {
        // Inicializar el grafo de captura si no se ha inicializado
        CoInitialize(nullptr);

        HRESULT hr;

        // Crear el grafo de filtros si no existe
        if (!graphBuilder) {
            hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&graphBuilder);
            if (FAILED(hr)) {
                std::cerr << "Error al crear FilterGraph: " << hr << std::endl;
                return;
            }
        }

        // Crear el capturador de gráficos si no existe
        if (!captureGraphBuilder) {
            hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&captureGraphBuilder);
            if (FAILED(hr)) {
                std::cerr << "Error al crear CaptureGraphBuilder2: " << hr << std::endl;
                return;
            }
            captureGraphBuilder->SetFiltergraph(graphBuilder);
        }

        // Seleccionar la cámara por su nombre
        videoCaptureFilter = GetCameraByName(cameraName);
        if (!videoCaptureFilter) {
            std::cerr << "Error: No se pudo encontrar o seleccionar la cámara: " << cameraName << std::endl;
            return;
        }

        // Agregar el filtro de captura de video al grafo
        hr = graphBuilder->AddFilter(videoCaptureFilter, L"Video Capture");
        if (FAILED(hr)) {
            std::cerr << "Error al agregar filtro de captura de video al grafo: " << hr << std::endl;
            return;
        }

        // Crear y agregar el filtro de multiplexación (muxFilter)
        if (!muxFilter) {
            hr = CoCreateInstance(CLSID_AviDest, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&muxFilter);
            if (FAILED(hr)) {
                std::cerr << "Error al crear AVI Mux: " << hr << std::endl;
                return;
            }
            graphBuilder->AddFilter(muxFilter, L"AVI Mux");
        }

        // Crear y agregar el filtro de escritura a archivo (fileWriterFilter)
        if (!fileWriterFilter) {
            hr = CoCreateInstance(CLSID_FileWriter, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&fileWriterFilter);
            if (FAILED(hr)) {
                std::cerr << "Error al crear File Writer: " << hr << std::endl;
                return;
            }
            graphBuilder->AddFilter(fileWriterFilter, L"File Writer");

            // Obtener la interfaz IFileSinkFilter para establecer el archivo de salida
            fileWriterFilter->QueryInterface(IID_IFileSinkFilter, (void**)&fileSink);
        }

        // Establecer el archivo de destino
        std::wstring wideFilePath = stringToWideString(filePath);
        hr = fileSink->SetFileName(wideFilePath.c_str(), nullptr);
        if (FAILED(hr)) {
            std::cerr << "Error al establecer el archivo de destino: " << hr << std::endl;
            return;
        }

        // Conectar los filtros: video capture -> AVI Mux -> File Writer
        hr = captureGraphBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, videoCaptureFilter, nullptr, muxFilter);
        if (FAILED(hr)) {
            std::cerr << "Error al conectar captura de video con AVI Mux: " << hr << std::endl;
            return;
        }

        hr = captureGraphBuilder->RenderStream(nullptr, nullptr, muxFilter, nullptr, fileWriterFilter);
        if (FAILED(hr)) {
            std::cerr << "Error al conectar AVI Mux con el File Writer: " << hr << std::endl;
            return;
        }

        // Comenzar la grabación
        mediaControl->Run();
        isRecording = true;
        std::cout << "Grabación iniciada con la cámara: " << cameraName << std::endl;
    } else {
        std::cout << "Grabación ya estaba en curso." << std::endl;
    }
}
// Método para detener la grabación de video
void StopRecording() {
    std::cout << "Deteniendo grabación" << std::endl;
    if (isRecording) {
        mediaControl->Stop();
        isRecording = false;
        std::cout << "Grabación detenida." << std::endl;
    } else {
        std::cout << "No había grabación en curso." << std::endl;
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
          std::string cameraName = std::get<std::string>(arguments->at(flutter::EncodableValue("camera")));
          StartRecording(filePath, cameraName);
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

