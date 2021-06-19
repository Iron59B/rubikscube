/* standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <array>
#include <vector>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

/* We use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* SOIL is used for loading (texture) images */
#include <SOIL.h>
/* GLFW is used for creating and manipulating graphics windows */
#include<GLFW/glfw3.h>
/* GLNM is used for handling vector and matrix math */
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "rubikscube/cube.h"

#define GLSL(src) "#version 330 core\n" #src
#define GLM_FORCE_RADIANS

using namespace std;

int MIDDLE = 0; //+0 -> Front, +9 -> Middle, +18 -> Back
int LEFT = 1;
int RIGHT = 2;
int TOP = 3;
int BOTTOM = 4;
int TOP_LEFT = 5;
int TOP_RIGHT = 6;
int BOTTOM_LEFT = 7;
int BOTTOM_RIGHT = 8;

static int MIDDLE_MIDDLE = MIDDLE+9; //+0 -> Front, +9 -> Middle, +18 -> Back
static int LEFT_MIDDLE = LEFT+9;
static int RIGHT_MIDDLE = RIGHT+9;
static int TOP_MIDDLE = TOP+9;
static int BOTTOM_MIDDLE = BOTTOM+9;
static int TOP_LEFT_MIDDLE = TOP_LEFT+9;
static int TOP_RIGHT_MIDDLE = TOP_RIGHT+9;
static int BOTTOM_LEFT_MIDDLE = BOTTOM_LEFT+9;
static int BOTTOM_RIGHT_MIDDLE = BOTTOM_RIGHT+9;

static int MIDDLE_BACK = MIDDLE+18; //+0 -> Front, +9 -> Middle, +18 -> Back
static int LEFT_BACK = LEFT+18;
static int RIGHT_BACK = RIGHT+18;
static int TOP_BACK = TOP+18;
static int BOTTOM_BACK = BOTTOM+18;
static int TOP_LEFT_BACK = TOP_LEFT+18;
static int TOP_RIGHT_BACK = TOP_RIGHT+18;
static int BOTTOM_LEFT_BACK = BOTTOM_LEFT+18;
static int BOTTOM_RIGHT_BACK = BOTTOM_RIGHT+18;

static int nrRotations = 0;
static int j = 0;

static GLint uniformAnim;
static array<array<GLfloat,6*36>,27> vtxArray;
/*                                                                           */
/* GLFW callback functions for event handling                                */
/*                                                                           */
static void errorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}


static void keyCallback(GLFWwindow* myWindow, int key, int scanCode,
                        int action, int mod)
{
    if (((key == GLFW_KEY_ESCAPE) || (key == GLFW_KEY_Q))  &&
        (action == GLFW_PRESS))
    /* close window upon hitting the escape key or Q/q */
        glfwSetWindowShouldClose(myWindow, GL_TRUE);
}

static void cursorPosCallBack (GLFWwindow* myWindow, double x_pos, double y_pos)
{
    printf("Mouse is at (%6.1f, %6.1f) \n", x_pos, y_pos);
}

static void mouseButtonCallBack(GLFWwindow* myWindow, int button, int action, int mods) {
    if((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_PRESS)) {
        double x_pos, y_pos;
        glfwGetCursorPos(myWindow, &x_pos, &y_pos);
        printf("Left mouse button pressed at (%6.1f, %6.1f) \n", x_pos, y_pos);
    }
}


bool checkShaderCompileStatus(GLuint shaderID)
{
    GLint status;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length);
        GLchar* log = new char[length + 1];
        glGetShaderInfoLog(shaderID, length, &length, &log[0]);
        fprintf(stderr, "%s", log);
        return false;
    }
    return true;
}


bool checkShaderProgramLinkStatus(GLuint programID)
{
    GLint status;
    glGetProgramiv(programID, GL_LINK_STATUS, &status);
    if(status == GL_FALSE) {
        GLint length;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &length);
        GLchar* log = new char[length + 1];
        glGetProgramInfoLog(programID, length, &length, &log[0]);
        fprintf(stderr, "%s", log);
        return false;
    }
    return true;
}

