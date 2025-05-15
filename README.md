# CppMultiShooter

## TOKNOW
- 언리얼 에디터 파이에서 랙 설정하기
    - console command (opened with ~) -> `NetEmulation.PktLag N`
    - 서버 -> 클라이언트에 추가적인 지연을 발생
- 몽타주 중단되는 경우 처리-재장전 중 점프 등 -[링크](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/31658008#questions/21017278)
    - XXXMontage_Play(XXXMontage, 1, EMontagePlayReturnType::MontageLength, 0, `false`) -> 인수 추가 전달, 마지막 bool 변수 전달이 핵심적
- 무기 사격 발사음 에코가 가끔 들리지 않는 현상 수정 -> 무기별 사격 애니메이션마다 사운드 notify의 Follow 활성화

## TODO
- 웅크린 상태에서 조준 중 이동속도 느리게 하기 ( CombatComponent -> BaseWalkSpeed, AimWalkSpeed 와 같은 변수 하나 추가로 수정 가능할 것으로 보임)
- stand 상태에서 TurnInPlace 애니메이션 전이 시, 과도한 상체 이동 수정 (블랜드 스페이스를 사용하여 하반신에만 애니메이션 재생하는 것 제안)
- 제자리 회전 중, 무기 회전이 캐릭터 회전에 비해 너무 느리게 이루어지는 현상 수정하기 -> AnimInstance 클래스에서 오른손 회전 보간을 월드 기준이 아닌 본 기준 좌표값을 사용할 것
- 프록시 캐릭터 일부 모션(점프 등)에서 오른손 회전이 복제처리되지 않는 문제 수정 - [참조](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/31479896#questions/17917798)
  - [시도해보기](https://discord.com/channels/807733033192390676/955151938582876170/1296503295635882054)
- 카메라가 사격음 음원에 너무 가까운 경우, 에코 생략됨
- 폰트 적용 - [링크](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/31545426#questions/18972252)
- match state 리플리케이션 관련 코드 리펙토링 권장 - [링크](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/31686162#questions)
- 무기 장비 중 쿨다운 상태 진입시, 크로스헤어 UI가 사라지지 않는 문제 ( UCombatComponent::SetHUDCrosshairs()에서 game state 검사 로직 추가 고려)
- 1인칭 시점 크로스헤어 스타일 - [참조](https://discord.com/channels/807733033192390676/955151938582876170/1335061468835610804)
- 반동 구현하기 - [참조](https://discord.com/channels/807733033192390676/955151938582876170/1261158551577956452), [참조2](https://mstone8370.tistory.com/37?category=1118176)
- 산탄총 개별 장전 모션마다 실제 1발씩 장전하도록 적용하기
- 재장전 애니메이션이 멀티플레이에서 정상 동작하는 지 확인하고 문제 있는 경우 개선 - [참조](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/31975848#questions/18778318)
- 발사체 무기 클래스에서 사격 분산 정보가 동기화 되고 있는 지 확인할 것
- 발사체 SSR 정확한 PathParams.ProjectileRadius 설정 [참조](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/32182616#questions/19142128)
- 배낭과 모자 끝 부분 히트 박스 지우기
- 핑이 너무 높은 경우, SSR 사용 비활성화 [참조](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/32182626#questions)
- 무기 교체 또는 획득 시 SSR 적용 [참조](https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/32182626#questions/18556222)