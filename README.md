# CppMultiShooter

## TODO
- 웅크린 상태에서 조준 중 이동속도 느리게 하기 ( CombatComponent -> BaseWalkSpeed, AimWalkSpeed 와 같은 변수 하나 추가로 수정 가능할 것으로 보임)
- stand 상태에서 TurnInPlace 애니메이션 전이 시, 과도한 상체 이동 수정 (블랜드 스페이스를 사용하여 하반신에만 애니메이션 재생하는 것 제안)
- 무기 사격 발사음 에코가 가끔 들리지 않는 현상 수정 -> 무기별 사격 애니메이션마다 사운드 notify의 Follow 활성화
- 제자리 회전 중, 무기 회전이 캐릭터 회전에 비해 너무 느리게 이루어지는 현상 수정하기 -> AnimInstance 클래스에서 오른손 회전 보간을 월드 기준이 아닌 본 기준 좌표값을 사용할 것
- 시뮬레이트 프록시 캐릭터dml 제자리 회전 중 떨림 현상 수정
  - 떨림이 발생하는 것은 네트워크 동기화가 매 프레임마다 발생하지 않기 때문임
  - 시뮬레이트 프록시 권한인 경우 tick 마다 별도의 회전 수정 처리하는 것을 고려하였으나, 개선에 실패함 ([참조](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/31515548#questions))
- 프록시 캐릭터 오른손 회전이 복제처리되지 않는 문제 수정(특히 점프 중 잘 나타남) ([참조](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/31479896#questions/17917798))
  - [시도해보기](https://discord.com/channels/807733033192390676/955151938582876170/1296503295635882054)
- 카메라가 사격음 음원에 너무 가까운 경우, 에코 생략됨
- 폰트 적용 ([링크](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/31545426#questions/18972252))
- 몽타주 중단되는 경우 처리-재장전 중 점프 등([링크](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/31658008#questions/21017278))
- match state 리플리케이션 관련 코드 리펙토링 권장 ([링크](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/31686162#questions))