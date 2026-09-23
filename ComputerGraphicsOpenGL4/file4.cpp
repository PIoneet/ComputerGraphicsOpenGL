#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <random>
#include <algorithm>
#include <map>
#include <functional>
#include <cmath>


std::random_device rd;
std::mt19937 gen(rd());

std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };

std::uniform_real_distribution<float> disR{ 0.08f, 0.18f };

std::uniform_int_distribution<int> dir{ 0,3 };


/*
1. 배경색은 짙은 회색으로 지정한다
- 1.1 사각형은 랜덤한 색상으로 한다. 
- 1.2 마우스를 클릭한 곳이 중심이 되어 사각형을 그린다. 
- 1.3 최대 5개까지 그리는 것이 가능하다. 
2. 키보드 입력
- 2.1 1 입력하면 사각형들이 각각 대각선으로 이동하며 벽에 닿았을떄 튕기면 다른 방향으로 이동한다. 
- 2.2 2 입력하면 가로 지그재그로 이동한다. 
- 2.3 3 입력하면 모든 사각형들이 시계방향을 따라 윈도우의 가장자리를 따라 이동한다.
- 2.4 4 입력하면 크기가 커졌다 작아졌다를 반복한다. 
- 2.5 5 입력하면 사각형의 색상이 랜덤하게 변한다. 
- 2.6 s 누르면 모든 애니메이션이 멈춘다. 
- 2.7 m 사각형 처음 생성됐던 그 위치로 이동시킨다. 
- 2.8 r 사각형 삭제해서 다시 마우스로 사각형들 생성 가능한 상태가된다. 
- 2.9 q 프로그램을 종료한다.
3. 1~5까지는 다시 입력하면 스케일링이나 위치 이동을 멈춘다. 
*/

#define WIDTH  1400.0
#define HEIGHT 1400.0


struct Rect {

    float x{};  // 마우스 클릭했을떄 중점이다.
    float y{};

    float r{};
    glm::vec3 rgb{};  // 랜덤의 색깔을 담는다. 

    glm::vec2 velocity{};

    float baseY{};
    float baseR{};
    int edgeLine{};
    float changeColorTime{};
  
};



void diagnolMove(double& prevFrame);
void zigzagMove(double& prevFrame);
void edgeMove(double& prevFrame);
void scaleSinCos(double& prevFrame);
void changeColor(double& prevFrame);
void originMove(double& prevFrame);
void removeRect();


void TransformScreenToNDC(double& i, double& j);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

void PlayAnimation(double& prevFrame);
void DrawScene();


int drawnRect{};  // 사각형 5개까지 그릴 수 있음을 표현한 변수. 
std::vector<Rect> randRects;
std::array<glm::vec2, 5> initPos;  // 초기 중점의 위치 저장하는 컨테이너.
double prevFrame = glfwGetTime();

int currentAnimation{};


std::map<int, std::function<void(double& prevFrame)>> animeMap{
    {1, diagnolMove},
    {2, zigzagMove},
    {3, edgeMove},
    {4, scaleSinCos},
    {5, changeColor},
    {6, originMove}
};

// 이거 scalSinCos는 bool 반환이라 다른 map이 필요함.

// 애니메이션 재생을 토글할 전역변수가 필요한데. 



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
   

    randRects.reserve(50);

    

    while (!glfwWindowShouldClose(window)) {

        PlayAnimation(prevFrame);  // 여기서 좌표를 움직이고 그 다음에 그리는 작업이 진행된다. 
        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}



void diagnolMove(double& prevFrame)
{
    double currentFrame = glfwGetTime();
    
    for (auto& rect : randRects)
    {
        double dealth = currentFrame - prevFrame;

        // 벽에 튕기면 방향을 바꾸도록 해야한다. 
        if (rect.x-rect.r <= -1.0f || rect.x + rect.r >= 1.0f)
        {
            rect.velocity.x = -1 * rect.velocity.x;
        }
        
        if (rect.y - rect.r <= -1.0f || rect.y + rect.r >= 1.0f)
        {
            rect.velocity.y = -1 * rect.velocity.y;

        }

        rect.x = std::clamp(rect.x, -1.0f + rect.r, 1.0f - rect.r);
        rect.y = std::clamp(rect.y, -1.0f + rect.r, 1.0f - rect.r);
        
        
        rect.x += rect.velocity.x * dealth;
        rect.y += rect.velocity.y * dealth;
        

    }

}


