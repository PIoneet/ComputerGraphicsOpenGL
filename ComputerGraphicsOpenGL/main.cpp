#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <random>

std::random_device rd;

std::mt19937 gen(rd());

std::uniform_real_distribution<float> dis{0.0f, 1.0f};


/*
1. 화면에 800x600 크기의 윈도우를 띄운다. 
- 1.1 초기 배경색은 흰색이다. 
2. 배경색을 키보드 입력에 따라 다르게 적용하기
- 2.1 c: 청록색 (초록 + 파랑)
- 2.2 m: 자홍색 (빨강 + 파랑)
- 2.3 y: 노란색 (빨강 + 초록)
- 2.4 a: 랜덤의 색깔
- 2.5 g: 회색
- 2.6 k: 검정색
- 2.7 t: 타이머를 설정하여 특정 시간마다 랜덤색으로 계속 바뀌게 한다. 
- 2.8 s: 타이머 종료
- esc : 프로그램 종료



*/

struct vec4 {
	float x{};
	float y{};
	float z{};
	float a{1.0f};
};


void InputProcess(GLFWwindow* window, vec4& colorVec, bool& time);
void DrawScene(const vec4& colorVec);

int main() {
	//--- GLFW 초기화
	if (!glfwInit()) {
		std::cerr << "GLFW 초기화 실패!" << std::endl;
		return -1;
	}

	
	//--- OpenGL 버전 설정(예: 3.3 Core Profile)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	
	//--- 윈도우생성
	GLFWwindow* window = glfwCreateWindow(800, 600, "First Try: Window", nullptr, nullptr);
	if (!window) {
		std::cerr << "윈도우생성실패!" << std::endl;
		glfwTerminate();
		return -1;
	}
	//GLFW는 윈도우 생성 뿐만 아니라, 키보드 마우스 입출력, 타이머까지도 담당한다. 
	
	
	//--- 컨텍스트설정
	glfwMakeContextCurrent(window);

	//openGL의 컨텍스트는 윈도우마다 다를 수 있음. 컨텍스트는 openGL에서의 상태머신이라고 보면됨.
	
	
	
	//--- GLEW 초기화
	glewExperimental = GL_TRUE; // 최신 기능 사용
	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW 초기화 실패!" << std::endl;
		return -1;
	}

	//GLEW가 최신 openGL 함수들을 가져와준다. 


	// -- - 뷰포트설정
		glViewport(0, 0, 800, 600);
		
		vec4 colorVec{ 1.0f, 1.0f, 1.0f, 1.0f };
		bool time{ false };
		double lastTime{ 0.0f };
	//--- 메인 루프
	while (!glfwWindowShouldClose(window)) {

		//colorVec을 안에서 선언하면 기껏 

		//이게 폴링 방식이라고함. 매 프레임마다 뭐 눌렀는지 확인하는
		InputProcess(window, colorVec, time);



		if (time) {
			double curTime = glfwGetTime();
			std::cout << curTime << std::endl;
			if ( curTime - lastTime >= 1.0f ) {
				std::cout << "is?" << std::endl;
				colorVec = vec4{ dis(gen), dis(gen), dis(gen), 1.0f };
				lastTime = curTime;
			}
		}

		DrawScene(colorVec);
		
		// 버퍼교체
		glfwSwapBuffers(window);
		glfwPollEvents();
		// 아 이게 큐에 쌓인 이벤트들을 처리하게 해주는 함수입니다. 이게 없으면 esc 키도 안먹히고 마우스도 안 먹힙니다.
		// 이벤트 처리가 안돼서.
	}
	//--- 종료 처리
	glfwDestroyWindow(window);
	glfwTerminate();
}


// -- - 키보드입력처리함수
void InputProcess(GLFWwindow* window, vec4& colorVec, bool& time)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
		colorVec = vec4{0.0f, 1.0f,1.0f,1.0f};
	else if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		colorVec = vec4{ 1.0f, 0.0f, 1.0f, 1.0f };
	else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
		colorVec = vec4{ 1.0f, 1.0f, 0.0f, 1.0f };
	else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		colorVec = vec4{ dis(gen), dis(gen) , dis(gen) ,1.0f};
	else if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
		colorVec = vec4{ 0.5f, 0.5f, 0.5f, 1.0f };
	else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		colorVec = vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
	else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)  //Time 설정하기 
	{
		// 타이머를 끄기 전까지는 계속 색깔이 특정 기준으로 바뀌어야 하는 것이 중요함.
		time = true;
		std::cout << "Timer On" << std::endl;
	}
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) 
	{
		time = false;
		std::cout << "Timer Off" << std::endl;
	}
	
		
}


//--- 렌더링함수

void DrawScene(const vec4& colorVec)
{
	glClearColor(colorVec.x, colorVec.y, colorVec.z, colorVec.a); // RGBA (파랑)
	glClear(GL_COLOR_BUFFER_BIT);
}

