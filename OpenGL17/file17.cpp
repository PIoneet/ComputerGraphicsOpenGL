// 실습 17 : 모델링 변환하기 (좌: 원뿔, 우: 육면체 / c 키로 원기둥, 구로 교체)
// - 셰이더(VS.glsl / PS.glsl)는 15, 16번과 같은 것을 사용 (uniform mat4 modelTransform)
// - 모든 도형은 "중심이 원점"인 모델로 만든다. 위치는 T(pos)로 정한다.
// - TODO 표시된 곳만 채우면 된다. (도형 데이터 / 그리기 / 셰이더는 완성)

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

#define WIDTH  1600
#define HEIGHT 1600

const float PI = 3.14159265f;
const float animDuration = 2.7f;   // 초 단위. 실행해 보고 조정

struct Mesh
{
    GLuint vao{}, vbo{};
    int vertexCount{};
};


// ---------- 도형 / 선택 / 애니메이션 ----------
enum Shape { CUBE = 0, CONE, CYLINDER, SPHERE };

// 1: 좌측만, 2: 우측만, 3: 둘 다  (변환 키가 적용될 대상)
enum Target { LEFT_SH = 0, RIGHT_SH, BOTH };

// t, u, v 애니메이션 (두 도형 모두에 적용)
enum Anime { NONE = 0, MOVE_T, MOVE_U, MOVE_V };


// TODO: 도형 하나의 "시간에 따라 변하는" 변환 상태
//       (위치, 자전 각도, 공전 각도, 크기 ... 무엇을 어떻게 나눌지 직접 정한다)
struct ObjState
{
    glm::vec3 pos{ 0.0f };
    // TODO
    // 객체의 x축 자전과 y축 자전이 둘다 있다. 
    float rotX{ 0.0f };
    float rotY{ 0.0f };
    float orbit{ 0.0f };
    float scale{ 1.0f };
    float originScale{ 1.0f }; // 원점 기준 scale값.
};


// ---------- 선언 ----------
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
std::string filetobuf(const char* file);

std::vector<float> MakeAxisData();
std::vector<float> MakeCubeData();
std::vector<float> MakeConeData();
std::vector<float> MakeCylinderData();
std::vector<float> MakeSphereData();
Mesh makeMesh(const std::vector<float>& data);
void DeleteMesh(Mesh& m);

glm::mat4 MakeTiltMatrix();
glm::mat4 MakeModelMatrix(const ObjState& st);
void SendModel(const glm::mat4& m);
bool SwitchMode(Anime m);

void ResetObjects();
void CharCallback(GLFWwindow* window, unsigned int codepoint);
void UpdateAnimation();
void DrawScene();


// ---------- 전역 ----------
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;
GLint  modelLoc{ -1 };

Mesh axisMesh, cubeMesh, coneMesh, cylinderMesh, sphereMesh;

double prevFrame{};

std::array<ObjState, 2> objs;      // [0] = 좌측 도형, [1] = 우측 도형
Target target{ BOTH };             // 1/2/3 키로 바뀌는 적용 대상
Anime  currentMode{ NONE };        // 지금 돌고 있는 t/u/v 애니메이션
bool   altShape{ false };          // c 키: false = 원뿔/육면체, true = 원기둥/구

const float spinSpeed{ 90.0f };    // 초당 도 (필요하면 다른 상수도 추가)
float animProgress{};

glm::vec3 targetPos1{};
glm::vec3 targetPos2{};
glm::vec3 startPos1{};
glm::vec3 startPos2{};


int main()
{
    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Practice 17", nullptr, nullptr);
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
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);   // 슬라이드처럼 선(와이어프레임)으로 그리기

    glfwSetCharCallback(window, CharCallback);

    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();

    modelLoc = glGetUniformLocation(shaderProgramID, "modelTransform");

    axisMesh = makeMesh(MakeAxisData());
    cubeMesh = makeMesh(MakeCubeData());
    coneMesh = makeMesh(MakeConeData());
    cylinderMesh = makeMesh(MakeCylinderData());
    sphereMesh = makeMesh(MakeSphereData());

    ResetObjects();
    prevFrame = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        UpdateAnimation();
        DrawScene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    DeleteMesh(axisMesh);
    DeleteMesh(cubeMesh);
    DeleteMesh(coneMesh);
    DeleteMesh(cylinderMesh);
    DeleteMesh(sphereMesh);
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


