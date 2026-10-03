#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <fstream>
#include <string>
#include <algorithm>
#include <random>

std::random_device rd;
std::mt19937 gen(rd());

std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };
std::uniform_real_distribution<float> disSize{ 0.05f, 0.12f };

std::uniform_int_distribution<int> disRand{ 1,4 };
std::uniform_int_distribution<int> disDir{ 1,2 };

#define WIDTH  1500.0
#define HEIGHT 1500.0

/*
작은 도형으로 새로운 모양 만들기. 
1. 화면의 좌측에는 작은 삼각형, 정삼각형, 직각삼각형 랜덤한 개수가 랜덤 위치에 있다.
- 1.1 화면의 우측에는 새 모양판이 그려져 있다. 
- 1.2 모양판 : 사각형 4개로 분리된거, 삼각형 윗꼭짓점이 사방에서 만나는 모양. 
, 사각형 삼각형 2개로 나뉜거 말고 2개 더 모양판 추가하면됨. 
2. 좌측의 작은 도형들을 마우스로 선택해서 드래그 하여 우측의 모양판에 맞춘다.
- 2.1 모양판에 도형이 모두 올라가면 그 도형은 완성되었고 더 이상 움직일 수 없다. 
모양판에 있는 도형을 이전에는 다른 위치로 바꾸고 할 수 있나본데. 
3. 키보드 명령어
- 3.1 r: 리셋하여 새로 시작한다. 
- 3.2 q 프로그램 종료.

*/

// 세로로 모양판 5개 나열하면 될듯. 

enum ShapeType {
    SQUARE,
    EQUI_TRI,           // 일반적 정삼각형
    
    RIGHT_TRI_UP_LEFT,  // 직각이 좌상단
    RIGHT_TRI_UP_RIGHT,  // 직각이 우상단
    RIGHT_TRI_DOWN_LEFT,  // 직각이 좌하단
    RIGHT_TRI_D_RIGHT,   // 직각이 우하단
    
    PINWHEEL_UP,     // 바람개비 위쪽 조각
    PINWHEEL_DOWN,   // 바람개비 아래쪽 조각
    PINWHEEL_LEFT,   // 바람개비 왼쪽 조각
    PINWHEEL_RIGHT   // 바람개비 오른쪽 조각
};


struct Shape
{
    GLuint vao{};
    GLuint vbo{};

    int vertexCount{};
    glm::vec2 pos{};
    glm::vec3 rgb{};

    float size{};
    bool isDraw{ true };  // 채워지지 않았으면 그리기. 
    ShapeType type{};  

    GLenum mode{GL_LINE_LOOP};
    
};


struct Board
{
    std::vector<Shape> slot;
    bool completed{ false };
};


void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
std::string filetobuf(const char* file);

void GetLocalTrinagle(ShapeType type, float size, glm::vec2& A, glm::vec2& B, glm::vec2& C);
float Cross2D(glm::vec2 a, glm::vec2 b);
bool IsInsideTriangle(glm::vec2 p, glm::vec2 A, glm::vec2 B, glm::vec2 C);
bool IsInsideRectangle(glm::vec2 p, float size);


void addShape(const std::vector<float>& data, const float size, const glm::vec3& rgb, const glm::vec2& pos,
    int vertexCount, int cnt, ShapeType type);
GLvoid GenShape1();
GLvoid GenShape2();
GLvoid GenShape3();
GLvoid GenShape4();
GLvoid GenShape5();
void TransformScreenToNDC(double& i, double& j);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);

GLvoid DrawLine();
GLvoid DrawScene();

GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;

GLuint offsetLoc{};

std::vector<Shape> randShapes;
std::array<Board, 5> board;  // 채워질 모양판
int pickedIdx{};

double prevFrame{};

bool Dragged{ false };
glm::vec2 mousePos{};


