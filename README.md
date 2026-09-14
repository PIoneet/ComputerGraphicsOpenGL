# ComputerGraphicsOpenGL

# 2026 09 14

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


colorVec를 무한 루프 안에서 선언하니까 키보드를 누르고 있지 않는 상황에서는 무조건 다시
흰색으로 초기화되서 화면이 이상해지는 버그가 존재했습니다. 
타이머로 1초마다 색깔이 바뀌게 설정했음에도 1초마다 바뀌고 나서 바로 다음 프레임에 흰색으로
초기화가 되버려서 그걸 가시적으로 볼 수가 없는 문제에 직면했고 변수를 바깥에 선언했을떄
문제가 해결됐습니다. 
