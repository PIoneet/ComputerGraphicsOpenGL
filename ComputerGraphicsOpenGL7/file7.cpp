#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <fstream>
#include <string>
#include <random>

std::random_device rd;
std::mt19937 gen(rd());

std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };
std::uniform_real_distribution<float> disSize{ 0.05f, 0.12f };

#define WIDTH  1300.0
#define HEIGHT 1300.0
#define MOVE 10.0

struct Shape 
{
    GLuint vao;
    GLuint vbo;

    GLenum mode;

    int vertexCount{};
    glm::vec2 pos;

    float size;
    
    glm::vec2 velocity;

};

/*
1. 화면에 기본 도형 그리기
- 1.1 키보드 명령으로 도형을 그리고, 마우스 명령으로 도형을 선택한 후 추가의 키보드 명령을 수행한다.
- 1.2 위치, 색상,크기는 자율적으로 정하고, 최대 50개의 도형을 그린다.
2. 키보드 명령
- 2.1 p: 점 그리기
- 2.2 e/t/r: 선 그리기/ 삼각형그리기/ 사각형그리기(삼각형 2개로 그리기)
- 2.3 w/a/s/d: 그린 모든 도형 중 마우스로 한개를 선택한후 화면에서 상/좌/하/우측으로 이동한다.
- 2.4 i/j/k/l: 그린 도형 중 마우스로 한개를 선택한 후 화면에서 대각선(좌상/우상/좌하/우하)으로 이동한다.
- 2.5 1/2/3/4: 모든 도형들이 선택되어 좌/우/상/하로 이동한다.
- 2.6 c: 모든 도형을 삭제한다.
*/


float Cross2D(glm::vec2 a, glm::vec2 b);
bool IsInsidePoint(glm::vec2 p);
bool IsInsideLine(glm::vec2 p, float size);
bool IsInsideRectangle(glm::vec2 p, float size);


void addShape(int type);
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
std::string filetobuf(const char* file);


void TransformScreenToNDC(double& i, double& j);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void updatePos();
void updatePosAll(int direction);

GLvoid DrawScene();


GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;


std::vector<Shape> randShapes;
int selected = -1;
GLint offsetLoc{};
double prevFrame{};



glm::vec2 up{0.0f, 1.0f};
glm::vec2 down{ 0.0f, -1.0f };
glm::vec2 left{ -1.0f, 0.0f };
glm::vec2 right{ 1.0f, 0.0f };
glm::vec2 upRight = glm::normalize(glm::vec2(1.0f, 1.0f));
glm::vec2 upLeft = glm::normalize(glm::vec2(-1.0f, 1.0f));
glm::vec2 downLeft = glm::normalize(glm::vec2(-1.0f, -1.0f));
glm::vec2 downRight = glm::normalize(glm::vec2(1.0f, -1.0f));


int main()
{

    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // CORE_PROFILE로 변경해서 glRect 못쓴다. 


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
    randShapes.reserve(100);
    
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);


    //--- 세이더읽어와서세이더프로그램만들기
    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();

   
    prevFrame = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {

        
        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

}



// 점, 선, 삼각형, 사각형의 로컬 좌표계의 두 벡터
float Cross2D(glm::vec2 a, glm::vec2 b)
{
    return a.x * b.y - a.y * b.x;
    // 이건 사실상 2x2 행렬식의 계산이다. 
}


bool IsInsidePoint(glm::vec2 p)
{
    const float radius = 0.05f;

    return glm::length(p) <= radius;
}


bool IsInsideLine(glm::vec2 p, float size)
{
    const float thickness = 0.03f;

    return p.x >= -size &&
        p.x <= size &&
        std::abs(p.y) <= thickness;
    // p.x가 size의 크기 절대값 안에 있으면 되는거니까 그 거리가. 
}

bool IsInsideTriangle(glm::vec2 p, float size)
{
    glm::vec2 A(-size, -size);
    glm::vec2 B(size, -size);
    glm::vec2 C(0.0f, size);

    float c1 = Cross2D(B - A, p - A);
    float c2 = Cross2D(C - B, p - B);
    float c3 = Cross2D(A - C, p - C);

    bool allPositive =
        c1 >= 0.0f &&
        c2 >= 0.0f &&
        c3 >= 0.0f;

    bool allNegative =
        c1 <= 0.0f &&
        c2 <= 0.0f &&
        c3 <= 0.0f;

    return allPositive || allNegative;
}


