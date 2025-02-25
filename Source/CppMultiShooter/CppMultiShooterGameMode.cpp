// Copyright Epic Games, Inc. All Rights Reserved.

#include "CppMultiShooterGameMode.h"
#include "CppMultiShooterCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACppMultiShooterGameMode::ACppMultiShooterGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
