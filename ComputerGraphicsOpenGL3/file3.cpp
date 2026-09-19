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

std::uniform_real_distribution<float> disWH{ 0.05f, 0.4f };

std::uniform_real_distribution<float> disXY{ -1.0f, 1.0f };

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
int drawnShapes{};


void TransformScreenToNDC(double& i, double& j);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
bool isOverlap(int i, const Rect& a, const Rect& b);
Rect mergeRect(const Rect& a, const Rect& b);
void splitRects(const Rect& a);

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
    double transHalfHeight = 2 / HEIGHT;
    
    i = i * transHalfWidth - 1;
    j = -1 * (j * transHalfHeight) + 1;
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
        if (drawnShapes < 10) 
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


bool isOverlap(int i, const Rect& a, const Rect& b)
{
    if ( b.x1 + b.w <= a.x1 || 
         a.x1 + a.w <= b.x1 ||
         a.y1 + a.h <= b.y1 || 
         b.y1 + b.h <= a.y1  )
    {
        std::cout << "삼각형이 겹치지 않았습니다." << std::endl;
        return false;
    }
    else {
        std::cout << "삼각형이 겹쳤습니다." << " ";
        std::cout << "사각형 인덱스: " << i << " " << "드래그 하는 사각형: " << rectNum << std::endl;

        return true;
    }

}

Rect mergeRect(const Rect& a, const Rect& b)
{

    float x1 = std::min(a.x1, b.x1);
    float y1 = std::min(a.y1, b.y1);
    float x2 = std::max(a.x1 + a.w, b.x1 + b.w);
    float y2 = std::max(a.y1 + a.h, b.y1 + b.h);

    glm::vec3 rgb{ disColor(gen), disColor(gen), disColor(gen)};

    Rect newRect{x1, y1, x2-x1, y2-y1, rgb};

    return newRect;
}


void splitRects(const Rect& a)
{
    glm::vec3 rgb1{disColor(gen),disColor(gen) ,disColor(gen) };
    glm::vec3 rgb2{ disColor(gen),disColor(gen) ,disColor(gen) };

    Rect b{ disXY(gen), disXY(gen), disWH(gen), disWH(gen), rgb1};
    Rect c{ disXY(gen), disXY(gen), disWH(gen), disWH(gen), rgb2 };

    randRects.push_back(b);
    randRects.push_back(c);

}



// --- 마우스 콜백 (선택 처리) ---
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        // 위 함수를 사용하면 마우스 클릭했을떄 마우스 커서의 현 위치를 바로 알 수 있다. 

        TransformScreenToNDC(xpos, ypos);

        // 마우스의 이전 좌표를 저장한다. [-1~1]로 변환한 후에
        mousePos.x = xpos;
        mousePos.y = ypos;

        // 히트 테스트를 진행해야 한다. 

        bool hit{ false };
        for(int i = randRects.size() - 1; i >= 0; --i)
        {
            if (randRects[i].x1 <= xpos && xpos <= randRects[i].x1 + randRects[i].w
                && randRects[i].y1 <= ypos && ypos <= randRects[i].y1 + randRects[i].h
                && randRects[i].isDraw == true)
            {
                rectNum = i;
                Dragged = true;
                hit = true;
                break;
            }
        }

        
        if (!hit) { // 순회했는데 조건 만족하는 경우가 한번도 없다면
            rectNum = -1;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        //split 할려고 할떄 로직.
    {
        std::cout << drawnShapes << std::endl;

        if (drawnShapes >= 20)
            return;

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos); 

        TransformScreenToNDC(xpos, ypos);

        // 히트 테스트를 진행해야 한다. 

        for (int i = randRects.size() - 1; i >= 0; --i)
        {
            if (randRects[i].x1 <= xpos && xpos <= randRects[i].x1 + randRects[i].w
                && randRects[i].y1 <= ypos && ypos <= randRects[i].y1 + randRects[i].h
                && randRects[i].isDraw == true)
            {
                splitRects(randRects[i]);
                randRects[i].isDraw = false;
                break;
            }
        }

        

    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // 이떄 사각형 간의 충돌을 검사해야 된다. 따로 함수 만들어서 하는게 나을듯. 
        if (rectNum != -1)
        {
            Dragged = false;
            Rect newRect;

            for (int i = 0; i < randRects.size(); ++i)
            {
                if (i == rectNum || randRects[i].isDraw == false)
                    continue;
                else 
                {
                    // 이걸 검사하기 위해 사각형 1개의 모든 점을 순회하고 다른 사각형의 x,y범위를 체크하는 것은
                    // 너무나 비효율적이다. 

                    if (isOverlap(i, randRects[i], randRects[rectNum]))
                    // 사각형끼리 겹친 경우의 처리
                    {
                        randRects[i].isDraw = false;
                        randRects[rectNum].isDraw = false;

                        newRect = mergeRect(randRects[i], randRects[rectNum]);
                        randRects.push_back(newRect);

                        break;
                    }

                  
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

    drawnShapes = 0;

    for (auto& rect : randRects)
    {
        if (rect.isDraw)
        {
            ++drawnShapes;
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


// 사각형 두개가 겹쳤을떄 min max를 계산해서 병합한 사각형을 만들어야 한다. 
// a를 눌렀을떄 사각형이 겹쳐있는 상태일수도 있어서 단순히 DrawScene에서 겹쳐있는 경우를 고려하면 안됨.
// 드래그 중이고 그걸 놓았을때 RELEASE 상태일떄 검사를 해야됨. 