// ---------- 정점 데이터 (x,y,z,r,g,b) : 모든 도형은 중심이 원점 ----------
std::vector<float> MakeAxisData()
{
    std::vector<float> axisData{ 1,0,0 , 1,0,0, -1,0,0, 1,0,0,
                                 0,1,0 , 0,1,0, 0,-1,0, 0,1,0,
                                 0,0,1 , 0,0,1, 0,0,-1, 0,0,1 };
    return axisData;
}

// 검은 배경에서 안 보이는 어두운 색을 피한 랜덤 색
glm::vec3 RandColor()
{
    return { 0.3f + 0.7f * disColor(gen), 0.3f + 0.7f * disColor(gen), 0.3f + 0.7f * disColor(gen) };
}

// 삼각형 하나(정점 3개)를 data 뒤에 붙인다
void PushTri(std::vector<float>& d, const glm::vec3& a, const glm::vec3& b,
    const glm::vec3& c, const glm::vec3& rgb)
{
    for (const glm::vec3* p : { &a, &b, &c }) //a->b->c 순서대로 p가 주소를 가져옴. 
        d.insert(d.end(), { p->x, p->y, p->z, rgb.x, rgb.y, rgb.z });
}

// 육면체 (16번과 같은 방식). 면마다 색 1개, 면 f의 정점은 f*6 ~ f*6+5
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

    std::vector<float> cubeData{};
    for (int i = 0; i < 6; ++i)
    {
        glm::vec3 rgb = RandColor();
        int a = faceIdx[i][0], b = faceIdx[i][1], c = faceIdx[i][2], d = faceIdx[i][3];

        std::vector<int> order{ a,b,c,a,c,d };
        for (int j = 0; j < 6; ++j)
        {
            auto& p = pos[order[j]];
            cubeData.insert(cubeData.end(), { p.x, p.y, p.z, rgb.x, rgb.y, rgb.z });
        }
    }
    return cubeData;
}


// 실습 17 : 도형 데이터 (MakeCubeData 와 같은 방식)
//   1) pos      : 정점 위치 배열      (정점 번호 = 배열 인덱스)
//   2) faceIdx  : 면(삼각형)마다 정점 번호 3개
//   3) faceColor: 면마다 색 1개
//   4) 조립     : 면 f 의 정점 j 를 꺼내서 { 위치 3개, 색 3개 } 로 이어 붙인다
//
// 필요한 것: PI, RandColor()  (기존 틀 코드에 있음)
// 기존 PushTri 함수는 더 이상 쓰지 않으므로 지워도 된다.

// ---------- 공통 조립 함수 ----------
// faceIdx[f] 의 정점 번호 3개 -> pos 에서 위치를 꺼내고, faceColor[f] 를 붙인다.
std::vector<float> BuildFromFaces(const std::vector<glm::vec3>& pos,
    const std::vector<std::vector<int>>& faceIdx,
    const std::vector<glm::vec3>& faceColor)
{
    std::vector<float> data{};
    for (int f = 0; f < (int)faceIdx.size(); ++f)
    {
        for (int j = 0; j < 3; ++j)
        {
            auto& p = pos[faceIdx[f][j]];
            auto& rgb = faceColor[f];
            data.insert(data.end(), { p.x, p.y, p.z, rgb.x, rgb.y, rgb.z });
        }
    }
    return data;
}


