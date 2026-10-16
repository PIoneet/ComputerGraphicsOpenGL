#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <random>
#include <algorithm>
#include <initializer_list>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>


std::random_device rd;
std::mt19937 gen(rd());

std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };
std::uniform_int_distribution<int> disRandCube{ 0,5 };
std::uniform_int_distribution<int> disRandPyramid{ 0,3 };


#define WIDTH  1200
#define HEIGHT 1200


struct Mesh {
    GLuint vao{}, vbo{};
    int vertexCount{};
};

// ---------- 선언 ----------
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
std::string filetobuf(const char* file);

std::vector<float> MakeAxisData();
std::vector<float> MakeCubeData();
std::vector<float> MakePyramidData();
Mesh makeMesh(const std::vector<float>& data);
void DeleteMesh(Mesh& m);


glm::mat4 MakeModelMatrix();
void SendModel(const glm::mat4& m);

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void CharCallback(GLFWwindow* window, unsigned int codepoint);

void UpdateAnimation();
void DrawScene();

// ---------- 전역 ----------
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;
GLint  modelLoc{ -1 };


Mesh axisMesh, cubeMesh, pyramidMesh;


bool showCube{ true };

// 라디안이 아니라 각도 기준이다. ex) 45.0f
float angleX{};
float angleY{};

// x축 y축 회전 방향
float dirX{};
float dirY{};

// 이동량
float posX{};
float posY{};

bool depthOn{true}; // 처음은 은면제거가 켜진 상태이다. 
double prevFrame{};

// 추가로 있으면 좋은 전역 변수들
const float spinSpeed{ 90.0f };   // 초당 도
const float moveStep{ 0.05f };    // 방향키 한 번당 이동량


int main()
{
    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Practice 13", nullptr, nullptr);
    if (!window) {
        std::cerr << "윈도우생성실패!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);


    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패!" << std::endl;
        return -1;
    }


    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    glEnable(GL_DEPTH_TEST);                       // 윈도우 생성 후 한 번

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCharCallback(window, CharCallback);

    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();

    modelLoc = glGetUniformLocation(shaderProgramID, "modelTransform");

    axisMesh = makeMesh(MakeAxisData());
    cubeMesh = makeMesh(MakeCubeData());
    pyramidMesh = makeMesh(MakePyramidData());
       


    prevFrame = glfwGetTime();  // 시작 시점에 업데이트

    while (!glfwWindowShouldClose(window)) {

        UpdateAnimation();
        DrawScene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    DeleteMesh(axisMesh);
    DeleteMesh(cubeMesh);
    DeleteMesh(pyramidMesh);
    glDeleteProgram(shaderProgramID);

    glfwDestroyWindow(window);
    glfwTerminate();
}


// ---------- 셰이더 (예시 코드 그대로) ----------
void make_vertexShaders()
{
    std::string vertexSource = filetobuf("VS.glsl");
    const char* source = vertexSource.c_str();

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &source, NULL);
    glCompileShader(vertexShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
        std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
}

void make_fragmentShaders()
{
    std::string fragmentSource = filetobuf("PS.glsl");
    const char* source = fragmentSource.c_str();

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &source, NULL);
    glCompileShader(fragmentShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
        std::cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
}

GLuint make_shaderProgram()
{
    GLint result;
    GLchar errorLog[512];

    GLuint shaderID = glCreateProgram();
    glAttachShader(shaderID, vertexShader);
    glAttachShader(shaderID, fragmentShader);
    glLinkProgram(shaderID);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
    if (!result) {
        glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
        std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
        return 0;
    }

    glUseProgram(shaderID);
    return shaderID;
}



std::string filetobuf(const char* file)
{
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open())
        return "";
    std::string source((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
    shaderFile.close();
    return source;
}




// ---------- 정점 데이터 (x,y,z,r,g,b) ----------
std::vector<float> MakeAxisData()
{

    std::vector<float> axisData{ 1,0,0 , 1,0,0, -1,0,0, 1,0,0,
                                0,1,0 , 0,1,0, 0,-1,0, 0,1,0,
                                0,0,1 , 0,0,1, 0,0,-1, 0,0,1 };
    return axisData;
}



std::vector<float> MakeCubeData()
{

    std::vector<glm::vec3> pos{ { -0.3,-0.3,0.3}, {0.3,-0.3,0.3}, {0.3,0.3,0.3}, {-0.3,0.3,0.3},
        {-0.3,-0.3,-0.3}, {0.3,-0.3,-0.3}, {0.3,0.3,-0.3}, {-0.3,0.3,-0.3} };

    std::vector<std::vector<int>> faceIdx{ {0, 1, 2, 3},
                                           {0, 4, 5, 1},
                                           {3, 2, 6, 7},
                                           {0, 4, 7, 3},
                                           {1, 5, 6, 2},
                                           {4, 7, 6, 5}
    };

    std::vector<glm::vec3> vertexColor{ 
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)} };


    std::vector<float> cubeData{};
    for (int i = 0; i < 6; ++i)
    {
        int a = faceIdx[i][0];
        int b = faceIdx[i][1];
        int c = faceIdx[i][2];
        int d = faceIdx[i][3];

        std::vector<int> order{ a,b,c,a,c,d };

        for (int j = 0; j < 6; ++j)
        {
            auto& p = pos[order[j]];
            auto& cp = vertexColor[order[j]];

            cubeData.insert(cubeData.end(), { p.x, p.y, p.z, cp.x, cp.y, cp.z });

        }
    }

    return cubeData;
}