int main()
{

    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "First Try: Window", nullptr, nullptr);
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

    glViewport(0, 0, WIDTH, HEIGHT);
    randShapes.reserve(50);

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);


    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();
    offsetLoc = glGetUniformLocation(shaderProgramID, "uOffset");

    prevFrame = glfwGetTime();

    GenShape1();
    GenShape2();
    GenShape3();
    GenShape4();
    GenShape5();

    while (!glfwWindowShouldClose(window)) {

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


void GetLocalTrinagle(ShapeType type, float size, glm::vec2& A, glm::vec2& B, glm::vec2& C)
{
    switch (type)
    {
    case PINWHEEL_UP:
        A = { -size, size };  B = { size, size };  C = { 0.0f, 0.0f };
        break;
    case PINWHEEL_DOWN:
        A = { -size, -size }; B = { size, -size }; C = { 0.0f, 0.0f };
        break;
    case PINWHEEL_LEFT:
        A = { -size, -size }; B = { -size, size }; C = { 0.0f, 0.0f };
        break;
    case PINWHEEL_RIGHT:
        A = { size, -size };  B = { size, size };  C = { 0.0f, 0.0f };
        break;
    case RIGHT_TRI_UP_LEFT:
        A = { -size, -size }; B = { size, 2 * size }; C = { -size, 2 * size };
        break;
    case RIGHT_TRI_D_RIGHT:
        A = { -size, -size }; B = { size, -size }; C = { size, 2 * size };
        break;
    case EQUI_TRI:
        A = { -size, 0.0f };  B = { size, 0.0f };  C = { 0.0f, size * 1.8f };
        break;
    }
}


// 점, 선, 삼각형, 사각형의 로컬 좌표계의 두 벡터
float Cross2D(glm::vec2 a, glm::vec2 b)
{
    return a.x * b.y - a.y * b.x;
    // 이건 사실상 2x2 행렬식의 계산이다. 
}


bool IsInsideTriangle(glm::vec2 p, glm::vec2 A, glm::vec2 B, glm::vec2 C)
{
    float c1 = Cross2D(B - A, p - A);
    float c2 = Cross2D(C - B, p - B);
    float c3 = Cross2D(A - C, p - C);

    bool allPositive = c1 >= 0.0f && c2 >= 0.0f && c3 >= 0.0f;
    bool allNegative = c1 <= 0.0f && c2 <= 0.0f && c3 <= 0.0f;

    return allPositive || allNegative;
}



bool IsInsideRectangle(glm::vec2 p, float size)
{
    return p.x >= -size &&
        p.x <= size &&
        p.y >= -size &&
        p.y <= size;
}




void reshapeTr()
{
    for (auto& tr : randShapes)
    {
        std::vector<float> tempData;
        tempData.reserve(50);

        tempData = { -tr.size,-tr.size,0, tr.rgb.x,tr.rgb.y,tr.rgb.z,
                    tr.size,-tr.size,0, tr.rgb.x,tr.rgb.y,tr.rgb.z,
                    0,2 * tr.size,0, tr.rgb.x,tr.rgb.y,tr.rgb.z };

        glBindVertexArray(tr.vao);
        glBindBuffer(GL_ARRAY_BUFFER, tr.vbo);
        glBufferData(GL_ARRAY_BUFFER, tempData.size() * sizeof(float), tempData.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

    }
}

void addShape(const std::vector<float>& data, const float size, const glm::vec3& rgb, const glm::vec2& pos,
    int vertexCount, int cnt, ShapeType type)
{
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    GLuint vao1, vbo1;
    glGenVertexArrays(1, &vao1);
    glBindVertexArray(vao1);
    glGenBuffers(1, &vbo1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo1);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    std::uniform_real_distribution<float> disX{ -1.0f + size, 0.4f - size };
    std::uniform_real_distribution<float> disY{ -1.0f + size, 1.0f - size };

    Shape t;
    Shape t2;

    t.vao = vao;
    t.vbo = vbo;

    t.pos = {disX(gen), disY(gen)};
    t.rgb = rgb;

    t.size = size;
    t.vertexCount = vertexCount;

    t.type = type;

    // board에 넣을 삼각형들

    t2.vao = vao1;
    t2.vbo = vbo1;

    t2.pos = pos;
    t2.rgb = rgb;

    t2.size = size;
    t2.vertexCount = vertexCount;

    t2.type = type;

    randShapes.push_back(t);
    board[cnt].slot.push_back(t2);

}



GLvoid GenShape1()
{
    //1번 모양판
    float size = 0.04f;

    glm::vec2 pos{ 0.7f, 0.8f };
    glm::vec3 rgb{ disColor(gen), disColor(gen), disColor(gen) };

    int vertexCount = 6;
    std::vector<float> tempData;
    tempData.reserve(50);
    tempData = { -size,-size,0, rgb.x,rgb.y,rgb.z, size,-size,0, rgb.x,rgb.y,rgb.z, 
        -size,size,0, rgb.x,rgb.y,rgb.z, 
        size,-size,0, rgb.x,rgb.y,rgb.z, size, size, 0, rgb.x,rgb.y,rgb.z ,
        -size, size ,0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 0, SQUARE);

    pos = { 0.82f, 0.8f };
  

    addShape(tempData, size, rgb, pos, vertexCount, 0, SQUARE);

    pos = { 0.7f, 0.92f };
 

    addShape(tempData, size, rgb, pos, vertexCount, 0, SQUARE);

    pos = { 0.82f, 0.92f };

    addShape(tempData, size, rgb, pos, vertexCount, 0, SQUARE);
  
}


GLvoid GenShape2()
{
    //2번 모양판
    float size = 0.08f;

    glm::vec2 pos{ 0.78f, 0.49f };
    glm::vec3 rgb{ disColor(gen), disColor(gen), disColor(gen) };

    int vertexCount = 3;
    std::vector<float> tempData;
    tempData.reserve(50);

    // 위쪽 삼각형
    tempData = { -size, size, 0, rgb.x,rgb.y,rgb.z,
                  size, size, 0, rgb.x,rgb.y,rgb.z,
                  0.0f, 0.0f, 0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 1, PINWHEEL_UP);

    pos = { 0.78f, 0.45f };

    // 아래쪽 삼각형
    tempData = {  -size, -size, 0, rgb.x,rgb.y,rgb.z,
                   size, -size, 0, rgb.x,rgb.y,rgb.z,
                  0.0f,  0.0f, 0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 1, PINWHEEL_DOWN);

    pos = { 0.72f, 0.47f };
    // 왼쪽 삼각형
    tempData = {  -size, -size, 0, rgb.x,rgb.y,rgb.z,
                  -size,  size, 0, rgb.x,rgb.y,rgb.z,
                  0.0f,  0.0f, 0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 1, PINWHEEL_LEFT);

    pos = { 0.84f, 0.47f };
    // 오른쪽 삼각형
    tempData = { size, -size, 0, rgb.x,rgb.y,rgb.z,
                 size,  size, 0, rgb.x,rgb.y,rgb.z,
                 0.0f,  0.0f, 0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 1, PINWHEEL_RIGHT);
}


GLvoid GenShape3()
{
    //3번 모양판
    float size = 0.08f;

    glm::vec2 pos{ 0.78f, 0.00f };
    glm::vec3 rgb{ disColor(gen), disColor(gen), disColor(gen) };

    int vertexCount = 3;
    std::vector<float> tempData;
    tempData.reserve(50);

    // 직각이 좌상단인 삼각형
    tempData = { -size, -size, 0, rgb.x,rgb.y,rgb.z,
                  size,  2 * size, 0, rgb.x,rgb.y,rgb.z,
                 -size,  2 * size, 0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 2, RIGHT_TRI_UP_LEFT);

    // 직각이 우하단인 삼각형
    tempData = { -size, -size, 0, rgb.x,rgb.y,rgb.z,
                  size, -size, 0, rgb.x,rgb.y,rgb.z,
                  size,  2 * size, 0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 2, RIGHT_TRI_D_RIGHT);
}


GLvoid GenShape4()
{
    //4번 모양판
    float size = 0.08f;

    glm::vec2 pos{ 0.78f, -0.38f };
    glm::vec3 rgb{ disColor(gen), disColor(gen), disColor(gen) };

    int vertexCount = 3;
    std::vector<float> tempData;
    tempData.reserve(50);

    // 위쪽 삼각형
    tempData = { -size, size, 0, rgb.x,rgb.y,rgb.z,
                  size, size, 0, rgb.x,rgb.y,rgb.z,
                  0.0f, 0.0f, 0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 3, PINWHEEL_UP);

    // 아래쪽 삼각형
    tempData = { -size, -size, 0, rgb.x,rgb.y,rgb.z,
                  size, -size, 0, rgb.x,rgb.y,rgb.z,
                  0.0f,  0.0f, 0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 3, PINWHEEL_DOWN);

    // 왼쪽 삼각형
    tempData = { -size, -size, 0, rgb.x,rgb.y,rgb.z,
                 -size,  size, 0, rgb.x,rgb.y,rgb.z,
                  0.0f,  0.0f, 0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 3, PINWHEEL_LEFT);

    // 오른쪽 삼각형
    tempData = { size, -size, 0, rgb.x,rgb.y,rgb.z,
                 size,  size, 0, rgb.x,rgb.y,rgb.z,
                 0.0f,  0.0f, 0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 3, PINWHEEL_RIGHT);
}


GLvoid GenShape5()
{
    //5번 모양판
    float size = 0.07f;

    glm::vec2 pos{ 0.78f, -0.80f };
    glm::vec3 rgb{ disColor(gen), disColor(gen), disColor(gen) };

    int vertexCount = 6;
    std::vector<float> tempData;
    tempData.reserve(50);

    // 몸통 (사각형)
    tempData = { -size,-size,0, rgb.x,rgb.y,rgb.z,
                  size,-size,0, rgb.x,rgb.y,rgb.z,
                 -size, size,0, rgb.x,rgb.y,rgb.z,
                  size,-size,0, rgb.x,rgb.y,rgb.z,
                  size, size,0, rgb.x,rgb.y,rgb.z,
                 -size, size,0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, pos, vertexCount, 4, SQUARE);

    // 지붕 (정삼각형) - 몸통 바로 위에 오도록 pos를 몸통 윗변 위치로 올림
    glm::vec2 roofPos{ pos.x, pos.y + size };
    vertexCount = 3;

    tempData = { -size, 0.0f, 0, rgb.x,rgb.y,rgb.z,
                  size, 0.0f, 0, rgb.x,rgb.y,rgb.z,
                  0.0f, size * 1.8f, 0, rgb.x,rgb.y,rgb.z };

    addShape(tempData, size, rgb, roofPos, vertexCount, 4, EQUI_TRI);
}



void TransformScreenToNDC(double& i, double& j)
{
    double transHalfWidth = 2 / WIDTH;
    double transHalfHeight = 2 / HEIGHT;

    i = i * transHalfWidth - 1;
    j = -1 * (j * transHalfHeight) + 1;
}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return; // 누르는 순간만 처리

    switch (key)
    {

    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_R:
        // 모든것 리셋하기
        randShapes.clear();
        
        for (int i = 0; i < board.size(); ++i) 
        {
            board[i].slot.clear();
        }

        Dragged = false;
        pickedIdx = 0;

        GenShape1();
        GenShape2();
        GenShape3();
        GenShape4();
        GenShape5();

        break;
    }

}


void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        TransformScreenToNDC(xpos, ypos);

        mousePos.x = xpos;
        mousePos.y = ypos;

        bool inside{ false };

        for (int i = randShapes.size() - 1; i >= 0; --i)
        {
            glm::vec2 local = mousePos - randShapes[i].pos;

            // 삼각형 혹은 사각형이랑 마우스 피킹 하는 로직입니다.
            switch (randShapes[i].vertexCount)
            {
            case 3:
                glm::vec2 A{};
                glm::vec2 B{};
                glm::vec2 C{};
                GetLocalTrinagle(randShapes[i].type, randShapes[i].size, A, B, C);
                inside = IsInsideTriangle(local,A,B,C);
                break;
            case 6:
                inside = IsInsideRectangle(local, randShapes[i].size);
                break;

            }

            if (inside)
            {
                Dragged = true;
                pickedIdx = i;
                break;
            }
        }
        

    }
    // 이렇게도 할 수 있구나.
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        Dragged = false;

        Shape& DraggedSh = randShapes[pickedIdx];

        for (int i = 0; i < board.size(); ++i)
        {
            for (int j = 0; j < board[i].slot.size(); ++j)
            {
                Shape& sh = board[i].slot[j];

                if (sh.mode == GL_TRIANGLES) continue; // 이미 채워진 슬롯은 건너뜀
                if (DraggedSh.type != sh.type) continue;

                float dist = glm::length(DraggedSh.pos - sh.pos);

                if (dist <= sh.size) // 스냅 허용 반경
                {
                    DraggedSh.isDraw = false;
                    sh.mode = GL_TRIANGLES;
                    return;
                }
            }
        }
    }


}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (Dragged)
    {
        TransformScreenToNDC(xpos, ypos);

        std::pair<double, double> move{ xpos - mousePos.x , ypos - mousePos.y };
        // 현재 위치 xpos에서 이전 위치 mousePos.x를 빼서 이동량을 구해야 한다. 

        randShapes[pickedIdx].pos.x += move.first;
        randShapes[pickedIdx].pos.y += move.second;

        mousePos.x = xpos;
        mousePos.y = ypos;

    }
}


