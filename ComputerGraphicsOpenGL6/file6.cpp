#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <random>
#include <algorithm>
#include <map>

std::random_device rd;
std::mt19937 gen(rd());

std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };

std::uniform_real_distribution<float> disR{ 0.12f, 0.18f };

std::uniform_int_distribution<int> disNum{ 5, 10 }; // 랜덤으로 생성될 사각형의 개수

std::uniform_real_distribution<float> disPos{ -1.0f, 1.0f };

std::uniform_int_distribution<int> randAnimation{ 1,4 };

std::uniform_int_distribution<int> isDark{ 0,1 };


#define WIDTH  1300.0
#define HEIGHT 1300.0
#define ANIME_TIME 2.0


struct Rect {
    float x{};
    float y{};
    float r{};
    glm::vec3 rgb{};
    float spawnTime{};
    
    
    glm::vec3 rgbOrigin{};
    float rOrigin{};
    glm::vec2 velocity{};

    int animType{};
    bool isDraw{ true };
    bool getDarker{ true };

    std::vector<Rect> child;
};



/*
1. 화면의  랜덤한 위치에 사각형 5~10개의 사각형을 랜덤한 색상, 크기로 그린다.
- 1.1 마우스로 사각형 내부를 클릭하면 사등분되어 위치,색상,크기가 변한다. 
- 1.2 위치 애니메이션 아래 여러개 중에 하나가 랜덤으로 실행된다. 
- 1.3 크기는 점점 작아진다. 특정 크기가 되면 완전히 사라질 것이다. 
- 1.4 색상은 처음 그려진 색상에서 점점 밝기가 밝게 또는 어둡게 바뀐다. 

2. 위치 이동 애니메이션 종류
- 2.1 좌우상하 이동(랜덤 배정으로 이동시킬 예정.)
- 2.2 대각선 이동
- 2.3 사등분 된 사각형이 한쪽 방향으로 같이 이동한다. 
- 2.4 좌우상하와 대각선 이동(8방향 이동으로, 이때는 축소된 사각형이 8개 나온다.)
  사각형 8개가 각기 다른 velocity에 따라 움직이면 될듯. 

*/


void xyMove(int focusRect, double& prevFrame);
void diagnolMove(int focusRect, double& prevFrame);
void oneSideMove(int focusRect, double& prevFrame);
void xyDiagnolMove(int focusRect, double& prevFrame);

void generateRect();
void TransformScreenToNDC(double& i, double& j);

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void PlayAnimation(double& prevFrame);
void DrawScene();


std::map<int, std::function<void(int focusRect, double& prevFrame)>> animeMap{
    {1, xyMove},
    {2, diagnolMove},
    {3, oneSideMove},
    {4, xyDiagnolMove}
};

int currentAnimation{};
std::vector<Rect> randRects;

int focusRect{};



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


    glfwSetMouseButtonCallback(window, MouseButtonCallback);

    randRects.reserve(100);
    generateRect();

    double prevFrame{};

    while (!glfwWindowShouldClose(window)) {

        PlayAnimation(prevFrame);
        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

}


void xyMove(int focusRect, double& prevFrame)
{
    auto& childVec = randRects[focusRect].child;
    double currentFrame = glfwGetTime();
   
    childVec[0].velocity = glm::vec2{ 0.0f, 1.0f };
    childVec[1].velocity = glm::vec2{ 1.0f, 0.0f };
    childVec[2].velocity = glm::vec2{ -1.0f, 0.0f };
    childVec[3].velocity = glm::vec2{ 0.0f, -1.0f };

    //위치 이동하고 색깔 랜덤으로 어두워지거나 밝아지고 크기 작아져야함.
    for (int i = 0; i < childVec.size(); ++i)
    {
        double dealth = currentFrame - prevFrame;

        childVec[i].x += childVec[i].velocity.x * dealth;
        childVec[i].y += childVec[i].velocity.y * dealth;

        float elapsed = currentFrame - childVec[i].spawnTime;
        float progress = elapsed / ANIME_TIME;  // ANIME_TIME을 5.0으로 바꾸면 5초짜리
        progress = std::min(progress, 1.0f);

        if (progress >= 1.0f)
        {
            childVec[i].isDraw = false;  // 5초 다 지나면 사라짐
        }
        else
        {
            // 크기: 시작 크기에서 progress만큼 0쪽으로 줄어듦
            childVec[i].r = (randRects[focusRect].rOrigin/2) * (1.0f - progress);

            // 색상 계산은 기존 그대로
            if (randRects[focusRect].getDarker)
                childVec[i].rgb = randRects[focusRect].rgbOrigin * (1.0f - progress);
            else
                childVec[i].rgb = randRects[focusRect].rgbOrigin + (glm::vec3(1.0f) - randRects[focusRect].rgbOrigin) * progress;
        }

    }

}


