#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <random>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };

// 옆면 어떤거 선택할지 
std::uniform_int_distribution<int> disSide{ 1,2 };
std::uniform_int_distribution<int> disLine{ 0,3 };

#define WIDTH  1400
#define HEIGHT 1400

struct Mesh
{
    GLuint vao{}, vbo{};
    int vertexCount{};
};


// 육면체 면 번호 (13번 faceIdx 기준)
// 0: 뒤(z+)  1: 아랫면(y-)  2: 윗면(y+)  3: 왼쪽(x-)  4: 오른쪽(x+)  5: 앞(z-)
enum Face
{
    FRONT = 0,
    BOTTOM = 1,
    TOP = 2,
    LEFT = 3,
    RIGHT = 4,
    BACK = 5
};


enum Anime
{
    NONE = 0,
    T = 1,
    F = 2,
    S = 3,
    B = 4,

    ONE = 5,
    TWO = 6,
    FOUR = 7,
    FIVE = 8
};

// 면 하나의 애니메이션 상태
struct FaceAnim
{
    bool  active{ false };   // 이 면이 지금 움직이는 중인가
    float angle{ 0.0f };     // 도(degree) 단위
    float scale{ 1.0f };     // b 키(축소/복귀)용
    float dir{ 1.0f };       // +1 / -1
    Anime mode{ NONE };
    int idx{}; // sideLine idx 담는 용도

    // TODO: 필요한 값이 더 있으면 추가 (예: 목표 각도, 모드 구분 등)

