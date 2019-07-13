#include "glad/glad.h"  //Include order can matter here

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

#include "camera.h"
#include "crowd.h"

Camera* cam;
Crowd* c;

static const float frustNear = 0.25;
static const float frustFar = 50;

int screenWidth = 800;
int screenHeight = 600;

bool DEBUG_ON = false;
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);

GLuint vao[2], vbo[2];
int texturedShader, dumbShader;

int numVertsFloor, numVertsAgent, numVertsObstacle;

glm::mat4 view, proj;
GLint uniView, uniView1;
GLint uniProj, uniProj1;

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
        model.insert(model.end(), data, data + 8);
      }
      index_offset += fv;
    }
  }
  return model;
}

void drawGeometry() {
  glm::mat4 model = glm::mat4();
  glm::vec3 colVec = glm::vec3(0, 1, 0);

  // glBindVertexArray(vao[1]);
  // glUseProgram(dumbShader);

  // GLint uniColor1 = glGetUniformLocation(dumbShader, "inColor");
  // GLint uniModel1 = glGetUniformLocation(dumbShader, "model");

  // // Roadmap points
  // // model = glm::translate(model, glm::vec3(0.5, 0, 0.5));
  // glUniformMatrix4fv(uniModel1, 1, GL_FALSE, glm::value_ptr(model));
  // glUniform3fv(uniColor1, 1, glm::value_ptr(colVec));
  // glUniformMatrix4fv(uniView1, 1, GL_FALSE, glm::value_ptr(view));
  // glUniformMatrix4fv(uniProj1, 1, GL_FALSE, glm::value_ptr(proj));

  // glPointSize(4);
  // glDrawArrays(GL_LINES, 0, 2 * c->g->numEdges);
  // // for (long unsigned i = 6 * c->g->numEdges;
  // //      i < 6 * c->g->numEdges + 3 * c->g->v.size(); i += 3) {
  // //   printf("Drawing Vertex at index %lu, %f %f %f\n", i, c->mapData[i],
  // //          c->mapData[i + 1], c->mapData[i + 2]);
  // // }
  // // printf("\n");
  // glDrawArrays(GL_POINTS, 2 * c->g->numEdges, c->g->v.size());

  glBindVertexArray(vao[0]);
  glUseProgram(texturedShader);

  GLint uniModel = glGetUniformLocation(texturedShader, "model");
  GLint uniColor = glGetUniformLocation(texturedShader, "inColor");
  GLint uniTexID = glGetUniformLocation(texturedShader, "texID");
  glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
  glUniform1i(uniTexID, -1);

  // Floor
  for (int i = 0; i < c->width; i++) {
    for (int j = 0; j < c->height; j++) {
      if ((i + j) % 2)
        colVec = glm::vec3(1, 1, 1) * 0.3f;
      else
        colVec = glm::vec3(1, 1, 1) * 0.1f;
      glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

      model = glm::mat4();
      model = glm::translate(model, glm::vec3(i, -0.01, j));
      glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

      glDrawArrays(GL_TRIANGLES, 0, numVertsFloor);
    }
  }

  // Agents
  for (int i = 0; i < c->numAgents; i++) {
    model = glm::mat4();
    model = glm::translate(model, c->pos[i] + glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(c->rad, 1, c->rad));
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
    // colVec = glm::vec3(1, 0, 0);
    switch (i % 4) {
      case 0:
        colVec = glm::vec3(1, 0, 0);
        break;
      case 1:
        colVec = glm::vec3(1, 1, 0);
        break;
      case 2:
        colVec = glm::vec3(0, 1, 1);
        break;
      case 3:
        colVec = glm::vec3(0, 1, 0);
        break;
    }
    glUniform3fv(uniColor, 1, glm::value_ptr(colVec));
    glDrawArrays(GL_TRIANGLES, numVertsFloor, numVertsAgent);
  }

  // Obstacles
  for (int i = 0; i < c->numObstacles; i++) {
    model = glm::mat4();
    model = glm::translate(model, c->oPos[i] + glm::vec3(0, 1, 0));
    model = glm::scale(model, glm::vec3(c->oRad[i], 1, c->oRad[i]));
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
    colVec = glm::vec3(0, 0, 1);
    glUniform3fv(uniColor, 1, glm::value_ptr(colVec));
    glDrawArrays(GL_TRIANGLES, numVertsFloor, numVertsObstacle);
  }
}

