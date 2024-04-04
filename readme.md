# cpp_project_template

cpp_project_template - A template for C++ projects.

### Prerequisites for Building the Project

- CMake.
- Ninja.
- Clang compiler (other compilers may work, but are not officially supported).
- Git (to load submodules).
- Visual Studio Code (recommended for development).
  - Clangd extension (recommended for code analysis).

### Clone the Repository

```
git clone --recursive https://github.com/marleeeeeey/cpp_project_template.git
```

### Build, run and debug via VSCode tasks (Windows)

- Open the project folder in VSCode.
- Run task: `(Windows) 03. Install vcpkg as subfolder`.
- Run task: `(Windows) 30. + Run`.
- For debugging press `F5`.

### Build, run and debug manually (Windows)

To build cpp_project_template on Windows, it's recommended to obtain the dependencies by using vcpkg. The following instructions assume that you will follow the vcpkg recommendations and install vcpkg as a subfolder. If you want to use "classic mode" or install vcpkg somewhere else, you're on your own.

This project define it's dependences:
1. In a `vcpkg.json` file, and you are pulling in vcpkg's cmake toolchain file.
2. As git submodules in the `thirdparty` directory. Because some of the libraries not available in vcpkg or have an error in the vcpkg port file.

First, we bootstrap a project-specific installation of vcpkg ("manifest mode") in the default location, `<project root>/vcpkg`. From the project root, run these commands:

```
cd cpp_project_template
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
```

Now we ask vcpkg to install of the dependencies for our project, which are described by the file `<project root>/vcpkg.json`. Note that this step is optional, as cmake will automatically do this. But here we are doing it in a separate step so that we can isolate any problems, because if problems happen here don't have anything to do with your cmake files.

```
.\vcpkg\vcpkg install --triplet=x64-windows
```

Next build the project files. There are different options for:
1. Select Ninja project generator.
2. Select Clang compiler.
3. Telling cmake how to integrate with vcpkg: here we use `CMAKE_TOOLCHAIN_FILE` on the command line.
4. Enable `CMAKE_EXPORT_COMPILE_COMMANDS` to generate a `compile_commands.json` file for clangd.

```
cmake -S . -B build -G "Ninja" -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cd build
cmake --build .
```

Run the cpp_project_template:

```
cpp_project_template.exe
```
