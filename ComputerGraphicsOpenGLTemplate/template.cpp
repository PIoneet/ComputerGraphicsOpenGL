#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <fstream>
#include <string>

#define WIDTH  1300.0
#define HEIGHT 1300.0



void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
std::string filetobuf(const char* file);

GLvoid DrawScene();
GLvoid Reshape(int w, int h);


//--- 필요한변수선언
GLint width, height;
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;


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


    //glfwSetMouseButtonCallback(window, MouseButtonCallback);


    //--- 세이더읽어와서세이더프로그램만들기
    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();


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



void DrawScene()
{
    GLfloat rColor, gColor, bColor;
    rColor = gColor = 0.0f;
    bColor = 1.0f;

    //--- 배경색을파란색으로설정
    glClearColor(rColor, gColor, bColor, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //--- 셰이더프로그램사용
    glUseProgram(shaderProgramID);

    //--- 점의 크기설정
    glPointSize(5.0f);

    //--- 0번 인덱스에서1개의버텍스를사용하여점그리기
    glDrawArrays(GL_POINTS, 0, 1);
}