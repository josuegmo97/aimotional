import AVFoundation

class VideoRecorder {
  private var session: AVCaptureSession?
  private var videoOutput: AVCaptureMovieFileOutput?
  private var videoDevice: AVCaptureDevice?

  // Configurar la sesión de captura
  func startRecording(to outputURL: URL) {
    session = AVCaptureSession()

    // Configurar el dispositivo de captura
    guard let videoDevice = AVCaptureDevice.default(for: .video) else {
      print("No se encontró ningún dispositivo de captura de video.")
      return
    }
    self.videoDevice = videoDevice

    // Agregar el dispositivo de captura como entrada
    do {
      let videoInput = try AVCaptureDeviceInput(device: videoDevice)
      if session?.canAddInput(videoInput) == true {
        session?.addInput(videoInput)
      }
    } catch {
      print("Error al agregar el dispositivo de video como entrada: \(error)")
      return
    }

    // Configurar la salida del archivo
    videoOutput = AVCaptureMovieFileOutput()
    if session?.canAddOutput(videoOutput!) == true {
      session?.addOutput(videoOutput!)
    }

    // Iniciar la sesión de captura
    session?.startRunning()

    // Empezar la grabación
    videoOutput?.startRecording(to: outputURL, recordingDelegate: self)
  }

  // Parar la grabación
  func stopRecording() {
    videoOutput?.stopRecording()
    session?.stopRunning()
  }
}

extension VideoRecorder: AVCaptureFileOutputRecordingDelegate {
  // Funciones de delegado para manejar los eventos de grabación
  func fileOutput(_ output: AVCaptureFileOutput, didFinishRecordingTo outputFileURL: URL, from connections: [AVCaptureConnection], error: Error?) {
    if let error = error {
      print("Error durante la grabación: \(error)")
    } else {
      print("Grabación completada. Archivo guardado en: \(outputFileURL.path)")
    }
  }
}
