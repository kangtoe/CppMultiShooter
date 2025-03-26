# CppMultiShooter

## TODO
- 웅크린 상태에서 조준 중 이동속도 느리게 하기 ( CombatComponent -> BaseWalkSpeed, AimWalkSpeed 와 같은 변수 하나 추가로 수정 가능할 것으로 보임)
- stand 상태에서 TurnInPlace 애니메이션 전이 시, 과도한 상체 이동 수정 (블랜드 스페이스를 사용하여 하반신에만 애니메이션 재생하는 것 제안)
- 무기 사격 발사음 에코가 가끔 들리지 않는 현상 수정 -> 무기별 사격 애니메이션마다 사운드 notify의 Follow 활성화