void zigzagMove(double& prevFrame)
{
    double currentFrame = glfwGetTime();

    for (auto& rect : randRects)
    {
        double dealth = currentFrame - prevFrame;


        if (rect.x - rect.r <= -1.0f || rect.x + rect.r >= 1.0f)
        {
            rect.velocity.x = -1 * rect.velocity.x;
        }

        if (rect.y - rect.r <= -1.0f || rect.y + rect.r >= 1.0f)
        {
            rect.velocity.y = -1 * rect.velocity.y;
        
        }

        rect.x += rect.velocity.x * dealth;
        rect.y = rect.baseY + 0.2f * sin(4.0f * currentFrame);


        rect.x = std::clamp(rect.x, -1.0f + rect.r, 1.0f - rect.r);
        rect.y = std::clamp(rect.y, -1.0f + rect.r, 1.0f - rect.r);

    }

}


void edgeMove(double& prevFrame)
{
    double currentFrame = glfwGetTime();

    for (auto& rect : randRects)
    {
        double dealth = currentFrame - prevFrame;

        switch (rect.edgeLine)
        {
        // edgeLine을 계속 바꾸면서 가장자리 이동이 가능하게 해야되나. 
        case 0: // 위쪽
            if (rect.x >= 1.0f - rect.r)
            {
                rect.edgeLine = 1;
                break;
            }
            rect.velocity = {1.0f, 0.0f};

            rect.x += rect.velocity.x * dealth;

            break;
        case 1: // 오른쪽
            if (rect.y <= -1.0f + rect.r)
            {
                rect.edgeLine = 2;
                break;
            }
            rect.velocity = { 0.0f, -1.0f };

            rect.y += rect.velocity.y * dealth;

            break;
        case 2: // 아래쪽
            if (rect.x <= -1.0f + rect.r)
            {
                rect.edgeLine = 3;
                break;
            }
            rect.velocity = { -1.0f, 0.0f };

            rect.x += rect.velocity.x * dealth;

            break;
        case 3: // 왼쪽
            if (rect.y >= 1.0f - rect.r)
            {
                rect.edgeLine = 0;
                break;
            }
            rect.velocity = { 0.0f, 1.0f };

            rect.y += rect.velocity.y * dealth;

            break;
        }
    }

}

void scaleSinCos(double& prevFrame)
{
    double currentFrame = glfwGetTime();

    for (auto& rect : randRects)
    {
        rect.r = rect.baseR + 0.13 * sin(3.0f * currentFrame);
        // 반지름이 매우 커지는것을 방지하기 위해 0.03을 따로 곱해준다. sin은 최대 -1~1의 값을 반환 가능하니까.
    }
 }


void changeColor(double& prevFrame)
{
    double currentFrame = glfwGetTime();

    for (auto& rect : randRects)
    {
        if (currentFrame - rect.changeColorTime >= 1.0f)
        {
            rect.rgb = {disColor(gen), disColor(gen), disColor(gen)};

            rect.changeColorTime = currentFrame;
        }

    }
      
}



void originMove(double& prevFrame)
{
    double currentFrame = glfwGetTime();
    bool arriveFlag{ true };
    // initPos로 향하는 단위 벡터가 필요하다. 대각선으로 이동하면될듯. 

    int j{};
    for (; j < randRects.size(); ++j)
    {
        double dx = initPos[j].x - randRects[j].x;
        double dy = initPos[j].y - randRects[j].y;

        if (abs(dx) <= 0.01f && abs(dy) <= 0.01f)
        {
            // 여기는 도달을 했다는 뜻이지. 
            randRects[j].x = initPos[j].x;
            randRects[j].y = initPos[j].y;

        }
        else  // 한번이라도 다른 사각형 중에 도착 안한게 있으면 그냥 탈출. 다 도달하면 애니메이션 종료 
        {
            double dealth = currentFrame - prevFrame;

            glm::vec2 dirOrigin{ glm::normalize(initPos[j] - glm::vec2{randRects[j].x, randRects[j].y}) };

            randRects[j].x += dirOrigin.x * dealth;
            randRects[j].y += dirOrigin.y * dealth;
            arriveFlag = false;

            // 여기다 return을 박아넣으니까 사각형 1개씩 움직이는 것처럼 작동했구나. 

            
        }
    }
    
    if(arriveFlag)
        currentAnimation = 0;
 
}



void removeRect()
{
    // 사각형들을 다 삭제하고 다시 입력 받을 수 있게 해야됨. 

    randRects.clear();
    drawnRect = 0;
    currentAnimation = 0;
}




// 마우스 좌표계에서 openGL 좌표계로 변환하고자 한다. 
void TransformScreenToNDC(double& i, double& j)
{
    double transHalfWidth = 2 / WIDTH;
    double transHalfHeight = 2 / HEIGHT;

    i = i * transHalfWidth - 1;
    j = -1 * (j * transHalfHeight) + 1;
}



