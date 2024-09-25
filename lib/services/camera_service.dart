import 'package:flutter/services.dart';

class CameraService {
  // Instancia estática privada para el Singleton
  static final CameraService _instance = CameraService._internal();

  // Canal de método para la comunicación nativa
  static const platform = MethodChannel('camera_channel');

  // Constructor privado
  CameraService._internal();

  // Constructor factory que devuelve la misma instancia siempre
  factory CameraService() {
    return _instance;
  }

  // Método para obtener las cámaras disponibles
  Future<List<String>> getAvailableCameras() async {
    try {
      final List<dynamic> cameras = await platform.invokeMethod('getCameras');
      return cameras.cast<String>();
    } catch (e) {
      print("Error al obtener cámaras: $e");
      return [];
    }
  }

  // Iniciar grabación
  Future<void> startRecording(String filePath, String cameraName) async {
    try {
      await platform.invokeMethod('startRecording', {
        'path': filePath,
        'camera': cameraName,  // Pasar el nombre del dispositivo de cámara
      });
      print("Grabación iniciada con $cameraName en: $filePath");
    } catch (e) {
      print("Error al iniciar grabación: $e");
    }
  }

  // Detener grabación
  Future<void> stopRecording() async {
    try {
      await platform.invokeMethod('stopRecording');
      print("Grabación detenida");
    } catch (e) {
      print("Error al detener grabación: $e");
    }
  }
}