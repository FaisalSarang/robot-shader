// CS 6983 Module 2 Week 3
// Sample OpenGL Program

#include "gl_loader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct App {
    // screen size
    int width = 1920;
    int height = 1080;
    // initial camera position
    float yaw = 0.7f;
    float pitch = 0.4f;
    float distance = 11.0f;
    // dragging state
    bool dragging = false;
    // last mouse position
    double lastX = 0.0;
    double lastY = 0.0;
};

static App gApp;

static GLuint gProgram = 0;
static GLint gMvpLoc = -1;
static GLint gColorLoc = -1;
static GLuint gLitProgram = 0;      // Blinn-Phong shaders
static GLuint gArmVao = 0;          // the uploaded robot arm
static GLuint gArmVertexCount = 0;  // how many corners to draw
static GLuint gGridVao = 0;
static GLuint gGridVertexCount = 0;

// colors
const glm::vec3 LIGHT_GRAY(0.28f, 0.31f, 0.34f); // Gray
const glm::vec3 DARK_GRAY(0.11f, 0.12f, 0.14f); // Dark Gray
const glm::vec3 YELLOW(0.83f, 0.64f, 0.36f); // Yellow
const glm::vec3 BLUE(0.32f, 0.58f, 0.78f); // Blue
const glm::vec3 RED(0.80f, 0.36f, 0.36f); // Red

// ###########################################################################################
// Shaders
// ###########################################################################################

// helper function to read the text from a file
// Thanks Cursor AI
static std::string read_text(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open " + path);
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// Thanks Cursor AI
static GLuint compile_shader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* text = source.c_str();
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << log << "\n";
        throw std::runtime_error("Shader compile failed");
    }
    return shader;
}

// Thanks Cursor AI
static GLuint link_program(const std::string& vertPath, const std::string& fragPath) {
    GLuint vert = compile_shader(GL_VERTEX_SHADER, read_text(vertPath));
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, read_text(fragPath));
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << log << "\n";
        throw std::runtime_error("Program link failed");
    }
    return program;
}

// ###########################################################################################
// Meshes
// ###########################################################################################


static GLuint upload_positions(const std::vector<float>& positions) {
    GLuint vao = 0; // vertex array object: how to interpret the vertex data
    GLuint vbo = 0; // vertex buffer object: where to store the vertex data
    // generate the vertex array and buffer
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    // bind the vertex array and buffer
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // upload the data to the buffer
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(positions.size() * sizeof(float)), positions.data(), GL_STATIC_DRAW);
    // set the vertex attribute pointer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    // enable the vertex attribute array
    glEnableVertexAttribArray(0);
    // return the vertex array
    return vao;
}


// each corner is 6 floats: x y z nx ny nz
static GLuint upload_positions_normals(const std::vector<float>& data) {
    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);

    const GLsizei stride = 6 * sizeof(float);   // blank 1: how many floats per corner?
    // slot 0: position, starts at float 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);
    // slot 1: normal, starts after the 3 position floats
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));  // blank 2
    glEnableVertexAttribArray(1);   // blank 3
    return vao;
}