glm::vec3* getDirectionRightUp(GLFWwindow* myWindow, glm::vec3 position, GLfloat horizontalAngle, GLfloat verticalAngle, GLfloat initialFoV, GLfloat speed, GLfloat mouseSpeed, GLfloat deltaTime)
{
    static glm::vec3 look[3];
    double x_pos, y_pos;

    glfwGetCursorPos(myWindow, &x_pos, &y_pos);
    //glfwSetCursorPos(myWindow, 800/2, 600/2);

    horizontalAngle += mouseSpeed * deltaTime * float(800/2 - x_pos);
    verticalAngle   += mouseSpeed * deltaTime * float(600/2 - y_pos);

    glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );

    glm::vec3 right = glm::vec3(
        sin(horizontalAngle - 3.14f/2.0f),
        0,
        cos(horizontalAngle - 3.14f/2.0f)
    );

    glm::vec3 up = glm::cross(right, direction);

    if (glfwGetKey(myWindow, GLFW_KEY_UP) == GLFW_PRESS){
        position += direction * deltaTime * speed;
    }
    // Move backward
    if (glfwGetKey(myWindow, GLFW_KEY_DOWN) == GLFW_PRESS){
        position -= direction * deltaTime * speed;
    }
    // Strafe right
    if (glfwGetKey(myWindow, GLFW_KEY_RIGHT) == GLFW_PRESS){
        position += right * deltaTime * speed;
    }
    // Strafe left
    if (glfwGetKey(myWindow, GLFW_KEY_LEFT) == GLFW_PRESS){
        position -= right * deltaTime * speed;
    }

    //float FoV = initialFoV - 5 * glfwGetMouseWheel(myWindow);
    look[0] = direction;
    look[1] = right;
    look[2] = up;

    return look;
}

void changeCubePositions(int row, int direction) {
  int tmp_top = TOP;
  int tmp_topleft = TOP_LEFT;
  int tmp_topright = TOP_RIGHT;

  if (direction == 1 && row == 0) {

    TOP_RIGHT = BOTTOM_RIGHT;
    std::cout << "TOPRIGHT: " << TOP_RIGHT << std::endl;
    TOP_LEFT = tmp_topright;
    std::cout << "TOPLEFT: " << TOP_LEFT << std::endl;
    TOP = RIGHT;
    std::cout << "TOP: " << TOP << std::endl;
    BOTTOM_RIGHT = BOTTOM_LEFT;
    std::cout << "BOTTOMRIGHT: " << BOTTOM_RIGHT << std::endl;
    BOTTOM_LEFT = tmp_topleft;
    std::cout << "BOTTOMLEFT: " << BOTTOM_LEFT << std::endl;
    RIGHT = BOTTOM;
    std::cout << "RIGHT: " << RIGHT << std::endl;
    BOTTOM = LEFT;
    std::cout << "BOTTOM: " << BOTTOM << std::endl;
    LEFT = tmp_top;
    std::cout << "LEFT: " << LEFT << std::endl;
  }
}

void createAnim(GLuint shaderProgram, glm::mat4 anim) {
  const char* uniformName = "anim";
  uniformAnim = glGetUniformLocation(shaderProgram, uniformName);
  if (uniformAnim == -1) {
      fprintf(stderr, "Error: could not bind uniform %s\n", uniformName);
      exit(EXIT_FAILURE);
  }
  glUniformMatrix4fv(uniformAnim, 1, GL_FALSE, glm::value_ptr(anim));
}

glm::mat4 spinObj(glm::mat4 anim, float orientation) {
    float angle = 1.0f * orientation;

    anim = glm::rotate(anim, glm::radians(angle), glm::vec3(0.0f, 0.0f, 2.0f));

    return anim;
}