// ---------- 원뿔 ----------
// 정점 번호:  0 = 꼭짓점 (0, +h, 0)
//             1 = 바닥 중심 (0, -h, 0)
//             2 + i = 바닥 원 둘레의 i번째 점 (i = 0 ~ N-1)
std::vector<float> MakeConeData()
{
    const int N = 32;                 // 원을 몇 조각으로 나눌지
    const float r = 0.3f, h = 0.3f;   // 반지름, 높이의 절반

    // 1) 정점 위치
    std::vector<glm::vec3> pos;
    pos.push_back({ 0.0f, h, 0.0f });       // 0
    pos.push_back({ 0.0f, -h, 0.0f });      // 1
    for (int i = 0; i < N; ++i)
    {
        float angle = 2 * PI * i / N;
        pos.push_back({ r * cosf(angle), -h, r * sinf(angle) });   // 2 + i
    }

    // 2) 면 + 3) 색
    glm::vec3 sideColor = RandColor();
    glm::vec3 bottomColor = RandColor();

    std::vector<std::vector<int>> faceIdx;
    std::vector<glm::vec3> faceColor;
    for (int i = 0; i < N; ++i)
    {
        int cur = 2 + i;                  // 이번 둘레 점
        int next = 2 + (i + 1) % N;       // 다음 둘레 점 (마지막이면 처음으로)

        faceIdx.push_back({ 0, cur, next });   // 옆면 삼각형: 꼭짓점 - cur - next
        faceColor.push_back(sideColor);

        faceIdx.push_back({ 1, next, cur });   // 바닥 삼각형: 중심 - next - cur
        faceColor.push_back(bottomColor);
    }

    // 4) 조립
    return BuildFromFaces(pos, faceIdx, faceColor);
}


// ---------- 원기둥 ----------
// 정점 번호:  0 = 윗면 중심 (0, +h, 0)
//             1 = 아랫면 중심 (0, -h, 0)
//             2 + i     = 윗면 둘레의 i번째 점
//             2 + N + i = 아랫면 둘레의 i번째 점
std::vector<float> MakeCylinderData()
{
    const int N = 32;
    const float r = 0.25f, h = 0.3f;

    // 1) 정점 위치
    std::vector<glm::vec3> pos;
    pos.push_back({ 0.0f, h, 0.0f });       // 0
    pos.push_back({ 0.0f, -h, 0.0f });      // 1
    for (int i = 0; i < N; ++i)             // 윗면 둘레: 2 ~ N+1
    {
        float angle = 2 * PI * i / N;
        pos.push_back({ r * cosf(angle), h, r * sinf(angle) });
    }
    for (int i = 0; i < N; ++i)             // 아랫면 둘레: N+2 ~ 2N+1
    {
        float angle = 2 * PI * i / N;
        pos.push_back({ r * cosf(angle), -h, r * sinf(angle) });
    }

    // 2) 면 + 3) 색
    glm::vec3 sideColor = RandColor();
    glm::vec3 capColor = RandColor();

    std::vector<std::vector<int>> faceIdx;
    std::vector<glm::vec3> faceColor;
    for (int i = 0; i < N; ++i)
    {
        int topCur = 2 + i;
        int topNext = 2 + (i + 1) % N;
        int botCur = 2 + N + i;
        int botNext = 2 + N + (i + 1) % N;

        // 옆면: 사각형(botCur, botNext, topNext, topCur) = 삼각형 2개
        faceIdx.push_back({ botCur, botNext, topNext });
        faceColor.push_back(sideColor);
        faceIdx.push_back({ botCur, topNext, topCur });
        faceColor.push_back(sideColor);

        // 윗뚜껑: 윗면 중심 - topCur - topNext
        faceIdx.push_back({ 0, topCur, topNext });
        faceColor.push_back(capColor);

        // 아랫뚜껑: 아랫면 중심 - botNext - botCur
        faceIdx.push_back({ 1, botNext, botCur });
        faceColor.push_back(capColor);
    }

    // 4) 조립
    return BuildFromFaces(pos, faceIdx, faceColor);
}