bool IsInsideRectangle(glm::vec2 p, float size)
{
    return p.x >= -size &&
        p.x <= size &&
        p.y >= -size &&
        p.y <= size;
}




void addShape(int type)
{
    if (randShapes.size() >= 50)
        return;

    float size = disSize(gen);

    std::uniform_real_distribution<float> disXY{-1.0f + size, 1.0f - size};

    glm::vec2 xy{ disXY(gen), disXY(gen) }; // 무작위 오프셋

    float r = disColor(gen);
    float g = disColor(gen);
    float b = disColor(gen);

    std::vector<float> tempData;
    tempData.reserve(20);
    GLenum mode{};
    int vertCount{};

    if (type == 0) {  // 점
        tempData = { 0,0,0, r,g,b };
        mode = GL_POINTS;
        vertCount = 1;
    }
    else if (type == 1) {  // 선
        tempData = { -size,0,0, r,g,b, size,0,0, r,g,b };
        mode = GL_LINES;
        vertCount = 2;
    }
    else if (type == 2) {  // 삼각형
        tempData = { -size,-size,0, r,g,b, size,-size,0, r,g,b, 0,size,0, r,g,b };
        mode = GL_TRIANGLES;
        vertCount = 3;
    }
    else if (type == 3) {  // 사각형
        tempData = { -size,-size,0, r,g,b, size,-size,0, r,g,b, -size,size,0, r,g,b, 
            size,-size,0, r,g,b, size,size,0, r,g,b, -size,size,0, r,g,b };
        mode = GL_TRIANGLES;
        vertCount = 6;
    }

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

    Shape s;
    s.vao = vao;
    s.vbo = vbo;
    s.mode = mode;
    s.vertexCount = vertCount;
    s.pos = xy;
    s.size = size;

    randShapes.push_back(s);

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
        
        std::cout << "에러 있습니다." << std::endl;
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
        
        std::cout << "에러 있습니다." << std::endl;
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
        
        std::cout << "에러 있습니다." << std::endl;
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
        std::cout << "에러 있습니다." << std::endl;
        return "";

    }
    //--- 파일전체를문자열로읽기
    std::string source((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
    shaderFile.close();
    return source;
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
    case GLFW_KEY_P:
        addShape(0);
        break;
    case GLFW_KEY_E:
        addShape(1);
        break;
    case GLFW_KEY_T:
        addShape(2);
        break;
    case GLFW_KEY_R:
        addShape(3);
        break;
    
    case GLFW_KEY_W:
        if (selected != -1) {
            randShapes[selected].velocity = up;
            updatePos();
        }
            
        break;
    case GLFW_KEY_A:
        if (selected != -1) {
            randShapes[selected].velocity = left;
            updatePos();
        }
            
        break;
    case GLFW_KEY_S:
        if (selected != -1) {
            randShapes[selected].velocity = down;
            updatePos();
        }
            
        break;
    case GLFW_KEY_D:
        if (selected != -1) {
            randShapes[selected].velocity = right;
            updatePos();
        }
        
        break;
    
    
    case GLFW_KEY_I:
        if (selected != -1) {
            randShapes[selected].velocity = upLeft;
            updatePos();
            
        }
        
        break;
    case GLFW_KEY_J:
        if (selected != -1) {
            randShapes[selected].velocity = upRight;
            updatePos();
        }
        
        break;
    case GLFW_KEY_K:
        if (selected != -1) {
            randShapes[selected].velocity = downLeft;
            updatePos();
        }
        
        break;
    case GLFW_KEY_L:
        if (selected != -1) {
            randShapes[selected].velocity = downRight;
            updatePos();
        
        }
        
        break;
    

    case GLFW_KEY_1:
       

        updatePosAll(0);
        break;
    case GLFW_KEY_2:
       

        updatePosAll(1);
        break;
    case GLFW_KEY_3:
        

        updatePosAll(2);
        break;
    case GLFW_KEY_4:
     

        updatePosAll(3);
        break;


    case GLFW_KEY_C:
        // 모든 도형을 삭제한다. 

        randShapes.clear();
        selected = -1;
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

        glm::vec2 mousePos{xpos, ypos};

        bool inside{ false };

        for (int i = randShapes.size() - 1; i >= 0; --i)
        {
            glm::vec2 local = mousePos - randShapes[i].pos;

            switch (randShapes[i].vertexCount)
            {
            case 1:
                inside = IsInsidePoint(local);
                break;
            case 2:
                inside = IsInsideLine(local, randShapes[i].size);
                break;
            case 3:
                inside = IsInsideTriangle(local, randShapes[i].size);
                break;
            case 6:
                inside = IsInsideRectangle(local, randShapes[i].size);
                break;

            }

            if (inside)
            {
                selected = i;
                break;
            }


        }

    }
}

