

import 'package:flutter/foundation.dart';

class SettingProvider extends ChangeNotifier {

  // Camara seleccionada
  String? camera;

  // Ruta para guardar el archivo
  String? filePathOutput;

  // Ruta para subir el archivo
  String? filePathInput;

  void notify() => notifyListeners();

  void disposeValues() {}
}