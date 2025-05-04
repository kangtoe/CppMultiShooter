// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShooterHUD.generated.h"

// BP에 사용 가능하도록 구조체 선언
USTRUCT(BlueprintType)
struct FHUDPackage
{
    GENERATED_BODY()

public:
    UTexture2D* CrosshairsCenter = nullptr;
    UTexture2D* CrosshairsLeft = nullptr;
    UTexture2D* CrosshairsRight = nullptr;
    UTexture2D* CrosshairsTop = nullptr;
    UTexture2D* CrosshairsBottom = nullptr;

    float CrosshairSpread;
    FLinearColor CrosshairsColor;
};

/**
 *
 */
UCLASS()
class CPPMULTISHOOTER_API AShooterHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    UPROPERTY(EditAnywhere, Category = "Player Stats")
    TSubclassOf<class UUserWidget> CharacterOverlayClass;
    void AddCharacterOverlay();

    UPROPERTY()
    class UCharacterOverlay* CharacterOverlay;

    UPROPERTY(EditAnywhere, Category = "Announcements")
    TSubclassOf<UUserWidget> AnnouncementClass;
    UPROPERTY()
    class UAnnouncement* Announcement;    
    void AddAnnouncement();
       
    UPROPERTY()
    class UScopeWidget* ScopeWidget;
    void AddScopeWidget(TSubclassOf<UScopeWidget> ScopeClass);

protected:
    virtual void BeginPlay() override;    

private:    
    FHUDPackage HUDPackage;

    void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);
    UPROPERTY(EditAnywhere)
    float CrosshairSpreadMax = 16.f;

public:
    FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};