#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <random>

std::random_device rd;
std::mt19937 gen(rd());

std::uniform_real_distribution<float> disPlus{ 0.0f, 1.0f };
std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };

#define WIDTH  2000
#define HEIGHT 1200

const float MIN_HALF_SIZE = 0.02f;
const float MAX_HALF_SIZE = 0.5f;   // 사분면 경계를 넘지 않는 최대 크기
const float RESIZE_STEP = 0.02f;
const int   MAX_RECTS_PER_QUADRANT = 5;

struct Rect {
    float cx, cy;       // 중심 좌표
    float halfSize;     // 반지름 개념 (한 변의 절반)
    float r, g, b;
    int   quadrant;     // 1~4
};

// 4분면 배경색 (전역으로 옮김 - r 키 리셋에서 다시 접근해야 하므로)
glm::vec4 first, second, third, fourth;

std::vector<Rect> rects;
int selectedIndex = -1; // 선택된 사각형의 rects 내 인덱스, 없으면 -1

// 사분면 중심 좌표
void GetQuadrantCenter(int q, float& cx, float& cy)
{
    switch (q) {
    case 1: cx = 0.5f;  cy = 0.5f;  break;
    case 2: cx = -0.5f; cy = 0.5f;  break;
    case 3: cx = -0.5f; cy = -0.5f; break;
    case 4: cx = 0.5f;  cy = -0.5f; break;
    }
}

void RandomizeBackground()
{
    first = glm::vec4{ disColor(gen), disColor(gen), disColor(gen), 1.0f };
    second = glm::vec4{ disColor(gen), disColor(gen), disColor(gen), 1.0f };
    third = glm::vec4{ disColor(gen), disColor(gen), disColor(gen), 1.0f };
    fourth = glm::vec4{ disColor(gen), disColor(gen), disColor(gen), 1.0f };
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void AddOrReplaceRect(int quadrant);
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

    RandomizeBackground();

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);

    while (!glfwWindowShouldClose(window)) {

        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}

// --- 키보드 콜백 ---
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return; // 누르는 순간만 처리

    switch (key)
    {
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

    case GLFW_KEY_1: AddOrReplaceRect(1); break;
    case GLFW_KEY_2: AddOrReplaceRect(2); break;
    case GLFW_KEY_3: AddOrReplaceRect(3); break;
    case GLFW_KEY_4: AddOrReplaceRect(4); break;

    case GLFW_KEY_EQUAL: // '+' 키 (Shift 없이 '=' 키로 인식되는 경우가 많음)
    case GLFW_KEY_KP_ADD:
        if (selectedIndex != -1) {
            rects[selectedIndex].halfSize =
                std::min(rects[selectedIndex].halfSize + RESIZE_STEP, MAX_HALF_SIZE);
        }
        break;

    case GLFW_KEY_MINUS:
    case GLFW_KEY_KP_SUBTRACT:
        if (selectedIndex != -1) {
            rects[selectedIndex].halfSize =
                std::max(rects[selectedIndex].halfSize - RESIZE_STEP, MIN_HALF_SIZE);
        }
        break;

    case GLFW_KEY_C:
        if (selectedIndex != -1) {
            rects[selectedIndex].r = disColor(gen);
            rects[selectedIndex].g = disColor(gen);
            rects[selectedIndex].b = disColor(gen);
        }
        break;

    case GLFW_KEY_R:
        rects.clear();
        selectedIndex = -1;
        RandomizeBackground();
        break;
    }
}

