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

std::random_device rd;
std::mt19937 gen(rd());

std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };
std::uniform_real_distribution<float> disSize{ 0.025f, 0.045f };  // 칸 반폭(0.05)보다 작게 → 칸 안에 들어옴
std::uniform_real_distribution<float> disProb{ 0.0f, 1.0f };
std::uniform_int_distribution<int> disType{ 1, 3 };               // TRIANGLE, SQUARE, INV_TRIANG

#define WIDTH  1400.0
#define HEIGHT 1400.0

/*
1. 화면에 보드판을 그리고 도형 이동시키기
- 1.1 화면에 20x20 크기의 보드판을 그린다.
- 1.2 삼각형, 사각형, 역삼각형 장애물이 다양한 색상/크기로 랜덤하게 놓여 있다.
- 1.3 크기는 보드칸 안에 들어와야 한다.
2. 좌측 상단 주인공 사각형이 좌우 지그재그로 한 칸씩 이동. a: 시작/정지
3. 장애물과 부딪치면 주인공은 장애물 모양, 장애물은 주인공 모양. 충돌 효과 표시.
- 3.2 마지막 칸에 닿을 때까지 계속 이동
*/

const int    GRID_NUM = 20;
const float  CELL = 2.0f / GRID_NUM;        // 한 칸 크기 0.1
const float  CELL_HALF = CELL / 2.0f;
const double moveInterval = 0.15;         // 주인공 이동 간격
const double EFFECT_TIME = 0.6;           // 충돌 지속

enum ShapeType
{
    NONE, TRIANGLE, SQUARE, INV_TRIANG // 역삼각형
};


struct Cell
{
    ShapeType type{ NONE };
    float size{};
    glm::vec3 rgb{};
};


struct Mesh
{
    GLuint vao{}, vbo{};
    int vertexCount{};
};


struct Hero
{
    int col{ 0 }, row{ 0 };  // 주인공은 어느 인덱스에 있는지 필요함.
    ShapeType type{ SQUARE };
    float size{ 0.04f };
    glm::vec3 rgb{ 0.1f, 0.4f, 0.9f };
};

struct Effect
{
    int col{}, row{};
    double startTime{};
};

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
std::string filetobuf(const char* file);

Mesh makeMesh(const std::vector<float>& data);
void InitMeshes();
const Mesh& GetMesh(ShapeType type);
glm::vec2 CellCenter(int col, int row);
void InitObstacles();
void ResetGame();
void StepHero();
void CheckCollision();
void DrawMesh(const Mesh& m, GLenum mode, glm::vec2 offset, float size, glm::vec3 color);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
GLvoid DrawScene();

GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;

std::array<std::array<Cell, GRID_NUM>, GRID_NUM> grid;   // grid[row][col]

GLint offsetLoc{};
GLint offsetSize{};
GLint colorLoc{};

Mesh meshTriangle, meshInvTri, meshSquare, meshOutline, meshGrid;

Hero hero;
std::vector<Effect> effects;

bool moving{ false };
bool finished{ false };
int dirX{ 1 };
double lastTime{};


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

    while (!glfwWindowShouldClose(window)) {

        double now = glfwGetTime();

        // 이동 간격이 지났으면 한 칸 이동
        if (moving && now - lastTime >= moveInterval)
        {
            StepHero();
            lastTime = now;
        }

        // 충돌 시간이 끝난 것은 제거
        effects.erase(std::remove_if(effects.begin(), effects.end(),
            [now](const Effect& e) { return now - e.startTime >= EFFECT_TIME; }), effects.end());

        // 이것도 DrawScene에서 이펙트 그리기 전에 시간 지난건 바로 삭제 로직.


        // remove_if는 삭제할 것들은 뒤로 빼서 그 시작 이터레이터 반환함.
        // e.startTime는 충돌했을떄 그 타이밍의 시각이다. 

        DrawScene();



        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &meshTriangle.vao);
    glDeleteBuffers(1, &meshTriangle.vbo);

    glDeleteVertexArrays(1, &meshSquare.vao);
    glDeleteBuffers(1, &meshSquare.vbo);

    glDeleteVertexArrays(1, &meshInvTri.vao);
    glDeleteBuffers(1, &meshInvTri.vbo);

    glDeleteVertexArrays(1, &meshOutline.vao);
    glDeleteBuffers(1, &meshOutline.vbo);

    glDeleteVertexArrays(1, &meshGrid.vao);
    glDeleteBuffers(1, &meshGrid.vbo);

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
// 색깔이나 z값 받지 않고 오로지 x,y 좌표만
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


