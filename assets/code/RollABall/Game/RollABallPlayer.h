// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "RollABallPlayer.generated.h"

UCLASS()
class ROLLABALL_API ARollABallPlayer : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARollABallPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 声明 Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent *Mesh; // 静态网格体
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent *SpringArm; // 弹簧臂
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent *Camera; // 摄像机

	// 定义变量
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveForce = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpImpulse = 800.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxJumpCount = 1;

	// 声明输入触发回调函数
	void MoveForward(const FInputActionValue &Value);
	void MoveRight(const FInputActionValue &Value);
	void Jump();

private:
	// 声明角色的输入映射上下文和输入动作
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext *DefaultIMC;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction *JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction *MoveForwardAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction *MoveRightAction;

	int32 JumpCount = 0; // 落地前的跳跃次数

	// 声明碰撞回调函数
	UFUNCTION()
	void OnHit(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;
};
