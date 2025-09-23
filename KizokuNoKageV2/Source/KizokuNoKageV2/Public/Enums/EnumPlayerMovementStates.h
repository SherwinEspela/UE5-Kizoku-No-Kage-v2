#pragma once
// Copyright 2025 Sherwin Espela. All rights reserved.
#pragma once

UENUM(BlueprintType)
enum class EPlayerMovementStates : uint8
{
	EPMS_Idling					UMETA(DisplayName = "Idling"),
	EPMS_Moving					UMETA(DisplayName = "Moving"),
	EPMS_Running				UMETA(DisplayName = "Running"),
	EPMS_TakingCover			UMETA(DisplayName = "Taking Cover"),
	EPMS_WallHugging			UMETA(DisplayName = "Wall Hugging"),
	EPMS_WallPeeking			UMETA(DisplayName = "Wall Peeking"),
	EPMS_UnHuggingWall			UMETA(DisplayName = "UnHugging Wall"),
	EPMS_UnpeekingWall			UMETA(DisplayName = "Unpeek Wall"),
	EPMS_Jumping				UMETA(DisplayName = "Jumping"),
	EPMS_WallJumping			UMETA(DisplayName = "Wall Jumping"),
	EPMS_Climbing				UMETA(DisplayName = "Climbing"),
	EPMS_ClimbingUp				UMETA(DisplayName = "Climbing Up"),
	EPMS_Crouching				UMETA(DisplayName = "Crouching"),
	EPMS_StealthAttacking		UMETA(DisplayName = "Stealth Attacking"),
	EPMS_StandingToCrouching	UMETA(DisplayName = "Standing to Crouching"),
	EPMS_CrouchingToStanding	UMETA(DisplayName = "Crouching to Standing"),
	EPMS_Default				UMETA(DisplayName = "Default")
};

FORCEINLINE void PrintPlayerMovementState(EPlayerMovementStates Value) {
	UE_LOG(LogTemp, Log, TEXT("Player Movement State: %s"), *UEnum::GetValueAsName(Value).ToString());
}

FORCEINLINE FString DebugGetPlayerMovementState(EPlayerMovementStates Value) {
	return UEnum::GetValueAsName(Value).ToString();
}
