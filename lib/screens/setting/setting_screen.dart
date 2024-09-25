import 'package:aimotional/providers/setting_provider.dart';
import 'package:aimotional/services/services.dart';
import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

class SettingScreen extends StatefulWidget {
   
  const SettingScreen({super.key});

  @override
  State<SettingScreen> createState() => _SettingScreenState();
}

class _SettingScreenState extends State<SettingScreen> {

  List<String> devicesAvailables = [];
  int selectedIndex = 0;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      getAvailablesDevices();
      setSelectProviderCamera();
    });
  }

  void getAvailablesDevices() async {
    try {
      final service = CameraService();
      final List<String> availables = await service.getAvailableCameras();
      print("Disponibles: $availables");
      setState(() => devicesAvailables = availables);
      return;
    } catch (e) {
      print("Error al obtener cámaras: $e");
      return;
    }
  }

  void setSelectProviderCamera() async {
    try {
      final provider = Provider.of<SettingProvider>(context, listen: false);

      final service = CameraService();
      final List<String> availables = await service.getAvailableCameras();

      if(availables.isNotEmpty) {
        provider.camera = availables[0];
        provider.notify();
      }
      return;
    } catch (e) {
      print("Error al set provider");
      return;
    }
  }

  @override
  Widget build(BuildContext context) {

    final provider = Provider.of<SettingProvider>(context);

    return Column(
      children: [

        const SizedBox(height: 10),

        Row(
          children: [
            Expanded(
              child: TextFormField(
                initialValue: provider.filePathOutput,
                decoration: const InputDecoration(
                  labelText: "Ruta de salida",
                  hintText: "Ruta para guardar el archivo",
                ),
                onTap: () async {
                  final String? path = await _pickDirectory();

                  if(path != null) {
                    provider.filePathOutput = path;
                    provider.notify();
                  }
                },
              ),
            ),

            const SizedBox(width: 100),

            Expanded(
              child: TextFormField(
                initialValue: provider.filePathInput,
                decoration: const InputDecoration(
                  labelText: "Ruta de entrada",
                  hintText: "Ruta para subir el archivo",
                ),
                onTap: () async {
                  final String? path = await _pickDirectory();

                  if(path != null) {
                    provider.filePathInput = path;
                    provider.notify();
                  }
                },
              ),
            ),
          ],
        ),

        const SizedBox(height: 20),

        const Text("Cámaras disponibles", style: TextStyle(color: Colors.blue, fontSize: 20)),

        const SizedBox(height: 20),

        SizedBox(
          width: double.infinity,
          height: 300,
          child: ListView.builder(
            itemCount: devicesAvailables.length,
            itemBuilder: (context, index) {
              return ListTile(
                title: Text(devicesAvailables[index]),
                tileColor: selectedIndex == index ? Colors.blue.withOpacity(0.3) : null,
                onTap: () {
                  provider.camera = devicesAvailables[index];
                  provider.notify();
                  setState(() {
                    selectedIndex = index; // Cambia el índice seleccionado
                  });
                }, // Manejar la selección al tocar
              );
            },
          ),
        ),
      ],
    );
  }

  Future<String?> _pickDirectory() async {
    try {
      String? selectedDirectory = await FilePicker.platform.getDirectoryPath();
      return selectedDirectory;
    } catch (e) {
      print("Error al seleccionar directorio: $e");
      return null;
    }
  }
}