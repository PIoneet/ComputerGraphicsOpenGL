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

std::uniform_int_distribution<int> disRand{ 1,4 };
std::uniform_int_distribution<int> disDir{ 1,2 };

#define WIDTH  1300.0
#define HEIGHT 1300.0

/*
실습 8에서 그린 삼각형들이 각각 다른 방향과 속도로 이동한다. 
1. 실습 8과 같이 삼각형을 그릴 수 있다. 
- 1.1 키보드 1: 튕기기 이동. 
- 1.2 좌우로 지그재그 이동한다. 벽에 닿으면 반대 방향으로 생성해서 그려야 한다. 
- 1.3 상하로 뾰족하게 지그재그로 이동해야 한다. 
- 1.4 원 스파이럴로 이동해야 합니다. 방향에 따라 회전 필요하지 않다. 
2. 회전은 사용하지 않고 회전해야 되면 그 방향으로 생성해서 그리면 된다.
- 2.1 가장자리를 만나게 되면 삼각형의 방향이 바뀌게 된다. 
*/


struct Triangle
{
    GLuint vao{};
    GLuint vbo{};

    glm::vec2 pos{}; //오프셋
    glm::vec3 rgb{};
    float size{}; // 크기
    int vertexCount{};

    glm::vec2 velocity{};

    float angle{};
    float radius{};

    glm::vec2 origin{};

    // 이게 삼각형마다 경로가 존재해야 되서 멤버 변수로
    GLuint pathVao{};
    GLuint pathVbo{};

    int frameCnt{};
    std::vector<glm::vec2> path;
};


void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
std::string filetobuf(const char* file);

void addShape(const glm::vec2& mousePos);
void updatePos(Triangle& tr);
void TransformScreenToNDC(double& i, double& j);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

GLvoid DrawScene();

GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;

GLuint offsetLoc{};

std::vector<Triangle> triangles;

int currentMove{};
double prevFrame{};

glm::vec2 left{-1.0f, 0.0f};
glm::vec2 right{ 1.0f, 0.0f };

glm::vec2 dir1 = glm::normalize(glm::vec2{ 1.0f, 1.0f });              // 우상
glm::vec2 dir2 = glm::normalize(glm::vec2{ -1.0f, 1.0f });             // 좌상
glm::vec2 dir3 = glm::normalize(glm::vec2{ 1.0f, -1.0f });             // 우하
glm::vec2 dir4 = glm::normalize(glm::vec2{ -1.0f, -1.0f });

glm::vec2 diag1 = glm::normalize(glm::vec2{ 0.2f, -0.2f });

glm::vec2 upRight = glm::normalize(glm::vec2{0.1f, 1.0f});


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
    triangles.reserve(50);

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);


    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();
    offsetLoc = glGetUniformLocation(shaderProgramID, "uOffset");

    prevFrame = glfwGetTime();

   

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

//1번
void moveAlong(double& prevFrame)
{
    double currentFrame = glfwGetTime();

    for (auto& tr : triangles)
    {
        double dealth = currentFrame - prevFrame;

        // 벽에 튕기면 방향을 바꾸도록 해야한다. 
        if (tr.pos.x - tr.size <= -1.0f || tr.pos.x + tr.size >= 1.0f)
        {
            tr.velocity.x = -1 * tr.velocity.x;
        }

        if (tr.pos.y - tr.size <= -1.0f || tr.pos.y + tr.size >= 1.0f)
        {
            tr.velocity.y = -1 * tr.velocity.y;

        }

        tr.pos.x = std::clamp(tr.pos.x, -1.0f + tr.size, 1.0f - tr.size);
        tr.pos.y = std::clamp(tr.pos.y, -1.0f + tr.size, 1.0f - tr.size);


        tr.pos.x += tr.velocity.x * dealth;
        tr.pos.y += tr.velocity.y * dealth;

        updatePos(tr);

    }
}

