// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KNKCharacterBase.generated.h"

UCLASS()
class KIZOKUNOKAGEV2_API AKNKCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AKNKCharacterBase();

	UFUNCTION(BlueprintCallable)
	void ShouldDisableCapsuleComponent(bool Disable);

protected:
	// States
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Player Input")
	bool bIsDead = false;
};
