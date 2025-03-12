// Fill out your copyright notice in the Description page of Project Settings.


#include "GorillatagMovement.h"

#include "Kismet/KismetSystemLibrary.h"


// Sets default values for this component's properties
UGorillatagMovement::UGorillatagMovement()
{
	PrimaryComponentTick.bCanEverTick = true;
	bDisableMovement = false;
}

// Called when the game starts
void UGorillatagMovement::BeginPlay()
{
	Super::BeginPlay();
	MovementInitialization();
	// ...
}

void UGorillatagMovement::MovementInitialization()
{
	OwnerActor = GetOwner();
	if (!OwnerActor) return;

	BodyCollider = Cast<UCapsuleComponent>(OwnerActor->GetRootComponent());
	Head = OwnerActor->FindComponentByClass<UCameraComponent>();
	
	VelocityHistory.Init(FVector::ZeroVector, VelocityHistorySize);
	DenormalizedVelocityAverage = FVector::ZeroVector;
	VelocityIndex = 0;

	InitializeHandReferences();
	LastLocation = OwnerActor->GetActorLocation();
}

void UGorillatagMovement::InitializeHandReferences()
{
	OwnerActor = GetOwner();
	if (!OwnerActor) return;

	TArray<UMotionControllerComponent*> Components;
	OwnerActor->GetComponents(Components);
	UE_LOG(LogTemp, Warning, TEXT("Found %d MotionControllerComponents"), Components.Num());

	for (auto Hand : Components)
	{
		FVector Location = Hand->GetComponentLocation();
		FRotator Rotation = Hand->GetComponentRotation();
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = OwnerActor;
		TObjectPtr<AGorillaTagHand> NewHand = GetWorld()->SpawnActor<AGorillaTagHand>(GorillaHandClass ,Location, Rotation, SpawnInfo);

		if(Hand->GetTrackingSource() == EControllerHand::Left)
		{
			LeftHandFollower = NewHand;
			LeftHandTransform = Hand;
			LastLeftHandPosition = LeftHandTransform->GetComponentLocation();
		}
		else
		{
			RightHandFollower = NewHand;
			RightHandTransform = Hand;
			LastRightHandPosition = RightHandTransform->GetComponentLocation();
		}
	}
}

FVector UGorillatagMovement::CurrentHandPosition(FTransform HandTransform)
{
	FVector HeadLocation = Head->GetComponentLocation();
	if ((HandTransform.GetLocation() - HeadLocation).Length() < MaxArmLength)
	{
		return HandTransform.GetLocation();
	}
	return HeadLocation + (HandTransform.GetLocation() - HeadLocation).GetSafeNormal() * MaxArmLength;
}

void UGorillatagMovement::AddMinVelocityToJump(float Value)
{
	MinVelocityToJump = std::max(MinVelocityToJump + Value, 0.0f);
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 55, FColor::Blue, FString::Printf(TEXT("Velocity= %f"), MinVelocityToJump));
}

void UGorillatagMovement::SetHand(AGorillaTagHand* Hand, bool Left)
{
	if(Left)
	{
		LeftHandFollower = Hand;
		return;
	}
	RightHandFollower = Hand;
}