// -- - 키보드 콜백-- -
void KeyCallback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return; // 누르는 순간만 처리

    switch (key) 
    {
    
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;


    //1번을 한번 누르면 달리 뭔가를 누르기 전까지는 계속 실행되고 있어야 한다.
    case GLFW_KEY_1:
    {
        // 대각선으로 이동하며 벽에 닿았을떄 다른 방향으로 튕겨나가야 한다. 
        if (currentAnimation != 1)
        {
            for (auto& rect : randRects)
            {
                glm::vec2 dir1 = glm::normalize(glm::vec2{ rect.x + rect.r, rect.y + rect.r } - glm::vec2{ rect.x, rect.y });
                glm::vec2 dir2 = glm::vec2{ -1 * dir1.x, -1 * dir1.y };
                glm::vec2 dir3 = glm::normalize(glm::vec2{ rect.x - rect.r, rect.y + rect.r } - glm::vec2{ rect.x, rect.y });
                glm::vec2 dir4 = glm::vec2{ -1 * dir3.x, -1 * dir3.y };

                int key = dir(gen);

                switch (key) {

                case 0:
                    rect.velocity = dir1;
                    break;
                case 1:
                    rect.velocity = dir2;
                    break;
                case 2:
                    rect.velocity = dir3;
                    break;
                case 3:
                    rect.velocity = dir4;
                    break;
                }
            }
            currentAnimation = 1;
        }
        else
            currentAnimation = 0;
        break;
    }
    case GLFW_KEY_2:
    {
        if (currentAnimation != 2)
        {
            for (auto& rect : randRects)
            {
                rect.velocity.x = 1;
                rect.baseY = rect.y;

            }

            currentAnimation = 2;

        }
            
        else
            currentAnimation = 0;
        break;
    }
    case GLFW_KEY_3:
    {
        if (currentAnimation != 3)
        {
            for (auto& rect : randRects)
            {
                float up = 1.0f - rect.y;
                float down = 1.0f + rect.y;
                float left = 1.0f + rect.x;
                float right = 1.0f - rect.x;

                std::array<float, 4> dirArr{ up, right, down, left };

                auto minIt = std::min_element(dirArr.begin(), dirArr.end());
                int minIdx = std::distance(dirArr.begin(), minIt);

                rect.edgeLine = minIdx;

                switch (minIdx)
                {
                case 0: // 위쪽 변
                    rect.y = 1.0f - rect.r;
                    break;
                case 1: // 오른쪽 변
                    rect.x = 1.0f - rect.r;
                    break;
                case 2: // 아래쪽 변
                    rect.y = -1.0f + rect.r;
                    break;
                case 3: // 왼쪽 변
                    rect.x = -1.0f + rect.r;
                    break;

                }

                currentAnimation = 3;
            }
        }
        else
            currentAnimation = 0;
        break;
    }
    case GLFW_KEY_4:
    {
        if (currentAnimation != 4)
        {
            for (auto& rect : randRects)
            {
                rect.baseR = rect.r;
            }

            currentAnimation = 4;
        }
        else
            currentAnimation = 0;
        break;

    }
    case GLFW_KEY_5:
    {
        if (currentAnimation != 5)
        {
            for (auto& rect : randRects)
            {
                rect.changeColorTime = glfwGetTime();

            }
            currentAnimation = 5;
        }
        else
            currentAnimation = 0;
        break;
    }
    case GLFW_KEY_S:
    {
        currentAnimation = 0;
        break;
    }
    case GLFW_KEY_M:
    {
        currentAnimation = 6;
        break;
    }
    case GLFW_KEY_R:
    {
        removeRect();
        break;
    }

    }
}




// --- 마우스 콜백 (선택 처리) ---
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        // 위 함수를 사용하면 마우스 클릭했을떄 마우스 커서의 현 위치를 바로 알 수 있다. 

        TransformScreenToNDC(xpos, ypos);


        // 마우스 클릭하면 사각형 생성되야 하는데 drawnRect 개수에 따라 그리지 않는 경우도 고려해야함. 
        if (drawnRect < 5)
        {
            glm::vec3 randColor{ disColor(gen), disColor(gen), disColor(gen) };


            randRects.push_back(Rect{ (float)xpos, (float)ypos, disR(gen), randColor });


            initPos[drawnRect].x = xpos;
            initPos[drawnRect].y = ypos;

            ++drawnRect;
            // 5번 일어나면 drawnRect == 5가 될 것이다.
        }
        else 
        {

            std::cout << "더 이상 출력될 수 없습니다: "<< drawnRect << std::endl;

        }

    }
}


void PlayAnimation(double& prevFrame)
{

    //std::cout << currentAnimation << std::endl;

    auto it = animeMap.find(currentAnimation);
    if (it != animeMap.end()) //특정 인덱스에 따른 매핑된 함수를 못찾으면 탈출
    {
        it->second(prevFrame);
    }

    prevFrame = glfwGetTime();

}



void DrawScene()
{
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (auto& rect : randRects)
    {
        glColor3f(rect.rgb.x, rect.rgb.y, rect.rgb.z);
        glRectf(rect.x - rect.r, rect.y - rect.r, rect.x + rect.r, rect.y + rect.r);
        
    }

}