//2번
void moveLeftRight(double& prevFrame)
{
    double currentFrame = glfwGetTime();

    for (auto& tr : triangles)
    {
        double dealth = currentFrame - prevFrame;

        std::vector<float> tempData;
        tempData.reserve(50);

        // 벽에 튕기면 방향을 바꾸도록 해야한다. 
        if (tr.pos.x - tr.size <= -1.0f )
        {
            tr.velocity.x = -1 * tr.velocity.x;

            tempData = { -tr.size, tr.size, 0, tr.rgb.x,tr.rgb.y,tr.rgb.z,
                -tr.size, -tr.size, 0, tr.rgb.x,tr.rgb.y,tr.rgb.z,
                tr.size * 2, 0, 0, tr.rgb.x,tr.rgb.y,tr.rgb.z };

            tr.pos.y -= 1.0f * dealth * 200.0f;
            
        }
        else if (tr.pos.x + tr.size >= 1.0f)
        {
            tr.velocity.x = -1 * tr.velocity.x;
           
           
            tempData = { -tr.size * 2,0,0, tr.rgb.x,tr.rgb.y,tr.rgb.z,
                tr.size,-tr.size,0, tr.rgb.x,tr.rgb.y,tr.rgb.z,
                tr.size, tr.size, 0, tr.rgb.x,tr.rgb.y,tr.rgb.z };

            tr.pos.y -= 1.0f * dealth * 200.0f;
        }
      

        glBindVertexArray(tr.vao);
        glBindBuffer(GL_ARRAY_BUFFER, tr.vbo);
        glBufferData(GL_ARRAY_BUFFER, tempData.size() * sizeof(float), tempData.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);


        tr.pos.x = std::clamp(tr.pos.x, -1.0f + tr.size, 1.0f - tr.size);
        tr.pos.y = std::clamp(tr.pos.y, -1.0f + tr.size, 1.0f - tr.size);


        tr.pos.x += tr.velocity.x * dealth;
        tr.pos.y += tr.velocity.y * dealth;

        updatePos(tr);
    }
}