// 도형은 [-1,1] 단위 좌표로 한 번만 만들고, uSize/uOffset으로 크기,위치를 바꿔서 재사용함.
void InitMeshes()
{
    meshTriangle = makeMesh({ -1.0f,-1.0f,  1.0f,-1.0f,  0.0f, 1.0f });
    meshInvTri = makeMesh({ -1.0f, 1.0f,  1.0f, 1.0f,  0.0f,-1.0f });
    meshSquare = makeMesh({ -1.0f,-1.0f,  1.0f,-1.0f, -1.0f, 1.0f,
                             1.0f,-1.0f,  1.0f, 1.0f, -1.0f, 1.0f });
    meshOutline = makeMesh({ -1.0f,-1.0f,  1.0f,-1.0f,  1.0f, 1.0f, -1.0f, 1.0f });   // GL_LINE_LOOP용

    std::vector<float> lines;
    for (int i = 0; i <= GRID_NUM; ++i)
    {
        float v = -1.0f + i * CELL;
        lines.insert(lines.end(), { v, -1.0f, v, 1.0f });     // 세로선
        lines.insert(lines.end(), { -1.0f, v, 1.0f, v });     // 가로선
    }

    meshGrid = makeMesh(lines);
}


const Mesh& GetMesh(ShapeType type)
{
    switch (type)
    {
    case TRIANGLE:   return meshTriangle;
    case INV_TRIANG: return meshInvTri;
    default:         return meshSquare;
    }
}



glm::vec2 CellCenter(int col, int row)
{
    return { -1.0f + (col + 0.5f) * CELL, 1.0f - (row + 0.5f) * CELL };
}




void InitObstacles()
{
    for (int r = 0; r < GRID_NUM; ++r)
    {
        for (int c = 0; c < GRID_NUM; ++c)
        {
            grid[r][c] = Cell{};

            if (r == 0 && c == 0) continue;              // 주인공 시작 칸은 비워둠

            if (disProb(gen) < 0.35f)  
            {
                grid[r][c].type = (ShapeType)disType(gen);
                grid[r][c].size = disSize(gen);
                grid[r][c].rgb = glm::vec3{ disColor(gen), disColor(gen), disColor(gen) };
            }
        }
    }
}


void ResetGame()
{
    InitObstacles();
    hero = Hero{};
    effects.clear();
    dirX = 1;
    moving = false;
    finished = false;
}



void StepHero()
{
    int nextCol = hero.col + dirX; // 인덱스 증가하는 느낌으로다가

    if (nextCol >= 0 && nextCol < GRID_NUM)
    {
        hero.col = nextCol;
    }
    else
    {
        hero.row++;  // hero.col이 끝에 있는 시점에서 row만 증가하면되네
        dirX = -dirX;
    }

    CheckCollision();

    // 마지막 행의 진행 방향 끝 칸에 닿으면 종료
    if (hero.row == GRID_NUM - 1 &&
        ((dirX == 1 && hero.col == GRID_NUM - 1) || (dirX == -1 && hero.col == 0)))
    {
        moving = false;
        finished = true;
    }
}


void CheckCollision()
{
    Cell& c = grid[hero.row][hero.col];

    if (c.type != NONE)
    {
        ShapeType temp = hero.type;
        hero.type = c.type;
        c.type = temp;
        
        glm::vec3 tempRgb = hero.rgb;
        hero.rgb = c.rgb;
        c.rgb = tempRgb;

        float tempSize = hero.size;
        hero.size = c.size;
        c.size = tempSize;

        effects.push_back(Effect{ hero.col, hero.row, glfwGetTime() });
        // 충돌한 타이밍의 시간을 인자로 전달.
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
    case GLFW_KEY_A:
        if (!finished)
        {
            moving = !moving;
            if (moving) lastTime = glfwGetTime();
        }
        break;
    case GLFW_KEY_R:
        ResetGame();
        break;
    }
}


void DrawMesh(const Mesh& m, GLenum mode, glm::vec2 offset, float size, glm::vec3 color)
{
    glBindVertexArray(m.vao);
    glUniform2f(offsetLoc, offset.x, offset.y);
    glUniform1f(offsetSize, size);
    glUniform3f(colorLoc, color.x, color.y, color.z);
    glDrawArrays(mode, 0, m.vertexCount);
}


void DrawScene()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    
    glUseProgram(shaderProgramID);


    DrawMesh(meshGrid, GL_LINES, { 0.0f, 0.0f }, 1.0f, { 0.0f, 0.0f, 0.0f });

    
    for (int r = 0; r < GRID_NUM; ++r)
    {
        for (int c = 0; c < GRID_NUM; ++c)
        {
            const Cell& cell = grid[r][c];
            if (cell.type == NONE) continue;

            DrawMesh(GetMesh(cell.type), GL_TRIANGLES, CellCenter(c, r), cell.size, cell.rgb);
        }
    }

 
    DrawMesh(GetMesh(hero.type), GL_TRIANGLES, CellCenter(hero.col, hero.row), hero.size, hero.rgb);


    double now = glfwGetTime();

    for (const Effect& e : effects)
    {
        float progress = (float)((now - e.startTime) / EFFECT_TIME);
        progress = std::min(progress, 1.0f);

        float size = CELL_HALF * (1.0f + progress * 2.0f); // 1/20이 제일 작네.
        glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f) + (glm::vec3(1.0f) - glm::vec3(1.0f, 0.0f, 0.0f)) * progress;
        // 흰색이 되서 안보이게끔 하는 로직.

        glLineWidth(3.0f);
        DrawMesh(meshOutline, GL_LINE_LOOP, CellCenter(e.col, e.row), size, color);
    }
}