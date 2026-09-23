#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <random>

std::random_device rd;
std::mt19937 gen(rd());

std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };

std::uniform_real_distribution<float> disR{ 0.08f, 0.18f };


#define WIDTH  1400.0
#define HEIGHT 1400.0


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

void TransformScreenToNDC(double& i, double& j);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
//void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void DrawScene();


std::vector<Rect> randRects;

Rect eraseRect; // 지우기 사각형

bool Dragged{false};
glm::vec2 mousePos{};


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


    randRects.reserve(50);



    while (!glfwWindowShouldClose(window)) {

        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();



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
    }

}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
    }

}


/*
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (Dragged)
    {
        TransformScreenToNDC(xpos, ypos);

        std::pair<double, double> move{ xpos - mousePos.x , ypos - mousePos.y };
        // 현재 위치 xpos에서 이전 위치 mousePos.x를 빼서 이동량을 구해야 한다. 

        randRects[rectNum].x1 = randRects[rectNum].x1 + move.first;
        randRects[rectNum].y1 = randRects[rectNum].y1 + move.second;

        mousePos.x = xpos;
        mousePos.y = ypos;

    }
}
*/

void DrawScene()
{
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (auto& rect : randRects)
    {
        glColor3f(rect.rgb.x, rect.rgb.y, rect.rgb.z);
        glRectf(rect.x - rect.r, rect.y - rect.r, rect.x + rect.r, rect.y + rect.r);

    }

    glColor3f(eraseRect.rgb.x, eraseRect.rgb.y, eraseRect.rgb.z);
    glRectf(eraseRect.x - eraseRect.r, eraseRect.y - eraseRect.r, eraseRect.x + eraseRect.r, eraseRect.y + eraseRect.r);


}