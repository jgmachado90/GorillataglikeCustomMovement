// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MotionControllerComponent.h"
#include "Components/ActorComponent.h"
#include "GorillaTagHand.generated.h"



UCLASS(BlueprintType, Blueprintable, Abstract)
class CUSTOMMOVEMENT_API AGorillaTagHand : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	AGorillaTagHand();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Left;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

};