    //b를 할떄 scale이 0 ~ 1을 왔다갔다 하면 될려나.
     
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
glm::mat4 MakeFaceMatrix(int face, Anime mode);
void SendModel(const glm::mat4& m);

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

bool   showCube{ true };
bool   depthOn{ true };      // main()의 glEnable(GL_DEPTH_TEST)와 일치
double prevFrame{};


// 도형 전체 y축 자전 (y 키)
bool  yRotating{ false };
float angleY{ 0.0f };


//앞 오른쪽 모서리, 앞 왼쪽 모서리, 뒤 오른쪽 모서리, 뒤 왼쪽 모서리
glm::vec3 sideLine[4] = { { 0.3, 0.0, -0.3 }, { -0.3, 0.0, -0.3 }, 
                           { 0.3, 0.0, 0.3 }, { -0.3, 0.0, 0.3 } };


std::pair<int, int> sideLineIdx[4] = { {FRONT, RIGHT}, {FRONT, LEFT}, {RIGHT,BACK}, {LEFT, BACK} };

int sideIdx[4] = { FRONT, LEFT ,RIGHT, BACK };

std::array<FaceAnim, 6> faceAnim;

const float spinSpeed{ 90.0f };   // 초당 도
const float scaleSpeed{ 0.4f };

bool fourAnime{ false };

int sIdx{};

int main()
{
    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Practice 15", nullptr, nullptr);
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

    glfwSetCharCallback(window, CharCallback);   // 15번은 글자 키만 사용

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

// 면마다 색 1개 (13번 방식). 면 f의 정점은 f*6 ~ f*6+5
std::vector<float> MakeCubeData()
{
    std::vector<glm::vec3> pos{ {-0.3,-0.3,0.3}, {0.3,-0.3,0.3}, {0.3,0.3,0.3}, {-0.3,0.3,0.3},
                                {-0.3,-0.3,-0.3}, {0.3,-0.3,-0.3}, {0.3,0.3,-0.3}, {-0.3,0.3,-0.3} };
    /*
    std::vector<std::vector<int>> faceIdx{ {0, 1, 2, 3},
                                           {0, 4, 5, 1},
                                           {3, 2, 6, 7},
                                           {0, 4, 7, 3},
                                           {1, 5, 6, 2},
                                           {4, 7, 6, 5} };
                                           */

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

// 옆면 4x3=12개 먼저, 바닥 6개 마지막 -> 총 18개
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

    //바닥
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

    // y축 자전을 할떄는 기울기를 먼저 적용한 후에 y축 회전을 따로 적용하는 방식이다. 

    glm::mat4 finalMatrix(1.0f);

    finalMatrix = glm::rotate(finalMatrix, glm::radians(angleY), glm::vec3{ 0,1,0 });

    return finalMatrix * MakeTiltMatrix();
    //return MakeTiltMatrix() * finalMatrix;
}




// 면 f 하나에만 적용되는 애니메이션 행렬 (면 자기 좌표계 기준)
glm::mat4 MakeFaceMatrix(int face, Anime mode)
{
    glm::mat4 base(1.0f);

    switch (mode)
    {
    case T:
        base = glm::translate(base, { 0, 0.3, 0 });
        base = glm::rotate(base, glm::radians(faceAnim[face].angle), glm::vec3(0, 0, 1));
        base = glm::translate(base, { 0, -0.3, 0 });
        break;
    case F:
        // 앞면을 지정했는데 뒷면에서 열리는 버그가 발생함. 
        // 확인해보니까 깊이 테스트 떄문에 +z값들이 뒤에서 보이는 현상이었음.
        base = glm::translate(base, { 0, 0.3, -0.3 });
        base = glm::rotate(base, glm::radians(faceAnim[face].angle), glm::vec3(1, 0, 0));
        base = glm::translate(base, { 0, -0.3, 0.3 });
        break;
    case S:
        //z축을 기준으로 회전하면 되겠네. 
        base = glm::translate(base, sideLine[sIdx] );
        base = glm::rotate(base, glm::radians(faceAnim[face].angle), glm::vec3(0, 0, 1));
        base = glm::translate(base, -sideLine[sIdx] );

        break;
    case B:
        // 뒷면이 작아졌다가 다시 커져야 된다. 
        base = glm::translate(base, {0.0, 0.0, 0.3});
        base = glm::scale(base, { faceAnim[face].scale, faceAnim[face].scale, faceAnim[face].scale });
        base = glm::translate(base, { 0.0, 0.0, -0.3 });
        break;

    case ONE:
        base = glm::translate(base, sideLine[faceAnim[face].idx]);
        base = glm::rotate(base, glm::radians(faceAnim[face].angle), glm::vec3(0, 1, 0));
        base = glm::translate(base, -sideLine[faceAnim[face].idx]);
        break;
    case TWO:
        // 공전해야 되니까 그냥 원점을 기준으로 회전시키면 된다. 
        base = glm::rotate(base, glm::radians(faceAnim[face].angle), glm::vec3(0, 1, 0));
        break;
    case FOUR:
        base = glm::rotate(base, glm::radians(faceAnim[face].angle), glm::vec3(0, 1, 0));
        break;
    case FIVE:
        break;
    }



    return base;
}



void SendModel(const glm::mat4& m)
{
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
}


// ---------- 상태 ----------
void ResetAll()
{
    angleY = 0.0f;
    yRotating = false;
    showCube = true;
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

    case 'p': showCube = !showCube; break;     // 육면체/사각뿔 전환
    case 'y': yRotating = !yRotating; break;   // y축 자전 시작/멈춤
    case 'r': ResetAll(); break;

        // 한 면 애니메이션
    case 't':
    {
        int face = (int)TOP;

        auto& f = faceAnim[face];

        if (f.active)
        {
            f.active = false;
            f.mode = NONE;
            f.angle = 0.0f;
            f.scale = 1.0f;
        }
        else // 다시 활성화
        {
            f.active = true;
            f.mode = T;
            f.dir = 1.0f;
        }
        break;
    }
    case 'f':
    {
        /* TODO: 앞면(FRONT) 위쪽 모서리 기준 열기/닫기 */
        int face = (int)FRONT;

        auto& f = faceAnim[face];

        if (f.active)
        {
            f.active = false;
            f.mode = NONE;
            f.angle = 0.0f;
            f.scale = 1.0f;
        }
        else // 다시 활성화
        {
            f.active = true;
            f.mode = F;
            f.dir = 1.0f;
        }

        break;
    }
    case 's':
    {
        sIdx = sideIdx[disSide(gen)];

        auto& f = faceAnim[sideIdx[sIdx]];

        if (f.active)
        {
            f.active = false;
            f.mode = NONE;
            f.angle = 0.0f;
            f.scale = 1.0f;
        }
        else // 다시 활성화
        {
            f.active = true;
            f.mode = S;
            f.dir = 1.0f;
        }

        break;
    }
    case 'b': 
    {
        int face = (int)BACK;

        auto& f = faceAnim[face];

        if (f.active)
        {
            f.active = false;
            f.mode = NONE;
            f.angle = 0.0f;
            f.scale = 1.0f;
        }
        else // 다시 활성화
        {
            f.active = true;
            f.mode = B;
            f.dir = -1.0f;
        }

        break;
    }   
    // 두 면 애니메이션
    case '1': /* TODO: 옆면 중 같은 모서리 쓰는 두 면 중 랜덤 1개, 모서리 기준 360도 회전 */
    {
        // 먼저 옆면 모서리 중에 랜덤으로 돌려서 선택을 하고 두 면을 360도 까지 회전하는게 로직이겠네. 
        int line = disLine(gen);

        std::pair<int, int> tempLine = sideLineIdx[line];

        auto& f1 = faceAnim[tempLine.first];
        auto& f2 = faceAnim[tempLine.second];

        f1.idx = line;
        f2.idx = line;

        if (f1.active && f2.active)
        {
            f1.active = false;
            f2.active = false;
            f1.mode = NONE;
            f2.mode = NONE;
            
            f1.angle = 0.0f;
            f1.scale = 1.0f;
            
            f2.angle = 0.0f;
            f2.scale = 1.0f;
        }
        else // 다시 활성화
        {
            f1.active = true;
            f1.mode = ONE;
            f1.dir = 1.0f;

            f2.active = true;
            f2.mode = ONE;
            f2.dir = 1.0f;
        }

        break;
    }
    
    case '2': /* TODO: 같은 모서리 쓰는 두 면 중 랜덤 선택, 중앙 y축 기준 반대방향 90도 공전(자리 교환) */ 
    {         // 
        int line = disLine(gen);

        std::pair<int, int> tempLine = sideLineIdx[line];

        auto& f1 = faceAnim[tempLine.first];
        auto& f2 = faceAnim[tempLine.second];

        f1.idx = line;
        f2.idx = line;

        if (f1.active && f2.active)
        {
            f1.active = false;
            f2.active = false;
            f1.mode = NONE;
            f2.mode = NONE;
        }
        else // 다시 활성화
        {
            f1.active = true;
            f1.mode = TWO;
            f1.dir = -1.0f;

            f2.active = true;
            f2.mode = TWO;
            f2.dir = 1.0f;
        }

        f1.angle = 0.0f;
        f1.scale = 1.0f;

        f2.angle = 0.0f;
        f2.scale = 1.0f;
        break;
    }
        

        // 네 면 애니메이션
    case '4': /* TODO: 옆면 4개가 중앙 y축 기준 공전 */
    {
        for (int i = 0; i < 4; ++i)
        {
            auto& f = faceAnim[sideIdx[i]];

            if (!fourAnime)
                f.active = true;
            else
                f.active = false;

            if (f.active)
            {
                f.mode = FOUR;
                f.dir = 1.0f;
            }
            else // 다시 활성화
            {
                f.mode = NONE;
            }

            f.angle = 0.0f;
            f.scale = 1.0f;
        }

        fourAnime = !fourAnime;
        break;
    }
    case '5': /* TODO: 윗면, 아랫면, 옆면 2개가 중앙 x축 기준 공전 */ break;
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

    for (int i = 0; i < faceAnim.size(); ++i)
    {
        if (faceAnim[i].active == false)
            continue;

        faceAnim[i].angle = faceAnim[i].angle + faceAnim[i].dir * dt * spinSpeed;

        if (faceAnim[i].mode == B)
        {
            if(faceAnim[i].scale >= 1.0f)
                faceAnim[i].dir = -1.0f;
            else if(faceAnim[i].scale <= 0.0f)
                faceAnim[i].dir = 1.0f;
            
            faceAnim[i].scale = faceAnim[i].scale + faceAnim[i].dir * dt * scaleSpeed;

        }
        else if (faceAnim[i].mode == TWO)
        {
            faceAnim[i].angle = std::clamp(faceAnim[i].angle, -90.0f , 90.0f);
            
            if (faceAnim[i].angle >= 90.0f)
                faceAnim[i].active = false;
            else if (faceAnim[i].angle <= -90.0f)
                faceAnim[i].active = false;
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

    // 도형
    if (showCube)
    {
        glBindVertexArray(cubeMesh.vao);
        for (int f = 0; f < 6; ++f)
        {
            SendModel(MakeObjectMatrix() * MakeFaceMatrix(f, faceAnim[f].mode));
            //SendModel( MakeFaceMatrix(f) * MakeObjectMatrix());
            glDrawArrays(GL_TRIANGLES, f * 6, 6);
        }
    }
    else
    {
        // 사각뿔은 정적으로 전체 그리기 (애니메이션 없음)
        glBindVertexArray(pyramidMesh.vao);
        SendModel(MakeObjectMatrix());
        glDrawArrays(GL_TRIANGLES, 0, pyramidMesh.vertexCount);
    }
}