# SENT (SAE J2716) Analyzer for Saleae USB logic analyzer

## Building the plugin:

### Windows:

- Open "Visual Studio/SENTAnalyzer.sln" to open project in visual studio
- Depending on the version used, vs might ask to upgrade the project files
- Default build configuration is for "win32". You likely need to change this to "x64"
- Build project. Dll file is generated in "Visual Studio\x64\Release\SENTAnalyzer.dll" 

### Linux/OSX:

run the build_analyzer.py script. The compiled libraries can be found in the newly created debug and release folders.

```
python build_analyzer.py
```

## Installing the plugin:

Copy the .dll/.so over to the Saleae Logic analyzer folder. You can either copy it to the "Analyzers" folder in the Saleae Logic installation directory, or specify a custom path under "Preferences --> Developer"