// --- 마우스 콜백 (선택 처리) ---
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // 픽셀 좌표 -> 클립 좌표(-1~1) 변환. 화면 y는 아래로 증가하므로 뒤집어줌
    float clipX = static_cast<float>(xpos / WIDTH) * 2.0f - 1.0f;
    float clipY = 1.0f - static_cast<float>(ypos / HEIGHT) * 2.0f;

    selectedIndex = -1;

    // 나중에 그려진(= 위에 있는) 것부터 검사해야 진짜로 위에 보이는 걸 선택함
    for (int i = static_cast<int>(rects.size()) - 1; i >= 0; --i)
    {
        const Rect& r = rects[i];
        if (clipX >= r.cx - r.halfSize && clipX <= r.cx + r.halfSize &&
            clipY >= r.cy - r.halfSize && clipY <= r.cy + r.halfSize)
        {
            selectedIndex = i;
            break;
        }
    }
}

// --- 사각형 추가 (사분면당 최대 5개, 초과 시 가장 오래된 것 교체) ---
void AddOrReplaceRect(int quadrant)
{
    float cx, cy;
    GetQuadrantCenter(quadrant, cx, cy);

    Rect newRect;
    newRect.cx = cx;
    newRect.cy = cy;
    newRect.halfSize = disPlus(gen) * 0.3f + 0.1f; // 0.1~0.4 사이 랜덤 크기
    newRect.r = disColor(gen);
    newRect.g = disColor(gen);
    newRect.b = disColor(gen);
    newRect.quadrant = quadrant;

    // 같은 사분면에 몇 개 있는지 확인
    int countInQuadrant = 0;
    int oldestIndexInQuadrant = -1;
    for (size_t i = 0; i < rects.size(); ++i) {
        if (rects[i].quadrant == quadrant) {
            countInQuadrant++;
            if (oldestIndexInQuadrant == -1)
                oldestIndexInQuadrant = static_cast<int>(i);
        }
    }

    if (countInQuadrant < MAX_RECTS_PER_QUADRANT) {
        rects.push_back(newRect);
        selectedIndex = static_cast<int>(rects.size()) - 1; // 새로 그린 걸 자동 선택(선택사항)
    }
    else {
        // 5개 다 찼으면 가장 오래된 것을 지우고 맨 뒤에 새로 추가
        // (뒤로 보내야 "나중에 그려짐 = 위에 보임" 규칙이 유지됨)
        rects.erase(rects.begin() + oldestIndexInQuadrant);
        rects.push_back(newRect);
        selectedIndex = static_cast<int>(rects.size()) - 1;
    }
}

// --- 매 프레임 전체 장면 그리기 ---
void DrawScene()
{
    glEnable(GL_SCISSOR_TEST);

    glScissor(0, 0, WIDTH / 2, HEIGHT / 2);
    glClearColor(first.x, first.y, first.z, first.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glScissor(WIDTH / 2, 0, WIDTH / 2, HEIGHT / 2);
    glClearColor(second.x, second.y, second.z, second.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glScissor(0, HEIGHT / 2, WIDTH / 2, HEIGHT / 2);
    glClearColor(third.x, third.y, third.z, third.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glScissor(WIDTH / 2, HEIGHT / 2, WIDTH / 2, HEIGHT / 2);
    glClearColor(fourth.x, fourth.y, fourth.z, fourth.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_SCISSOR_TEST);

    // 저장된 사각형들을 순서대로 다시 그림 (뒤에 있을수록 나중에 그려짐 = 위에 보임)
    for (size_t i = 0; i < rects.size(); ++i)
    {
        const Rect& r = rects[i];

        glColor3f(r.r, r.g, r.b);
        glRectf(r.cx - r.halfSize, r.cy - r.halfSize,
            r.cx + r.halfSize, r.cy + r.halfSize);

        // 선택된 사각형이면 빨간 테두리 추가
        if (static_cast<int>(i) == selectedIndex)
        {
            glColor3f(1.0f, 0.0f, 0.0f);
            glLineWidth(2.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(r.cx - r.halfSize, r.cy - r.halfSize);
            glVertex2f(r.cx + r.halfSize, r.cy - r.halfSize);
            glVertex2f(r.cx + r.halfSize, r.cy + r.halfSize);
            glVertex2f(r.cx - r.halfSize, r.cy + r.halfSize);
            glEnd();
        }
    }
}