// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "HeliMoveComp.h"
#include "HeliGame.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Public/Engine.h"

UHeliMoveComp::UHeliMoveComp(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);



	BoneName = NAME_None; // default is root

	bAccelChange = true;
	bUseAddTorque = true;

	bAddToCurrent = true;
	bUseAddForceForThrust = true;

	bAddLift = true;
	GravityWeight = 0.70f;

	BaseThrust = 10000.f;

	MinimumTiltInclinationAcceleration = 5000.f;

	MaximumAngularVelocity = 100.f;

	MaxNetworkSmoothingFactor = 60.f;
	InterpolationSpeed = MaxNetworkSmoothingFactor;
	bUseInterpolationForMovementReplication = false;
}

/*
Controls
*/
void UHeliMoveComp::AddPitch(float InPitch)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics() && FMath::Abs(BaseComp->GetPhysicsAngularVelocityInDegrees().Size()) <= MaximumAngularVelocity)
	{
		const FVector AngularVelocity = BaseComp->GetRightVector() * InPitch;

		if (bUseAddTorque)
		{
			BaseComp->AddTorqueInRadians(AngularVelocity, BoneName, bAccelChange);
		}
		else
		{
			BaseComp->SetPhysicsAngularVelocityInDegrees(AngularVelocity, bAddToCurrent);
		}
	}
}

void UHeliMoveComp::AddYaw(float InYaw)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics() && FMath::Abs(BaseComp->GetPhysicsAngularVelocityInDegrees().Size()) <= MaximumAngularVelocity)
	{
		const FVector AngularVelocity = BaseComp->GetUpVector() * InYaw;

		if (bUseAddTorque)
		{
			BaseComp->AddTorqueInRadians(AngularVelocity, BoneName, bAccelChange);
		}
		else
		{
			BaseComp->SetPhysicsAngularVelocityInDegrees(AngularVelocity, bAddToCurrent);
		}
	}
}

void UHeliMoveComp::AddRoll(float InRoll)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics() && FMath::Abs(BaseComp->GetPhysicsAngularVelocityInDegrees().Size()) <= MaximumAngularVelocity)
	{
		const FVector AngularVelocity = BaseComp->GetForwardVector() * InRoll;

		if (bUseAddTorque)
		{
			BaseComp->AddTorqueInRadians(AngularVelocity, BoneName, bAccelChange);
		}
		else
		{
			BaseComp->SetPhysicsAngularVelocityInDegrees(AngularVelocity, bAddToCurrent);
		}
		//UE_LOG(LogTemp, Display, TEXT("InRoll = %f - Angular Velocity Rolling: (%s) = %f"), InRoll, *BaseComp->GetPhysicsAngularVelocityInDegrees().ToString(), FMath::Abs(BaseComp->GetPhysicsAngularVelocityInDegrees().Size()) );
	}
}



/*
Thrust
*/

FVector UHeliMoveComp::ComputeThrust(UPrimitiveComponent* BaseComp, float InThrust)
{
	FVector ThrustForce = FVector::ZeroVector;

	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		if (InThrust < 0.f)
		{
			ThrustForce = FVector::UpVector * InThrust * BaseThrust;
		}
		else
		{


			float InclinationAngle = FVector::DotProduct(FVector::UpVector, BaseComp->GetForwardVector());
			float ZWeight = 1.f - FMath::Abs(InclinationAngle);

			FVector UpForceCorrection = FVector(1.f, 1.f, ZWeight);

			ThrustForce = (BaseComp->GetUpVector() * UpForceCorrection) * BaseThrust * InThrust;
		}

	}

	return ThrustForce;
}

void UHeliMoveComp::AddThrust(float InThrust)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics())
	{
		FVector ThrustForce = ComputeThrust(BaseComp, InThrust);

		if (bUseAddForceForThrust)
		{
			BaseComp->AddForce(ThrustForce, BoneName, bAccelChange);
		}
		else
		{
			BaseComp->SetPhysicsLinearVelocity(ThrustForce, bAddToCurrent);
		}
	}
}



