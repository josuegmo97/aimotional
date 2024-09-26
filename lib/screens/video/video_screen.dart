import 'package:aimotional/providers/setting_provider.dart';
import 'package:aimotional/services/camera_service.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

class VideoScreen extends StatelessWidget {
  const VideoScreen({super.key});


  @override
  Widget build(BuildContext context) {
    
    final CameraService cameraService = CameraService();
    final provider = Provider.of<SettingProvider>(context);

    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          ElevatedButton(
            onPressed: () async {
              // Define el path donde guardar el archivo
              String? filePath = provider.filePathOutput;
              String? cameraName = provider.camera;

              if(filePath == null || filePath.isEmpty) {
                print("No hay path de salida");
                return;
              }

              if(cameraName == null || cameraName.isEmpty) {
                print("No hay cámara seleccionada");
                return;
              }

              await cameraService.startRecording('$filePath/video.mp4', cameraName);
            },
            child: const Text("Iniciar Grabación"),
          ),
          ElevatedButton(
            onPressed: () async {
              await cameraService.stopRecording();
            },
            child: const Text("Detener Grabación"),
          ),
        ],
      ),
    );
  }
}
