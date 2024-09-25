import Cocoa
import FlutterMacOS
import AVFoundation

@NSApplicationMain
class AppDelegate: FlutterAppDelegate, AVCaptureFileOutputRecordingDelegate {

  var captureSession: AVCaptureSession?
  var output: AVCaptureMovieFileOutput?

  override func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
    return true
  }

  override func applicationDidFinishLaunching(_ aNotification: Notification) {
    let controller: FlutterViewController = mainFlutterWindow?.contentViewController as! FlutterViewController
    let cameraChannel = FlutterMethodChannel(name: "camera_channel", binaryMessenger: controller.engine.binaryMessenger)
    
    cameraChannel.setMethodCallHandler { (call: FlutterMethodCall, result: @escaping FlutterResult) -> Void in
      if call.method == "getCameras" {
        self.requestCameraPermission { granted in
          if granted {
            let cameras = self.getVideoDevices()
            result(cameras) // Devuelve la lista de cámaras
          } else {
            result(FlutterError(code: "CAMERA_ACCESS_DENIED", message: "Permiso de cámara denegado", details: nil))
          }
        }
      } else if call.method == "startRecording" {
        guard let args = call.arguments as? [String: Any],
              let filePath = args["path"] as? String,
              let cameraName = args["camera"] as? String else {
          result(FlutterError(code: "INVALID_ARGUMENTS", message: "Argumentos inválidos", details: nil))
          return
        }
        self.startRecording(filePath: filePath, cameraName: cameraName)
        result(nil)
      } else if call.method == "stopRecording" {
        self.stopRecording()
        result(nil)
      } else {
        result(FlutterMethodNotImplemented)
      }
    }
  }
  
  private func requestCameraPermission(completion: @escaping (Bool) -> Void) {
    AVCaptureDevice.requestAccess(for: .video) { granted in
      completion(granted)
    }
  }

  private func getVideoDevices() -> [String] {
    let devices = AVCaptureDevice.devices(for: .video)
    return devices.map { $0.localizedName }
  }

  private func startRecording(filePath: String, cameraName: String) {
    captureSession = AVCaptureSession()
    
    guard let device = AVCaptureDevice.devices(for: .video).first(where: { $0.localizedName == cameraName }) else {
      print("Dispositivo no encontrado: \(cameraName)")
      return
    }

    do {
      let input = try AVCaptureDeviceInput(device: device)
      if captureSession?.canAddInput(input) == true {
        captureSession?.addInput(input)
      }

      output = AVCaptureMovieFileOutput()
      if captureSession?.canAddOutput(output!) == true {
        captureSession?.addOutput(output!)
      }

      captureSession?.startRunning()

      let fileUrl = URL(fileURLWithPath: filePath)
      output?.startRecording(to: fileUrl, recordingDelegate: self)
      print("Grabación iniciada con: \(cameraName)")
    } catch {
      print("Error al iniciar grabación: \(error)")
    }
  }

  private func stopRecording() {
    output?.stopRecording()
    captureSession?.stopRunning()
    print("Grabación detenida")
  }

  private func testCameraAccess() {
    let devices = AVCaptureDevice.devices(for: .video)
    if devices.isEmpty {
        print("No hay dispositivos de video disponibles.")
    } else {
        for device in devices {
            print("Dispositivo de cámara encontrado: \(device.localizedName)")
        }
    }
  }

  func fileOutput(_ output: AVCaptureFileOutput, didFinishRecordingTo outputFileURL: URL, from connections: [AVCaptureConnection], error: Error?) {
    if let error = error {
      print("Error durante la grabación: \(error.localizedDescription)")
    } else {
      print("Grabación finalizada correctamente. Archivo guardado en: \(outputFileURL.path)")
    }
  }
}
