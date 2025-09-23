// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/KNKCharacterBase.h"

// Sets default values
AKNKCharacterBase::AKNKCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AKNKCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AKNKCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AKNKCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