void diagnolMove(int focusRect, double& prevFrame)
{
    auto& childVec = randRects[focusRect].child;
    double currentFrame = glfwGetTime();

    childVec[0].velocity = glm::normalize(glm::vec2{randRects[focusRect].x + randRects[focusRect].r, 
        randRects[focusRect].y + randRects[focusRect].r} - glm::vec2{ randRects[focusRect].x, randRects[focusRect].y});
    
    childVec[1].velocity = glm::vec2{ -1 * childVec[0].velocity.x, -1 * childVec[0].velocity.y };
    
    childVec[2].velocity = glm::normalize(glm::vec2{ randRects[focusRect].x - randRects[focusRect].r,
        randRects[focusRect].y + randRects[focusRect].r } - glm::vec2{ randRects[focusRect].x, randRects[focusRect].y });
    
    childVec[3].velocity = glm::vec2{ -1 * childVec[2].velocity.x, -1 * childVec[2].velocity.y };

    for (int i = 0; i < childVec.size(); ++i)
    {
        double dealth = currentFrame - prevFrame;

        childVec[i].x += childVec[i].velocity.x * dealth;
        childVec[i].y += childVec[i].velocity.y * dealth;


        float elapsed = currentFrame - childVec[i].spawnTime;
        float progress = elapsed / ANIME_TIME;  // ANIME_TIME을 5.0으로 바꾸면 5초짜리
        progress = std::min(progress, 1.0f);

        if (progress >= 1.0f)
        {
            childVec[i].isDraw = false;  // 5초 다 지나면 사라짐
        }
        else
        {
            // 크기: 시작 크기에서 progress만큼 0쪽으로 줄어듦
            childVec[i].r = (randRects[focusRect].rOrigin / 2) * (1.0f - progress);

            // 색상 계산은 기존 그대로
            if (randRects[focusRect].getDarker)
                childVec[i].rgb = randRects[focusRect].rgbOrigin * (1.0f - progress);
            else
                childVec[i].rgb = randRects[focusRect].rgbOrigin + (glm::vec3(1.0f) - randRects[focusRect].rgbOrigin) * progress;
        }
    }

    
}


void oneSideMove(int focusRect, double& prevFrame)
{
    auto& childVec = randRects[focusRect].child;
    double currentFrame = glfwGetTime();

    glm::vec2 dir;

    if (isDark(gen) == 0)
    {
        dir = glm::vec2{ 1.0f, 0.0f };
    }
    else
    {
        dir = glm::vec2{ 0.0f, -1.0f };
    }

    for (int j = 0; j < childVec.size(); ++j) {
        childVec[j].velocity = dir;

    }
        


    for (int i = 0; i < childVec.size(); ++i)
    {
        double dealth = currentFrame - prevFrame;

        childVec[i].x += childVec[i].velocity.x * dealth;
        childVec[i].y += childVec[i].velocity.y * dealth;


        float elapsed = currentFrame - childVec[i].spawnTime;
        float progress = elapsed / ANIME_TIME;  // ANIME_TIME을 5.0으로 바꾸면 5초짜리
        progress = std::min(progress, 1.0f);

        if (progress >= 1.0f)
        {
            childVec[i].isDraw = false;  // 5초 다 지나면 사라짐
        }
        else
        {
            // 크기: 시작 크기에서 progress만큼 0쪽으로 줄어듦
            childVec[i].r = (randRects[focusRect].rOrigin / 2) * (1.0f - progress);

            // 색상 계산은 기존 그대로
            if (randRects[focusRect].getDarker)
                childVec[i].rgb = randRects[focusRect].rgbOrigin * (1.0f - progress);
            else
                childVec[i].rgb = randRects[focusRect].rgbOrigin + (glm::vec3(1.0f) - randRects[focusRect].rgbOrigin) * progress;
        }
    }

}

void xyDiagnolMove(int focusRect, double& prevFrame)
{
    auto& childVec = randRects[focusRect].child;
    double currentFrame = glfwGetTime();

    childVec[0].velocity = glm::vec2{ 0.0f, 1.0f };                              // 위
    childVec[1].velocity = glm::vec2{ 0.0f, -1.0f };                             // 아래
    childVec[2].velocity = glm::vec2{ 1.0f, 0.0f };                              // 오른쪽
    childVec[3].velocity = glm::vec2{ -1.0f, 0.0f };                             // 왼쪽
    childVec[4].velocity = glm::normalize(glm::vec2{ 1.0f, 1.0f });              // 우상
    childVec[5].velocity = glm::normalize(glm::vec2{ -1.0f, 1.0f });             // 좌상
    childVec[6].velocity = glm::normalize(glm::vec2{ 1.0f, -1.0f });             // 우하
    childVec[7].velocity = glm::normalize(glm::vec2{ -1.0f, -1.0f });            // 좌하

    for (int i = 0; i < childVec.size(); ++i)
    {
        double dealth = currentFrame - prevFrame;

        childVec[i].x += childVec[i].velocity.x * dealth;
        childVec[i].y += childVec[i].velocity.y * dealth;


        float elapsed = currentFrame - childVec[i].spawnTime;
        float progress = elapsed / ANIME_TIME;  // ANIME_TIME을 5.0으로 바꾸면 5초짜리
        progress = std::min(progress, 1.0f);

        if (progress >= 1.0f)
        {
            childVec[i].isDraw = false;  // 5초 다 지나면 사라짐
        }
        else
        {
            // 크기: 시작 크기에서 progress만큼 0쪽으로 줄어듦
            childVec[i].r = (randRects[focusRect].rOrigin / 2) * (1.0f - progress);

            // 색상 계산은 기존 그대로
            if (randRects[focusRect].getDarker)
                childVec[i].rgb = randRects[focusRect].rgbOrigin * (1.0f - progress);
            else
                childVec[i].rgb = randRects[focusRect].rgbOrigin + (glm::vec3(1.0f) - randRects[focusRect].rgbOrigin) * progress;
        }
    }

}