// Called every frame
void UGorillatagMovement::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//Initializations
	bool bLeftHandColliding = false;
	bool bRightHandColliding = false;
	FVector FinalPosition;
	FVector FirstIterationLeftHand = FVector::Zero();
	FVector FirstIterationRightHand = FVector::Zero();
	FVector RigidBodyMovement = FVector::Zero();

	FVector CurrentLeftHandPosition = CurrentHandPosition(LeftHandTransform->GetComponentTransform()); 
	FVector CurrentRightHandPosition = CurrentHandPosition(RightHandTransform->GetComponentTransform());

	FVector DistanceTraveled = CurrentLeftHandPosition - LastLeftHandPosition + FVector::DownVector * 2 * 980 * DeltaTime * DeltaTime;

	//LeftHand
	if (IterativeCollisionSphereCast(LastLeftHandPosition, MinimumRaycastRadius, DistanceTraveled, DefaultPrecision,  FinalPosition, true))
	{
		if (bWasLeftHandTouching)
		{
			FirstIterationLeftHand = LastLeftHandPosition - CurrentLeftHandPosition;
		}
		else
		{
			FirstIterationLeftHand = FinalPosition - CurrentLeftHandPosition;
		}
		//BodyCollider->SetPhysicsLinearVelocity(FVector::Zero());

		bLeftHandColliding = true;
	}

	//RightHand
	DistanceTraveled = CurrentRightHandPosition - LastRightHandPosition + FVector::DownVector * 2 * 980 * DeltaTime * DeltaTime;

	if (IterativeCollisionSphereCast(LastRightHandPosition, MinimumRaycastRadius, DistanceTraveled, DefaultPrecision, FinalPosition, true))
	{
		if (bWasRightHandTouching)
		{
			FirstIterationRightHand = LastRightHandPosition - CurrentRightHandPosition;
		}
		else
		{
			FirstIterationRightHand = FinalPosition - CurrentRightHandPosition;
		}
		
		//BodyCollider->SetPhysicsLinearVelocity(FVector::Zero());

		bRightHandColliding = true;
	}

	//Average or Add
	if ((bLeftHandColliding || bWasLeftHandTouching) && (bRightHandColliding || bWasRightHandTouching))
	{
		//this lets you grab stuff with both hands at the same time
		RigidBodyMovement = (FirstIterationLeftHand + FirstIterationRightHand) / 2;
	}
	else
	{
		RigidBodyMovement = FirstIterationLeftHand + FirstIterationRightHand;
	}
	
	//Do Final LeftHand Position
	DistanceTraveled = CurrentLeftHandPosition - LastLeftHandPosition;

	if (IterativeCollisionSphereCast(LastLeftHandPosition, MinimumRaycastRadius, DistanceTraveled, DefaultPrecision, FinalPosition, !((bLeftHandColliding || bWasLeftHandTouching) && (bRightHandColliding || bWasRightHandTouching))))
	{
		LastLeftHandPosition = FinalPosition;
		bLeftHandColliding = true;
	}
	else
	{
		LastLeftHandPosition = CurrentLeftHandPosition;
	}
	//Do Final RightHand Position
	DistanceTraveled = CurrentRightHandPosition - LastRightHandPosition;

	if (IterativeCollisionSphereCast(LastRightHandPosition, MinimumRaycastRadius, DistanceTraveled, DefaultPrecision, FinalPosition, !((bLeftHandColliding || bWasLeftHandTouching) && (bRightHandColliding || bWasRightHandTouching))))
	{
		LastRightHandPosition = FinalPosition;
		bRightHandColliding = true;
	}
	else
	{
		LastRightHandPosition = CurrentRightHandPosition;
	}

	StoreVelocities(DeltaTime);

	bool bIsGoingDown = DenormalizedVelocityAverage.Z < 0.01;
	
	if ((bRightHandColliding || bLeftHandColliding) && !bDisableMovement)
	{
		if(!bIsGoingDown && DenormalizedVelocityAverage.Length() > MinVelocityToJump)
		{
			BodyCollider->SetPhysicsLinearVelocity(JumpMultiplier * DenormalizedVelocityAverage * DeltaTime, true);
			if(BodyCollider->GetPhysicsLinearVelocity().Length() > MaxJumpSpeed)
			{
				BodyCollider->SetPhysicsLinearVelocity(BodyCollider->GetPhysicsLinearVelocity().GetSafeNormal() * MaxJumpSpeed);
			}
		}
		else if(RigidBodyMovement != FVector::Zero())
		{
			OwnerActor->SetActorLocation(OwnerActor->GetActorLocation() + RigidBodyMovement);
			BodyCollider->SetPhysicsLinearVelocity(FVector::Zero());
		}
	}

	CheckHandUnstick(CurrentLeftHandPosition, bLeftHandColliding, true);
	CheckHandUnstick(CurrentRightHandPosition, bRightHandColliding, false);
	
	LeftHandFollower->SetActorLocation(LastLeftHandPosition);
	RightHandFollower->SetActorLocation(LastRightHandPosition);

	bWasLeftHandTouching = bLeftHandColliding;
	bWasRightHandTouching = bRightHandColliding;
}

void UGorillatagMovement::CheckHandUnstick(FVector& CurrentHandPosition, bool& HandColliding, bool bLeft)
{
	if (!HandColliding) return;

	FVector LastHandPosition = bLeft ? LastLeftHandPosition : LastRightHandPosition;
	
	// Verifica se a mão se moveu o suficiente para descolar
	if ((CurrentHandPosition - LastHandPosition).Size() > UnStickDistance)
	{
		FVector HeadPosition = BodyCollider->GetComponentLocation();
		FVector Direction = (CurrentHandPosition - HeadPosition).GetSafeNormal();
		float Distance = (CurrentHandPosition - HeadPosition).Size() - MinimumRaycastRadius;
		FHitResult HitInfo;

		// Realiza o SweepSingleByChannel (equivalente ao SphereCast)
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(GetOwner());

		if (!GetWorld()->SweepSingleByChannel(HitInfo, HeadPosition, HeadPosition + Direction * Distance, FQuat::Identity, ECC_WorldStatic , FCollisionShape::MakeSphere(MinimumRaycastRadius * DefaultPrecision), CollisionParams))
		{
			HandColliding = false;

			if(bLeft)
			{
				LastLeftHandPosition = CurrentHandPosition;
				return;
			}
			LastRightHandPosition = CurrentHandPosition;
		}
	}
}

void UGorillatagMovement::StoreVelocities(float DeltaTime)
{
	VelocityIndex = (VelocityIndex + 1) % VelocityHistorySize;
	FVector OldestVelocity = VelocityHistory[VelocityIndex];
	CurrentVelocity = (OwnerActor->GetActorLocation() - LastLocation) / DeltaTime;
	DenormalizedVelocityAverage += (CurrentVelocity - OldestVelocity) / (float)VelocityHistorySize;
	VelocityHistory[VelocityIndex] = CurrentVelocity;
	LastLocation = OwnerActor->GetActorLocation();
}