static GLuint load_obj(const std::string& path, GLuint& vertexCount) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open " + path);
    }

    std::vector<glm::vec3> positions; // every "v" line
    std::vector<glm::vec3> normals;   // every "vn" line
    std::vector<float> data;          // for the GPU: x y z nx ny nz per corner

    std::string line;
    while (std::getline(in, line)) {   // read the file one line at a time
        std::istringstream ss(line);
        std::string tag;
        ss >> tag;                     // first word of the line: "v", "vn", "f", ...

        if (tag == "v") {
            glm::vec3 p;
            ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (tag == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (tag == "f") {
            for (int corner = 0; corner < 3; ++corner) {
                std::string token;     // e.g. "12//7"
                ss >> token;
                const size_t slash = token.find("//");
                const int vi = std::stoi(token.substr(0, slash)) -1;  // blank 5: position number, fixed for C++ counting
                const int ni = std::stoi(token.substr(slash + 2)) -1; // blank 6: normal number, same fix
                const glm::vec3 p = positions[vi];
                const glm::vec3 n = normals[ni];
                data.insert(data.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
            }
        }
    }

    vertexCount = static_cast<GLuint>(data.size() / 6);  // blank 7: floats per corner
    std::cout << "loaded " << vertexCount / 3 << " triangles\n";
    return upload_positions_normals(data);
}

// Thanks Cursor AI
static GLuint make_grid() {
    // create a vector to store the vertices
    std::vector<float> positions;
    // create the vertices of the grid
    for (int i = -4; i <= 4; ++i) {
        const float x = static_cast<float>(i);
        positions.insert(positions.end(), {x, 0.0f, -4.0f, x, 0.0f, 4.0f});
        positions.insert(positions.end(), {-4.0f, 0.0f, x, 4.0f, 0.0f, x});
    }
    // set the number of vertices
    gGridVertexCount = static_cast<GLuint>(positions.size() / 3);
    return upload_positions(positions);
}

static void draw_mesh(GLuint vao, GLuint count, GLenum mode, const glm::mat4& mvp, const glm::vec3& color) {
    // set the Model-View-Projection matrix
    // glUniformMatrix4fv(location, count, transpose, value)
    glUniformMatrix4fv(gMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    // pass color to the fragment shader
    glUniform3f(gColorLoc, color.r, color.g, color.b);
    // bind the vertex array
    glBindVertexArray(vao);
    // draw the vertices
    glDrawArrays(mode, 0, static_cast<GLint>(count));
}

// ###########################################################################################
// Transform
// ###########################################################################################

// Unity Transform: translate, then rotate, then scale. Scale applies first
// to the vertex because the vector sits on the right: M * p = T * R * S * p.
static glm::mat4 model_matrix(const glm::vec3& position, float radians, const glm::vec3& scale) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, radians, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, scale);
    return model;
}

// ###########################################################################################
// Camera
// ###########################################################################################

// camera orbits this point, about halfway up the arm
const glm::vec3 CAMERA_TARGET(0.0f, 2.0f, 0.0f);

// eye position is calculated based on the pitch and yaw angles
static glm::vec3 eye_position() {
    const float cosPitch = std::cos(gApp.pitch);
    return CAMERA_TARGET + glm::vec3(
        gApp.distance * cosPitch * std::sin(gApp.yaw), // x coordinate
        gApp.distance * std::sin(gApp.pitch), // y coordinate
        gApp.distance * cosPitch * std::cos(gApp.yaw) // z coordinate
    );
}

// Unity Camera position and orientation
static glm::mat4 view_matrix() {
    // lookAt(camera position, target position, up vector)
    return glm::lookAt(eye_position(), CAMERA_TARGET, glm::vec3(0.0f, 1.0f, 0.0f));
}

// Unity Camera field of view, aspect, near plane, far plane
static glm::mat4 projection_matrix() {
    // calculate the aspect ratio
    const float aspect = gApp.height > 0
        ? static_cast<float>(gApp.width) / static_cast<float>(gApp.height)
        : 1.0f;

    // 70 degrees is the camera field of view (FOV)
    float fov = 40.0f;      
    // set clipping planes
    // near plane must stay above 0. camera-space z = 0 cannot be projected
    float near = 0.1f;
    float far = 40.0f;
    return glm::perspective(glm::radians(fov), aspect, near, far);
}

// ###########################################################################################
// Scene
// ###########################################################################################

static void draw_scene(float time) {
    const glm::mat4 view = view_matrix();
    const glm::mat4 projection = projection_matrix();

    // draw the grid using lines with the unlit shaders
    glUseProgram(gProgram);
    draw_mesh(gGridVao, gGridVertexCount, GL_LINES, projection * view, LIGHT_GRAY);

    // switch to the Blinn-Phong shaders for the arm
    glUseProgram(gLitProgram);
    // shorthand: look up a uniform in the lit program by name
    auto loc = [](const char* name) { return glGetUniformLocation(gLitProgram, name); };

    // matrices: the arm slowly turns so the highlights move across it
    const glm::mat4 model = model_matrix(glm::vec3(0.0f), time * 0.3f, glm::vec3(1.0f));
    // normal matrix: inverse-transpose of the model matrix's 3x3 part
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    glUniformMatrix4fv(loc("uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(loc("uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(loc("uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix3fv(loc("uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // light and camera
    const glm::vec3 eye = eye_position();
    glUniform3f(loc("uLightPos"), 5.0f, 8.0f, 5.0f);
    glUniform3f(loc("uViewPos"), eye.x, eye.y, eye.z);
    glUniform3f(loc("uLightColor"), 1.0f, 1.0f, 1.0f);  // Il: white light
    glUniform3f(loc("uAmbient"), 0.35f, 0.38f, 0.45f);  // Ia: soft bluish fill

    // material
    glUniform1f(loc("uKa"), 0.4f);
    glUniform3f(loc("uKd"), RED.r, RED.g, RED.b); // model color
    glUniform1f(loc("uKs"), 0.5f);
    glUniform1f(loc("uShininess"), 48.0f);

    glBindVertexArray(gArmVao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLint>(gArmVertexCount));
}

// ###########################################################################################
// Callbacks functions
// ###########################################################################################

static void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    gApp.width = width;
    gApp.height = height;
}

// ###########################################################################################
// Input Callbacks
// ###########################################################################################

// Thanks Cursor AI
static void key_callback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) {
        return;
    }
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_LEFT) {
        gApp.yaw -= 0.08f;
    } else if (key == GLFW_KEY_RIGHT) {
        gApp.yaw += 0.08f;
    } else if (key == GLFW_KEY_UP) {
        gApp.pitch += 0.06f;
    } else if (key == GLFW_KEY_DOWN) {
        gApp.pitch -= 0.06f;
    } else if (key == GLFW_KEY_MINUS) {
        gApp.distance += 0.3f;
    } else if (key == GLFW_KEY_EQUAL) {
        gApp.distance -= 0.3f;
    }
}

// Thanks Cursor AI
static void mouse_button_callback(GLFWwindow*, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        gApp.dragging = action == GLFW_PRESS;
    }
}