/*
Lift
*/
FVector UHeliMoveComp::ComputeLift(UPrimitiveComponent* BaseComp)
{
	FVector LiftForce = FVector::ZeroVector;

	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		float GravityAcceleration = FMath::Abs(GetGravityZ()) * GravityWeight;

		float InclinationAngle = FVector::DotProduct(FVector::UpVector, BaseComp->GetForwardVector());
		float TiltAngle = FVector::DotProduct(FVector::UpVector, BaseComp->GetRightVector());

		float InclinationWeight = 1 - FMath::Abs(InclinationAngle);
		float TiltWeight = 1 - FMath::Abs(TiltAngle);

		// weights gravity acceleration based on inclination and tilt
		float WeightedGravityAcceleration = GravityAcceleration * InclinationWeight * TiltWeight;

		// up momentum will produce zero net force against gravity force when stationary (F = m*a)
		FVector UpMomentum = BaseComp->GetUpVector() * (BaseComp->GetMass() * WeightedGravityAcceleration);

		// adds momentum for inclination and tilt (m = a * tanh(-angle) a.k.a. hyperbolic tangent function) 
		float InclinationAcceleration = MinimumTiltInclinationAcceleration * FMath::Tan(-InclinationAngle);
		float TiltAcceleration = MinimumTiltInclinationAcceleration * FMath::Tan(-TiltAngle);

		FVector InclinationMomentum = BaseComp->GetForwardVector() * GravityAcceleration * InclinationAcceleration;
		FVector TiltMomentum = BaseComp->GetRightVector() * GravityAcceleration *  TiltAcceleration;

		// compute net lift force
		LiftForce = UpMomentum + InclinationMomentum + TiltMomentum;
	}

	return LiftForce;
}

void UHeliMoveComp::AddLift()
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (IsActive() && BaseComp && BaseComp->IsSimulatingPhysics())
	{
		FVector Lift = ComputeLift(BaseComp);

		BaseComp->AddForce(Lift, BoneName, false);
	}
}

FVector UHeliMoveComp::GetPhysicsLinearVelocity()
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		return BaseComp->GetPhysicsLinearVelocity();
	}

	return FVector::ZeroVector;
}

FVector UHeliMoveComp::GetPhysicsAngularVelocity()
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		return BaseComp->GetPhysicsAngularVelocityInDegrees();
	}

	return FVector::ZeroVector;
}

void UHeliMoveComp::SetPhysicsLinearVelocity(FVector NewLinearVelocity)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		BaseComp->SetPhysicsLinearVelocity(NewLinearVelocity, false);
	}
}

void UHeliMoveComp::SetPhysicsAngularVelocity(FVector NewAngularVelocity)
{
	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp && BaseComp->IsSimulatingPhysics())
	{
		BaseComp->SetPhysicsAngularVelocityInDegrees(NewAngularVelocity, false);
	}
}


void UHeliMoveComp::SetPhysMovementState(const FPhysMovementState& TargetPhysMovementState)
{
	if (TargetPhysMovementState.Location.IsNearlyZero())
	{
		return;
	}

	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp)
	{
		FRotator rotation = TargetPhysMovementState.Rotation;
		FVector location = TargetPhysMovementState.Location;
		FVector linearVelocity = TargetPhysMovementState.LinearVelocity;
		FVector angularVelocity = TargetPhysMovementState.AngularVelocity;

		BaseComp->SetWorldRotation(rotation.Quaternion());
		BaseComp->SetWorldLocation(location);
		BaseComp->SetPhysicsLinearVelocity(linearVelocity);
		BaseComp->SetPhysicsAngularVelocityInDegrees(angularVelocity);
	}
}

