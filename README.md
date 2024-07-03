# august21-event
Sources for rplace.live 'august 21st' event game.


## Developing & Building:
1. **Clone the Project:**
Make sure to clone this project with the --recursive flag to include the godot-cpp submodule:
   ```bash
   git clone --recursive https://github.com/Zekiah-A/august21-event
   ```

2. **Install SCons:**
To compile the C++ modules, you need the SCons build system. Follow the 
[Godot documentation](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/gdextension_cpp_example.html)
to install SCons.

3. **Generate Necessary Headers:**
Before you can start working with the C++ modules, you need to generate the necessary headers,
as not all required headers are provided within the godot-cpp repository.
You can follow the guide in the aforementioned Godot documentation,
or you can use the gen-headers.sh script on Linux, which will automate the process:
   ```bash
   chmod +x ./gen-headers.sh
   ./gen-headers.sh
   ```

4. **Perform a Debug Build:**
To perform a debug build of the project, navigate to the august21-event directory and run scons.
This should automatically output the compiled library to `august21-event/project/bin/YOUR_TARGET_PLATFORM`:
   ```bash
   cd august21-event
   scons # Tip: Use -jNUMBER_OF_THREADS to increase build performance, i.e -j4
   ```

5. **Run with Godot 4.2:**
The project requires and must be run with the Godot 4.2 editor (non-mono edition).
This version will dynamically link and make use of the compiled C++ module when the project is run.

6. **Building for Web:**
To build for web, you need emscripten setuup, with the emscripten environment defined in your terminal
session. You can then run the command `scons platform=web` in order to compile for the wasm target. A
convenience script has been provided that will automatically handle downloading ande setting up emscripten,
along with producing a web build with scons, which will automate the process:
   ```bash
   chmod +x ./build-web.sh
   ./build-web.sh
   ```

### Additional notes:
- Ensure you have the Godot 4.2 editor installed and available in your PATH.

### Useful links:
 - https://github.com/godotengine/godot-cpp
 - https://www.youtube.com/watch?v=8I_G-3Nii4k


## License:
The contents of the august21-event folder are based off the public domain template
project, [godot-cpp](https://github.com/godotengine/godot-cpp). All modified files,
including everything under [august21-event/project](./august21-event/godot),
[august21-event/test-server](./august21-event/server), and
[august21-event/src](./august21-event/src) are original works that are **NOT** public
domain, and are licensed under [GNU GPL 3.0](https://www.gnu.org/licenses/gpl-3.0.en.html).
For the terms of this license, refer to the [license file](./LICENSE) provided.