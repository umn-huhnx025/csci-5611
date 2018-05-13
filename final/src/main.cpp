#include "glad/glad.h"  // Include order can matter here

#include <SDL.h>
#include <SDL_opengl.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/vector_angle.hpp"

// Library to load obj files
// https://github.com/syoyo/tinyobjloader
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "camera.h"
#include "config.h"
#include "sound.h"
#include "sph_fluid.h"
#include "spring_system.h"

SPHFluid* fluid;
SpringSystem* ss = nullptr;

Camera* cam;

float audio_position = 0;
float lastFrame = 0, currentFrame = 0;
std::vector<bubbleSound> bubbles;
SDL_AudioDeviceID dev;

static const float frustNear = 0.25;
static const float frustFar = 20;

int screenWidth = 800;
int screenHeight = 600;

bool DEBUG_ON = false;
bool saveOutput = false;

bool audio = false;
bool heat = true;

glm::mat4 view, proj;
GLint uniView, uniProj;
GLint uniView1, uniProj1;
GLint uniView2, uniProj2;

static int particleShader;
static int lineShader;
static int phongShader;
static const int NUM_VAO = 2;
static const int NUM_VBO = 2;
GLuint vao[NUM_VAO];
GLuint vbo[NUM_VAO];

GLuint InitShader(std::string vShaderFileName, std::string fShaderFileName);
std::vector<tinyobj::real_t> loadModel(const char* filename);
void Win2PPM(int width, int height);

void CallBack(void* _beeper, Uint8* _stream, int _len) {
  short* stream = (short*)_stream;
  int len = _len / 2;

  currentFrame = SDL_GetTicks();
  float dt = (currentFrame - lastFrame) / 1000.f;
  lastFrame = currentFrame;

  // printf("-------------\n");
  std::vector<bubbleSound>::iterator bub = bubbles.begin();
  short* mixAudio = new short[len];
  SDL_memset(mixAudio, 0, len);
  for (; bub != bubbles.end(); bub++) {
    short* sampleBuffer;
    sampleBuffer = audioBufferGeneration(*bub, len, dt);
    // SDL_memset(stream, 0, len);
    // SDL_MixAudio(stream, sampleBuffer, len, SDL_MIX_MAXVOLUME);
    for (int j = 0; j < len; j++) {
      mixAudio[j] += sampleBuffer[j];
    }
    bub->audioPosition += dt;
  }
  for (int j = 0; j < len; j++) {
    stream[j] = mixAudio[j];
  }

  return;
}