// ---------- 구 ----------
// 위도 i (0 = 북극 ~ stacks = 남극), 경도 j (0 ~ slices) 격자의 점이 정점이다.
// 정점 번호 = i * (slices + 1) + j   (j = slices 는 j = 0 과 같은 위치지만 번호를 따로 둔다)
std::vector<float> MakeSphereData()
{
    const int stacks = 16, slices = 32;
    const float r = 0.3f;

    // 1) 정점 위치
    std::vector<glm::vec3> pos;
    for (int i = 0; i <= stacks; ++i)
    {
        float phi = PI * i / stacks;                 // 위에서 아래로 0 ~ PI
        for (int j = 0; j <= slices; ++j)
        {
            float theta = 2 * PI * j / slices;       // 한 바퀴 0 ~ 2PI
            pos.push_back({ r * sinf(phi) * cosf(theta),
                            r * cosf(phi),
                            r * sinf(phi) * sinf(theta) });
        }
    }

    // 2) 면 + 3) 색  (격자 한 칸 = 사각형 = 삼각형 2개)
    glm::vec3 color = RandColor();

    std::vector<std::vector<int>> faceIdx;
    std::vector<glm::vec3> faceColor;
    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < slices; ++j)
        {
            int a = i * (slices + 1) + j;            // 왼쪽 위
            int b = (i + 1) * (slices + 1) + j;      // 왼쪽 아래
            int c = (i + 1) * (slices + 1) + (j + 1);// 오른쪽 아래
            int d = i * (slices + 1) + (j + 1);      // 오른쪽 위

            faceIdx.push_back({ a, b, c });
            faceColor.push_back(color);
            faceIdx.push_back({ a, c, d });
            faceColor.push_back(color);
        }
    }

    // 4) 조립
    return BuildFromFaces(pos, faceIdx, faceColor);
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
// 좌표계와 모든 도형이 공통으로 갖는 고정 기울기 (x, y축으로 30도)
glm::mat4 MakeTiltMatrix()
{
    glm::mat4 base(1.0f);
    base = glm::rotate(base, glm::radians(-30.0f), glm::vec3(1, 0, 0));
    base = glm::rotate(base, glm::radians(-30.0f), glm::vec3(0, 1, 0));
    return base;
}

// 도형 하나의 최종 모델 행렬
glm::mat4 MakeModelMatrix(const ObjState& st)
{
    glm::mat4 m(1.0f);

    // TODO: 코드 순서(왼쪽이 바깥)는 아래처럼 곱해진다.
    //   Tilt * Ry(공전) * [원점 기준 확대] * T(pos) * 자전(x, y) * [제자리 확대]
    //   - translate / rotate / scale 은 "반환값"을 대입해야 한다 (m = glm::...(m, ...))
    //   - 각 변환이 왜 그 위치에 와야 하는지 a, b, r, x, y 키별로 확인해 보기

    m = glm::rotate(m, glm::radians(st.orbit), glm::vec3{0.0f,1.0f,0.0f});
    m = glm::scale(m, glm::vec3{st.originScale, st.originScale, st.originScale});  //원점 기준 확대 축소임. 이동 후에
    m = glm::translate(m, st.pos);

    m = glm::rotate(m, glm::radians(st.rotY), glm::vec3{ 0.0f,1.0f,0.0f });
    m = glm::rotate(m, glm::radians(st.rotX), glm::vec3{ 1.0f,0.0f,0.0f });
    m = glm::scale(m, glm::vec3{ st.scale, st.scale, st.scale });

    // glm::rotate는 항상 웑점을 기준으로 회전한다. 월드공간의 원점을 기준으로
    return MakeTiltMatrix() * m;
}

void SendModel(const glm::mat4& m)
{
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
}


