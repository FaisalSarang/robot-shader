# CS6983 Coding Assignment 2 – Robot Arm in OpenGL

A simple robot arm modeled in Blender and rendered in OpenGL using the Blinn-Phong material model.

## Files

| Path | Description |
|---|---|
| `models/robot_arm.blend` | Blender file of the robot arm |
| `models/robot_arm.obj` | Exported mesh (triangulated, with normals, Y-up) |
| `shaders/blinn_phong_vert.glsl` | Vertex shader |
| `shaders/blinn_phong_frag.glsl` | Fragment shader (Blinn-Phong lighting) |
| `shaders/simple_vert.glsl`, `shaders/simple_frag.glsl` | Unlit shaders used for the ground grid |
| `src/main.cpp` | OBJ loader, uniforms, camera and scene |
| `src/gl_loader.hpp` | Loads the OpenGL functions used by the program |

## Lighting model

```
C = (ka*Ia + kd*max(N·L, 0)) * Il + ks * Il * max(N·H, 0)^α
```

## Build and run

GLFW and GLM are downloaded automatically by CMake the first time you configure.

**Windows** (Developer Command Prompt for Visual Studio):

```
cmake -S . -B build
cmake --build build --config Release
.\build\Release\cs6983_opengl_intro.exe
```

**Mac:**

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/cs6983_opengl_intro
```

Controls: drag or arrow keys to orbit, scroll or `-` / `=` to zoom, Esc to quit.

## Reflection

### 1. What was the most difficult portion of this project and how did you overcome it?
Ans) Creating the robot went smoothly considering I have no experience with Blender. After I moved to coding the vertex and pixel shader files I felt like I hit a wall. I generated code from Claude and tried to understand it but I wasted hours achieving nothing. Then I used a different approach. I let Claude give me the boilerplate code to speed things up while I filled in the important parts of the code. For example, in the vertex shader file I would have never paid attention to which variables need 3x3 matrices and which need 4x4 (and why) unless I coded them myself.

### 2. What was your biggest learning outcome from this assignment?
Ans) My biggest learning outcome was definitely light physics. There are so many variables and angles to consider when trying to mimic real world lighting. After I was done with the assignment, I played around with different values of variables to see what difference it made. The Ks variable changes how bright the shiny are on a material is, the Ka changed how dark the dark sides (sides where light does not fall) of the robot become and plenty other variables changed what I saw on my screen.

### 3. What does the vertex shader do? What does the fragment shader do?
Ans) From what I understand from this assignment, the vertex shader runs once per corner then decides where it is going to show up on my screen. The fragment shader runs once per pixel to decide what color that pixel would be after considering all the factors of the Blinn Phong model. The vertex shader passes a corner's world position and normal on to the fragment shader. The rest of the values like light position and material settings are sent from main.cpp and these values can be changed by the user.

### 4. How does this project relate back to your knowledge of Physical AI?
Ans) A robot's camera sees lit surfaces so choosing the right lighting model like Blinn Phong can make a difference when training robots in simulations. I also understood that different points of view can create potential blind spots for robots. I never thought about this when working on computer vision projects or robotic projects myself but it makes sense now.