GLvoid DrawLine()
{
    std::vector<float> tempData;
    tempData.reserve(30);

    tempData = { 0.4f, 1.0f,0.0f, 0.0f,0.0f,1.0f, 0.4f,-1.0f,0, 0.0f,0.0f,1.0f };

     glUniform2f(offsetLoc, 0.0f, 0.0f);

     GLuint vao, vbo;
	 glGenVertexArrays(1, &vao);
	 glBindVertexArray(vao);
	 glGenBuffers(1, &vbo);
	 glBindBuffer(GL_ARRAY_BUFFER, vbo);
	 glBufferData(GL_ARRAY_BUFFER, tempData.size() * sizeof(float), tempData.data(), GL_STATIC_DRAW);
	 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	 glEnableVertexAttribArray(0);
	 glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	 glEnableVertexAttribArray(1);

	 glDrawArrays(GL_LINES, 0, 2);
    
}





GLvoid DrawScene()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //--- 셰이더프로그램사용
    glUseProgram(shaderProgramID);


    prevFrame = glfwGetTime();

    DrawLine();


    for (int k = 0; k < randShapes.size(); ++k)
    {
        Shape& sh = randShapes[k];

        if (sh.isDraw == false)
            continue;

        glBindVertexArray(sh.vao);

        glUniform2f(offsetLoc, sh.pos.x, sh.pos.y);

        glDrawArrays(GL_TRIANGLES, 0, sh.vertexCount);
            
    }

    glLineWidth(5.0f);

    for (int i = 0; i < board.size(); ++i)
    {
        for (int j = 0; j < board[i].slot.size(); ++j)
        {
            Shape& sh = board[i].slot[j];

            glBindVertexArray(sh.vao);

            glUniform2f(offsetLoc, sh.pos.x, sh.pos.y);
            
            glDrawArrays(sh.mode , 0, sh.vertexCount);
        }
    }
    glLineWidth(5.0f);

}


// 그니까 지금 해야되는게 삼각형 클릭하면 드래그 되고 그걸 release 했을떄 충돌 검사해서 성공하면
// sh.isDraw == false로 하고 board 판의 slot 삼각형은 GL_TRIANGLES 로 바꿔주면됨.