void UHeliMoveComp::SetPhysMovementStateSmoothly(const FPhysMovementState& TargetPhysMovementState, float DeltaTime)
{	
	if (TargetPhysMovementState.Location.IsNearlyZero())
	{
		return;
	}

	UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
	if (BaseComp)
	{
		// NOTE(andrey): should we use slerp or rinterpto for rotation interpolation?
		//Quat rotation = FQuat::Slerp(BaseComp->GetComponentRotation().Quaternion(), TargetPhysMovementState.Rotation.Quaternion(), 0.1f);
		FRotator rotation = FMath::RInterpTo(BaseComp->GetComponentRotation(), TargetPhysMovementState.Rotation, DeltaTime, InterpolationSpeed);
		FVector location = FMath::VInterpTo(BaseComp->GetComponentLocation(), TargetPhysMovementState.Location, DeltaTime, InterpolationSpeed);
		FVector linearVelocity = FMath::VInterpTo(BaseComp->GetPhysicsLinearVelocity(), TargetPhysMovementState.LinearVelocity, DeltaTime, InterpolationSpeed);
		FVector angularVelocity = FMath::VInterpTo(BaseComp->GetPhysicsAngularVelocityInDegrees(), TargetPhysMovementState.AngularVelocity, DeltaTime, InterpolationSpeed);

		//BaseComp->SetWorldRotation(rotation);
		BaseComp->SetWorldRotation(rotation.Quaternion());
		BaseComp->SetWorldLocation(location);
		BaseComp->SetPhysicsLinearVelocity(linearVelocity);
		BaseComp->SetPhysicsAngularVelocityInDegrees(angularVelocity);
	}
}

void UHeliMoveComp::MovementReplication()
{
	if ((GetPawnOwner() && GetPawnOwner()->IsLocallyControlled())) // && GetPawnOwner()->Role >= ENetRole::ROLE_AutonomousProxy))
	{
		UPrimitiveComponent* BaseComp = Cast<UPrimitiveComponent>(UpdatedComponent);
		if (BaseComp && BaseComp->IsSimulatingPhysics())
		{
			Server_SetPhysMovementState(FPhysMovementState(
				BaseComp->GetComponentLocation(),
				BaseComp->GetComponentRotation(),
				BaseComp->GetPhysicsLinearVelocity(),
				BaseComp->GetPhysicsAngularVelocityInDegrees()
			));
		}
	}
}

bool UHeliMoveComp::Server_SetPhysMovementState_Validate(const FPhysMovementState& NewPhysMovementState)
{
	return true;
}

void UHeliMoveComp::Server_SetPhysMovementState_Implementation(const FPhysMovementState& NewPhysMovementState)
{
	PhysMovementState = NewPhysMovementState;
}

bool UHeliMoveComp::IsNetworkSmoothingFactorActive()
{
	return bUseInterpolationForMovementReplication;
}

