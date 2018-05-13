#include "glad/glad.h"  //Include order can matter here

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/vector_angle.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

#include "camera.h"
#include "p_ball.h"
#include "p_fire.h"
#include "p_magic.h"
#include "p_snow.h"
#include "p_water.h"
#include "particle_system.h"

ParticleSystem* particles;

Camera* cam;

static const float frustNear = 0.25;
static const float frustFar = 20;

int screenWidth = 800;
int screenHeight = 600;

bool DEBUG_ON = false;
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
bool fullscreen = false;

static glm::vec3 contextTranslate = glm::vec3(0, 0, 0);
static glm::vec3 contextScale = glm::vec3(1, 1, 1);
static int contextTextureID = -1;

GLuint vao[1];
GLuint vbo[1];
int texturedShader;

void drawGeometry(float dt) {
  glm::mat4 model;
  glm::vec3 colVec(0.2, 0.3, 0.8);
  glPointSize(4);

  glBindVertexArray(vao[0]);
  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
  glBufferData(GL_ARRAY_BUFFER, particles->numParticles * 3 * sizeof(float),
               particles->data, GL_STREAM_DRAW);

  glUseProgram(texturedShader);

  GLint uniModel = glGetUniformLocation(texturedShader, "model");
  GLint uniColor = glGetUniformLocation(texturedShader, "inColor");
  // GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");

  // Draw context
  // model = glm::translate(model, contextTranslate);
  // model = glm::scale(model, contextScale);
  // glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

  // glUniform3fv(uniColor, 1, glm::value_ptr(colVec));
  // glUniform1i(uniTexID,
  //             contextTextureID);  // Set texture ID to use (-1 = no texture)
  // glDrawArrays(GL_TRIANGLES, numVertsParticle, numVertsContext);

  glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
  glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

  glDrawArrays(GL_POINTS, 0, particles->numParticles);

  particles->update(dt);
}

