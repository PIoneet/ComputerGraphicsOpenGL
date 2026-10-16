// 실습 16 : 사각뿔 애니메이션 (6 ~ 9)
// - 셰이더(VS.glsl / PS.glsl)는 15번과 같은 것을 사용 (uniform mat4 modelTransform)
// - 육면체는 정적으로만 그린다 (15번 애니메이션은 따로 합치기)
// - TODO 표시된 곳만 채우면 된다.

#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <random>
#include <array>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };
std::uniform_int_distribution<int> disPair{ 0, 1 };   // 8번: 마주보는 쌍 고르기용 (0: 왼쪽-오른쪽, 1: 앞-뒤)

#define WIDTH  1400
#define HEIGHT 1400

struct Mesh
{
    GLuint vao{}, vbo{};
    int vertexCount{};
};


// ---------- 사각뿔 면 번호 (MakePyramidData의 faceIdx 순서와 같다) ----------
// 옆면 0~3 : 정점 i*3 ~ i*3+2 (삼각형 1개씩),  바닥 : 정점 12 ~ 17 (삼각형 2개)
enum PFace
{
    P_LEFT = 0,    // x = -0.3
    P_FRONT = 1,   // z = -0.3
    P_RIGHT = 2,   // x = +0.3
    P_BACK = 3,    // z = +0.3
    P_BOTTOM = 4
};


enum Anime
{
    NONE = 0,
    SIX = 1,
    SEVEN = 2,
    EIGHT = 3,
    NINE = 4
};

// 옆면 하나의 애니메이션 "상태" (시간에 따라 변하는 값만 둔다)
struct FaceAnim
{
    bool  active{ false };
    float angle{ 0.0f };   // 도(degree)
    float dir{ 1.0f };     // 특정 조건 만족할떄 방향 틀거나 하는 용도. 
    Anime mode{ NONE };
};

// 옆면 힌지 정보 (한 번 정해지면 안 변하는 기하 데이터 -> FaceAnim과 분리)
struct Hinge
{
    glm::vec3 pivot;   // 회전 중심이 되는 점
    glm::vec3 axis;    // 회전축 방향
    float     open;    // 바깥으로 여는 부호 (+1 / -1)

