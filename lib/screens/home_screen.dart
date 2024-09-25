import 'package:aimotional/screens/screens.dart';
import 'package:flutter/material.dart';

class HomeScreen extends StatefulWidget {
  static const String routerName = 'HomeScreen';
  
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {

  bool isSetting = true;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          children: [
            Row(
              children: [
                Expanded(
                  child: ElevatedButton(
                    onPressed: () => setState(() => isSetting = true),
                    child: const Text('Settings'),
                  ),
                ),
      
                const SizedBox(width: 20),
      
                Expanded(
                  child: ElevatedButton(
                    onPressed: () => setState(() => isSetting = false),
                    child: const Text('Video'),
                  ),
                ),
              ],
            ),

            const SizedBox(height: 20),

            (isSetting) ? const SettingScreen() : const VideoScreen(),
          ],
        ),
      ),
    );
  }
}