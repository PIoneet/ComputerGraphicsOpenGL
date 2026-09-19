#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <random>
#include <vector>


std::random_device rd;

std::mt19937 gen(rd());

std::uniform_real_distribution<float> disPlus{ 0.0f, 1.0f };
std::uniform_real_distribution<float> disMinus{ -1.0f, 0.0f };

std::uniform_real_distribution<float> disColor{ 0.0f, 1.0f };

std::uniform_real_distribution<float> disR{ 0.05f, 0.3f };   // 반지름의 랜덤값


#define WIDTH	2000.0
#define HEIGHT	1200.0


/*
1. 윈도우를 띄우고 마우스를 실행한다.
- 1.1 원하는 색으로 배경을 그린다.
- 1.2 화면의 가로 세로를 2등분하여 4개의 영역을 그린다
- 1.3 각 영역은 랜덤으로 다른 색으로 설정한다.


2. 키보드/마우스 입력으로 도형을 그리고 속성을 바꾸자.
- 2.1 1/2/3/4 입력하면 각 사분면의 중앙에 사격형을 그린다. 색상 크기는 원한는대로
// 내가 기억하기로 겹쳐졌을떄 나중에 그려진 사각형이 위에서 그려지게 구현해야된다.
- 2.2 명령어를 다시 누르면 다른 크기와 색상으로 다시 그린다.
- 2.3 각 영역에 최대 5개까지 그릴 수 있다.
- 2.4 왼쪽마우스 : 그려진 사각형을 선택함. 선택됐다는 것을 표시하기 위해 빨간 테두리 준다.
- 2.5 +/-: 선택된 사각형의 크기가 커지고/작아진다. 영역을 넘어서면 더 이상 안커짐.
- 2.6 c: 선택된 사각형의 색상을 랜덤하게 바꾼다.
- 2.7 r: 그려진 사각형을 모두 지우고 영역의 색상도 바꿔서 리셋한다.
- 2.8 q: 프로그램 종료.

*/

bool faceNumberFull(int i);
void makeRect(int i);
void TransformScreenToNDC(double& i, double& j);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void DrawBackground();

void DrawScene();



// 1사분면의 중앙에 사각형을 만들거니까 아예 사각형 정보도 중점을 받는게 나을듯.
struct Rect {
	
	// 중점
	float x;
	float y;

	float r; // 이건 반지름. 
	glm::vec3 rgb;

	int faceNumber;  // 몇사분면인지.
	bool isSelect{ false };
};


std::vector<Rect> randRects;
int rectSelect{-1};

glm::vec4 first{ disColor(gen), disColor(gen), disColor(gen), 1.0f };
glm::vec4 second{ disColor(gen), disColor(gen), disColor(gen), 1.0f };
glm::vec4 third{ disColor(gen), disColor(gen), disColor(gen), 1.0f };
glm::vec4 fourth{ disColor(gen), disColor(gen), disColor(gen), 1.0f };


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

	randRects.reserve(50);


	

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);

	while (!glfwWindowShouldClose(window)) {


		DrawBackground();

		DrawScene();

		
		glfwSwapBuffers(window);
		glfwPollEvents();

	}
	

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwDestroyWindow(window);
	glfwTerminate();
}


bool faceNumberFull(int i)
{
	int size{};
	for (auto& rect : randRects)
	{
		if (rect.faceNumber == i)
			++size;

	}

	if (size >= 5)
		return true;
	else
		return false;

}



void makeRect(int i)  // i는 몇사분면인지. 
{
	glm::vec3 randColor{disColor(gen), disColor(gen), disColor(gen)};

	if (i == 1)
	{
		randRects.push_back(Rect{0.5, 0.5, disR(gen), randColor, 1, false});
	}
	else if (i == 2)
	{
		randRects.push_back(Rect{ -0.5, 0.5, disR(gen), randColor, 2, false });
	}
	else if (i == 3)
	{
		randRects.push_back(Rect{ -0.5, -0.5, disR(gen), randColor, 3, false });
	}
	else if (i == 4)
	{
		randRects.push_back(Rect{ 0.5, -0.5, disR(gen), randColor, 4, false });
	}


}




void TransformScreenToNDC(double& i, double& j)
{
	double transHalfWidth = 2 / WIDTH;
	double transHalfHeight = 2 / HEIGHT;

	i = i * transHalfWidth - 1;
	j = -1 * (j * transHalfHeight) + 1;

}





