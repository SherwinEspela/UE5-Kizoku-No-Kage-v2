#include "Characters/KNKCharacterBase.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AKNKCharacterBase::AKNKCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AKNKCharacterBase::ShouldDisableCapsuleComponent(bool Disable)
{
	if (Disable)
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else {
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	}
}
