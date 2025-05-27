## 개요
|![1](https://github.com/user-attachments/assets/78529630-9ed5-470a-8fb2-2d7c6352b4c4)|![2](https://github.com/user-attachments/assets/90b37975-4655-4dca-96e8-0f2994d842d6)|
|:---:|:---:|
|로비 화면|플레이 화면|

## 프로젝트 설명
실시간 멀티플레이어 3인칭 슈터 게임을 제작하였습니다.<br>
Steam 계정과 연동, 호스트가 일시적 서버 역할을 하는 P2P 구조를 구현하였습니다.<br>
[시연영상 보러가기](https://www.youtube.com/watch?v=aeM0kpd-G2I)

- 사용 기술 : Unreal 5.4, C++17
- 1인 개발

## 구현 기능
1. Online Subsystem 플러그인 적용 및 Steam 계정 연동
2. 캐릭터 애니메이션 시스템 구현
3. 서버-클라이언트 구조 기반의 복제 (Replication)
4. 네트워크 지연 보완 (Lag Compensation) - SSR(Server Side Rewind)
5. 클라이언트 예측 및 서버 처리 요청
