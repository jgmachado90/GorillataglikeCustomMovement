// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GorillaTagHand.h"
#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "GorillatagMovement.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CUSTOMMOVEMENT_API UGorillatagMovement : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGorillatagMovement();

	void InitializeHandReferences();
	
	UFUNCTION( BlueprintCallable)
	void MovementInitialization();

	UFUNCTION(BlueprintCallable)
	FVector CurrentHandPosition(FTransform HandTransform);

	UFUNCTION(BlueprintCallable)
	void AddMinVelocityToJump(float Value);

	UFUNCTION(BlueprintCallable)
	void SetHand(AGorillaTagHand* Hand, bool Left);

	UFUNCTION()
	void CheckHandUnstick(FVector& CurrentHandPosition, bool& HandColliding, bool bLeft);


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AGorillaTagHand> GorillaHandClass;
	
	UPROPERTY(BlueprintReadWrite)
	AActor* OwnerActor;
	
	UPROPERTY(BlueprintReadWrite)
	UCapsuleComponent* BodyCollider;

	UPROPERTY(BlueprintReadWrite)
	UCameraComponent* Head;

	UPROPERTY(BlueprintReadWrite)
	AGorillaTagHand* LeftHandFollower;

	UPROPERTY(BlueprintReadWrite)
	AGorillaTagHand* RightHandFollower;

	UPROPERTY(BlueprintReadWrite)
	UMotionControllerComponent* LeftHandTransform;

	UPROPERTY(BlueprintReadWrite)
	UMotionControllerComponent* RightHandTransform;

	UPROPERTY(BlueprintReadWrite)
	FVector LastLeftHandPosition;

	UPROPERTY(BlueprintReadWrite)
	FVector LastRightHandPosition;

	UPROPERTY(BlueprintReadWrite)
	bool bWasLeftHandTouching;

	UPROPERTY(BlueprintReadWrite)
	bool bWasRightHandTouching;
	
	UPROPERTY(BlueprintReadWrite)
	FVector LeftHandOffset;

	UPROPERTY(BlueprintReadWrite)
	FVector RightHandOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxArmLength = 150;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinimumRaycastRadius = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefaultPrecision = 0.995f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefaultSlideFactor = 0.3;

	UPROPERTY(BlueprintReadWrite)
	int VelocityIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> VelocityHistory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int VelocityHistorySize = 10;

	UPROPERTY(BlueprintReadWrite)
	FVector CurrentVelocity;

	UPROPERTY(BlueprintReadWrite)
	FVector LastLocation;

	UPROPERTY(BlueprintReadWrite)
	FVector DenormalizedVelocityAverage;

	UPROPERTY(BlueprintReadWrite)
	bool bDisableMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinVelocityToJump = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpMultiplier = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxJumpSpeed = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UnStickDistance = 10;

	

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	void StoreVelocities(float DeltaTime);

	void StoreVelocitiesNew(float DeltaTime);
	
	bool IterativeCollisionSphereCast(FVector StartPosition, float SphereRadius, FVector MovementVector,
	                                  float Precision,
	                                  FVector& OutEndPosition, bool bSingleHand);
	bool CollisionsSphereCast(FVector StartPosition, float SphereRadius, FVector MovementVector, float Precision,
	                          FVector& FinalPosition, FHitResult& HitInfo) const;
};