void reshapeTr()
{
    for (auto& tr : triangles)
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

//3번
void zigzagMove(double& prevFrame)
{
    double currentFrame = glfwGetTime();

    for (auto& tr : triangles)
    {
        double dealth = currentFrame - prevFrame;

        std::vector<float> tempData;
        tempData.reserve(50);

        // 벽에 튕기면 방향을 바꾸도록 해야한다. 
        if (tr.pos.y - tr.size <= -1.0f)
        {
            tr.velocity.y = -1 * tr.velocity.y;

            tempData = { -tr.size, -tr.size, 0, tr.rgb.x,tr.rgb.y,tr.rgb.z,
                tr.size, -tr.size, 0, tr.rgb.x,tr.rgb.y,tr.rgb.z,
                0, 2 * tr.size, 0, tr.rgb.x,tr.rgb.y,tr.rgb.z };

            tr.pos.x -= dealth * 200.0f;

        }
        else if (tr.pos.y + tr.size >= 1.0f)
        {
            tr.velocity.y = -1 * tr.velocity.y;


            tempData = { -tr.size ,tr.size,0, tr.rgb.x,tr.rgb.y,tr.rgb.z,
                0,-2 * tr.size,0, tr.rgb.x,tr.rgb.y,tr.rgb.z,
                tr.size, tr.size, 0, tr.rgb.x,tr.rgb.y,tr.rgb.z };

            tr.pos.x -= dealth * 200.0f;
        }

        if (tr.pos.x - tr.size <= -1.0f || tr.pos.x + tr.size >= 1.0f)
        {
            tr.velocity.x = -1 * tr.velocity.x;
        }

        glBindVertexArray(tr.vao);
        glBindBuffer(GL_ARRAY_BUFFER, tr.vbo);
        glBufferData(GL_ARRAY_BUFFER, tempData.size() * sizeof(float), tempData.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);


        tr.pos.x = std::clamp(tr.pos.x, -1.0f + tr.size, 1.0f - tr.size);
        tr.pos.y = std::clamp(tr.pos.y, -1.0f + tr.size, 1.0f - tr.size);


        tr.pos.x += tr.velocity.x * dealth;
        tr.pos.y += tr.velocity.y * dealth;

        updatePos(tr);
    }
}

//4번
void circleMove(double& prevFrame)
{
    float currentFrame = glfwGetTime();
    
    float dealth = currentFrame - prevFrame;
    
    for (auto& tr : triangles)
    {
        tr.angle += 1.2 * dealth;
        tr.radius += 0.2 * dealth;

        tr.pos.x = tr.origin.x + tr.radius * cos(tr.angle);
        tr.pos.y = tr.origin.y + tr.radius * sin(tr.angle);

        updatePos(tr);
    }


}


void addShape(const glm::vec2& mousePos)
{ // 삼각형 그리기. 

    float size = disSize(gen);

    glm::vec2 pos{ mousePos };
    glm::vec3 rgb{ disColor(gen), disColor(gen), disColor(gen) };
   
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

    t.pos = pos;
    t.rgb = rgb;

    t.size = size;
    t.vertexCount = vertexCount;

    t.origin.x = pos.x;
    t.origin.y = pos.y;

    t.path.push_back(t.pos);
    glGenVertexArrays(1, &t.pathVao);
    glGenBuffers(1, &t.pathVbo);
    
    triangles.push_back(t);

}


void updatePos(Triangle& tr)
{

    ++tr.frameCnt;
    if (tr.frameCnt < 120) return; 
    tr.frameCnt = 0;

    tr.path.push_back(tr.pos);

    std::vector<float> data;

    data.reserve(6 * sizeof(float));

    for (auto& p : tr.path)
    {
        data.insert(data.end(), {p.x, p.y ,0, 1.0f, 0.0f, 0.0f});
    }

    glBindVertexArray(tr.pathVao);
    glBindBuffer(GL_ARRAY_BUFFER, tr.pathVbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
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
    case GLFW_KEY_1:
        if (currentMove != 1)
        {
            for (auto& tr : triangles)
            {
                switch (disRand(gen))
                {
                case 1:
                    tr.velocity = dir1;
                    break;
                case 2:
                    tr.velocity = dir2;
                    break;
                case 3:
                    tr.velocity = dir3;
                    break;
                case 4:
                    tr.velocity = dir4;
                    break;
                }
            }

            currentMove = 1;
        }
        else
            currentMove = 0;
        break;

    case GLFW_KEY_2:
        if (currentMove != 2)
        {
            for (auto& tr : triangles)
            {
                switch (disDir(gen))
                {
                case 1:
                    tr.velocity = left;
                    break;
                case 2:
                    tr.velocity = right;
                    break;
                }

            }

            currentMove = 2;
        }
        else
        {
            reshapeTr();
            currentMove = 0;
        }
            
        break;

    case GLFW_KEY_3:
        if (currentMove != 3)
        {
            for (auto& tr : triangles)
            {
                tr.velocity = upRight;
            }

            currentMove = 3;
        }
        else
        {
            reshapeTr();
            currentMove = 0;
        }
            
        break;
        
    case GLFW_KEY_4:
        if (currentMove != 4)
        {
         
            currentMove = 4;
        }
        else
        {
            for (auto& tr : triangles)
            {
                tr.angle = 0.0f;
                tr.radius = 0.0f;
            }
            currentMove = 0;
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
        

        addShape(mousePos);

        
    }
}



void DrawScene()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //--- 셰이더프로그램사용
    glUseProgram(shaderProgramID);


    switch (currentMove)
    {
    case 1:
        moveAlong(prevFrame);
        break;
    case 2:
        moveLeftRight(prevFrame);
        break;
    case 3:
        zigzagMove(prevFrame);
        break;
    case 4:
        circleMove(prevFrame);
        break;

    }
    
    prevFrame = glfwGetTime();

    for (int i = 0; i < triangles.size(); ++i)
    {
        Triangle& tr = triangles[i];

        glBindVertexArray(tr.vao);

        glUniform2f(offsetLoc, tr.pos.x, tr.pos.y);

        glDrawArrays(GL_TRIANGLES, 0, tr.vertexCount);


        if (tr.path.size() >= 1)
        {
            glBindVertexArray(tr.pathVao);
            glUniform2f(offsetLoc, 0.0f, 0.0f);

            glDrawArrays(GL_LINE_STRIP, 0, tr.path.size());
        }
    }

}

