#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <fstream>
#include <string>
#include <random>
#include <algorithm>
#include <cmath>

std::random_device rd;
std::mt19937 gen(rd());


const float SPEED_MIN = 0.35f;
const float SPEED_MAX = 0.55f;

const float SPACE_X[2] = { -0.7f, -0.3f };   // 두 공간의 중심 x
const float SPACE_HALF_W = 0.15f;              // 공간 반폭
const float SPACE_TOP = 0.9f;
const float SPACE_BOTTOM = -0.9f;

const float BAND_CX = -0.5f;             // 점선 띠(판정 구간)
const float BAND_CY = 0.0f;
const float BAND_HALF_W = 0.4f;
const float BAND_HALF_H = 0.12f;

const float RECT_HALF_W = 0.1f;              // 사각형 반폭/반높이
const float RECT_HALF_H = 0.05f;

const float TOWER_X = 0.6f;              // 탑의 x 위치
const int   MAX_STACK = 18;                // (1.8 / 0.1) 탑에 쌓을 수 있는 최대 개수
const double STACK_TIME = 0.8;               // 쌓이는 애니메이션 시간(초)


std::uniform_real_distribution<float> disSpeed{ SPEED_MIN, SPEED_MAX };

std::uniform_real_distribution<float> disColor{ 0.2f, 1.0f };
std::uniform_int_distribution<int>    disDir{ 0, 1 };

#define WIDTH  1400.0
#define HEIGHT 1400.0

/*
실습 12. 화면의 도형 이동 맞추기
- 직사각형 공간 2개, 각 공간에서 사각형이 위아래로 움직인다.
- 두 사각형이 같은 높이대(점선 띠)에 들어왔을 때 엔터: 위아래 이동을 멈추고 우측으로 이동해 탑으로 쌓인다.
- 다시 새로운 사각형이 나타나 이동한다.
- r: 리셋, q: 종료
*/



enum RectState { MOVING, STACKING, STACKED };

struct Rect
{
    glm::vec2 pos{};
    glm::vec3 rgb{};
    float vy{};
    RectState state{ MOVING }; // 기본적으론 계속 이동중이니까

    glm::vec2 startPos{};     // 쌓이기 시작할 때 위치 (한 번만 기록)
    glm::vec2 targetPos{};    // 탑에서의 목표 위치
    double startTime{};
};

struct Mesh
{
    GLuint vao{}, vbo{};
    int vertexCount{};
};

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
std::string filetobuf(const char* file);

Mesh makeMesh(const std::vector<float>& data);
void InitMeshes();
void SpawnPair();
void ResetGame();
bool InBand(const Rect& r);
void TryStack();
void UpdateRects(double now, float dt);
void DrawMesh(const Mesh& m, GLenum mode, glm::vec2 offset, glm::vec2 size, glm::vec3 color);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
GLvoid DrawScene();

GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;

GLint offsetLoc{};
GLint offsetSize{};
GLint colorLoc{};

Mesh meshSquare, meshOutline, meshBand;

std::array<Rect, 2> active;          // 지금 움직이는(또는 쌓이는 중인) 두 사각형
std::vector<Rect>   stacked;         // 탑에 쌓인 사각형들
bool finished{ false };              // 탑이 가득 참