void UHeliMoveComp::SetNetworkSmoothingFactor(float inNetworkSmoothingFactor)
{			
	/*Server_SetNetworkSmoothingFactor(inNetworkSmoothingFactor, true);
	InterpolationSpeed = inNetworkSmoothingFactor;
	bUseInterpolationForMovementReplication = true;
	UE_LOG(LogTemp, Display, TEXT("UHeliMoveComp::SetNetworkSmoothingFactor ~ InterpolationSpeed = %f"), InterpolationSpeed);
	if (inNetworkSmoothingFactor <= 0.f)
	{
		bUseInterpolationForMovementReplication = false;
		Server_SetNetworkSmoothingFactor(inNetworkSmoothingFactor, false);
		UE_LOG(LogTemp, Warning, TEXT("UHeliMoveComp::SetNetworkSmoothingFactor ~ Network Smoothing Factor Deactivated!"));
	}*/



	//
	if (inNetworkSmoothingFactor < 1.f)
	{
		// turn interpolation off
		Server_SetNetworkSmoothingFactor(MaxNetworkSmoothingFactor, false);
		InterpolationSpeed = MaxNetworkSmoothingFactor;
		bUseInterpolationForMovementReplication = false;
		UE_LOG(LogTemp, Display, TEXT("UHeliMoveComp::SetNetworkSmoothingFactor ~ Network Smoothing Factor Deactivated!"));
	}
	else if (inNetworkSmoothingFactor >= 100.f)
	{
		Server_SetNetworkSmoothingFactor(1.f, true);
		InterpolationSpeed = 1.f;
		bUseInterpolationForMovementReplication = true;
		UE_LOG(LogTemp, Display, TEXT("UHeliMoveComp::SetNetworkSmoothingFactor ~ Network Smoothing Factor is 100%, interpolation speed set to 1!"));
	}
	else
	{
		// normalize it between 1 and MaxNetworkSmoothingFactor
		// ((limitMax - limitMin) * (valueIn - baseMin) / (baseMax - baseMin)) + limitMin;		
		float smoothFactorNormalized = ((MaxNetworkSmoothingFactor - 1.f) * (inNetworkSmoothingFactor - 0.f) / (100.f - 0.f)) + 1.f;
		
		// smaller values means more interpolation speed, greater values means that interpolation should happen more smoothly (slow interp speed)
		float smoothFactor = MaxNetworkSmoothingFactor - smoothFactorNormalized;

		UE_LOG(LogTemp, Warning, TEXT("UHeliMoveComp::SetNetworkSmoothingFactor ~ inNetworkSmoothingFactor = %f, smoothFactorNormalized = %f, smoothFactor = %f"), inNetworkSmoothingFactor, smoothFactorNormalized, smoothFactor);

		Server_SetNetworkSmoothingFactor(smoothFactor, true);
		InterpolationSpeed = smoothFactor;
		bUseInterpolationForMovementReplication = true;
	}
}

bool UHeliMoveComp::Server_SetNetworkSmoothingFactor_Validate(float InInterpolationSpeed, bool InbUseInterpolationForMovementReplication)
{
	return true;
}

void UHeliMoveComp::Server_SetNetworkSmoothingFactor_Implementation(float InInterpolationSpeed, bool InbUseInterpolationForMovementReplication)
{
	InterpolationSpeed = InInterpolationSpeed;

	bUseInterpolationForMovementReplication = InbUseInterpolationForMovementReplication;
}


/* overrides */
void UHeliMoveComp::InitializeComponent()
{
	Super::InitializeComponent();
}

void UHeliMoveComp::BeginPlay()
{
	Super::BeginPlay();	
}

void UHeliMoveComp::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent)
	{
		//UE_LOG(LogTemp, Warning, TEXT("UHeliMoveComp::TickComponent PawnOwner or UpdatedComponent not found!"));
		return;
	}

	bool bLocalPlayerAuthority = GetPawnOwner() && GetPawnOwner()->IsLocallyControlled() && GetPawnOwner()->Role >= ENetRole::ROLE_AutonomousProxy;

	// add lift
	if (bAddLift && bLocalPlayerAuthority) {
		AddLift();
	}

	// apply received replicated movement state
	if (!bLocalPlayerAuthority)
	{
		if (bUseInterpolationForMovementReplication)
		{
			SetPhysMovementStateSmoothly(PhysMovementState, DeltaTime);
		}
		else
		{
			SetPhysMovementState(PhysMovementState);
		}
	}

	// send movement state
	// Don't need to replicate movement in single player games
	if (GEngine->GetNetMode(GetWorld()) != NM_Standalone)
	{
		MovementReplication();
	}
}

//							Replication List
void UHeliMoveComp::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UHeliMoveComp, PhysMovementState, COND_SimulatedOnly);
	
	DOREPLIFETIME(UHeliMoveComp, InterpolationSpeed);
	DOREPLIFETIME(UHeliMoveComp, bUseInterpolationForMovementReplication);
	//DOREPLIFETIME_CONDITION(UHeliMoveComp, InterpolationSpeed, COND_SimulatedOnly);
	//DOREPLIFETIME_CONDITION(UHeliMoveComp, bUseInterpolationForMovementReplication, COND_SimulatedOnly);
}






