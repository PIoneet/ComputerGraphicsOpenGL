#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <array>
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

// 일단 DrawScene 초반 부분에 x축 y축 line 그려야 함. 

// 4개의 사분면을 저장할 구조체.

struct Triangle
{
    GLuint vao{};
    GLuint vbo{};

    GLenum drawMode{};  // LINE_LOOP 이거나 GL_TRIANGLES
    
    glm::vec2 pos{}; //오프셋
    glm::vec3 rgb{};
    float size{}; // 크기
    int vertexCount{};

    // 근데 진짜 내 생각에는 flag로 안그려지게 하고 c로 vector.clear()하는게 나은듯. 
    bool isDraw{true};

};


struct Rect
{
    // 어쩌피 그릴건 아니니까 vao vbo 필요없을 것 같은데
    Triangle t;
    int idx{};

    bool isTriangle{ false };

    glm::vec2 pos{};
    float size{ 0.5 };
};


void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
std::string filetobuf(const char* file);

void rectInit();
bool IsInsideRectangle(glm::vec2 p, float size);
void scaleShape(const glm::vec2& mousePos, int idx);
void addShape(const glm::vec2& mousePos, int idx);
void TransformScreenToNDC(double& i, double& j);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
GLvoid DrawLine();
GLvoid DrawScene();

GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;

std::array<Rect, 4> area;
GLuint offsetLoc{};
GLboolean toggleScale{false};

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
    rectInit();

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);


    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();
    offsetLoc = glGetUniformLocation(shaderProgramID, "uOffset");

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

void rectInit()
{
    area[0].pos.x = 0.5f;
    area[0].pos.y = 0.5f;

    area[1].pos.x = -0.5f;
    area[1].pos.y = 0.5f;

    area[2].pos.x = -0.5f;
    area[2].pos.y = -0.5f;

    area[3].pos.x = 0.5f;
    area[3].pos.y = -0.5f;

}


bool IsInsideRectangle(glm::vec2 p, float size)
{
    return p.x >= -size &&
        p.x <= size &&
        p.y >= -size &&
        p.y <= size;
}


void scaleShape(const glm::vec2& mousePos, int idx)
{
    float size;
    
    if(toggleScale)
        size = 2 * disSize(gen);
    else
        size = disSize(gen) / 2;
    
    std::vector<float> tempData;
    tempData.reserve(50);

    auto& tr = area[idx].t;

    tempData = { -size,-size,0, tr.rgb.x,tr.rgb.y,tr.rgb.z, 
        size,-size,0, tr.rgb.x,tr.rgb.y,tr.rgb.z, 0, 2 * size, 0, tr.rgb.x,tr.rgb.y,tr.rgb.z };

   
    glBindVertexArray(tr.vao);
    glBindBuffer(GL_ARRAY_BUFFER, tr.vbo);
    glBufferData(GL_ARRAY_BUFFER, tempData.size() * sizeof(float), tempData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    area[idx].t.size = size;

}

void addShape(const glm::vec2& mousePos, int idx)
{ // 삼각형 그리기. 

    float size = disSize(gen);

    glm::vec2 pos{ mousePos };
    glm::vec3 rgb{ disColor(gen), disColor(gen), disColor(gen) };
    GLenum drawMode{ GL_TRIANGLES };
    int vertexCount{ 3 };

    std::vector<float> tempData;
    tempData.reserve(50);

    tempData = { -size,-size,0, rgb.x,rgb.y,rgb.z, size,-size,0, rgb.x,rgb.y,rgb.z, 0,2 * size,0, rgb.x,rgb.y,rgb.z };
       
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

    Triangle t;

    t.vao = vao;
    t.vbo = vbo;

    t.drawMode = drawMode;
    t.pos = pos;
    t.rgb = rgb;

    t.size = size;
    t.vertexCount = vertexCount;

    area[idx].t = t;
    area[idx].isTriangle = true;

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
    case GLFW_KEY_A:
        for (auto& a : area)
        {
            a.t.drawMode = GL_TRIANGLES;
        }

        break;
    case GLFW_KEY_B:
        for (auto& a : area)
        {
            a.t.drawMode = GL_LINE_LOOP;
        }

        break;
    case GLFW_KEY_C:
        
        int i{};
        for (auto& a : area)
        {
            a.isTriangle = false;
            //std::uniform_real_distribution<float> disXY{ -1.0f, 1.0f };
            //addShape(glm::vec2{ disXY(gen), disXY(gen) }, i);
            //++i;
        }

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

        glm::vec2 mousePos{ xpos, ypos };

        bool inside{ false };

        for (int i = 0; i < area.size(); ++i)
        {
            glm::vec2 localPos{mousePos - area[i].pos};

            inside = IsInsideRectangle(localPos, area[i].size);

            if (inside)
            { // 피킹된 사분면을 찾았다  
                std::cout << "삼각형 추가하겠습니다." << std::endl;
                addShape(mousePos, i);

                // 찾았으니까 루프 종료.
                break;
            }
            
        }
    }
    
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {   //토글해서 한번 키우고 한번 축소하고 하면됨.
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        TransformScreenToNDC(xpos, ypos);

        glm::vec2 mousePos{ xpos, ypos };

        bool inside{ false };

        for (int i = 0; i < area.size(); ++i)
        {
            glm::vec2 localPos{ mousePos - area[i].pos };

            inside = IsInsideRectangle(localPos, area[i].size);

            if (inside)
            { // 피킹된 사분면을 찾았다  
                std::cout << "삼각형 확대/축소" << std::endl;
                scaleShape(mousePos, i);

                if (toggleScale)
                    toggleScale = false;
                else
                    toggleScale = true;

                // 찾았으니까 루프 종료.
                break;
            }

        }

    }

}

void DrawLine()
{
    std::vector<float> tempData;
    tempData.reserve(30);

    tempData = { -1.0f, 0.0f,0.0f, 0.0f,0.0f,1.0f, 1.0f,0.0f,0, 0.0f,0.0f,1.0f};

    for (int i = 0; i < 2; ++i)
    {
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

        tempData.clear();
        tempData = { 0.0f, 1.0f,0.0f, 0.0f,0.0f,1.0f, 0.0f,-1.0f,0, 0.0f,0.0f,1.0f };
    }

}

void DrawScene()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //--- 셰이더프로그램사용
    glUseProgram(shaderProgramID);

    // x축 y축 경계선 그리기
    DrawLine();

    

    for (int i = 0; i < area.size(); ++i)
    {
        Rect& a = area[i];

        if (!a.isTriangle)
            continue;

        glBindVertexArray(a.t.vao);

        glUniform2f(offsetLoc, a.t.pos.x, a.t.pos.y);

        glDrawArrays(a.t.drawMode, 0, a.t.vertexCount);
    }
    
}