int main(int argc, char* argv[]) {
  srand(time(NULL));
  SDL_Init(SDL_INIT_VIDEO);  // Initialize Graphics (for OpenGL)

  // Ask SDL to get a recent version of OpenGL (3 or greater)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  // Create a window (offsetx, offsety, width, height, flags)
  SDL_Window* window = SDL_CreateWindow("Particle System", 0, 0, screenWidth,
                                        screenHeight, SDL_WINDOW_OPENGL);

  // Create a context to draw in
  SDL_GLContext context = SDL_GL_CreateContext(window);

  // Turn off VSync
  // if (SDL_GL_SetSwapInterval(0)) {
  //   fprintf(stderr, "Disabling VSync failed: %s\n", SDL_GetError());
  // }

  SDL_bool captureMouse = SDL_FALSE;
  SDL_SetRelativeMouseMode(captureMouse);

  // Load OpenGL extentions with GLAD
  if (gladLoadGLLoader(SDL_GL_GetProcAddress)) {
    printf("\nOpenGL loaded\n");
    printf("Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version:  %s\n\n", glGetString(GL_VERSION));
  } else {
    printf("ERROR: Failed to initialize OpenGL context.\n");
    return -1;
  }

  std::vector<tinyobj::real_t> modelContext;
  if (argc > 1) {
    const char* kind = argv[1];
    int numParticles = 1000;
    if (argc > 2) {
      numParticles = atoi(argv[2]);
    }
    if (!strcmp(kind, "water")) {
      particles = new WaterSystem(numParticles);
      modelContext = ParticleSystem::loadModel("models/fountain.obj");
      contextScale = glm::vec3(2, 4, 2);
    } else if (!strcmp(kind, "fire")) {
      particles = new FireSystem(numParticles);
      modelContext = ParticleSystem::loadModel("models/logs.obj");
      contextTextureID = 0;
      contextTranslate = glm::vec3(0, 0, -0.25);
    } else if (!strcmp(kind, "magic")) {
      particles = new MagicSystem(numParticles);
      modelContext = ParticleSystem::loadModel("models/floor.obj");
    } else if (!strcmp(kind, "ball")) {
      particles = new BallSystem();
      modelContext = ParticleSystem::loadModel("models/floor.obj");
      contextScale = glm::vec3(3, 3, 1);
    } else {
      printf(
          "Usage: %s <kind>\n"
          "kinds\n"
          "\twater\n"
          "\tfire\n"
          "\tmagic\n"
          "\tball\n",
          argv[0]);
      exit(1);
    }
  } else {
    particles = new BallSystem();
    modelContext = ParticleSystem::loadModel("models/floor.obj");
    contextScale = glm::vec3(3, 3, 1);
  }

  // Load Models
  // std::vector<tinyobj::real_t> modelParticle = particles->Model();
  // int numVertsParticle = modelParticle.size() / 8;

  // int numVertsContext = modelContext.size() / 8;

  // int totalNumVerts = numVertsParticle + numVertsContext;
  // std::vector<tinyobj::real_t> modelData;
  // modelData.reserve(modelParticle.size() + modelContext.size());
  // modelData.insert(modelData.end(), modelParticle.begin(),
  // modelParticle.end()); modelData.insert(modelData.end(),
  // modelContext.begin(), modelContext.end());

  //// Allocate Texture 0 (Gravel) ///////
  SDL_Surface* surface = SDL_LoadBMP("textures/bark.bmp");
  if (surface == NULL) {  // If it failed, print the error
    printf("Error: \"%s\"\n", SDL_GetError());
    return 1;
  }
  GLuint tex0;
  glGenTextures(1, &tex0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex0);

  // What to do outside 0-1 range
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Load the texture into memory
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGR,
               GL_UNSIGNED_BYTE, surface->pixels);
  glGenerateMipmap(GL_TEXTURE_2D);

  SDL_FreeSurface(surface);
  //// End Allocate Texture ///////

  //// Allocate Texture 1 (Brick) ///////
  SDL_Surface* surface1 = SDL_LoadBMP("textures/smiley.bmp");
  if (surface1 == NULL) {  // If it failed, print the error
    printf("Error: \"%s\"\n", SDL_GetError());
    return 1;
  }
  GLuint tex1;
  glGenTextures(1, &tex1);

  // Load the texture into memory
  glActiveTexture(GL_TEXTURE1);

  glBindTexture(GL_TEXTURE_2D, tex1);
  // What to do outside 0-1 range
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // How to filter
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface1->w, surface1->h, 0, GL_BGRA,
               GL_UNSIGNED_BYTE, surface1->pixels);

  glGenerateMipmap(GL_TEXTURE_2D);

  SDL_FreeSurface(surface1);
  //// End Allocate Texture ///////

  // Build a Vertex Array Object. This stores the VBO and attribute mappings in
  // one object
  glGenVertexArrays(1, vao);  // Create a VAO
  glBindVertexArray(
      vao[0]);  // Bind the above created VAO to the current context

  // Allocate memory on the graphics card to store geometry (vertex buffer
  // object)
  glGenBuffers(1, vbo);                   // Create 1 buffer called vbo
  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);  // Set the vbo as the active array
                                          // buffer (Only one buffer can be
                                          // active at a time)

  texturedShader = InitShader("src/shaders/particle_vertex.glsl",
                              "src/shaders/particle_fragment.glsl");

  // Tell OpenGL how to set fragment shader input
  GLint posAttrib = glGetAttribLocation(texturedShader, "position");
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
  // Attribute, vals/attrib., type, normalized?, stride, offset
  // Binds to VBO current GL_ARRAY_BUFFER
  glEnableVertexAttribArray(posAttrib);

  GLint uniView = glGetUniformLocation(texturedShader, "view");
  GLint uniProj = glGetUniformLocation(texturedShader, "proj");

  glBindVertexArray(0);  // Unbind the VAO in case we want to create a new one

  glEnable(GL_DEPTH_TEST);

  cam = new Camera(glm::vec3(0, 1, 4), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0));

  // printf("camera: %f %f %f\nview dir: %f %f %f\npitch: %f\nyaw: %f\n",
  // camPos.x,
  //        camPos.y, camPos.z, camFront.x, camFront.y, camFront.z, pitch, yaw);

  // printf("Cam right: %f %f %f\n", camRight.x, camRight.y, camRight.z);
  // printf("Cam up   : %f %f %f\n", camUp.x, camUp.y, camUp.z);
  // printf("Cam front: %f %f %f\n", camFront.x, camFront.y, camFront.z);

  // For FPS counter
  int frame = 0;
  unsigned t0 = SDL_GetTicks();
  unsigned t1 = t0;

  unsigned tLastFrame = t0;
  unsigned timePast = SDL_GetTicks();

  // Event Loop (Loop forever processing each event as fast as possible)
  SDL_Event windowEvent;
  bool quit = false;
  while (!quit) {
    tLastFrame = timePast;
    timePast = SDL_GetTicks();
    float delta = (timePast - tLastFrame) / 1000.f;

    while (SDL_PollEvent(&windowEvent)) {  // inspect all events in the queue
      if (windowEvent.type == SDL_QUIT) quit = true;
      // List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch
      // many special keys  Scancode referes to a keyboard position, keycode
      // referes to the letter (e.g., EU keyboards)
      // if (windowEvent.type == SDL_KEYUP &&
      //     windowEvent.key.keysym.sym == SDLK_ESCAPE)
      //   SDL_SetRelativeMouseMode(SDL_FALSE);
      // if (windowEvent.type == SDL_KEYUP &&
      //     windowEvent.key.keysym.sym == SDLK_f) {
      //   fullscreen = !fullscreen;
      //   SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN :
      //   0);
      // }
      // if (windowEvent.type == SDL_KEYUP &&
      //     windowEvent.key.keysym.sym == SDLK_SPACE) {
      //   // Add random velocity
      //   for (int i = 0; i < particles->NumParticles(); i++) {
      //     glm::vec3 vRand = glm::vec3(3 * rand01() - 1.5, 3 * rand01() - 1.5,
      //                                 2 + 2 * rand01());
      //     particles->Velocity(i, particles->Velocity(i) + vRand);
      //   }
      // }
      // cam->processMouseWheel(&windowEvent);
    }

    const Uint8* keyState = SDL_GetKeyboardState(NULL);
    cam->processMovement(keyState, delta);

    // Clear the screen to default color
    glClearColor(0.1, 0.1, 0.1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = cam->getLookAt();
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

    glm::mat4 proj =
        glm::perspective(cam->fov, screenWidth / (float)screenHeight, frustNear,
                         frustFar);  // FOV, aspect, near, far
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, tex0);
    // glUniform1i(glGetUniformLocation(texturedShader, "tex0"), 0);

    // glActiveTexture(GL_TEXTURE1);
    // glBindTexture(GL_TEXTURE_2D, tex1);
    // glUniform1i(glGetUniformLocation(texturedShader, "tex1"), 1);

    drawGeometry(delta);

    SDL_GL_SwapWindow(window);  // Double buffering

    // FPS Counter
    frame++;
    t1 = SDL_GetTicks();
    if (t1 - t0 >= 1000) {
      char buf[100];
      sprintf(buf, "Particle System | FPS: %.4f\r",
              frame / ((t1 - t0) / 1000.f));
      SDL_SetWindowTitle(window, buf);
      t0 = t1;
      frame = 0;
    }
  }

  // Clean Up
  delete particles;
  delete cam;

  glDeleteProgram(texturedShader);
  glDeleteBuffers(1, vbo);
  glDeleteVertexArrays(1, vao);

  SDL_GL_DeleteContext(context);
  SDL_Quit();
  return 0;
}

// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile) {
  FILE* fp;
  long length;
  char* buffer;

  // open the file containing the text of the shader code
  fp = fopen(shaderFile, "r");

  // check for errors in opening the file
  if (fp == NULL) {
    printf("can't open shader source file %s\n", shaderFile);
    return NULL;
  }

  // determine the file size
  fseek(fp, 0, SEEK_END);  // move position indicator to the end of the file;
  length = ftell(fp);      // return the value of the current position

  // allocate a buffer with the indicated number of bytes, plus one
  buffer = new char[length + 1];

  // read the appropriate number of bytes from the file
  fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
  if (fread(buffer, 1, length, fp) !=
      (unsigned)length) {  // read all of the bytes
    fprintf(stderr, "Error reading shader source\n");
  }

  // append a NULL character to indicate the end of the string
  buffer[length] = '\0';

  // close the file
  fclose(fp);

  // return the string
  return buffer;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName) {
  GLuint vertex_shader, fragment_shader;
  GLchar *vs_text, *fs_text;
  GLuint program;

  // check GLSL version
  // printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

  // Create shader handlers
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  // Read source code from shader files
  vs_text = readShaderSource(vShaderFileName);
  fs_text = readShaderSource(fShaderFileName);

  // error check
  if (vs_text == NULL) {
    printf("Failed to read from vertex shader file %s\n", vShaderFileName);
    exit(1);
  } else if (DEBUG_ON) {
    printf("Vertex Shader:\n=====================\n");
    printf("%s\n", vs_text);
    printf("=====================\n\n");
  }
  if (fs_text == NULL) {
    printf("Failed to read from fragent shader file %s\n", fShaderFileName);
    exit(1);
  } else if (DEBUG_ON) {
    printf("\nFragment Shader:\n=====================\n");
    printf("%s\n", fs_text);
    printf("=====================\n\n");
  }

  // Load Vertex Shader
  const char* vv = vs_text;
  glShaderSource(vertex_shader, 1, &vv, NULL);  // Read source
  glCompileShader(vertex_shader);               // Compile shaders

  // Check for errors
  GLint compiled;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    printf("Vertex shader failed to compile:\n");
    if (DEBUG_ON) {
      GLint logMaxSize, logLength;
      glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
      printf("printing error message of %d bytes\n", logMaxSize);
      char* logMsg = new char[logMaxSize];
      glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
      printf("%d bytes retrieved\n", logLength);
      printf("error message: %s\n", logMsg);
      delete[] logMsg;
    }
    exit(1);
  }

  // Load Fragment Shader
  const char* ff = fs_text;
  glShaderSource(fragment_shader, 1, &ff, NULL);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);

  // Check for Errors
  if (!compiled) {
    printf("Fragment shader failed to compile\n");
    if (DEBUG_ON) {
      GLint logMaxSize, logLength;
      glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
      printf("printing error message of %d bytes\n", logMaxSize);
      char* logMsg = new char[logMaxSize];
      glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
      printf("%d bytes retrieved\n", logLength);
      printf("error message: %s\n", logMsg);
      delete[] logMsg;
    }
    exit(1);
  }

  // Create the program
  program = glCreateProgram();

  // Attach shaders to program
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);

  // Link and set program to use
  glLinkProgram(program);

  return program;
}