glm::mat4 spinObj2(glm::mat4 anim, float orientation, glm::vec3 rot) {
    float angle = 1.0f * orientation;

    anim = glm::translate(anim, glm::vec3(0.0f, 0.0f, -2.1f) );
    anim = glm::rotate(anim, glm::radians(angle), rot);
    anim = glm::translate(anim, -(glm::vec3(0.0f, 0.0f, -2.1f)) );


    return anim;
}

glm::mat4 spinRight(glm::mat4 anim, float orientation, int i) {
    glm::vec3 rot;

    if(i == RIGHT || i == TOP_RIGHT || i == BOTTOM_RIGHT
        || i == RIGHT_MIDDLE || i == TOP_RIGHT_MIDDLE || i == BOTTOM_RIGHT_MIDDLE
        || i == RIGHT_BACK || i == TOP_RIGHT_BACK || i == BOTTOM_RIGHT_BACK)
    {
        // createAnim(shaderProgram, anim);
        // if(nrRotations <= 90*9) {
        if(i == RIGHT || i == TOP_RIGHT || i == BOTTOM_RIGHT) {
            rot = glm::vec3(0.0f, 1.0f, 0.0f);
            orientation *= -1;
        } else {
            rot = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        anim = spinObj2(anim, orientation, rot);
            // cout << nrRotations << endl;
            nrRotations +=1;
        // }

        // glUniformMatrix4fv(uniformAnim, 1, GL_FALSE, glm::value_ptr(anim));

    }

    return anim;
}

glm::mat4 spinLeft(glm::mat4 anim, float orientation, int i) {
    if(i < 9) {
        // if(nrRotations <= 90*9) {
        anim = spinObj(anim, orientation);
        // cout << nrRotations << endl;
        nrRotations +=1;
        //    glUniformMatrix4fv(uniformAnim, 1, GL_FALSE, glm::value_ptr(anim));
        // }
    }
    // }

    return anim;
}

// glm::mat4 handleCube(GLfloat* vtx[], GLuint VAOArray[], GLuint VBOArray[], glm::mat4 anim, int arraySize, bool state) {
//     for(int i = 0; i < arraySize; i+=1) {

//         if(i == 1) {
//         //    anim = spinObj(anim, state);
//         }

//         glBufferData(GL_ARRAY_BUFFER, 6*36*4, vtx[i], GL_STATIC_DRAW);
//         glDrawArrays(GL_TRIANGLES, 0, 36);
//     }
//     return anim;
// }



void printCube(GLfloat* vtx) {
  for(int i = 0; i+5 < 6*36; i+=6) {
    printf("%f, %f, %f \n", vtx[i], vtx[i+1], vtx[i+2]);
  }
}

void printCube(std::array<GLfloat,6*36> vtx) {
  for(int i = 0; i+5 < 6*36; i+=6) {
    printf("%f, %f, %f \n", vtx[i], vtx[i+1], vtx[i+2]);
  }
}


int main()
{
    /* window dimensions */
    const GLuint WIDTH = 800, HEIGHT = 600;

    /*                                                                        */
    /* initialization and set-up                                              */
    /*                                                                        */
    /* initialization of GLFW */
    glfwSetErrorCallback(errorCallback);
    if (glfwInit() != GL_TRUE) {
        fprintf(stderr, "Cannot initialize GLFW\n");
        exit(EXIT_FAILURE);
    }

    /* set some GLFW options: we require OpenGL 3.3 (or more recent) context */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* create GFLW window (monitor in windowed mode), do not share resources */
    GLFWwindow* myWindow = glfwCreateWindow(WIDTH, HEIGHT, "Rubikscube",
                                            NULL, NULL);
    if (myWindow == NULL) {
        fprintf(stderr, "Cannot open GLFW window\n");
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(myWindow);

    /* initialization of GLEW */
    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(glewStatus));
        exit(EXIT_FAILURE);
    }

    if (!GLEW_VERSION_2_0) {
        fprintf(stderr, "Error: GPU does not support GLEW 2.0\n");
        exit(EXIT_FAILURE);
    }

    int vtxSize = 6*36;
    int arraySize = 27;
    static Cube cube[] ={
        Cube(MIDDLE, 0.0f),
        Cube(LEFT, 0.0f),
        Cube(RIGHT, 0.0f),
        Cube(TOP, 0.0f),
        Cube(BOTTOM, 0.0f),
        Cube(TOP_LEFT, 0.0f),
        Cube(TOP_RIGHT, 0.0f),
        Cube(BOTTOM_LEFT, 0.0f),
        Cube(BOTTOM_RIGHT, 0.0f),
        Cube(MIDDLE, -2.2f),
        Cube(LEFT, -2.2f),
        Cube(RIGHT, -2.2f),
        Cube(TOP, -2.2f),
        Cube(BOTTOM, -2.2f),
        Cube(TOP_LEFT, -2.2f),
        Cube(TOP_RIGHT, -2.2f),
        Cube(BOTTOM_LEFT, -2.2f),
        Cube(BOTTOM_RIGHT, -2.2f),
        Cube(MIDDLE, -4.4f),
        Cube(LEFT, -4.4f),
        Cube(RIGHT, -4.4f),
        Cube(TOP, -4.4f),
        Cube(BOTTOM, -4.4f),
        Cube(TOP_LEFT, -4.4f),
        Cube(TOP_RIGHT, -4.4f),
        Cube(BOTTOM_LEFT, -4.4f),
        Cube(BOTTOM_RIGHT, -4.4f),
        // Cube(BOTTOM, 0.0f)
    };

    //std::vector<GLfloat> vtxArray;


    GLuint myVAO[arraySize];
    glGenVertexArrays(arraySize, &myVAO[0]);
    // glBindVertexArray(myVAO[0]);

    /* generate and bind one Vertex Buffer Object */
    GLuint myVBO[arraySize];
    glGenBuffers(arraySize, &myVBO[0]);

    /* copy the vertex data to it */

    for(int i = 0; i < arraySize; i++) {

       vtxArray[i] = cube[i].createCubes();

        glBindVertexArray(myVAO[i]);
        glBindBuffer(GL_ARRAY_BUFFER, myVBO[i]);
        glBufferData(GL_ARRAY_BUFFER, vtxArray[i].size()*4, &vtxArray[i], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*) (3 * sizeof(GLfloat)));
    }

    /* OpenGL settings */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* define and compile the vertex shader */
    const char* vertexShaderSource = GLSL(
      layout(location=0) in vec3 position;
      //in vec2 textureCoordIn;
      layout(location=1) in vec3 colorVtxIn;
      uniform mat4 proj;
      uniform mat4 view;
      uniform mat4 anim;
      out vec3 colorVtxOut;
      //out vec2 textureCoordOut;
      void main() {
        //textureCoordOut = vec2(textureCoordIn.x,
        //                     1.0 - textureCoordIn.y);
        colorVtxOut = colorVtxIn;
        gl_Position = proj * view * anim * vec4(position, 1.0);
    }
                                          );
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    /* check whether the vertex shader compiled without an error */
    if (!checkShaderCompileStatus(vertexShader)) {
        fprintf(stderr, "Vertex shader did not compile\n");
        exit(EXIT_FAILURE);
    }

    /* define and compile the fragment shader */
    const char* fragmentShaderSource = GLSL(
        in vec3 colorVtxOut;
        out vec4 outColor;
        void main() {
            outColor = vec4(colorVtxOut, 1.0f);
    }
                                            );
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    /* check whether the fragment shader compiled without an error */
    if (!checkShaderCompileStatus(fragmentShader)) {
        fprintf(stderr, "Fragment shader did not compile\n");
        exit(EXIT_FAILURE);
    }

    /* create a shader program by linking the vertex and fragment shader */
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);

    /* check whether the shader program linked without an error */
    if (!checkShaderProgramLinkStatus(shaderProgram)) {
        fprintf(stderr, "Shader did not link\n");
        exit(EXIT_FAILURE);
    }

    /* make the shader program active */
    glUseProgram(shaderProgram);

    /* define how the input is organized */
    const char* attributeName;
    attributeName = "position";
    GLint posAttrib = glGetAttribLocation(shaderProgram, attributeName);
    if (posAttrib == -1) {
        fprintf(stderr, "Error: could not bind attribute %s\n", attributeName);
        exit(EXIT_FAILURE);
    }
    // glEnableVertexAttribArray(posAttrib);
    // glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE,
    //                       6 * sizeof(GLfloat), 0);

    attributeName = "colorVtxIn";
    GLint colAttrib = glGetAttribLocation(shaderProgram, attributeName);
    if (colAttrib == -1) {
        fprintf(stderr, "Error: could not bind attribute %s\n", attributeName);
    }
    // glEnableVertexAttribArray(colAttrib);
    // glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE,
    //                       6 * sizeof(GLfloat), (void*) (3 * sizeof(GLfloat)));

    /*attributeName = "textureCoordIn";
     GLint texAttrib = glGetAttribLocation(shaderProgram, attributeName);
     if (texAttrib == -1) {
     fprintf(stderr, "Error: could not bind attribute %s\n", attributeName);
     exit(EXIT_FAILURE);
     }
     glEnableVertexAttribArray(texAttrib);
     glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE,
     5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));*/

    /* load texture image */
    /*GLint texWidth, texHeight;
     GLint channels;*/
    /*unsigned char* texImage = SOIL_load_image("../img/katze.png",
     &texWidth, &texHeight, &channels,
     SOIL_LOAD_RGB);
     if (texImage == NULL) {
     fprintf(stderr, "Image file could no_BUFFER, vtxSize2*4, vtx2, GL_STATIC_DRAW);

        // glDrawArrays(GL_TRIANGLES, 0, 12*36);

     }*/

    /* generate texture */
    /*GLuint textureID;
     glActiveTexture(GL_TEXTURE0);
     glGenTextures(1, &textureID);
     glBindTexture(GL_TEXTURE_2D, textureID);
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB,
     GL_UNSIGNED_BYTE, texImage);
     SOIL_free_image_data(texImage);*/

    /* set texture parameters */
    /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/

    /* define a view transformation */

    // position
    glm::vec3 position = glm::vec3( 0, 0, 5 );
    // horizontal angle : toward -Z
    GLfloat horizontalAngle = 3.14f;
    // vertical angle : 0, look at the horizon
    GLfloat verticalAngle = 0.0f;
    // Initial Field of View
    GLfloat initialFoV = 45.0f;

    GLfloat speed = 3.0f; // 3 units / second
    GLfloat mouseSpeed = 0.005f;
    GLfloat lastTime = 0;
    glm::vec3* look;
    GLfloat deltaTime = 0;


    glm::mat4 view = glm::lookAt(glm::vec3(4.0f, 4.0f, 6.0f),
                                 glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));

    //glm::mat4 view = glm::lookAt(look[0], look[1], look[2]);

    /* define a  projection transformation */
    glm::mat4 proj = glm::perspective(glm::radians(100.0f), 4.0f/3.0f, 0.1f, 40.0f);

    /* define a transformation matrix for the animation */
    glm::mat4 anim = glm::mat4(1.0f);

    glm::mat4 anim2 = glm::mat4(1.0f);

    glm::mat4 anim3 = glm::mat4(1.0f);

    glm::mat4 anim4 = glm::mat4(1.0f);

    /* bind uniforms and pass data to the shader program */
    const char* uniformName;
    /*uniformName = "textureData";
     GLint uniformTex = glGetUniformLocation(shaderProgram, uniformName);
     if (uniformTex == -1) {
     fprintf(stderr, "Error: could not bind uniform %s\n", uniformName);
     exit(EXIT_FAILURE);
     }
     glUniform1i(uniformTex, 0);*/

    uniformName = "view";
    GLint uniformView = glGetUniformLocation(shaderProgram, uniformName);
    if (uniformView == -1) {
        fprintf(stderr, "Error: could not bind uniform %s\n", uniformName);
        exit(EXIT_FAILURE);
    }
    glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));

    uniformName = "proj";
    GLint uniformProj = glGetUniformLocation(shaderProgram, "proj");
    if (uniformProj == -1) {
        fprintf(stderr, "Error: could not bind uniform %s\n", uniformName);
        exit(EXIT_FAILURE);
    }
    glUniformMatrix4fv(uniformProj, 1, GL_FALSE, glm::value_ptr(proj));

    /* register callback functions */
    glfwSetKeyCallback(myWindow, keyCallback);
    //glfwSetCursorPosCallback(myWindow, cursorPosCallBack);
    glfwSetMouseButtonCallback(myWindow, mouseButtonCallBack);

    /*                                                                        */
    /* event-handling and rendering loop                                      */
    /*                                                                        */

    bool state = true;
    glm::mat4 myAnim;
    nrRotations = 0;

    std::vector<int> moves {1,2,3};
    int move = 0;
    array<glm::mat4,27> animArray;

    for(int i = 0; i < animArray.size(); i++) {
        animArray[i] = anim;
    }

    move = 1;

    // createAnim(shaderProgram, anim2);
    while (!glfwWindowShouldClose(myWindow)) {
        /* set the window background to black */
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for(int i = 0; i < arraySize; i+=1) {   //TODO: Cube Array aufteilen mit veränderte und unveränderte Cubes IDEE!
            glBindVertexArray(myVAO[i]);
            //glBindBuffer(GL_ARRAY_BUFFER, myVBO[i]);
            myAnim = animArray[i];
            // cout << glm::to_string(myAnim) << endl;
            // createAnim(shaderProgram, anim);

            if(move == 1) {
                myAnim = spinLeft(myAnim, 1.0, i);
                animArray[i] = myAnim;
            }
            else if(move == 2) {
                myAnim = spinRight(myAnim, -1.0, i);
                // nrRotations = 0;
                animArray[i] = myAnim;

            }
            // printf("/-----------------------------------------/ \n");

            // cout << glm::to_string(myAnim) << endl;
            // printf("/-----------------------------------------/ \n");


            glUniformMatrix4fv(uniformAnim, 1, GL_FALSE, glm::value_ptr(myAnim));
            glDrawArrays(GL_TRIANGLES, 0, 6*36*4);
        }
        // printCube(vtxArray[1]);
        // printf("/-----------------------------------------/ \n");
        // printCube(vtxArray[2]);
        // printf("/-----------------------------------------/ \n");
        // break;
        // move ++;
        if(nrRotations == 90*9) {
            changeCubePositions(0, 1);
            nrRotations = 0;
            move++;
        }


        deltaTime = GLfloat(glfwGetTime() - lastTime);
        lastTime = deltaTime;

        look = getDirectionRightUp(myWindow, position, horizontalAngle, verticalAngle, initialFoV, speed, mouseSpeed, deltaTime);
        view = glm::lookAt(look[0], look[1], look[2]);

        /* Swap buffers */
        glfwSwapBuffers(myWindow);

        /* poll events */
        glfwPollEvents();

        cout << "/--------------------------------------------------/" << endl;
    }

    /*                                                                        */
    /* clean-up and release resources                                         */
    /*                                                                        */
    //glDeleteTextures(1, &textureID);

    glUseProgram(0);
    glDetachShader(shaderProgram, vertexShader);
    glDetachShader(shaderProgram, fragmentShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteProgram(shaderProgram);

    for(int i = 0; i < 5; i+=1) {
        glDeleteBuffers(arraySize,myVBO);
        // glDeleteBuffers(1, &myVBO);

        glDeleteVertexArrays(arraySize, myVAO);
    }



    /*                                                                        */
    /* termination of GLFW                                                    */
    /*                                                                        */
    glfwTerminate();

    exit(EXIT_SUCCESS);
}