void UGorillatagMovement::StoreVelocitiesNew(float DeltaTime)
{
	VelocityIndex = (VelocityIndex + 1) % VelocityHistorySize;
	CurrentVelocity = (OwnerActor->GetActorLocation() - LastLocation) / DeltaTime;
	VelocityHistory[VelocityIndex] = CurrentVelocity;
	LastLocation = OwnerActor->GetActorLocation();
	
	DenormalizedVelocityAverage = FVector::ZeroVector;
	for (int i = 0; i < VelocityHistorySize; ++i)
	{
		DenormalizedVelocityAverage += VelocityHistory[i];
	}
	DenormalizedVelocityAverage /= (float)VelocityHistorySize;
}

bool UGorillatagMovement::IterativeCollisionSphereCast(FVector StartPosition, float SphereRadius, FVector MovementVector, float Precision, FVector& EndPosition, bool bSingleHand)
{
    FHitResult HitInfo;
    FVector MovementToProjectedAboveCollisionPlane;
    float SlipPercentage = DefaultSlideFactor;
    
    // First spherecast from the starting position to the final position
    if (CollisionsSphereCast(StartPosition, SphereRadius * Precision, MovementVector, Precision, EndPosition, HitInfo))
    {
        // If we hit a surface, do a bit of a slide
        FVector FirstPosition = EndPosition;
        MovementToProjectedAboveCollisionPlane = FVector::VectorPlaneProject(StartPosition + MovementVector - FirstPosition, HitInfo.Normal) * SlipPercentage;
        
        if (CollisionsSphereCast(EndPosition, SphereRadius, MovementToProjectedAboveCollisionPlane, Precision * Precision, EndPosition, HitInfo))
        {
            // If we hit trying to move perpendicularly, stop there and our end position is the final spot we hit
            return true;
        }
        if (CollisionsSphereCast(MovementToProjectedAboveCollisionPlane + FirstPosition, SphereRadius, StartPosition + MovementVector - (MovementToProjectedAboveCollisionPlane + FirstPosition), Precision * Precision * Precision, EndPosition, HitInfo))
        {
            // If we hit, then return the spot we hit
            return true;
        }
        // This shouldn't really happen, so back off
        EndPosition = FirstPosition;
        return true;
    }
    // As a sanity check, try a smaller spherecast
    if (CollisionsSphereCast(StartPosition, SphereRadius * Precision * 0.66f, MovementVector.GetSafeNormal() * (MovementVector.Size() + SphereRadius * Precision * 0.34f), Precision * 0.66f, EndPosition, HitInfo))
    {
        EndPosition = StartPosition;
        return true;
    }
	
    EndPosition = FVector::ZeroVector;
    return false;
}

bool UGorillatagMovement::CollisionsSphereCast(FVector StartPosition, float SphereRadius, FVector MovementVector, float Precision, FVector& FinalPosition, FHitResult& HitInfo) const
{
	FHitResult InnerHit;
	FCollisionQueryParams CollisionParams;
	CollisionParams.bTraceComplex = true;
	CollisionParams.bReturnPhysicalMaterial = true;
	CollisionParams.AddIgnoredActor(OwnerActor);

	// Initial spherecast
	if (GetWorld()->SweepSingleByChannel(HitInfo, StartPosition, StartPosition + MovementVector, FQuat::Identity, ECC_WorldStatic , FCollisionShape::MakeSphere(SphereRadius * Precision), CollisionParams))
	{
		// If we hit, we're trying to move to a position a sphere radius distance from the normal
		FinalPosition = HitInfo.Location + HitInfo.Normal * SphereRadius;

		// Check a spherecast from the original position to the intended final position
		if (GetWorld()->SweepSingleByChannel(InnerHit, StartPosition, FinalPosition, FQuat::Identity, ECC_WorldStatic , FCollisionShape::MakeSphere(SphereRadius * Precision * Precision), CollisionParams))
		{
			FinalPosition = StartPosition + (FinalPosition - StartPosition).GetSafeNormal() * FMath::Max(0.0f, HitInfo.Distance - SphereRadius * (1.0f - Precision * Precision));
			HitInfo = InnerHit;
		}
		// Bonus raycast check to make sure that something odd didn't happen with the spherecast
		else if (GetWorld()->LineTraceSingleByChannel(InnerHit, StartPosition, FinalPosition, ECC_WorldStatic , CollisionParams))
		{
			FinalPosition = StartPosition;
			HitInfo = InnerHit;
			return true;
		}
		return true;
	}
	// Anti-clipping through geometry check
	if (GetWorld()->LineTraceSingleByChannel(HitInfo, StartPosition, StartPosition + MovementVector, ECC_WorldStatic , CollisionParams))
	{
		FinalPosition = StartPosition;
		return true;
	}
	
	FinalPosition = FVector::ZeroVector;
	return false;
}