    // 아 그니까 그 회전축이 시계방향으로 돌아야 바깥쪽인지 반시계로 돌아야 바깥쪽인지 미리 판단해서 넣는값.
    // 면이 어디 방향 쪽으로 이동해야 하는지는 고정되어있는 값이다. 
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
glm::mat4 MakeObjectMatrix();
glm::mat4 MakeFaceMatrix(int face);
void SendModel(const glm::mat4& m);

void ResetFaces();
bool SwitchMode(Anime m);
void ResetAll();
void CharCallback(GLFWwindow* window, unsigned int codepoint);
void UpdateAnimation();
void DrawScene();

// ---------- 전역 ----------
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;
GLint  modelLoc{ -1 };

Mesh axisMesh, cubeMesh, pyramidMesh;

bool   showCube{ false };    // 16번은 사각뿔을 먼저 보여준다
bool   depthOn{ true };
double prevFrame{};

bool  yRotating{ false };
float angleY{ 0.0f };

// 옆면 4개 상태 (인덱스는 PFace 순서)
std::array<FaceAnim, 4> faceAnim;
Anime currentMode{ NONE };       // 지금 유효한 애니메이션 키

const float spinSpeed{ 90.0f };  // 초당 도

// TODO: 모드별 목표 각도 / 속도 상수를 정해서 채우기
const float SIX_MAX = 130.0f;
const float SEVEN_MAX = 75.0f;
const float EIGHT_ANGLE = 180.0f;
const float NINE_MAX = 130.0f;


// TODO: 힌지 표 채우기 (PFace 순서: 왼쪽, 앞, 오른쪽, 뒤)
//       pivot = 아래 모서리 중점, axis = 힌지 방향(월드 +x 또는 +z), open = 바깥으로 여는 부호
Hinge sideHinge[4] = {
    { glm::vec3(-0.3f, -0.3f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 1.0f },   // 왼쪽
    { glm::vec3(0.0f, -0.3f, -0.3f), glm::vec3(1.0f, 0.0f, 0.0f), -1.0f },   // 앞
    { glm::vec3(0.3f, -0.3f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), -1.0f },   // 오른쪽
    { glm::vec3(0.0f, -0.3f, 0.3f), glm::vec3(1.0f, 0.0f, 0.0f), 1.0f }    // 뒤
};


glm::vec3 apex{ 0.0f, 0.3f, 0.0f };   // 사각뿔 꼭짓점 (8, 9번의 피벗)


Hinge sideHingeNine[4] = {
    { apex, glm::vec3(0.0f, 0.0f, 1.0f), -1.0f },   // 왼쪽
    { apex, glm::vec3(1.0f, 0.0f, 0.0f), 1.0f },   // 앞
    { apex, glm::vec3(0.0f, 0.0f, 1.0f), 1.0f },   // 오른쪽
    { apex, glm::vec3(1.0f, 0.0f, 0.0f), -1.0f }    // 뒤
};





// TODO: 7번에서 "지금 몇 번째 면 차례인지" 같은 상태가 필요하면 여기에 추가
int sevenIdx{ 0 };


int main()
{
    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Practice 16", nullptr, nullptr);
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

    glEnable(GL_DEPTH_TEST);

    glfwSetCharCallback(window, CharCallback);

    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();

    modelLoc = glGetUniformLocation(shaderProgramID, "modelTransform");

    axisMesh = makeMesh(MakeAxisData());
    cubeMesh = makeMesh(MakeCubeData());
    pyramidMesh = makeMesh(MakePyramidData());

    prevFrame = glfwGetTime();

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


// ---------- 셰이더 ----------
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

// 육면체 (정적으로만 그림). 면마다 색 1개, 면 f의 정점은 f*6 ~ f*6+5
std::vector<float> MakeCubeData()
{
    std::vector<glm::vec3> pos{ {-0.3,-0.3,0.3}, {0.3,-0.3,0.3}, {0.3,0.3,0.3}, {-0.3,0.3,0.3},
                                {-0.3,-0.3,-0.3}, {0.3,-0.3,-0.3}, {0.3,0.3,-0.3}, {-0.3,0.3,-0.3} };

    std::vector<std::vector<int>> faceIdx{ {4, 5, 6, 7},
                                           {0, 4, 5, 1},
                                           {3, 2, 6, 7},
                                           {0, 4, 7, 3},
                                           {1, 5, 6, 2},
                                           {0, 3, 2, 1} };

    std::vector<glm::vec3> faceColor;
    for (int i = 0; i < 6; ++i)
        faceColor.push_back({ disColor(gen), disColor(gen), disColor(gen) });

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
            auto& rgb = faceColor[i];
            cubeData.insert(cubeData.end(), { p.x, p.y, p.z, rgb.x, rgb.y, rgb.z });
        }
    }
    return cubeData;
}

// 옆면 4x3 = 12개 정점 먼저, 바닥 6개 정점 마지막 -> 총 18개
// 옆면 순서: 왼쪽(P_LEFT), 앞(P_FRONT), 오른쪽(P_RIGHT), 뒤(P_BACK)
std::vector<float> MakePyramidData()
{
    std::vector<glm::vec3> pos{ {-0.3,-0.3,0.3}, {-0.3,-0.3,-0.3}, {0.3,-0.3,-0.3},
                                {0.3,-0.3,0.3}, {0.0,0.3,0.0} };

    std::vector<std::vector<int>> faceIdx{ {0, 1, 4},
                                           {1, 2, 4},
                                           {2, 3, 4},
                                           {0, 4, 3} };

    std::vector<int> bottomIdx{ 0,3,2, 0,2,1 };

    std::vector<glm::vec3> faceColor;
    for (int i = 0; i < 5; ++i)
        faceColor.push_back({ disColor(gen), disColor(gen), disColor(gen) });

    std::vector<float> pyramidData{};
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            auto& p = pos[faceIdx[i][j]];
            auto& rgb = faceColor[i];
            pyramidData.insert(pyramidData.end(), { p.x, p.y, p.z, rgb.x, rgb.y, rgb.z });
        }
    }

    // 바닥
    for (int i = 0; i < 6; ++i)
    {
        auto& p = pos[bottomIdx[i]];
        auto& rgb = faceColor[4];
        pyramidData.insert(pyramidData.end(), { p.x, p.y, p.z, rgb.x, rgb.y, rgb.z });
    }
    return pyramidData;
}


