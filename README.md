# august21st-game
Sources for rplace.live 'august 21st' event game.


## Developing & Building:
1. **Clone the Project:**
Make sure to clone this project with the --recursive flag to include the godot-cpp submodule:
   ```bash
   git clone --recursive https://github.com/august21st/game
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
To perform a debug build of the project, navigate to the august21st-game directory and run scons.
This should automatically output the compiled library to `august21st-game/project/bin/YOUR_TARGET_PLATFORM`:
   ```bash
   cd august21st-game
   scons debug_symbols=yes
   ```

5. **Run with Godot 4.3:**
The project requires and must be run with the Godot 4.3 editor (non-mono edition).
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

7. **Generating documentation:**
To generate the doxygen documentation, both `doxygen` and the `DoxygenBuilder` from scons-contrib must be
installed. To install the scons-contrib package, run `python -m pip install --user git+https://github.com/SCons/scons-contrib.git`,
or see https://github.com/SCons/scons-contrib/tree/master for more information.

### Additional notes:
- Ensure you have the Godot 4.3 editor installed and available in your PATH.
- For building web without threads, make sure to compile extensions using the
 threads=no parameter, reference https://github.com/godotengine/godot/issues/94537.

### Useful links:
 - https://github.com/godotengine/godot-cpp
 - https://www.youtube.com/watch?v=8I_G-3Nii4k


## License:
The contents of the august21st-game folder are based off the public domain template
project, [godot-cpp-template](https://github.com/godotengine/godot-cpp-template). All modified files,
including everything under [august21st-game/project](./august21st-game/godot),
[august21st-game/test-server](./august21st-game/server), and
[august21st-game/src](./august21st-game/src) are original works that are **NOT** public
domain, and are licensed under [GNU GPL 3.0](https://www.gnu.org/licenses/gpl-3.0.en.html),
unless stated otherwise. For the terms of this license, refer to the [license file](./LICENSE)
provided.