int main()
{
    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow((int)WIDTH, (int)HEIGHT, "First Try: Window", nullptr, nullptr);
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

    glViewport(0, 0, (int)WIDTH, (int)HEIGHT);

    glfwSetKeyCallback(window, KeyCallback);

    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();

    offsetLoc = glGetUniformLocation(shaderProgramID, "uOffset");
    offsetSize = glGetUniformLocation(shaderProgramID, "uSize");
    colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    InitMeshes();
    ResetGame();

    double prevFrame = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {

        double now = glfwGetTime();
        float dt = (float)(now - prevFrame);
        prevFrame = now;
        dt = std::min(dt, 0.05f);        // 창을 끌거나 멈췄다 돌아왔을 때 튀는 것 방지

        UpdateRects(now, dt);

        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}


void make_vertexShaders()
{
    //--- 셰이더코드읽어오기
    std::string vertexSource = filetobuf("VS.glsl");
    const char* source = vertexSource.c_str();

    //--- 셰이더생성하기
    vertexShader = glCreateShader(GL_VERTEX_SHADER);

    //--- 셰이더에코드연결하고컴파일하기
    glShaderSource(vertexShader, 1, &source, NULL);
    glCompileShader(vertexShader);
    GLint result;
    GLchar errorLog[512];

    //--- 에러 체크하기
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
    //--- 셰이더코드읽어오기
    std::string fragmentSource = filetobuf("PS.glsl");
    const char* source = fragmentSource.c_str();

    //--- 셰이더생성하기
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    //--- 셰이더에코드연결하고컴파일하기
    glShaderSource(fragmentShader, 1, &source, NULL);
    glCompileShader(fragmentShader);
    GLint result;
    GLchar errorLog[512];

    //--- 에러체크하기
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

    //--- 셰이더프로그램생성
    GLuint shaderID = glCreateProgram();

    //--- 버텍스셰이더와프래그먼트셰이더연결
    glAttachShader(shaderID, vertexShader);
    glAttachShader(shaderID, fragmentShader);

    //--- 셰이더프로그램링크
    glLinkProgram(shaderID);

    //--- 링크가끝났으므로셰이더객체삭제
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    //--- 링크 성공여부확인
    glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
    if (!result) {
        glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
        std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
        return 0;
    }

    //--- 셰이더프로그램사용
    glUseProgram(shaderID);
    return shaderID;
}


std::string filetobuf(const char* file)
{
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open())
    {
        return "";
    }
    //--- 파일전체를문자열로읽기
    std::string source((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
    shaderFile.close();
    return source;
}


// --- 정점 데이터(x,y 쌍)로 VAO/VBO 하나 만들기 ---
Mesh makeMesh(const std::vector<float>& data)
{
    Mesh m;
    m.vertexCount = (int)(data.size() / 2);

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    return m;
}



static void addDashedEdge(std::vector<float>& v, glm::vec2 a, glm::vec2 b, int n)
{
    for (int i = 0; i < n; ++i)
    {
        glm::vec2 p0 = a + (b - a) * ((float)i / n);
        glm::vec2 p1 = a + (b - a) * ((i + 0.5f) / n);
        v.insert(v.end(), { p0.x, p0.y, p1.x, p1.y });
    }
}



void InitMeshes()
{
    meshSquare = makeMesh({ -1.0f,-1.0f,  1.0f,-1.0f, -1.0f, 1.0f,
                             1.0f,-1.0f,  1.0f, 1.0f, -1.0f, 1.0f });
    meshOutline = makeMesh({ -1.0f,-1.0f,  1.0f,-1.0f,  1.0f, 1.0f, -1.0f, 1.0f });   // GL_LINE_LOOP용

    // 점선 사각형(판정 띠): 가로 변은 20조각, 세로 변은 4조각
    std::vector<float> dashed;

    // dashed를 계속 업데이트해나감.

    addDashedEdge(dashed, { -1.0f,  1.0f }, { 1.0f,  1.0f }, 20); //맨 위쪽 가로 점선
    addDashedEdge(dashed, { 1.0f,  1.0f }, { 1.0f, -1.0f }, 4);
    addDashedEdge(dashed, { 1.0f, -1.0f }, { -1.0f, -1.0f }, 20);
    addDashedEdge(dashed, { -1.0f, -1.0f }, { -1.0f,  1.0f }, 4);
    meshBand = makeMesh(dashed);
}


// 두 공간에 새 사각형을 하나씩 만든다 (속도, 방향, 시작 높이, 색은 랜덤)
void SpawnPair()
{
    float minY = SPACE_BOTTOM + RECT_HALF_H;
    float maxY = SPACE_TOP - RECT_HALF_H;
    std::uniform_real_distribution<float> disY{ minY, maxY };

    for (int i = 0; i < 2; ++i)
    {
        Rect r;
        //r.pos = { SPACE_X[i], disY(gen) };
        
        r.pos = { SPACE_X[i], 0.8f };
        
        r.rgb = { disColor(gen), disColor(gen), disColor(gen) };
        //r.vy = disSpeed(gen) * (disDir(gen) ? 1.0f : -1.0f);

        r.vy = 0.85f * 1.0f;

        r.state = MOVING;
        active[i] = r;
    }
}


void ResetGame()
{
    stacked.clear();
    finished = false;
    SpawnPair();
}


bool InBand(const Rect& r)
{
    float rectTop = r.pos.y + RECT_HALF_H;
    float rectBottom = r.pos.y - RECT_HALF_H;

    float bandTop = BAND_CY + BAND_HALF_H;
    float bandBottom = BAND_CY - BAND_HALF_H;

    
    return rectBottom <= bandTop && rectTop >= bandBottom;
}



void TryStack()
{
    if (finished) return;

    for (const Rect& r : active)
    {
        if (r.state != MOVING || !InBand(r)) return;    // 실패는 그냥 무시
    }

    double now = glfwGetTime();
    float baseY = SPACE_BOTTOM + RECT_HALF_H;

    for (int i = 0; i < 2; ++i)
    {
        int level = (int)stacked.size() + i;            // 탑에서 몇 번째 칸인가
        active[i].state = STACKING;
        active[i].startPos = active[i].pos;
        active[i].startTime = now;
        active[i].targetPos = { TOWER_X, baseY + level * (RECT_HALF_H * 2.0f) };
    }
}


void UpdateRects(double now, float dt)
{
    if (finished) return;

    float minY = SPACE_BOTTOM + RECT_HALF_H;
    float maxY = SPACE_TOP - RECT_HALF_H;

    for (Rect& r : active)
    {
        if (r.state == MOVING)
        {
            r.pos.y += r.vy * dt;
            if (r.pos.y > maxY) { r.pos.y = maxY; r.vy = -r.vy; }
            if (r.pos.y < minY) { r.pos.y = minY; r.vy = -r.vy; }
        }
        else if (r.state == STACKING)
        {
            float progress = (float)((now - r.startTime) / STACK_TIME);
            progress = std::min(progress, 1.0f);

            r.pos = r.startPos + (r.targetPos - r.startPos) * progress;

            if (progress >= 1.0f) r.state = STACKED;
        }
    }

    // 둘 다 탑에 도착했으면 탑에 확정하고 새 사각형 생성
    if (active[0].state == STACKED && active[1].state == STACKED)
    {
        stacked.push_back(active[0]);
        stacked.push_back(active[1]);

        if ((int)stacked.size() >= MAX_STACK) finished = true;
        else SpawnPair();   // 끝났으니까 다시 사각형 생성
    }
}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return; // 누르는 순간만 처리

    switch (key)
    {
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_ENTER:
    case GLFW_KEY_KP_ENTER:
        TryStack();
        break;
    case GLFW_KEY_R:
        ResetGame();
        break;
    }
}


void DrawMesh(const Mesh& m, GLenum mode, glm::vec2 offset, glm::vec2 size, glm::vec3 color)
{
    glBindVertexArray(m.vao);
    glUniform2f(offsetLoc, offset.x, offset.y);
    glUniform2f(offsetSize, size.x, size.y);
    glUniform3f(colorLoc, color.x, color.y, color.z);
    glDrawArrays(mode, 0, m.vertexCount);
}


void DrawScene()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);


    glUseProgram(shaderProgramID);

    glLineWidth(2.0f);

    // 1) 두 공간 테두리
    for (int i = 0; i < 2; ++i)
    {
        float halfH = (SPACE_TOP - SPACE_BOTTOM) / 2.0f;
        float cy = (SPACE_TOP + SPACE_BOTTOM) / 2.0f;
        DrawMesh(meshOutline, GL_LINE_LOOP, { SPACE_X[i], cy }, { SPACE_HALF_W, halfH }, { 0.2f, 0.4f, 0.7f });
    }

    // 2) 점선 띠. 두 사각형이 모두 띠 안이면 초록색으로 알려준다.
    bool ready = !finished;
    for (const Rect& r : active)
    {
        if (r.state != MOVING || !InBand(r)) ready = false;
    }
    glm::vec3 bandColor = ready ? glm::vec3(0.0f, 0.7f, 0.2f) : glm::vec3(0.2f, 0.4f, 0.7f);
    DrawMesh(meshBand, GL_LINES, { BAND_CX, BAND_CY }, { BAND_HALF_W, BAND_HALF_H }, bandColor);

    // 3) 탑에 쌓인 사각형
    for (const Rect& r : stacked)
    {
        DrawMesh(meshSquare, GL_TRIANGLES, r.pos, { RECT_HALF_W, RECT_HALF_H }, r.rgb);
    }

    // 4) 쌓이는 중인 사각형
    if (!finished)
    {
        for (const Rect& r : active)
        {
            DrawMesh(meshSquare, GL_TRIANGLES, r.pos, { RECT_HALF_W, RECT_HALF_H }, r.rgb);
        }
    }
}