// ---------- VAO/VBO ----------
Mesh makeMesh(const std::vector<float>& data)
{
    Mesh m;
    m.vertexCount = (int)(data.size() / 6);

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
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


// ---------- 변환 ----------
glm::mat4 MakeTiltMatrix()
{
    glm::mat4 base(1.0f);
    base = glm::rotate(base, glm::radians(-30.0f), glm::vec3(1, 0, 0));
    base = glm::rotate(base, glm::radians(-30.0f), glm::vec3(0, 1, 0));
    return base;
}

// 도형 전체에 적용되는 행렬 (모든 면 공통)
glm::mat4 MakeObjectMatrix()
{
    glm::mat4 finalMatrix(1.0f);
    finalMatrix = glm::rotate(finalMatrix, glm::radians(angleY), glm::vec3{ 0,1,0 });
    return finalMatrix * MakeTiltMatrix();
}

// 옆면 face(0~3) 하나에만 적용되는 애니메이션 행렬 (면 자기 좌표계 기준)
glm::mat4 MakeFaceMatrix(int face)
{
    glm::mat4 base(1.0f);
    const FaceAnim& fa = faceAnim[face];
    const auto& side = sideHinge[face];
    const auto& sideNine = sideHingeNine[face];

    switch (fa.mode)
    {
    case SIX:
    case SEVEN:
        // TODO: 아래 모서리 중점을 피벗으로, 힌지 축으로 회전 (translate -> rotate -> translate)
        base = glm::translate(base, side.pivot);  
        base = glm::rotate(base, glm::radians(fa.angle * side.open), side.axis);  
        base = glm::translate(base, -side.pivot); // 피벗을 기준으로 상대 좌표를 구하기 위한 이동
        break;
    
    case EIGHT:
        // TODO: 꼭짓점(apex)을 피벗으로 y축 회전 (마주보는 두 면의 자리 바꿈)
        base = glm::translate(base, apex);
        base = glm::rotate(base, glm::radians(fa.angle), glm::vec3{0.0f, 1.0f, 0.0f});
        base = glm::translate(base, -apex); // 피벗을 기준으로 상대 좌표를 구하기 위한 이동
        break;
    case NINE:
        // TODO: 꼭짓점(apex)을 피벗으로, 힌지와 평행한 축으로 회전
        base = glm::translate(base, sideNine.pivot);
        base = glm::rotate(base, glm::radians(fa.angle * sideNine.open), sideNine.axis);
        base = glm::translate(base, -sideNine.pivot);
        break;

    default:
        break;
    }

    return base;
}

void SendModel(const glm::mat4& m)
{
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
}


// ---------- 상태 ----------
// 옆면 4개를 전부 기본값으로 되돌린다 (angle = 0, mode = NONE, active = false)
void ResetFaces()
{
    // 이떄 EIGHT일떄 anlge = 0 하게 안하면 유지시키게 할 수 있기는 하다.
    for (auto& face : faceAnim)
    {
        face = FaceAnim{};
    }
}


// 키 하나를 눌렀을 때 공통으로 거치는 단계
//  - m이 이미 currentMode이면: 전부 초기화하고 currentMode = NONE, false 반환 (멈춤)
//  - 다른 모드이면:           전부 초기화하고 currentMode = m,    true  반환 (새로 시작)
bool SwitchMode(Anime m)
{
    // TODO
    ResetFaces();

    if (m != currentMode)
    {
        currentMode = m;
        return true;
    }
    else //currentMode가 이미 m이었다면
        currentMode = NONE;
    // 위 코드가 있어야 NONE != currentMode가 성립해서 currentMode가 다시 m이 될 수 있다. 

    return false;
}


void ResetAll()
{
    angleY = 0.0f;
    yRotating = false;
    showCube = false;
    // TODO: 면 상태도 초기화
    ResetFaces();
    currentMode = NONE;
}


// ---------- 키 처리 ----------
void CharCallback(GLFWwindow* window, unsigned int codepoint)
{
    switch (codepoint)
    {
    case 'q': glfwSetWindowShouldClose(window, GLFW_TRUE); break;

    case 'h':
        depthOn = !depthOn;
        if (depthOn) glEnable(GL_DEPTH_TEST);
        else         glDisable(GL_DEPTH_TEST);
        break;

    case 'p':
    {
        showCube = !showCube;
        ResetFaces();
        currentMode = NONE;
        break;
    }
    // 육면체/사각뿔 전환 (전환할 때 면 상태 초기화 필요)
    case 'y': yRotating = !yRotating; break;
    case 'r': ResetAll(); break;

    case '6':
    {
        // TODO: 사각뿔의 모든 옆면을 함께 열고/닫기 (각 면의 아래 모서리 기준, 180도까지)
        // angle 업데이트는 UpdateAnimation에서 한다. 
        
        if (SwitchMode(SIX))
        { //currentMode = SIX가 됐을떄
            for (auto& fa : faceAnim)
            {
                fa.active = true;
                fa.mode = SIX;
                fa.dir = 1.0f;
            }
        }
        break;
    }
    case '7':
    {
        // TODO: 옆면이 한 개씩 번갈아 가며 열리고/닫힘 (평면까지, 90도)
        if (SwitchMode(SEVEN))
        { 
            for (int i =0; i<(int)faceAnim.size(); ++i)
            {
                if (i != sevenIdx)
                    continue;

                faceAnim[i].active = true;
                faceAnim[i].mode = SEVEN;
                faceAnim[i].dir = 1.0f;
            }
        }
        break;
    }
    case '8':
    {
        if (SwitchMode(EIGHT))
        { 
            int idx = disPair(gen); //0은 왼 오 1은 앞 뒤

            if (idx == 0)
            {
                auto& f1 = faceAnim[P_LEFT];
                auto& f2 = faceAnim[P_RIGHT];

                f1.active = true;
                f1.mode = EIGHT;
                f1.dir = 1.0f;

                f2.active = true;
                f2.mode = EIGHT;
                f2.dir = -1.0f;
            }
            else if (idx == 1)
            {
                auto& f1 = faceAnim[P_FRONT];
                auto& f2 = faceAnim[P_BACK];
                
                f1.active = true;
                f1.mode = EIGHT;
                f1.dir = 1.0f;

                f2.active = true;
                f2.mode = EIGHT;
                f2.dir = -1.0f;
            }
        }
        break;
        // TODO: 마주보는 두 옆면이 꼭짓점 기준으로 회전하여 자리를 바꿈
        break;
    }
    case '9':
    {
        // TODO: 옆면들이 꼭짓점 기준으로 바깥쪽으로 열리고/닫힘
        if (SwitchMode(NINE))
        { //currentMode = SIX가 됐을떄
            for (auto& fa : faceAnim)
            {
                fa.active = true;
                fa.mode = NINE;
                fa.dir = 1.0f;
            }
        }
        break;
    }
    }
}


// ---------- 업데이트 ----------
void UpdateAnimation()
{
    double now = glfwGetTime();
    float dt = (float)(now - prevFrame);
    prevFrame = now;

    if (yRotating)
        angleY += spinSpeed * dt;

    for (int i = 0; i < (int)faceAnim.size(); ++i)
    {
        FaceAnim& f = faceAnim[i];
        if (!f.active)
            continue;

        switch (f.mode)
        {
        case SIX: 
        {
            /* TODO */ 
            if (f.angle >= SIX_MAX)
                f.dir = -1.0f;
            else if (f.angle <= 0)
                f.dir = 1.0f;

            f.angle = f.angle + f.dir * dt * spinSpeed;

            f.angle = std::clamp(f.angle, 0.0f, SIX_MAX);

            break;
        }
        case SEVEN:
        {
            /* TODO */
            if (f.angle >= SEVEN_MAX)
                f.dir = -1.0f;
            else if (f.dir < 0.0f && f.angle <= 0)
            {
                f = FaceAnim{};
                sevenIdx = (sevenIdx + 1) % 4;
                faceAnim[sevenIdx].active = true;
                faceAnim[sevenIdx].mode = SEVEN;
                faceAnim[sevenIdx].dir = 1.0f;
                break;
            }
                
            f.angle = f.angle + f.dir * dt * spinSpeed;

            f.angle = std::clamp(f.angle, 0.0f, SEVEN_MAX);
            break;
        }
        case EIGHT:
        {
            /* TODO */
            if (f.angle >= EIGHT_ANGLE || f.angle <= -EIGHT_ANGLE)
            {
                f.active = false;
            }
            
            f.angle = f.angle + f.dir * dt * spinSpeed;

            f.angle = std::clamp(f.angle, -EIGHT_ANGLE, EIGHT_ANGLE);

            break;
        }
        case NINE:
        {
            /* TODO */
            if (f.angle >= NINE_MAX)
                f.dir = -1.0f;
            else if (f.angle <= 0)
                f.dir = 1.0f;

            f.angle = f.angle + f.dir * dt * spinSpeed;

            f.angle = std::clamp(f.angle, 0.0f, NINE_MAX);

            break;
        }
        default: break;
        }
    }
}


// ---------- 그리기 ----------
void DrawScene()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    // 축: 고정 기울기만 적용
    SendModel(MakeTiltMatrix());
    glBindVertexArray(axisMesh.vao);
    glDrawArrays(GL_LINES, 0, axisMesh.vertexCount);

    if (showCube)
    {
        // 육면체는 정적으로만 (15번 애니메이션은 별도로 합치기)
        glBindVertexArray(cubeMesh.vao);
        SendModel(MakeObjectMatrix());
        glDrawArrays(GL_TRIANGLES, 0, cubeMesh.vertexCount);
    }
    else
    {
        glBindVertexArray(pyramidMesh.vao);

        // 옆면 4개: 면마다 애니메이션 행렬 적용 (삼각형 1개 = 정점 3개)
        for (int i = 0; i < 4; ++i)
        {
            SendModel(MakeObjectMatrix() * MakeFaceMatrix(i));
            glDrawArrays(GL_TRIANGLES, i * 3, 3);
        }

        // 바닥: 애니메이션 없음 (정점 12번부터 6개)
        SendModel(MakeObjectMatrix());
        glDrawArrays(GL_TRIANGLES, 12, 6);
    }
}