void generateRect()
{
    int randNum = disNum(gen);

    for (int i = 0; i < randNum; ++i)
    {
        glm::vec3 randColor{ disColor(gen), disColor(gen), disColor(gen) };

        float r = disR(gen);

        std::uniform_real_distribution<float> disXY{-1.0f + r, 1.0f - r};

        randRects.push_back(Rect{ disXY(gen), disXY(gen), r, randColor});
    }
}



void TransformScreenToNDC(double& i, double& j)
{
    double transHalfWidth = 2 / WIDTH;
    double transHalfHeight = 2 / HEIGHT;

    i = i * transHalfWidth - 1;
    j = -1 * (j * transHalfHeight) + 1;
}




void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // 사각형 내부 충돌 처리했을떄 사등분되는 로직.
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        TransformScreenToNDC(xpos, ypos);

        // 충돌검사하고 randAnimation 돌려서 특정 애니메이션 재생되게 만들어야함.

        for (int i=randRects.size()-1; i>=0; --i)
        {
            if (randRects[i].x - randRects[i].r <= xpos && xpos <= randRects[i].x + randRects[i].r
            && randRects[i].y - randRects[i].r <= ypos && ypos <= randRects[i].y + randRects[i].r
            && randRects[i].isDraw == true)
            {
                randRects[i].animType = randAnimation(gen);
                focusRect = i;
                randRects[i].isDraw = false;
                randRects[i].rgbOrigin = randRects[i].rgb;  // 기존 색깔 미리 저장해두기.
                randRects[i].rOrigin = randRects[i].r;      // 기존 반지름 저장하기.
                randRects[i].getDarker = (isDark(gen) == 0) ? true : false;

                float left = randRects[i].x - randRects[i].r / 2;
                float right = randRects[i].x + randRects[i].r / 2;
                float up = randRects[i].y + randRects[i].r / 2;
                float down = randRects[i].y - randRects[i].r / 2;
                float radius = randRects[i].r / 4;

                randRects[i].child.reserve(10);

                if (randRects[i].animType != 4)
                {
                    randRects[i].child.push_back(Rect{ left, up, radius, randRects[i].rgb, (float)glfwGetTime()});
                    randRects[i].child.push_back(Rect{ right, up, radius, randRects[i].rgb, (float)glfwGetTime() });
                    randRects[i].child.push_back(Rect{ left, down, radius, randRects[i].rgb, (float)glfwGetTime() });
                    randRects[i].child.push_back(Rect{ right, down, radius, randRects[i].rgb, (float)glfwGetTime() });
                    // x - r/2 뭐 이렇게 중점 표현하면 좋긴할듯.
                }
                else
                {
                    for (int k = 0; k < 8; ++k)
                    {
                        randRects[i].child.push_back(Rect{ randRects[i].x, randRects[i].y, radius, randRects[i].rgb, (float)glfwGetTime() });
                    }
                }
                

                break;
            }

        }
    

    }


}


void PlayAnimation(double& prevFrame)
{
    double currentFrame = glfwGetTime();
    for (int i = 0; i < randRects.size(); ++i)
    {
        if (randRects[i].animType != 0)  // 애니메이션이 시작된 부모라면
        {
            auto it = animeMap.find(randRects[i].animType);
            if (it != animeMap.end())
                it->second(i, prevFrame);  // i번째 부모를 대상으로 실행
        }
    }
    prevFrame = currentFrame;
}



void DrawScene()
{
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (auto& rect : randRects)
    {
        if (rect.isDraw)  // 부모가 살아있으면 부모를 그림
        {
            glColor3f(rect.rgb.x, rect.rgb.y, rect.rgb.z);
            glRectf(rect.x - rect.r, rect.y - rect.r, rect.x + rect.r, rect.y + rect.r);
        }

        for (auto& child : rect.child)
        {
            if (child.isDraw == false)
                continue;

            glColor3f(child.rgb.x, child.rgb.y, child.rgb.z);
            glRectf(child.x - child.r, child.y - child.r, child.x + child.r, child.y + child.r);
        }
    }
}
