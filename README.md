# ComputerGraphicsOpenGL

1. 실행/수정한 코드 내용 (계획)

2. 발생한 오류/특이사항 결과

3. 앞으로 수정/개선 방향

// 템플릿

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


# 2026 09 20

현재 openGL 실습 4번 진행중입니다. 
file4.cpp 수정중.

a. 실행/수정한 코드 내용 (계획)
	일단 현재 m 명령어를 고려하면 마우스가 처음 클릭해서 사각형 생성했을때 그 위치값을 따로 저장할 필요가 있을까? 
	아니지 근데 생각을 해보면 Rect 안의 변수들을 계속 매번 업데이트 해야 되긴 하다. DrawScene에서 그릴거면. 
	그러면 처음 좌표는 따로 전역 변수로 관리할 필요성이 있다고 현재로서는 판단했다. 
	
	r을 눌렀을떄 drawnRect가 0으로 바껴서 다시 사각형을 생성가능한 상태가 되어야 한다. 
	
	초기값 Pos를 저장 가능한 array<std::pair<float, float>> , 5> 타입을 이용해서 저장했다. 
	r 눌렀을떄 다시 처음부터 사각형의 초기값 pos를 저장 가능하게 된다.
	
b. 발생한 오류/특이사항 결과
	링커 명령줄에 디렉토리 파일 이름을 적지 않아 오류 여러개가 생겼습니다. 
	
c. 앞으로 수정/개선 방향
	현재까지는 따로 문제 사항 없습니다. 

	
	 

////////////////////////////////////////////////////////////////////////////////


1. 실행/수정한 코드 내용 (계획)
	저번에 마우스 드래그 했던 것과는 다르게 시간이 지남에 따라 위치가 변경되어야 하는 것이 특징이다. 
	결국 우리가 지금 구현해야되는건 diagnolMove()가 한번 호출되면 매 프레임 diagnolMove()가 실행되야 한다. 
	한번 실행되고 끝이 아니라는 점이다. 
	
	내가 생각했을떄는 DrawScene이전에 rect.x, rect.y를 시간에 따라 바꿔주는 함수가 필요하다고봄. 그리고 diagnolMove() 같은
	호출이 일어나면 몇번 애니메이션이 토글됐다는 걸 이 함수에 알려줘서 그걸 실행해야됨 계속. 

	여기서 그럼 의문인게 diagnolMove()가 DrawScene 이전에 매프레임 호출하게 할건지? -> ㅇㅇ 그게 맞을듯. 키보드 입력했을떄는
	1번 애니메이션 토글만 하고 진짜 애니메이션 갱신은 PlayAnimation()에서 따로 처리하는게 좋을듯.

2. 발생한 오류/특이사항 결과

3. 앞으로 수정/개선 방향

# 2026 09 22
file4.cpp 실습 완료

1. 실행/수정한 코드 내용 (계획)
	m을 눌렀을떄 다시 기존 위치로 돌아가게 하기 위해 기존 위치 좌표에서 현재 위치를 뺀 벡터를 정규화해서 매 프레임 이동하게 만들었다. 
	근데 생겼던 문제가 정규화해서 각 좌표에 이동량을 곱하는 로직이 위에 있어서 절대값이 0.01보다 작을떄 검사하는 로직이 의미가 없었다. 
	결국 오차 범위가 매우 적을떄 정규화된 값이 0이 되고 그걸 곱해서 rect.x에 대입하니까 사각형이 그냥 사라져버리는 문제가 생겼다. 
	그래서 검사하는 로직을 위로 올리고 bool 플래그를 통해 아직 도착하지 못한 사각형이 있으면 false상태로 모든 사각형이 도착했으면
	true 그대로 반환해서 true일떄 currentAnimation을 0으로 만들었다. 

	vector.clear가 있어서 r명령어 같은거 구현할떄 굉장히 편리했다. 한번에 벡터에 있는 것들 다 지울 수 있어서. 

	rect.r = rect.baseR + 0.13 * sin(3.0f * currentFrame); sin을 통해 크기나 이동을 조율하는 로직입니다. sin이 -1~1을 반환하기 때문에
1. 첫 시작 위치를 주고 그걸 기준으로 -1~1이라는 범위를 왔다갔다 하게 할 수 있고 여기서는 0.13을 곱해서 -1~1이라는 최대값 범위를 소폭 줄이는 것도
	
	가능한 굉장히 좋은 로직이었다. 매 프레임 sin 그래프를 따라 이동하는 걸 확인 가능했다. 

	특히 또 Rect 구조체에 velocity를 저장해둔게 여러므로 정말 편리했습니다. 특정 사각형을 움직일떄 어떤 방향으로 매 프레임 나아갈지를 결정할
	일이 아무래도 많다보니 필수적이라고 느꼈습니다. 

2. 발생한 오류/특이사항 결과
	
	그 와중에 flag를 쓰기전에 사각형 중에 도달 안했으면 이동하고 return;을 해버려가지고 m눌렀을떄 사각형이 하나씩 이동하는 문제점이 발생했었다. 

3. 앞으로 수정/개선 방향
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


# 2026 09 23
실습 5번 시작. computerGraphicsOpenGL file5.cpp

1. 실행/수정한 코드 내용 (계획)

	r 리셋 명령어를 의식해서 20~40개 랜덤 사각형 생성을 따로 정의된 함수로 제작할 예정. 그래서 r을 누르면 기존 vector clear하고
	바로 생성 가능할 수 있게 할 예정이다. 



