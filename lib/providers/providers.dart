import 'package:aimotional/providers/setting_provider.dart';
import 'package:provider/provider.dart';
import 'package:nested/nested.dart';

class Providers {

  static List<SingleChildWidget> get list {

    return [
        ChangeNotifierProvider(create: (_) => SettingProvider()),
    ];
  }


}