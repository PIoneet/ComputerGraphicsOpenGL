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

glm::mat4 MakeTiltMatrix();
void SendModel(const glm::mat4& m);

void ClearFaces();
void ShowCubeFaces(std::initializer_list<int> faces);
void ShowPyramidFaces(std::initializer_list<int> faces);
void PickRandomCubeFaces();
void PickBottomAndRandomSide();
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void DrawScene();

// ---------- 전역 ----------
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;
GLint  modelLoc{ -1 };

Mesh axisMesh, cubeMesh, pyramidMesh;

bool showCube{ true };
std::array<bool, 6> cubeFace{};      // 면 i를 그릴지
std::array<bool, 5> pyramidFace{};   // 0~3: 옆면, 4: 바닥


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

    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();

    modelLoc = glGetUniformLocation(shaderProgramID, "modelTransform");

    axisMesh = makeMesh(MakeAxisData());
    cubeMesh = makeMesh(MakeCubeData());
    pyramidMesh = makeMesh(MakePyramidData());

    ShowCubeFaces({ 0 });                          // 초기 상태

    while (!glfwWindowShouldClose(window)) {
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
    // TODO: 축 3개 x 정점 2개 = 6개. x 빨강, y 초록, z 파랑. (-1,0,0)~(1,0,0) 식으로 양방향

    std::vector<float> axisData{1,0,0 , 1,0,0, -1,0,0, 1,0,0,
                                0,1,0 , 0,1,0, 0,-1,0, 0,1,0,
                                0,0,1 , 0,0,1, 0,0,-1, 0,0,1};
    return axisData;
}



std::vector<float> MakeCubeData()
{
    // TODO: pos[8], face[6][4], faceColor[6] 정의
    //       면마다 삼각형 2개 (a,b,c / a,c,d) -> 총 36개 정점. 면 f의 정점은 f*6 ~ f*6+5

    std::vector<glm::vec3> pos{ { -0.3,-0.3,0.3}, {0.3,-0.3,0.3}, {0.3,0.3,0.3}, {-0.3,0.3,0.3},
        {-0.3,-0.3,-0.3}, {0.3,-0.3,-0.3}, {0.3,0.3,-0.3}, {-0.3,0.3,-0.3} };

    std::vector<std::vector<int>> faceIdx{ {0, 1, 2, 3},
                                           {0, 4, 5, 1},
                                           {3, 2, 6, 7},
                                           {0, 4, 7, 3},
                                           {1, 5, 6, 2},
                                           {4, 7, 6, 5}
    };

    std::vector<glm::vec3> faceColor{ {disColor(gen), disColor(gen), disColor(gen)},
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
            glm::vec3 rgb = faceColor[i];

            cubeData.insert(cubeData.end(), { p.x, p.y, p.z, rgb.x, rgb.y, rgb.z });

        }
    }

    return cubeData;
}



std::vector<float> MakePyramidData()
{
    // TODO: pos[5], side[4][3], bottom[4], 색 정의
    //       옆면 4x3=12개 먼저, 바닥 6개 마지막 -> 총 18개

    std::vector<glm::vec3> pos{ { -0.3,-0.3,0.3}, {-0.3,-0.3,-0.3}, {0.3,-0.3,-0.3}, 
        {0.3,-0.3,0.3}, {0.0,0.3, 0.0}};

    std::vector<std::vector<int>> faceIdx{ {0, 1, 4},
                                           {1, 4, 2},
                                           {3, 2, 4},
                                           {0, 3, 4}
    };

    std::vector<glm::vec3> faceBottom{ pos[0],pos[1],pos[2], pos[0],pos[2],pos[3]};

    std::vector<glm::vec3> faceColor{ {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)},
        {disColor(gen), disColor(gen), disColor(gen)}
    };

    std::vector<glm::vec3> faceBtmColor{ {disColor(gen), disColor(gen), disColor(gen)} };

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
            glm::vec3 rgb = faceColor[i];

            pyramidData.insert(pyramidData.end(), { p.x, p.y, p.z, rgb.x, rgb.y, rgb.z });
        }
    }

    // 마지막 받침 사각형 정보 넣기
    for (int i = 0; i < 6; ++i)
    {
        auto& btm = faceBottom[i];
        glm::vec3 rgb = faceBtmColor[0];

        pyramidData.insert(pyramidData.end(), { btm.x, btm.y, btm.z, rgb.x, rgb.y, rgb.z });
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
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    return m;
}



void DeleteMesh(Mesh& m)
{
    glDeleteVertexArrays(1, &m.vao);
    glDeleteBuffers(1, &m.vbo);
    m = Mesh{};
}


// ---------- 변환 ----------
glm::mat4 MakeTiltMatrix()
{
    // TODO: 단위 행렬에서 시작, x축 30도 -> y축 30도 순서로 rotate (정점에는 Ry가 먼저 적용)
    glm::mat4 base(1.0f);

    base = glm::rotate(base, glm::radians(30.0f), {1,0,0});
    base = glm::rotate(base, glm::radians(30.0f), { 0,1,0 });

    return base;
}

// 30.0f는 라디안 1710도인가 이상하다. 변환해서 넣어줘야함.



void SendModel(const glm::mat4& m)
{
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));

    // &m[0][0]과 glm::value_ptr(m)은 같다. float 16개 배열의 시작 주소로 인지함.
}



