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


# 2026 09 19

오늘의 리뷰 코드
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

실습 과제 2,3번 완료했습니다. 
사각형 간의 AABB 충돌 처리를 하는 부분이 핵심이었습니다. 원래는 이중 for문으로 꼭짓점 4개를 일일히 거리 범위 체크해서 
풀어볼려 했지만 너무 복잡하고 비효율적인 코드가 나왔습니다. 이에 오히려 절대 겹치지 않는 경우의 범위를 상정하고 그 이외의 경우에
겹침으로 판정을 해서 해결했습니다. a와 b 사각형이 왼쪽에 있을지 오른쪽에 있을지 유무를 모르기에 x축 y축에서 각각 두가지 경우를 전부
고려했습니다. 그리고 겹쳤을떄는 4개의 부등식 다 false가 나오기에 그 경우 겹치는 사각형을 판정할 수 있게 로직을 구성했습니다
