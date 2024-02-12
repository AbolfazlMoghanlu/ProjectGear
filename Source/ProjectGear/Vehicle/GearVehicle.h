// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Engine/DataAsset.h"
#include "GearVehicle.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogGearVehicle, Log, All);


USTRUCT(BlueprintType)
struct PROJECTGEAR_API FGearWheelData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SuspensionRestLength = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WheelRadius = 30.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SuspensionStrength = 20000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SuspensionDamping = 200000.0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEffectedByEngine = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Torque = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEffectedBySteering = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxSteerAngle = 45.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WheelMass = 20000;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SlideFrictionMin = 0.2;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SlideFrictionMax = 0.8;


// 	FGearWheelData operator=(FGearWheelData& R)
// 	{
// 		FGearWheelData L;
// 		L.SuspensionRestLength	= R.SuspensionRestLength;
// 		L.WheelRadius			= R.WheelRadius;
// 		L.SuspensionStrength	= R.SuspensionStrength;
// 		L.SuspensionDamping		= R.SuspensionDamping;
// 		L.bEffectedByEngine		= R.bEffectedByEngine;
// 		L.MaxSteerAngle			= R.MaxSteerAngle;
// 		L.WheelMass				= R.WheelMass;
// 		L.SlideFrictionMin		= R.SlideFrictionMin;
// 		L.SlideFrictionMax		= R.SlideFrictionMax;
// 
// 		return L;
// 	}
};


UCLASS(BlueprintType)
class PROJECTGEAR_API UGearWheelDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGearWheelData WheelData;
};

USTRUCT(BlueprintType)
struct PROJECTGEAR_API FGearWheelSetup
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WheelName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UGearWheelDataAsset* WheelDataAsset;
};


USTRUCT(BlueprintType)
struct PROJECTGEAR_API FGearWheel
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WheelName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGearWheelData WheelData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform WheelSocketTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RestLength = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SuspentionOffset = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SuspentionSpeed = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform WorldTransform = FTransform::Identity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Velocity = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LastSuspentionOffset = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform LastWorldTransform = FTransform::Identity;
};

UCLASS()
class PROJECTGEAR_API AGearVehicle : public APawn
{
	GENERATED_BODY()

public:	
	AGearVehicle();
	virtual void Tick(float DeltaTime) override;

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetMainCamera() const { return MainCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = input)
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = input)
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = input)
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wheel)
	TArray<FGearWheelSetup> WheelsSetup;


	UPROPERTY(BlueprintReadOnly, Category = Wheel)
	TArray<FGearWheel> Wheels;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wheel)
	bool bDrawDebug = true;

	FVector2D InputMoveValue;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = VehicleProxy, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* VehicleBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* MainCamera;

	void InitWheels();

	void DrawWheelDebug() const;
};
