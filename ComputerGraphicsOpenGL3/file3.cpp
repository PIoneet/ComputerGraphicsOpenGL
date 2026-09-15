#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <random>

std::random_device rd;
std::mt19937 gen(rd());

std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };

std::uniform_real_distribution<float> disWH{ 0.05f, 0.4f };

#define WIDTH  1200.0
#define HEIGHT 1200.0


struct Rect {

    float x1;
    float y1;
    float w;
    float h;

    glm::vec3 rgb;
    bool selected{ false };
    bool isDraw{ true };

};


std::vector<Rect> randRects;
bool Dragged{ false };
int rectNum{};
glm::vec2 mousePos{};


void TransformScreenToNDC(double& i, double& j);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void DrawScene();

int main() {

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

    randRects.reserve(50);

    while (!glfwWindowShouldClose(window)) {

        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}


// 마우스 좌표계에서 openGL 좌표계로 변환하고자 한다. 
void TransformScreenToNDC(double& i, double& j)
{
    double transHalfWidth = 2 / WIDTH;
    double transHalfHeight = - 2 / HEIGHT;
    
    i = i * transHalfWidth - 1;
    j = j * transHalfHeight + 1;
}

// --- 키보드 콜백 ---
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // 키보드 콜백은 키보드 검사만 해야 한다. 그릴떄는 DrawScene에서 해야한다. 
    // 여기서 그려버리면 DrawScene에서 화면 지우기 해버려서 날아갈 수도 있다. 

    if (action != GLFW_PRESS) return; // 누르는 순간만 처리

    switch (key){
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

    case GLFW_KEY_A:
    {
        if (randRects.size() < 10) 
        {
            glm::vec3 tempColor{ disColor(gen), disColor(gen) , disColor(gen) };

            float w = disWH(gen);
            float h = disWH(gen);

            std::uniform_real_distribution<float> disXY{ -1.0f, 1.0f - w };

            float x1 = disXY(gen);
            float y1 = disXY(gen);

            Rect temp{ x1, y1, w, h , tempColor };

            randRects.push_back(temp);

        }
        break;
    }
 
    }
}


// --- 마우스 콜백 (선택 처리) ---
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    else if( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        // 위 함수를 사용하면 마우스 클릭했을떄 마우스 커서의 현 위치를 바로 알 수 있다. 

        TransformScreenToNDC(xpos, ypos);

        mousePos.x = xpos;
        mousePos.y = ypos;

        // 히트 테스트를 진행해야 한다. 

        bool hit{ false };
        for(int i = randRects.size() - 1; i >= 0; --i)
        {
            if (randRects[i].x1 <= xpos && xpos <= randRects[i].x1 + randRects[i].w
                && randRects[i].y1 <= ypos && ypos <= randRects[i].y1 + randRects[i].h) 
            {
                rectNum = i;
                Dragged = true;
                hit = true;
                break;
            }
        
        }
        
        if (!hit) {
            rectNum = -1;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        Dragged = false;
    }
}


void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (Dragged)
    {
        TransformScreenToNDC(xpos, ypos);

        std::pair<double, double> move{ xpos - mousePos.x , ypos - mousePos.y };

        randRects[rectNum].x1 = randRects[rectNum].x1 + move.first;
        randRects[rectNum].y1 = randRects[rectNum].y1 + move.second;
        
        mousePos.x = xpos;
        mousePos.y = ypos;

    }
}


void DrawScene()
{
    glClearColor(0.5f, 0.3f, 0.4f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT);


    for (auto& rect : randRects)
    {
        if (rect.isDraw)
        {
            glColor3f(rect.rgb.x, rect.rgb.y, rect.rgb.z);
            glRectf(rect.x1, rect.y1, rect.x1 + rect.w, rect.y1 + rect.h);
        }
    }


}

// DrawScene의 핵심은 glClear()가 있다는 것이다. 이게 진짜 중요한게 더블 버퍼링이라는 개념 자체가 
// 백 버퍼와 프론트 버퍼를 교체하는 방식이다. 백 버퍼에 그리고 프론트 버퍼를 읽는 상황에 두 개를 교체하는 것이다. 
// glClear()가 없으면 그리고 싶지 않은건 그리지 않는다는 선택지 자체가 없어진다. 매 프레임 그려야 될 걸 그리면서
// 이번 프레임에는 그리지 않을걸 제외시키면 그것만 안 그리니까. 





// 오케이 지금부터 해야될 것을 정리를 해보자면 마우스 클릭 검사할떄 hit test를 1번 진행해야됩니다. 
// 히트 테스트는 마우스 좌표계의 x,y를 NDC로 변환하고 그걸 randRects[i]에서 x,y 좌표 검사를 진행해서
// 몇번 인덱스의 Rect를 클릭했는지를 따로 저장해야 한다. 
// 그리고 그 인덱스의 사각형의 위치를 계속 갱신하면됨. 마우스가 움직일떄마다 호출되는 콜백함수를 이용하면 된다. 

// bool Dragged , int rectNum, glm::vec2 mouseXY