bool SwitchMode(Anime m)
{

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


// ---------- 상태 ----------
// 두 도형을 초기 상태로 되돌린다 (s 키, 프로그램 시작)
void ResetObjects()
{
    // TODO: 좌/우 초기 위치 (예: 왼쪽 (-0.5, 0, 0), 오른쪽 (0.5, 0, 0))와
    //       나머지 상태(각도, 크기 등)를 기본값으로. 선택 대상, 애니메이션 모드, 도형도 초기화.
    objs[0] = ObjState{};
    objs[1] = ObjState{};
    target = BOTH;
    currentMode = NONE;
    altShape = false;

    objs[0].pos = { -0.5, 0, 0 };
    objs[1].pos = { 0.5, 0, 0 };
}


// ---------- 키 처리 ----------
// 적용 대상에 따라 objs[i]에 변환을 걸어 주는 도우미 (필요하면 직접 만들어 쓰기)
bool IsTargeted(int i) 
{
    return target == BOTH || target == (Target)i; 
}


void CharCallback(GLFWwindow* window, unsigned int codepoint)
{
    switch (codepoint)
    {
    case 'q': glfwSetWindowShouldClose(window, GLFW_TRUE); break;

        // TODO: 적용 대상 선택
    case '1': 
        target = LEFT_SH;
        std::cout << target << std::endl;
        break;   // 좌측만
    case '2': 
        target = RIGHT_SH;
        std::cout << target << std::endl;
        break;   // 우측만
    case '3': 
        target = BOTH;
        std::cout << target << std::endl;
        break;   // 둘 다

        // TODO: 선택된 도형에만 적용 (대문자 = 반대 방향)
    case 'x':
    {
        bool obj1{ IsTargeted(0) };
        bool obj2{ IsTargeted(1) };
        if (obj1 && obj2)
        {
            for (auto& ob : objs)
                ob.rotX += 1.5f;
        }
        else if (obj1)
            objs[0].rotX += 1.5f;
        else if(obj2)
            objs[1].rotX += 1.5f;
            
        break;
    }
    case 'X': 
    {
        bool obj1{ IsTargeted(0) };
        bool obj2{ IsTargeted(1) };
        if (obj1 && obj2)
        {
            for (auto& ob : objs)
                ob.rotX -= 1.5f;
        }
        else if (obj1)
            objs[0].rotX -= 1.5f;
        else if (obj2)
            objs[1].rotX -= 1.5f;

        break;   // 자기 x축 자전
    }
        
    case 'y':
    {
        bool obj1{ IsTargeted(0) };
        bool obj2{ IsTargeted(1) };
        if (obj1 && obj2)
        {
            for (auto& ob : objs)
                ob.rotY += 1.5f;
        }
        else if (obj1)
            objs[0].rotY += 1.5f;
        else if (obj2)
            objs[1].rotY += 1.5f;
        break;
    }
    case 'Y':
    {
        bool obj1{ IsTargeted(0) };
        bool obj2{ IsTargeted(1) };
        if (obj1 && obj2)
        {
            for (auto& ob : objs)
                ob.rotY -= 1.5f;
        }
        else if (obj1)
            objs[0].rotY -= 1.5f;
        else if (obj2)
            objs[1].rotY -= 1.5f;
        break;  
    }
    case 'r':
    {
        bool obj1{ IsTargeted(0) };
        bool obj2{ IsTargeted(1) };
        if (obj1 && obj2)
        {
            for (auto& ob : objs)
            {
               
                ob.orbit += 1.5f;
            }
                
        }
        else if (obj1)
        {
          
            objs[0].orbit += 1.5f;
        }
        else if (obj2)
        {
           
            objs[1].orbit += 1.5f;
        }
        break;
    }
    case 'R':
    {
        bool obj1{ IsTargeted(0) };
        bool obj2{ IsTargeted(1) };
        if (obj1 && obj2)
        {
            for (auto& ob : objs)
            {
                ob.orbit -= 1.5f;
            }

        }
        else if (obj1)
        {
            objs[0].orbit -= 1.5f;
        }
        else if (obj2)
        {
            objs[1].orbit -= 1.5f;
        }
        break;   // 중앙 y축 공전
    }
    
    case 'a':
    {
        for (int i = 0; i < 2; ++i)
            if (IsTargeted(i)) objs[i].scale += 0.01f;
        break;
    }
    case 'A':
    {
        for (int i = 0; i < 2; ++i)
            if (IsTargeted(i)) 
            {
                objs[i].scale -= 0.01f;
                objs[i].scale = std::max(0.1f, objs[i].scale);
            } 
        break;   // 제자리 확대/축소
    }
    
    case 'b':
    {
        for (int i = 0; i < 2; ++i)
            if (IsTargeted(i)) objs[i].originScale += 0.01f;
        break;
    }
    case 'B':
    {
        for (int i = 0; i < 2; ++i)
            if (IsTargeted(i))
            {
                objs[i].originScale -= 0.01f;
                objs[i].originScale = std::max(0.1f, objs[i].originScale);
            }
        break;   // 원점 기준 확대/축소 (위치도 바뀜)
    }
    
    case 'd':
    {
        for (int i = 0; i < 2; ++i)
            if (IsTargeted(i)) objs[i].pos.x -= 0.008f;
        break;
    }
    case 'D':
    {
        // 지금은 
        for (int i = 0; i < 2; ++i)
            if (IsTargeted(i)) objs[i].pos.x += 0.008f;
        break;   // x축 방향 좌/우 이동
    }
    
    case 'e':
    {
        for (int i = 0; i < 2; ++i)
            if (IsTargeted(i)) objs[i].pos.y += 0.008f;
        break;
    }
    case 'E':
    {
        for (int i = 0; i < 2; ++i)
            if (IsTargeted(i)) objs[i].pos.y -= 0.008f;
        break;   // y축 방향 위/아래 이동
    }

        // TODO: 두 도형 모두에 적용되는 애니메이션 (16번의 currentMode 패턴 참고)
    case 't':
    {
        if (SwitchMode(MOVE_T))
        {
            animProgress = 0.0f;
            targetPos2 = objs[0].pos;
            targetPos1 = objs[1].pos;

            startPos1 = objs[0].pos;
            startPos2 = objs[1].pos;
        }
        break;   // 원점을 통과하며 서로 자리 바꿈
    }
    case 'u':
    {
        if (SwitchMode(MOVE_U))
        {
            animProgress = 0.0f;
            targetPos2 = objs[0].pos;
            targetPos1 = objs[1].pos;

            startPos1 = objs[0].pos;
            startPos2 = objs[1].pos;
        }
        break;   // 한 개는 위로, 한 개는 아래로 이동하며 서로 자리 바꿈

    }    
    case 'v':
    {
        if (SwitchMode(MOVE_V))
        {
            
        }
        break;   // 한 개는 확대, 한 개는 축소되며 자전 + 공전
    }

    case 'c':
    {
        if (!altShape)
            altShape = true;
        else
            altShape = false;
        break;   // TODO: altShape 토글 (원뿔/육면체 <-> 원기둥/구)
    }
    
    case 's':
    case 'S':
        ResetObjects(); 
        break;
    }
}


// ---------- 업데이트 ----------
void UpdateAnimation()
{
    double now = glfwGetTime();
    float dt = (float)(now - prevFrame);
    prevFrame = now;

    // TODO: currentMode 에 따라 t / u / v 애니메이션 진행
    switch (currentMode)
    {
    case MOVE_T:
    {
        animProgress = std::min(1.0f, animProgress + dt / animDuration);

        objs[0].pos = startPos1 + (targetPos1 - startPos1) * animProgress;
        objs[1].pos = startPos2 + (targetPos2 - startPos2) * animProgress;
        
        if (animProgress >= 1.0f)
        {
            currentMode = NONE;
        }
        break;
    }
    case MOVE_U:
    {
        animProgress = std::min(1.0f, animProgress + dt / animDuration);

        float arc = 0.6f * sinf(PI * animProgress);

        objs[0].pos = startPos1 + (targetPos1 - startPos1) * animProgress;
        objs[1].pos = startPos2 + (targetPos2 - startPos2) * animProgress;
        
        objs[0].pos.y += arc;
        objs[1].pos.y -= arc;

        // y축은 오프셋을 추가로 더하는 로직임. 둘이 순서 바꾸면 pos = 으로 오프셋 의미가 없어짐. 
        // sin 그래프의 0~파이까지 그래프가 y축에 적용된다고보면됨. 

        if (animProgress >= 1.0f)
        {
            currentMode = NONE;
        }
        break;
    }
    case MOVE_V:
    {

        objs[0].orbit += dt * spinSpeed;
        objs[0].rotX += dt * spinSpeed;
        objs[0].rotY += dt * spinSpeed;
        objs[0].scale = std::min(2.0f, objs[0].scale + dt * 0.1f);

        objs[1].orbit -= dt * spinSpeed;
        objs[1].rotX -= dt * spinSpeed;
        objs[1].rotY -= dt * spinSpeed;
        
        objs[1].scale = std::max(0.1f, objs[1].scale - dt * 0.1f);
        break;
    }
    default: break;
    }
}


// ---------- 그리기 ----------
// 0: 좌측 도형, 1: 우측 도형 -> 지금 그려야 할 메시
Mesh& MeshOf(int objIndex)
{
    if (!altShape)
        return objIndex == 0 ? coneMesh : cubeMesh;
    return objIndex == 0 ? cylinderMesh : sphereMesh;
}

void DrawScene()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    // 축: 고정 기울기만 적용
    SendModel(MakeTiltMatrix());
    glBindVertexArray(axisMesh.vao);
    glDrawArrays(GL_LINES, 0, axisMesh.vertexCount);

    // 도형 2개
    for (int i = 0; i < 2; ++i)
    {
        Mesh& m = MeshOf(i);
        glBindVertexArray(m.vao);
        SendModel(MakeModelMatrix(objs[i]));
        glDrawArrays(GL_TRIANGLES, 0, m.vertexCount);
    }
}