//--- 콜백 함수 등록
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
			//--- 등록한 사용자 정의 콜백 함수
		case GLFW_KEY_Q:   glfwSetWindowShouldClose(window, GLFW_TRUE); break;
		case GLFW_KEY_1:
		{
			if (faceNumberFull(1))
				return;
			
			makeRect(1);
			break;
		}
		case GLFW_KEY_2:
		{
			if (faceNumberFull(2))
				return;

			makeRect(2);
			

			break;
		}
		case GLFW_KEY_3:
		{
			if (faceNumberFull(3))
				return;

			makeRect(3);
			
			
			break;
		}
		case GLFW_KEY_4:
		{

			if (faceNumberFull(4))
				return;

			makeRect(4);
			break;
		}
		case GLFW_KEY_EQUAL:
		{
			if (rectSelect == -1)
				return;

			if (randRects[rectSelect].isSelect)
			{
				if(randRects[rectSelect].r <= 0.5)
					randRects[rectSelect].r += 0.015;
				
			}

			break;
		}
		case GLFW_KEY_MINUS:
		{
			if (rectSelect == -1)
				return;

			if (randRects[rectSelect].isSelect)
			{
				randRects[rectSelect].r -= 0.015;

			}

			break;
		}
		case GLFW_KEY_C:
		{
			if (rectSelect == -1)
				return;


			randRects[rectSelect].rgb = glm::vec3{ disColor(gen),disColor(gen),disColor(gen) };
			break;
		}
		case GLFW_KEY_R:
		{
			randRects.clear();
			rectSelect = -1;
			
			first = glm::vec4{disColor(gen), disColor(gen) , disColor(gen) , 1.0f};
			second = glm::vec4{ disColor(gen), disColor(gen) , disColor(gen) , 1.0f };
			third = glm::vec4{ disColor(gen), disColor(gen) , disColor(gen) , 1.0f };
			fourth = glm::vec4{ disColor(gen), disColor(gen) , disColor(gen) , 1.0f };

			break;
		}


		}
	}
}


void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		//std::cout << "콜백 함수 호출되고 있나요? " << std::endl;

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		// 위 함수를 사용하면 마우스 클릭했을떄 마우스 커서의 현 위치를 바로 알 수 있다. 
		
		TransformScreenToNDC(xpos, ypos);
		
		// 먼저 false로 초기화를 한번 진행한다.
		for (int i = randRects.size() - 1; i >= 0; --i)
		{
			randRects[i].isSelect = false;
			
		}
		
		// 히트 테스트를 진행해야 한다. 

		for (int i = randRects.size() - 1; i >= 0; --i)
		{
			if (randRects[i].x - randRects[i].r <= xpos && xpos <= randRects[i].x + randRects[i].r
				&& randRects[i].y - randRects[i].r <= ypos && ypos <= randRects[i].y + randRects[i].r)
			{
				randRects[i].isSelect = true;
				rectSelect = i;
				break;
			}
		}

	}

}





void DrawBackground()
{


	glEnable(GL_SCISSOR_TEST);

	//3사분면
	glScissor(0, 0, WIDTH / 2, HEIGHT / 2);
	glClearColor(first.x, first.y, first.z, first.a);
	glClear(GL_COLOR_BUFFER_BIT);


	//4사분면
	glScissor(WIDTH / 2, 0, WIDTH / 2, HEIGHT / 2);
	glClearColor(second.x, second.y, second.z, second.a);
	glClear(GL_COLOR_BUFFER_BIT);


	//2사분면
	glScissor(0, HEIGHT / 2, WIDTH / 2, HEIGHT / 2);
	glClearColor(third.x, third.y, third.z, third.a);
	glClear(GL_COLOR_BUFFER_BIT);


	//1사분면
	glScissor(WIDTH / 2, HEIGHT / 2, WIDTH / 2, HEIGHT / 2);
	glClearColor(fourth.x, fourth.y, fourth.z, fourth.a);
	glClear(GL_COLOR_BUFFER_BIT);


	glDisable(GL_SCISSOR_TEST);

}





void DrawScene()
{
	
	for (auto& rect : randRects)
	{
		glColor3f(rect.rgb.x, rect.rgb.y, rect.rgb.z);
		glRectf(rect.x - rect.r, rect.y - rect.r, rect.x + rect.r, rect.y + rect.r);


		// 빨간색 테두리 그려주는 매커니즘.
		if (rect.isSelect) {
			glColor3f(1.0f, 0.0f, 0.0f);  // 빨간색
			glBegin(GL_LINE_LOOP);
			glVertex2f(rect.x - rect.r, rect.y - rect.r);
			glVertex2f(rect.x + rect.r, rect.y - rect.r);
			glVertex2f(rect.x + rect.r, rect.y + rect.r);
			glVertex2f(rect.x - rect.r, rect.y + rect.r);
			glEnd();
		}
	
	}
}







//  1,2,3,4 누르면 영역 내의 랜덤의 위치에 사각형 그려지고 색상 다르게 하면되고, 랜덤으로 생성된 것들끼리
// 겹쳐졌을떄 나중에 그린게 위에 있으면 된다.



// 중요한 부분이 4사분면 내의 사각형을 먼저 그리고 나중에 4사분면을 그리면 영역 내의 사각형이 보이지 않는다.
// 결국 코드의 순서가 뭘 보여주고 안보여줄지 서순을 나타낸다. 나중에 그려지는 사각형을 아래 코드에 추가하면
// 자동으로 나중에 그려지는게 화면에 나타나는 구조입니다. 



// 내 생각에는 DrawScene 호출을 가장 위에 두고 그 다음에 InputProcess를 해서 키보드를 눌렀을떄 사각형이 그려지게 
// 그리고 함수를 또 만들어서 호출되면 특정 범위의 영역에 사각형이 그려지게 만들면 되겠네. 