// ---------- 키 처리 ----------
void ClearFaces()
{
    // TODO: cubeFace, pyramidFace 전부 false

    for (int i = 0; i < cubeFace.size(); ++i)
    {
        cubeFace[i] = false;
    }

    for (int j = 0; j < pyramidFace.size(); ++j)
    {
        pyramidFace[j] = false;
    }
}



// initalizer_list는 임시 배열 {}을 따로 복사 할당 안하고 있는 그대로 읽고 보여줌. 
void ShowCubeFaces(std::initializer_list<int> faces)
{
    // TODO: ClearFaces(), showCube = true, 해당 면만 true
    ClearFaces(); 
    // 먼저 싹다 false로 초기화한다.
    showCube = true;

	for (size_t i = 0; i < faces.size(); ++i)
	{
		cubeFace[*(faces.begin() + i)] = true;
	}

    

}



void ShowPyramidFaces(std::initializer_list<int> faces)
{
    // TODO: ClearFaces(), showCube = false, 해당 면만 true
    ClearFaces();

    showCube = false;

	for (size_t i = 0; i < faces.size(); ++i)
	{
		pyramidFace[*(faces.begin() + i)] = true;
	}

}


void PickRandomCubeFaces()
{
    // TODO: 0~5 중 서로 다른 2개를 뽑아 ShowCubeFaces 호출 (c 키)

    int a = disRandCube(gen);
    int b = disRandCube(gen);

    while (a == b)
        b = disRandCube(gen);
    
    ShowCubeFaces({a, b});
    // rand 알고리즘으로 뽑으면 중복도 나와서 주의가 필요함.
}



void PickBottomAndRandomSide()
{
    // TODO: 바닥(4) + 옆면 0~3 중 하나를 뽑아 ShowPyramidFaces 호출 (t 키)

    int side = disRandPyramid(gen);

    ShowPyramidFaces({ side , 4 });
}




void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return;

    // TODO: Q 종료
    // TODO: 1~6 -> ShowCubeFaces({n-1})
    // TODO: 7,8,9,0 -> ShowPyramidFaces({0~3 중 해당 번호})
    // TODO: C -> PickRandomCubeFaces(), T -> PickBottomAndRandomSide()

    switch (key)
    {
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_1:
        ShowCubeFaces({ 0 });
        break;
    case GLFW_KEY_2:
        ShowCubeFaces({ 1 });
        break;
    case GLFW_KEY_3:
        ShowCubeFaces({ 2 });
        break;
    case GLFW_KEY_4:
        ShowCubeFaces({ 3 });
        break;
    case GLFW_KEY_5:
        ShowCubeFaces({ 4 });
        break;
    case GLFW_KEY_6:
        ShowCubeFaces({ 5 });
        break;

    case GLFW_KEY_7:
        ShowPyramidFaces({0});
        break;
    case GLFW_KEY_8:
        ShowPyramidFaces({1});
        break;
    case GLFW_KEY_9:
        ShowPyramidFaces({ 2 });
        break;
    case GLFW_KEY_0:
        ShowPyramidFaces({ 3 });
        break;

    case GLFW_KEY_C:
        PickRandomCubeFaces();
        break;

    case GLFW_KEY_T:
        PickBottomAndRandomSide();
        break;
    }

}



// ---------- 그리기 ----------
void DrawScene()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   // 깊이 버퍼도 함께 clear

    glUseProgram(shaderProgramID);

    // TODO: 축 -> SendModel(단위 행렬), bind axis VAO, glDrawArrays(GL_LINES, 0, 6)
    // TODO: SendModel(MakeTiltMatrix())
    // TODO: showCube면 cubeMesh bind 후 for i<6: cubeFace[i]면 glDrawArrays(GL_TRIANGLES, i*6, 6)
    // TODO: 아니면 pyramidMesh bind 후 for i<5:
    //       i<4 -> first=i*3, count=3 / i==4 -> first=12, count=6

    // 좌표축 먼저 그리기.
    SendModel(glm::mat4(1.0f));
    glLineWidth(3.0f);
    glBindVertexArray(axisMesh.vao);

    glDrawArrays(GL_LINES, 0, axisMesh.vertexCount);

    SendModel(MakeTiltMatrix());
    // 기울어진 행렬 Uniform으로 먼저 보낸다.

    if (showCube) // 육면체 그리기
    {
        glBindVertexArray(cubeMesh.vao);
        for (int i = 0; i < cubeFace.size(); ++i)
        {
            if (cubeFace[i] == false)
                continue;

            glDrawArrays(GL_TRIANGLES, i*6, 6);

            // first를 i*6으로 둬야 만약 i가 2이면 12니까 12번쨰 위치의 정점에서 6개 읽음.
            //이미 이 단계에서는 정점 1개를 x y z r g b로 인지하고 있음.
            // 12라는 건 이 정점들 12개 뒤를 애기함.
        }

    }
    else  //사각뿔 그리기
    {
        glBindVertexArray(pyramidMesh.vao);

        for (int j = 0; j < pyramidFace.size(); ++j)
        {
            if (pyramidFace[j] == false)
                continue;

            
            if(j < 4 ) glDrawArrays(GL_TRIANGLES, j * 3, 3);
            else glDrawArrays(GL_TRIANGLES, j * 3, 6);
            
            

        }
    }

}