std::vector<float> MakePyramidData()
{

    std::vector<glm::vec3> pos{ 
            {-0.3,-0.3,0.3}, {-0.3,-0.3,-0.3}, {0.3,-0.3,-0.3},
            {0.3,-0.3,0.3}, {0.0,0.3, 0.0} };

    std::vector<std::vector<int>> faceIdx{ {0, 1, 4},
                                           {1, 4, 2},
                                           {3, 2, 4},
                                           {0, 3, 4}
    };

    std::vector<glm::vec3> faceBottom{ pos[0],pos[1],pos[2], pos[0],pos[2],pos[3] };

    std::vector<glm::vec3> vertexColor{ 
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)}
    };

    std::vector<glm::vec3> vertexBtmColor{
            vertexColor[0], vertexColor[1], vertexColor[2],
            vertexColor[0], vertexColor[2], vertexColor[3]
    };

    std::vector<float> pyramidData{};
    for (int i = 0; i < 4; ++i)
    {
        int a = faceIdx[i][0];
        int b = faceIdx[i][1];
        int c = faceIdx[i][2];

        std::vector<int> order{ a,b,c };

        for (int j = 0; j < 3; ++j)
        {
            auto& p = pos[order[j]];
            auto& cp = vertexColor[order[j]];

            pyramidData.insert(pyramidData.end(), { p.x, p.y, p.z, cp.x, cp.y, cp.z });
        }
    }


    // 마지막 받침 사각형 정보 넣기
    for (int i = 0; i < 6; ++i)
    {
        auto& btm = faceBottom[i];
        auto& cbtm = vertexBtmColor[i];

        pyramidData.insert(pyramidData.end(), { btm.x, btm.y, btm.z, cbtm.x, cbtm.y, cbtm.z });
    }


    return pyramidData;
}



// ---------- VAO/VBO ----------
Mesh makeMesh(const std::vector<float>& data)
{
    Mesh m;
    m.vertexCount = (int)(data.size() / 6);
    // x,y,z,r,g,b 하나를 묶어서 정점 1개니까 6으로 나누는게 타당하다. 

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    // 0은 location =0 의미하고 3은 정점 size 위치 속성이 float 3개니까 6 *는 stride 의미함.

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return m;
}



void DeleteMesh(Mesh& m)
{
    glDeleteVertexArrays(1, &m.vao);
    glDeleteBuffers(1, &m.vbo);
    m = Mesh{};
}