2. 발생한 오류/특이사항 결과
	사각형 간의 충돌 처리를 했을때 하나랑 충돌했는데 사각형이 한번에 엄청나게 커지는 버그가 있었다.
					
	2번쨰 버그는 오른쪽 마우스를 눌러서 사각형을 작게 만들었을떄 생기는 버그였다. eraseRect.r -= RECT_SIZE / 2; 로 감소를 시키다보니
	eraseRect.r 자체가 음수가 되버리는 문제가 생겼음. max 알고리즘 함수를 사용해 반지름의 최한선을 설정해서 해결했다. 
	반지름의 크기는 음수가 되면 안된다는 예외 조건을 충분히 고려해야 한다. 특히 size를 다룰떄는.

3. 앞으로 수정/개선 방향
	그 버그를 수정하는 원인을 찾아보자. 사각형 충돌 처리 조건문에 randRects[i].isDraw가 false 일떄는 검사하지 말아야 한다는
	로직이 필요했다. 그게 없으면 그 자리에 있는 사각형과 매 프레임 충돌나면서 사각형이 계속 커지는 문제가 발생했다.




# 2026 09 24
실습 6번 구현 시작. file6.cpp

1. 실행/수정한 코드 내용 (계획)

	일단 확실한건 playAnimation을 시작부터 매 프레임 호출할 수는 없다. 그니까 currentAnimation을 계속 0인 상태로 두고
	마우스 왼쪽 눌러서 사등분 되면 1~4로 랜덤으로 매 프레임 애니메이션 호출되게 제작해야함. 그리고 그 애니메이션 끝나면
	다시 0으로 바꾸고 그걸 순환하는 시스템을 설계해야된다. 

2. 발생한 오류/특이사항 결과


3. 앞으로 수정/개선 방향





# 2026 09 26

삼각형 안에 p가 있다 했을떄 그 p가 삼각형 안에 있다면 세 변을 각각 외적했을떄 같은 방향이어야 한다는데
이거 잘 이해가 안되네 아직은.


# 2026 09 27

* 발생한 문제

selected가 되자마자 바로 위치를 업데이트 해버려서 누르자마자 사라져 버린다 도형이
일단 그 방향에 곱해지는 숫자가 너무 커서 한번에 NDC 공간 밖으로 나가버리는 걸 수도 있고
일단 누르면 선택만 되게 바꾸어야함.


* 실습 8번 구현중입니다. 
 
4개의 사분면은 Rect 같은 형태의 구조체에 담아서 충돌 처리를 하는게 좋기는 할듯. 
토글 방식으로 2번 누르면 확대했다가 작아지는 형태로 구현하면 될듯. 

DrawLine()을 DrawScene 안에서 호출해서 x축 y축을 그렸는데 마우스 클릭으로 삼각형을 생성할떄마다 그 위치로 선이 옮겨지는 
오류가 발생했음. 추적 결과 uOffsetLoc으로 매번 t.pos.x, t.pos.y를 주다 보니까 그게 선 그리는 거에도 적용되서 선이
그 특정 위치로 이동했던 문제였습니다.

일단 현재 내 계획은 array<Rect, 4>에다가 사분면 4개를 저장하고 거기에 멤버로 Triangle t가 하나 있고 그걸 덮어씌우는 방식으로
할까 생각중이다. 비어있은지 아닌지 식별 가능한 플래그랑 삼각형 인덱스 뭔지도 저장하는 idx랑 해서.

마우스를 클릭한 지점을 고려해서 먼저 그걸 Rect랑 충돌검사를 해서 판별하는게 좋겠네.

glGenVertexArrays(1, &tr.vao);
glBindVertexArray(tr.vao);
glGenBuffers(1, &tr.vbo);

미리 구조체에 저장해둔 vao, vbo 안 쓰고 새로 선언된거 쓰니까 확대 축소가 안되는 버그가 있었습니다. 




# 2026 10 03 

else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        Dragged = false;

        Shape& DraggedSh = randShapes[pickedIdx];

        for (int i = 0; i < board.size(); ++i)
        {
            for (int j = 0; j < board[i].slot.size(); ++j)
            {
                Shape& sh = board[i].slot[j];

                if (sh.mode == GL_TRIANGLES) continue; // 이미 채워진 슬롯은 건너뜀
                if (DraggedSh.type != sh.type) continue;

                float dist = glm::length(DraggedSh.pos - sh.pos);

                if (dist <= sh.size) // 스냅 허용 반경
                {
                    DraggedSh.isDraw = false;
                    sh.mode = GL_TRIANGLES;
                    return;
                }
            }
        }
    }

기존에는 mousePos 와 도형의 중점을 빼서 로컬 벡터를 구해서 삼각형, 사각형 피킹 처리를 해왔고
Release 즉 드래그 하다가 도형에서 손을 땟을떄 똑같은 방식으로 충돌 처리를 하려고 했는데 피킹이 되기도 하고
되지 않기도 하는 문제가 발생했다. 

정확한 원인은 현재 파악 중이지만 GetTriangleLocal()에서 가져온 좌표 자체가 원점 기준이 아니라 
좌표계가 다른 좌표들끼리 외적해서 문제가 생겼던 것 같다. 

glm::length로 도형의 로컬 중점에서 드래그하는 도형의 중점까지의 길이를 구해서 그 길이가 반지름보다 작았을때
충돌했다고 생각하자는 보다 간편하고 단순한 로직으로 변경하니 문제가 손쉽게 해결됐고 그걸 처음 알게 되었다. 