void updatePos()
{
    double currentFrame = glfwGetTime();

    double dealth = currentFrame - prevFrame;

    Shape& shape = randShapes[selected];

    shape.pos.x += shape.velocity.x * MOVE * dealth;
    shape.pos.y += shape.velocity.y * MOVE * dealth;
}



void updatePosAll(int direction)
{
    double currentFrame = glfwGetTime();

    for (int i = 0; i < randShapes.size(); ++i)
    {
        double dealth = currentFrame - prevFrame;

        Shape& shape = randShapes[i];

        switch (direction)
        {
        case 0:
            shape.velocity = left;
            break;
        case 1:
            shape.velocity = right;
            break;
        case 2:
            shape.velocity = up;
            break;
        case 3:
            shape.velocity = down;
            break;
        }

        shape.pos.x += shape.velocity.x * MOVE * dealth;
        shape.pos.y += shape.velocity.y * MOVE * dealth;
    }
    
}


void DrawScene()
{

    GLfloat rColor, gColor, bColor;
    rColor = gColor = 1.0f;
    bColor = 1.0f;
    
    //--- 배경색을파란색으로설정
    glClearColor(rColor, gColor, bColor, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    //--- 셰이더프로그램사용
    glUseProgram(shaderProgramID);
    
    //--- 점의 크기설정
    glPointSize(20.0f);
    
    
    
    offsetLoc = glGetUniformLocation(shaderProgramID, "uOffset");

    GLint useOverrideColorLoc =
        glGetUniformLocation(shaderProgramID, "useOverrideColor");

    GLint overrideColorLoc =
        glGetUniformLocation(shaderProgramID, "overrideColor");

    glUniform1i(useOverrideColorLoc, GL_FALSE);
    glUniform3f(overrideColorLoc, 0.0f, 0.0f, 0.0f);

    for (int i=0; i<randShapes.size(); ++i)
    {
        Shape& shape = randShapes[i];
        double currentFrame = glfwGetTime();

        double dealth = currentFrame - prevFrame;

        /* // 적어도 내가 생각했을떄는 DrawScene 말고 다른 곳에서 shape.pos를 업데이트 해야됨.
        if (selected == i)
        {
            // 면 그리는 로직. 그 현재 프레임 - 이전 프레임 만큼 MOVE 옆에 곱해줘야됨 지금.
            shape.pos.x = shape.pos.x + shape.velocity.x * dealth;
            shape.pos.y = shape.pos.y + shape.velocity.y * dealth;
        }
        */

        glBindVertexArray(shape.vao);

        glUniform2f(offsetLoc, shape.pos.x, shape.pos.y);
        glUniform1i(useOverrideColorLoc, GL_FALSE);
        

        glDrawArrays(shape.mode, 0, shape.vertexCount);  

        prevFrame = currentFrame;

        if (selected == i)
        {
            
            // 테두리 그리기
            glUniform1i(useOverrideColorLoc, GL_TRUE);
            glUniform3f(overrideColorLoc, 0.0f, 0.0f, 0.0f);

            glLineWidth(5.0f);
            glDrawArrays(GL_LINE_LOOP, 0, shape.vertexCount);

            glLineWidth(1.0f);
            
        }
     
        
    }
   
}


// glDrawArrays는 현재 바인딩된 VAO의 설정을 보고 데이터를 읽는다. 

// glBufferData와 glBufferSubData로 VBO 데이터를 바꿀 떄 신경을 써야 한다. 
// 이건 버퍼를 다시 올린다고 말한다. 변경된 VBO를 매번 줘서 쉐이더 코드 내에서 들어오는 값대로 처리하는 방식이다. 

// 예를 들어 원점이 (0,0)이라 가정했을떄 셰이더 코드에는 항상 (0,0)만 준다. 근데 누적 이동량을 계산해서
// uniform으로 쉐이더 코드에 주는 것도 하나의 방법이다. 매 프레임 이동량을 0.2,0.2로 정했으면 그걸 매프레임 0.4, 0.6
// 이렇게 증가시켜서 누적된 이동량을 전달해서 위치 계산하는 것이다. 

// 이 방법을 쓰는 이유는 셰이더 코드는 변화가 생겨 이동한 위치를 따로 저장해서 보내줄 수 없기 떄문이다. 