glm::mat4 MakeModelMatrix()
{

    glm::mat4 base(1.0f);

    base = glm::rotate(base, glm::radians(-30.0f), { 1,0,0 });
    base = glm::rotate(base, glm::radians(-30.0f), { 0,1,0 });
    base = glm::translate(base, { posX,posY,0 });
    base = glm::rotate(base, glm::radians(angleX), { 1,0,0 });
    base = glm::rotate(base, glm::radians(angleY), { 0,1,0 });
    

    return base;
}

// 30.0f는 라디안 1710도인가 이상하다. 변환해서 넣어줘야함.


glm::mat4 MakeTilMatrix()
{

    glm::mat4 base(1.0f);
    base = glm::rotate(base, glm::radians(30.0f), { 1,0,0 });
    base = glm::rotate(base, glm::radians(30.0f), { 0,1,0 });

    return base;
}




void SendModel(const glm::mat4& m)
{
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));

    // &m[0][0]과 glm::value_ptr(m)은 같다. float 16개 배열의 시작 주소로 인지함.
}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE) return;   // PRESS, REPEAT 허용

    // if(action != PRESS) return; 하면 REPEAT 즉 꾹 누르는 경우도 막는다.

    switch (key)
    {
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    //x축 이동
    case GLFW_KEY_LEFT:
        posX -= moveStep;
        break;
    case GLFW_KEY_RIGHT:
        posX += moveStep;
        break;

    //y축 이동
    case GLFW_KEY_UP:
        posY += moveStep;
        break;
    case GLFW_KEY_DOWN:
        posY -= moveStep;
        break;
    }

}


void CharCallback(GLFWwindow* window, unsigned int codepoint)
{
    switch (codepoint)
    {
    case 'w': glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break; // 와이어 객체
    case 'W': glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break; // 솔리드 객체
    case 'x': dirX = 1; dirY = 0; break;
    case 'X': dirX = -1; dirY = 0; break;
    case 'y': dirY = 1; dirX = 0; break;
    case 'Y': dirY = -1; dirX = 0; break;
    case 'c': showCube = true;  break;
    case 'p': showCube = false; break;
    case 's':
        dirX = 0.0f;
        dirY = 0.0f;
        angleX = 0.0f;
        angleY = 0.0f;
        posX = 0.0f;
        posY = 0.0f;
        break;
    case 'h':
        
        depthOn = !depthOn;

        if (depthOn)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
        break;
    }
    
}



void UpdateAnimation()
{
    double currentFrame = glfwGetTime();

    double dealth = currentFrame - prevFrame;

    angleX = angleX + dirX * dealth * spinSpeed;
    
    angleY = angleY + dirY * dealth * spinSpeed;

    prevFrame = currentFrame;

}


// ---------- 그리기 ----------
void DrawScene()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   // 깊이 버퍼도 함께 clear

    glUseProgram(shaderProgramID);

  
    SendModel(MakeTilMatrix());
    glLineWidth(3.0f);
    glBindVertexArray(axisMesh.vao);

    glDrawArrays(GL_LINES, 0, axisMesh.vertexCount);

    SendModel(MakeModelMatrix());
    // 기울어진 행렬 Uniform으로 먼저 보낸다.

    if (showCube) // 육면체 그리기
    {
        glBindVertexArray(cubeMesh.vao);
      
        glDrawArrays(GL_TRIANGLES, 0, cubeMesh.vertexCount);

            // first를 i*6으로 둬야 만약 i가 2이면 12니까 12번쨰 위치의 정점에서 6개 읽음.
            //이미 이 단계에서는 정점 1개를 x y z r g b로 인지하고 있음.
            // 12라는 건 이 정점들 12개 뒤를 애기함.
        

    }
    else  //사각뿔 그리기
    {
        glBindVertexArray(pyramidMesh.vao);

        glDrawArrays(GL_TRIANGLES, 0, pyramidMesh.vertexCount);
        
    }

}