// Thanks Cursor AI
static void cursor_callback(GLFWwindow*, double x, double y) {
    if (gApp.dragging) {
        gApp.yaw += static_cast<float>(x - gApp.lastX) * 0.008f;
        gApp.pitch += static_cast<float>(y - gApp.lastY) * 0.008f;
    }
    gApp.lastX = x;
    gApp.lastY = y;
}

// Thanks Cursor AI
static void scroll_callback(GLFWwindow*, double, double yOffset) {
    gApp.distance -= static_cast<float>(yOffset) * 0.4f;
}

// ###########################################################################################
// Camera Clamping
// ###########################################################################################

// Thanks Cursor AI
static void clamp_camera() {
    if (gApp.pitch > 1.15f) gApp.pitch = 1.15f;
    if (gApp.pitch < -1.15f) gApp.pitch = -1.15f;
    if (gApp.distance < 2.0f) gApp.distance = 2.0f;
    if (gApp.distance > 25.0f) gApp.distance = 25.0f;
}

// ###########################################################################################
// Main
// ###########################################################################################

int main() {
    glfwSetErrorCallback([](int code, const char* description) {
        std::cerr << "GLFW " << code << ": " << description << "\n";
    });
    if (!glfwInit()) {
        return 1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    // create the window
    GLFWwindow* window = glfwCreateWindow(gApp.width, gApp.height, "CS 6983 OpenGL", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    // make the window the current context so we can draw to it
    glfwMakeContextCurrent(window);
    // wait for the display to refresh before swapping the buffers
    glfwSwapInterval(1);
    // set the callback functions for the window
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwGetFramebufferSize(window, &gApp.width, &gApp.height);

    // run the driver 
    if (!load_gl()) {
        glfwTerminate();
        return 1;
    }

    // link the shaders and load the model
    try {
        // unlit shaders for the grid (the starter pointed at color_*.glsl, which don't exist)
        gProgram = link_program(std::string(SHADER_DIR) + "/simple_vert.glsl",
                                std::string(SHADER_DIR) + "/simple_frag.glsl");
        // Blinn-Phong shaders for the arm
        gLitProgram = link_program(std::string(SHADER_DIR) + "/blinn_phong_vert.glsl",
                                   std::string(SHADER_DIR) + "/blinn_phong_frag.glsl");
        // models/ sits next to shaders/
        gArmVao = load_obj(std::string(SHADER_DIR) + "/../models/robot_arm.obj", gArmVertexCount);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        glfwTerminate();
        return 1;
    }
    // get the location of the uniform variables
    gMvpLoc = glGetUniformLocation(gProgram, "uMVP");
    gColorLoc = glGetUniformLocation(gProgram, "uColor");

    // make the meshes
    gGridVao = make_grid();
    // enable the depth test
    glEnable(GL_DEPTH_TEST);
    // set the background color
    glClearColor(DARK_GRAY.r, DARK_GRAY.g, DARK_GRAY.b, 1.0f);
    //glClearColor(RED.r, RED.g, RED.b, 1.0f);

    std::cout << "drag or arrows to orbit, wheel or -/= to zoom, Esc quits\n";
    // main loop
    while (!glfwWindowShouldClose(window)) {
        // poll for events (inputs)
        glfwPollEvents();
        // get the time
        const float time = static_cast<float>(glfwGetTime());
        // clamp the camera to prevent it from going too far
        clamp_camera();
        
        // set the viewport and clear the color and depth buffers to prepare for the next frame
        glViewport(0, 0, gApp.width, gApp.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw the scene 
        draw_scene(time);
        // swap the buffers to display the new frame
        glfwSwapBuffers(window);
    }

    // terminate the window
    glfwTerminate();
    return 0;
}