void drawGeometry(float dt) {
  glm::mat4 model;
  glm::vec3 colVec = glm::vec3();

  // Draw box
  glBindVertexArray(vao[0]);
  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
  glBufferData(GL_ARRAY_BUFFER, fluid->vboSize(), fluid->VboData(),
               GL_STREAM_DRAW);

  glUseProgram(lineShader);

  GLint uniModel1 = glGetUniformLocation(lineShader, "model");
  GLint uniColor1 = glGetUniformLocation(lineShader, "inColor");

  glUniformMatrix4fv(uniModel1, 1, GL_FALSE, glm::value_ptr(model));
  glUniformMatrix4fv(uniView1, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(uniProj1, 1, GL_FALSE, glm::value_ptr(proj));
  glUniform3fv(uniColor1, 1, glm::value_ptr(colVec));

  glDrawArrays(GL_LINES, 0, SPHFluid::BOX_VERTICES);

  // Draw particles
  glUseProgram(particleShader);

  GLint uniModel = glGetUniformLocation(particleShader, "model");
  GLint uniColor = glGetUniformLocation(particleShader, "inColor");

  glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
  glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

  colVec = glm::vec3(0.05, 0.35, 1);
  glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

  glDrawArrays(GL_POINTS, SPHFluid::BOX_VERTICES, fluid->NumParticles());

  // Draw cloth
  if (ss) {
    glUseProgram(phongShader);

    GLint uniModel2 = glGetUniformLocation(phongShader, "model");
    GLint uniColor2 = glGetUniformLocation(phongShader, "inColor");
    GLint uniTexID = glGetUniformLocation(phongShader, "texID");

    glUniformMatrix4fv(uniModel2, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(uniView2, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uniProj2, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(uniColor2, 1, glm::value_ptr(colVec));
    glUniform1i(uniTexID, 0);

    glBindVertexArray(vao[1]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, ss->numVertices * sizeof(float), ss->vertices,
                 GL_STREAM_DRAW);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (int i = 0; i < ss->width; i++) {
      glDrawArrays(GL_TRIANGLE_STRIP, i * 2 * (ss->height + 1),
                   2 * (ss->height + 1));
    }
  }
}

int main(int argc, char* argv[]) {
  if (audio) {
    // Initialize audio
    SDL_Init(SDL_INIT_AUDIO);

    SDL_AudioSpec spec, have;

    spec.freq = 44100;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples = 4096;
    spec.callback = CallBack;
    spec.userdata = NULL;
    dev = SDL_OpenAudioDevice(NULL, 0, &spec, &have,
                              SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    if (dev == 0) {
      printf("Couldn't open audio: %s\n", SDL_GetError());
    } else {
      if (have.format != spec.format) {
        printf("We didn't get Float32 audio format\n");
      }
      SDL_PauseAudioDevice(dev, 0);
      // SDL_Delay(5000);
      // SDL_CloseAudioDevice(dev);
    }
  }

  srand(time(NULL));

  cam =
      new Camera(glm::vec3(0, 2, 5), glm::vec3(0, 0.5, 0), glm::vec3(0, 1, 0));

  if (argc == 1) {
    fluid = new SPHFluid(ss, heat);
  } else if (argc > 1 && !strncmp(argv[1], "cloth", 5)) {
    ss = new SpringSystem(21, 10);
    fluid = new SPHFluid(ss, heat);
  }

  SDL_Init(SDL_INIT_VIDEO);  // Initialize Graphics (for OpenGL)

  // Ask SDL to get a recent version of OpenGL (3 or greater)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  // Create a window (offsetx, offsety, width, height, flags)
  SDL_Window* window = SDL_CreateWindow("SPH Fluid", 0, 0, screenWidth,
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
    printf("Version:  %s\n", glGetString(GL_VERSION));
    printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
  } else {
    printf("ERROR: Failed to initialize OpenGL context.\n");
    return -1;
  }

  //// Allocate Texture 0 (Gravel) ///////
  std::string tex0name = TEXTURE_DIR + "/merica.bmp";
  SDL_Surface* surface = SDL_LoadBMP(tex0name.c_str());
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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGRA,
               GL_UNSIGNED_BYTE, surface->pixels);
  glGenerateMipmap(GL_TEXTURE_2D);

  SDL_FreeSurface(surface);
  //// End Allocate Texture ///////

  // Build a Vertex Array Object. This stores the VBO and attribute mappings in
  // one object
  glGenVertexArrays(NUM_VAO, vao);  // Create a VAO
  glBindVertexArray(vao[0]);  // Bind the created VAO to the current context

  // Allocate memory on the graphics card to store geometry (vertex buffer
  // object)
  glGenBuffers(NUM_VBO, vbo);             // Create 1 buffer called vbo
  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);  // Set the vbo as the active array
  // buffer (Only one buffer can be
  // active at a time)

  particleShader = InitShader(SHADER_DIR + "/particle_vertex.glsl",
                              SHADER_DIR + "/particle_fragment.glsl");

  // Tell OpenGL how to set fragment shader input
  GLint posAttrib = glGetAttribLocation(particleShader, "position");
  GLint heatAttrib = glGetAttribLocation(particleShader, "heat");
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
  glVertexAttribPointer(heatAttrib, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  // Attribute, vals/attrib., type, normalized?, stride, offset
  // Binds to VBO current GL_ARRAY_BUFFER
  glEnableVertexAttribArray(posAttrib);
  glEnableVertexAttribArray(heatAttrib);

  glUseProgram(particleShader);
  GLint uniUseHeat = glGetUniformLocation(particleShader, "useHeat");
  glUniform1i(uniUseHeat, heat);

  uniView = glGetUniformLocation(particleShader, "view");
  uniProj = glGetUniformLocation(particleShader, "proj");

  lineShader = InitShader(SHADER_DIR + "/line_vertex.glsl",
                          SHADER_DIR + "/line_fragment.glsl");

  GLint posAttrib1 = glGetAttribLocation(lineShader, "position");
  glVertexAttribPointer(posAttrib1, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        0);
  // Attribute, vals/attrib., type, normalized?, stride, offset
  // Binds to VBO current GL_ARRAY_BUFFER
  glEnableVertexAttribArray(posAttrib1);

  uniView1 = glGetUniformLocation(lineShader, "view");
  uniProj1 = glGetUniformLocation(lineShader, "proj");

  glBindVertexArray(vao[1]);
  glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);

  phongShader = InitShader(SHADER_DIR + "/phong_vertex.glsl",
                           SHADER_DIR + "/phong_fragment.glsl");

  // Tell OpenGL how to set fragment shader input
  GLint posAttrib2 = glGetAttribLocation(phongShader, "position");
  glVertexAttribPointer(posAttrib2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        0);
  // Attribute, vals/attrib., type, normalized?, stride, offset
  // Binds to VBO current GL_ARRAY_BUFFER
  glEnableVertexAttribArray(posAttrib2);

  GLint normAttrib = glGetAttribLocation(phongShader, "inNormal");
  glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(normAttrib);

  GLint texAttrib = glGetAttribLocation(phongShader, "inTexcoord");
  glEnableVertexAttribArray(texAttrib);
  glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void*)(6 * sizeof(float)));

  uniView2 = glGetUniformLocation(phongShader, "view");
  uniProj2 = glGetUniformLocation(phongShader, "proj");

  glBindVertexArray(0);  // Unbind the VAO in case we want to create a new one

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
  glEnable(0x8861);

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
    // float delta = (timePast - tLastFrame) / 1000.f;
    float delta = 1 / 240.;

    while (SDL_PollEvent(&windowEvent)) {  // inspect all events in the queue
      if (windowEvent.type == SDL_QUIT) quit = true;
      // List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch
      // many special keys  Scancode referes to a keyboard position, keycode
      // referes to the letter (e.g., EU keyboards)
      // cam->processMouseWheel(&windowEvent);
    }

    const Uint8* keyState = SDL_GetKeyboardState(NULL);
    cam->processMovement(keyState, delta);

    // Clear the screen to default color
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    view = cam->getLookAt();

    proj =
        glm::perspective(cam->fov, screenWidth / (float)screenHeight, frustNear,
                         frustFar);  // FOV, aspect, near, far

    drawGeometry(delta);
    fluid->update(delta);

    if (saveOutput) Win2PPM(screenWidth, screenHeight);

    SDL_GL_SwapWindow(window);  // Double buffering

    // FPS Counter
    frame++;
    t1 = SDL_GetTicks();
    if (t1 - t0 >= 1000) {
      char buf[100];
      sprintf(buf, "SPH Fluid | FPS: %.4f", frame / ((t1 - t0) / 1000.f));
      SDL_SetWindowTitle(window, buf);
      t0 = t1;
      frame = 0;
    }

    // Remove bubbles whose audio have already been playing for 1.5 seconds from
    // the list.
    SDL_LockAudioDevice(dev);
    std::vector<bubbleSound>::iterator bub = bubbles.begin();
    for (int i = 0; bub != bubbles.end(); i++) {
      // printf("%d: %f\n", i, bub->audioPosition);
      if (bub->audioPosition > 1.0f) {
        bub = bubbles.erase(bub);
      } else {
        bub++;
      }
    }
    SDL_UnlockAudioDevice(dev);
  }

  // Clean Up
  delete fluid;
  delete ss;
  delete cam;

  glDeleteProgram(particleShader);
  glDeleteProgram(lineShader);
  glDeleteProgram(phongShader);
  glDeleteBuffers(NUM_VBO, vbo);
  glDeleteVertexArrays(NUM_VAO, vao);

  SDL_GL_DeleteContext(context);
  SDL_Quit();
  return 0;
}

// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(std::string shaderFile) {
  FILE* fp;
  long length;
  char* buffer;

  // open the file containing the text of the shader code
  fp = fopen(shaderFile.c_str(), "r");

  // check for errors in opening the file
  if (fp == NULL) {
    printf("can't open shader source file %s\n", shaderFile.c_str());
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
GLuint InitShader(std::string vShaderFileName, std::string fShaderFileName) {
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
    printf("Failed to read from vertex shader file %s\n",
           vShaderFileName.c_str());
    exit(1);
  } else if (DEBUG_ON) {
    printf("Vertex Shader:\n=====================\n");
    printf("%s\n", vs_text);
    printf("=====================\n\n");
  }
  if (fs_text == NULL) {
    printf("Failed to read from fragent shader file %s\n",
           fShaderFileName.c_str());
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

std::vector<tinyobj::real_t> loadModel(const char* filename) {
  std::string inputfile = filename;
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string err;
  bool ret =
      tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputfile.c_str());
  if (!err.empty()) {  // `err` may contain warning message.
    std::cerr << err << std::endl;
  }

  if (!ret) {
    exit(1);
  }

  std::vector<tinyobj::real_t> model;

  // Loop over shapes
  for (size_t s = 0; s < shapes.size(); s++) {
    // Loop over faces(polygon)
    size_t index_offset = 0;
    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
      unsigned fv = shapes[s].mesh.num_face_vertices[f];
      // Loop over vertices in the face.
      for (size_t v = 0; v < fv; v++) {
        tinyobj::real_t vx, vy, vz, nx, ny, nz, tx, ty;
        // access to vertex
        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
        vx = attrib.vertices[3 * idx.vertex_index + 0];
        vy = attrib.vertices[3 * idx.vertex_index + 1];
        vz = attrib.vertices[3 * idx.vertex_index + 2];
        if (attrib.normals.size()) {
          nx = attrib.normals[3 * idx.normal_index + 0];
          ny = attrib.normals[3 * idx.normal_index + 1];
          nz = attrib.normals[3 * idx.normal_index + 2];
        } else {
          nx = 0.;
          ny = 0.;
          nz = 0.;
        }
        if (attrib.texcoords.size()) {
          tx = attrib.texcoords[2 * idx.texcoord_index + 0];
          ty = attrib.texcoords[2 * idx.texcoord_index + 1];
        } else {
          tx = 0.;
          ty = 0.;
        }
        tinyobj::real_t data[8] = {vx, vy, vz, tx, ty, nx, ny, nz};
        model.insert(model.end(), data, data + 6);
      }
      index_offset += fv;
    }
  }
  return model;
}

// Write out PPM image from screen
void Win2PPM(int width, int height) {
  char outdir[] = "out/";  // Must be defined!
  int i, j;
  FILE* fptr;
  static int counter = 0;
  char fname[32];
  unsigned char* image;

  /* Allocate our buffer for the image */
  image = (unsigned char*)malloc(3 * width * height * sizeof(unsigned char));
  if (image == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate memory for image\n");
  }

  /* Open the file */
  sprintf(fname, "%simage_%04d.ppm", outdir, counter);
  if ((fptr = fopen(fname, "w")) == NULL) {
    fprintf(stderr, "ERROR: Failed to open file to write image\n");
  }

  /* Copy the image into our buffer */
  glReadBuffer(GL_BACK);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);

  /* Write the PPM file */
  fprintf(fptr, "P6\n%d %d\n255\n", width, height);
  for (j = height - 1; j >= 0; j--) {
    for (i = 0; i < width; i++) {
      fputc(image[3 * j * width + 3 * i + 0], fptr);
      fputc(image[3 * j * width + 3 * i + 1], fptr);
      fputc(image[3 * j * width + 3 * i + 2], fptr);
    }
  }

  free(image);
  fclose(fptr);
  counter++;
}
