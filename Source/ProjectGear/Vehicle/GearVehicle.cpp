// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/GearVehicle.h"

#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogGearVehicle)

AGearVehicle::AGearVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	VehicleBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleBody"));
	SetRootComponent(VehicleBody);
	VehicleBody->SetSimulatePhysics(true);
	VehicleBody->SetMassOverrideInKg(NAME_None, 1000.0f, true);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	
	MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	MainCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	MainCamera->bUsePawnControlRotation = false;
}

void AGearVehicle::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AGearVehicle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *InputMoveValue.ToString());

	for (FGearWheel& Wheel : Wheels)
	{
		FVector UpForce = FVector::ZeroVector;
		FVector ForwardForce = FVector::ZeroVector;
		FVector RightForce = FVector::ZeroVector;
		
		const FTransform VehicleBodyTransform = GetTransform();
		Wheel.LastWorldTransform = Wheel.WorldTransform;
		//Wheel.WorldTransform = Wheel.WheelSocketTransform * VehicleBodyTransform;
		Wheel.WorldTransform = Wheel.WheelSocketTransform * VehicleBodyTransform;
		Wheel.WorldTransform = Wheel.WheelData.bEffectedBySteering ?
			FTransform(FRotator(0, Wheel.WheelData.MaxSteerAngle * InputMoveValue.X, 0)) * Wheel.WorldTransform :
			Wheel.WorldTransform;
		Wheel.Velocity = (Wheel.WorldTransform.GetLocation() - Wheel.LastWorldTransform.GetLocation()) / DeltaTime;

		const FVector SuspentionStartPos = Wheel.WorldTransform.GetLocation();
		const FVector SuspentionEndPos = SuspentionStartPos + Wheel.RestLength * -Wheel.WorldTransform.GetRotation().GetUpVector();
		
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		FCollisionResponseParams ResponseParams;
		
		GetWorld()->LineTraceSingleByChannel(HitResult, SuspentionStartPos, SuspentionEndPos,
			ECC_WorldStatic, QueryParams, ResponseParams);

		if (HitResult.bBlockingHit)
		{
			Wheel.LastSuspentionOffset = Wheel.SuspentionOffset;
			Wheel.SuspentionOffset = Wheel.RestLength - FVector::Distance(SuspentionStartPos, HitResult.Location);

			Wheel.SuspentionSpeed = Wheel.SuspentionOffset - Wheel.LastSuspentionOffset;



			float Amp = Wheel.SuspentionOffset * Wheel.WheelData.SuspensionStrength;
			float Drag = FVector::DotProduct(Wheel.WorldTransform.GetRotation().GetUpVector(), Wheel.Velocity) *
				Wheel.WheelData.SuspensionDamping;

			UpForce = Wheel.WorldTransform.GetRotation().GetUpVector() * (Amp - Drag);
			UpForce += Wheel.WorldTransform.GetRotation().GetUpVector() * Wheel.WheelData.WheelMass * -9.8f;

			// ------------------------------------------------------------------------

			if (Wheel.WheelData.bEffectedByEngine)
			{
				ForwardForce = Wheel.WorldTransform.GetRotation().GetForwardVector() * Wheel.WheelData.Torque * InputMoveValue.Y;
			}

			// ------------------------------------------------------------------------

			float ForwardAmp = FVector::DotProduct(Wheel.WorldTransform.GetRotation().GetForwardVector(), Wheel.Velocity);
			float RightAmp = FVector::DotProduct(Wheel.WorldTransform.GetRotation().GetRightVector(), Wheel.Velocity);
			
			float SlideRatio = FMath::Clamp(RightAmp / (RightAmp + ForwardAmp), 0, 1);
			float SlideFriction = FMath::Lerp(Wheel.WheelData.SlideFrictionMin, Wheel.WheelData.SlideFrictionMax, SlideRatio);

			//FVector LatFrictionForce = Wheel.WorldTransform.GetRotation().GetRightVector() * -RightAmp * SlideFriction;

 			RightForce = Wheel.WorldTransform.GetRotation().GetRightVector() * RightAmp *
 				Wheel.WheelData.WheelMass * -SlideFriction / DeltaTime;
//  			RightForce = Wheel.WorldTransform.GetRotation().GetRightVector() * RightAmp *
//  				Wheel.WheelData.WheelMass * -Wheel.WheelData.SlideFrictionMin / DeltaTime;

			// ------------------------------------------------------------------------

			VehicleBody->AddForceAtLocation(UpForce + RightForce + ForwardForce, Wheel.WorldTransform.GetLocation());
		}
	}

	InputMoveValue = FVector2D::ZeroVector;

	DrawWheelDebug();
}

void AGearVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGearVehicle::Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGearVehicle::Look);
	}
}


void AGearVehicle::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	InitWheels();
}

void AGearVehicle::Move(const FInputActionValue& Value)
{
	InputMoveValue = Value.Get<FVector2D>();
}

void AGearVehicle::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AGearVehicle::InitWheels()
{
	for (const FGearWheelSetup& WheelSetup : WheelsSetup)
	{
		if (WheelSetup.WheelDataAsset)
		{
			FGearWheel Wheel;
			Wheel.WheelName = WheelSetup.WheelName;
			Wheel.WheelData = WheelSetup.WheelDataAsset->WheelData;
			Wheel.WheelSocketTransform = VehicleBody->GetSocketTransform(Wheel.WheelName, RTS_Component);

			Wheel.RestLength = Wheel.WheelData.SuspensionRestLength + Wheel.WheelData.WheelRadius;

			Wheels.Add(Wheel);
		}
	}
}

void AGearVehicle::DrawWheelDebug() const
{
	if (!bDrawDebug)
	{
		return;
	}

	for (const FGearWheel& Wheel : Wheels)
	{
		const FVector LineStart = Wheel.WorldTransform.GetLocation();

		const float LineLength = 50.0f;

		FVector ForwardVector = Wheel.WorldTransform.GetRotation().GetForwardVector();
		FVector LineEndForward = LineStart + ForwardVector * LineLength;

		FVector RightVector = Wheel.WorldTransform.GetRotation().GetRightVector();
		FVector LineEndRight = LineStart + RightVector * LineLength;

		FVector UpVector = Wheel.WorldTransform.GetRotation().GetUpVector();
		FVector LineEndUp = LineStart + UpVector * LineLength;


		DrawDebugLine(GetWorld(), LineStart, LineEndForward, FColor::Red);
		DrawDebugLine(GetWorld(), LineStart, LineEndRight, FColor::Green);
		DrawDebugLine(GetWorld(), LineStart, LineEndUp, FColor::Blue);
	}
}