int main(int argc, char* argv[]) {
  // Graph* g = new Graph();
  // g->addVertex(new Vertex(glm::vec3(0, 0, 0)));
  // g->addVertex(new Vertex(glm::vec3(-3, 0, 0)));
  // g->addVertex(new Vertex(glm::vec3(-3, 5, 0)));
  // g->addVertex(new Vertex(glm::vec3(-1, 3, 0)));
  // g->addVertex(new Vertex(glm::vec3(0, 6, 0)));
  // g->addVertex(new Vertex(glm::vec3(4, 0, 0)));
  // g->addVertex(new Vertex(glm::vec3(4, 2, 0)));
  // g->addVertex(new Vertex(glm::vec3(4, 6, 0)));
  // g->connect(g->v[0], g->v[1]);
  // g->connect(g->v[0], g->v[3]);
  // g->connect(g->v[0], g->v[5]);
  // g->connect(g->v[0], g->v[6]);
  // g->connect(g->v[2], g->v[3]);
  // g->connect(g->v[3], g->v[4]);
  // g->connect(g->v[4], g->v[7]);
  // g->connect(g->v[5], g->v[6]);
  // g->connect(g->v[6], g->v[7]);
  // g->aStarSearch(g->v[0], g->v[7]);
  // exit(0);

  srand(time(NULL));
  SDL_Init(SDL_INIT_VIDEO);  // Initialize Graphics (for OpenGL)

  // Ask SDL to get a recent version of OpenGL (3 or greater)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  // Create a window (offsetx, offsety, width, height, flags)
  SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 0, 0, screenWidth,
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

  c = new Crowd(20);
  // printf("graph has %d vertices, %d edges\n", c->g->v.size(),
  // c->g->numEdges);

  // Load Models
  std::vector<tinyobj::real_t> floorModel = loadModel("models/floor.obj");
  numVertsFloor = floorModel.size() / 8;
  std::vector<tinyobj::real_t> agentModel = loadModel("models/cylinder.obj");
  numVertsAgent = agentModel.size() / 8;
  std::vector<tinyobj::real_t> obstacleModel = loadModel("models/cylinder.obj");
  numVertsObstacle = obstacleModel.size() / 8;
  int totalNumVerts = numVertsFloor + numVertsAgent + numVertsObstacle;
  std::vector<tinyobj::real_t> modelData;
  modelData.reserve(floorModel.size() + agentModel.size());
  modelData.insert(modelData.end(), floorModel.begin(), floorModel.end());
  modelData.insert(modelData.end(), agentModel.begin(), agentModel.end());
  modelData.insert(modelData.end(), obstacleModel.begin(), obstacleModel.end());

  //// Allocate Texture 0 (Gravel) ///////
  SDL_Surface* surface = SDL_LoadBMP("textures/brick.bmp");
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
  SDL_Surface* surface1 = SDL_LoadBMP("textures/wood.bmp");
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

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface1->w, surface1->h, 0, GL_BGR,
               GL_UNSIGNED_BYTE, surface1->pixels);

  glGenerateMipmap(GL_TEXTURE_2D);

  SDL_FreeSurface(surface1);
  //// End Allocate Texture ///////

  // Build a Vertex Array Object. This stores the VBO and attribute mappings in
  // one object
  glGenVertexArrays(2, vao);  // Create a VAO
  glBindVertexArray(
      vao[0]);  // Bind the above created VAO to the current context

  // Allocate memory on the graphics card to store geometry (vertex buffer
  // object)
  glGenBuffers(2, vbo);                   // Create 1 buffer called vbo
  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);  // Set the vbo as the active array
                                          // buffer (Only one buffer can be
                                          // active at a time)
  glBufferData(GL_ARRAY_BUFFER, totalNumVerts * 8 * sizeof(float),
               &modelData[0],
               GL_STATIC_DRAW);  // upload vertices to vbo
  // GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW =
  // geometry changes infrequently  GL_STREAM_DRAW = geom. changes frequently.
  // This effects which types of GPU memory is used

  texturedShader = InitShader("src/shaders/phong_vertex.glsl",
                              "src/shaders/phong_fragment.glsl");

  // Tell OpenGL how to set fragment shader input
  GLint posAttrib = glGetAttribLocation(texturedShader, "position");
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
  // Attribute, vals/attrib., type, normalized?, stride, offset
  // Binds to VBO current GL_ARRAY_BUFFER
  glEnableVertexAttribArray(posAttrib);

  GLint normAttrib = glGetAttribLocation(texturedShader, "inNormal");
  glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void*)(5 * sizeof(float)));
  glEnableVertexAttribArray(normAttrib);

  GLint texAttrib = glGetAttribLocation(texturedShader, "inTexcoord");
  glEnableVertexAttribArray(texAttrib);
  glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void*)(3 * sizeof(float)));

  uniView = glGetUniformLocation(texturedShader, "view");
  uniProj = glGetUniformLocation(texturedShader, "proj");

  glBindVertexArray(vao[1]);
  glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
  // printf("Making point buffer with %d floats:\n", c->numRoadmapPoints * 3);
  // for (int i = 0; i < c->numRoadmapPoints * 3; i += 3) {
  //   printf("%f %f %f\n", c->pointData[i], c->pointData[i + 1],
  //          c->pointData[i + 2]);
  // }
  glBufferData(GL_ARRAY_BUFFER,
               (c->numRoadmapPoints + 2 * c->g->numEdges) * 3 * sizeof(float),
               c->mapData, GL_STATIC_DRAW);  // upload vertices to vbo

  dumbShader = InitShader("src/shaders/dumb_vertex.glsl",
                          "src/shaders/dumb_fragment.glsl");

  GLint posAttrib1 = glGetAttribLocation(dumbShader, "position");
  glVertexAttribPointer(posAttrib1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                        0);
  // Attribute, vals/attrib., type, normalized?, stride, offset
  // Binds to VBO current GL_ARRAY_BUFFER
  glEnableVertexAttribArray(posAttrib1);

  uniView1 = glGetUniformLocation(dumbShader, "view");
  uniProj1 = glGetUniformLocation(dumbShader, "proj");

  glBindVertexArray(0);  // Unbind the VAO in case we want to create a new one

  glEnable(GL_DEPTH_TEST);

  cam = new Camera(glm::vec3(-20, 20, 3), glm::vec3(10.5, 0.5, 10.5),
                   glm::vec3(0, 1, 0));

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
      if (windowEvent.type == SDL_KEYUP &&
          windowEvent.key.keysym.sym == SDLK_ESCAPE)
        SDL_SetRelativeMouseMode(SDL_FALSE);
      // cam->processMouseWheel(&windowEvent);
    }

    const Uint8* keyState = SDL_GetKeyboardState(NULL);
    cam->processMovement(keyState, delta);

    // Clear the screen to default color
    glClearColor(0.5, 0.6, 0.7, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    view = cam->getLookAt();

    proj =
        glm::perspective(cam->fov, screenWidth / (float)screenHeight, frustNear,
                         frustFar);  // FOV, aspect, near, far

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex0);
    glUniform1i(glGetUniformLocation(texturedShader, "tex0"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex1);
    glUniform1i(glGetUniformLocation(texturedShader, "tex1"), 1);

    drawGeometry();

    c->update(delta);

    SDL_GL_SwapWindow(window);  // Double buffering

    // FPS Counter
    frame++;
    t1 = SDL_GetTicks();
    if (t1 - t0 > 1000) {
      printf("Average Frames Per Second: %.4f\r", frame / ((t1 - t0) / 1000.f));
      fflush(stdout);
      t0 = t1;
      frame = 0;
    }
  }

  // Clean Up
  delete cam;

  glDeleteProgram(texturedShader);
  glDeleteProgram(dumbShader);
  glDeleteBuffers(2, vbo);
  glDeleteVertexArrays(2, vao);

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
