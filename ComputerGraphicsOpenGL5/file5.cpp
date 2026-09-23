#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <random>
#include <algorithm>

std::random_device rd;
std::mt19937 gen(rd());

std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };

std::uniform_real_distribution<float> disR{ 0.08f, 0.18f };

std::uniform_int_distribution<int> disNum{20, 40}; // 랜덤으로 생성될 사각형의 개수

std::uniform_real_distribution<float> disPos{ -1.0f, 1.0f };


#define WIDTH  1600.0
#define HEIGHT 1600.0
#define RECT_SIZE 0.04


struct Rect {
    float x{};
    float y{};

    float r{};
    glm::vec3 rgb{};
    bool isDraw{true};
};

/*
1. 윈도우 띄우고 화면에 자동으로 같은 크기의 사각형들을 다양한 색으로 랜덤의 위치에 20~40개 생성한다. 
2. 왼쪽 마우스 버튼을 누르면 지우개 사각형이 생긴다. 지우개 사각형은 다른 사각형의 2배의 크기고 초기 색상은 검정이다. 
- 2.2 마우스 누른 채로 드래그하면 지우개 사각형이 움직인다. 
- 2.3 지우개 사각형과 부딪친 사각형은 사라진다. 사각형이 사라지면 지우개 사각형의 크기가 커지고, 지우개 색상은 부딪친 사각형
색상으로 변한다. 
- 2.4 왼쪽 마우스 버튼을 떄면 지우개 사각형이 사라지고, 사라졌던 사각형들은 원래의 위치에 나타난다. 
- 2.5 다시 왼쪽 마우스로 클릭하면 그 위치에 커졌던 지우개 사각형의 크기가 유지된채 검정색으로 나타난다. 
- 2.6 오른쪽 마우스 누르면 그 위치에 사각형이 새롭게 생긴다. 
- 2.7 새로운 사각형이 생기면 지우개 사각형의 크기가 줄어든다. 최대 10개까지 생성 가능하다. 콘솔로 출력하는게 좋을듯 이건. 
- 2.8 r을 누르면 20~40개 사각형도 다시 생성하고 다 삭제하고 지우개 사각형도 초기화 해야 한다. 
*/

void isOverlap();
void generateRect();
void TransformScreenToNDC(double& i, double& j);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void DrawScene();


std::vector<Rect> randRects;

Rect eraseRect; // 지우기 사각형

bool Dragged{false};
glm::vec2 mousePos{};
int genRect{};


int main()
{

    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

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



    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);


    randRects.reserve(100);
    generateRect();

    eraseRect.r = 2 * RECT_SIZE;
    eraseRect.rgb = glm::vec3{ 0.0f, 0.0f, 0.0f };
    eraseRect.isDraw = false;


    while (!glfwWindowShouldClose(window)) {

        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();



}




void isOverlap()
{
    for (int i = 0; i < randRects.size(); ++i)
    {
        if (eraseRect.x + eraseRect.r <= randRects[i].x - randRects[i].r ||
            randRects[i].x + randRects[i].r <= eraseRect.x - eraseRect.r ||
            eraseRect.y + eraseRect.r <= randRects[i].y - randRects[i].r ||
            randRects[i].y + randRects[i].r <= eraseRect.y - eraseRect.r ||
            randRects[i].isDraw == false)
        {
            std::cout << "사각형이 겹치지 않았습니다." << std::endl;
            
        }
        else {
            std::cout << "사각형이 겹쳤습니다." << " ";
            std::cout << "사각형 인덱스: " << i << " " << std::endl;

            randRects[i].isDraw = false;
            eraseRect.rgb = randRects[i].rgb;
            eraseRect.r += RECT_SIZE / 4;
            
        }

    }

}



void generateRect()
{
    int randNum = disNum(gen);

    for (int i = 0; i < randNum; ++i)
    {
        glm::vec3 randColor{disColor(gen), disColor(gen), disColor(gen)};

        randRects.push_back(Rect{ disPos(gen), disPos(gen), RECT_SIZE, randColor });
    }
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
    // 다 삭제하고 초기화하기       
        randRects.clear();
        generateRect();
        eraseRect.r = 2 * RECT_SIZE;
        eraseRect.rgb = glm::vec3{0.0f, 0.0f, 0.0f};
        eraseRect.isDraw = false;
        genRect = 0;

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

        eraseRect.x = xpos;
        eraseRect.y = ypos;
        eraseRect.isDraw = true;
        eraseRect.rgb = glm::vec3{ 0.0f,0.0f,0.0f };


        Dragged = true;

        mousePos.x = xpos;
        mousePos.y = ypos;
        
    
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if (genRect < 10)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            TransformScreenToNDC(xpos, ypos);

            glm::vec3 randColor{ disColor(gen), disColor(gen), disColor(gen) };

            randRects.push_back(Rect{ (float)xpos, (float)ypos, RECT_SIZE, randColor });


            eraseRect.r = std::max( eraseRect.r - RECT_SIZE / 2, RECT_SIZE/2 );

            ++genRect;
        }


    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        Dragged = false;


        for (auto& rect : randRects)
        {
            rect.isDraw = true;

        }

        eraseRect.isDraw = false;
    }
    
  
    
}




void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (Dragged)
    {
        TransformScreenToNDC(xpos, ypos);

        std::pair<double, double> move{ xpos - mousePos.x , ypos - mousePos.y };
        // 현재 위치 xpos에서 이전 위치 mousePos.x를 빼서 이동량을 구해야 한다. 

        isOverlap();

        eraseRect.x = eraseRect.x + move.first;
        eraseRect.y = eraseRect.y + move.second;

        mousePos.x = xpos;
        mousePos.y = ypos;

    }
}




void DrawScene()
{
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (auto& rect : randRects)
    {
        if (rect.isDraw == false)
            continue;

        glColor3f(rect.rgb.x, rect.rgb.y, rect.rgb.z);
        glRectf(rect.x - rect.r, rect.y - rect.r, rect.x + rect.r, rect.y + rect.r);

    }

    if (eraseRect.isDraw != false)
    {
        glColor3f(eraseRect.rgb.x, eraseRect.rgb.y, eraseRect.rgb.z);
        glRectf(eraseRect.x - eraseRect.r, eraseRect.y - eraseRect.r, eraseRect.x + eraseRect.r, eraseRect.y + eraseRect.r);
    }
    


}