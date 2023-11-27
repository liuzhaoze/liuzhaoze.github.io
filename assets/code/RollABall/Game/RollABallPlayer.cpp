// Fill out your copyright notice in the Description page of Project Settings.

#include "RollABallPlayer.h"

// Sets default values
ARollABallPlayer::ARollABallPlayer()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// 设置 Components 的默认值；字符串是 Component 的名字
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");

	// 设置 Components 的依赖关系
	RootComponent = Mesh;
	SpringArm->SetupAttachment(Mesh);
	Camera->SetupAttachment(SpringArm);

	// 设置 Components 的默认值
	Mesh->SetSimulatePhysics(true);

	// 设置 Mesh 的碰撞回调函数
	Mesh->OnComponentHit.AddDynamic(this, &ARollABallPlayer::OnHit);
}

// Called when the game starts or when spawned
void ARollABallPlayer::BeginPlay()
{
	Super::BeginPlay();

	// 根据质量设置施加给网格体的力
	MoveForce *= Mesh->GetMass();
	JumpImpulse *= Mesh->GetMass();

	// 在启动时添加 Input Mapping Context 文件
	if (APlayerController *PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultIMC, 0);
		}
	}
}

// Called every frame
void ARollABallPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ARollABallPlayer::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 绑定输入动作
	if (UEnhancedInputComponent *EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ARollABallPlayer::Jump);
		EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &ARollABallPlayer::MoveForward);
		EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &ARollABallPlayer::MoveRight);
	}
}

void ARollABallPlayer::Jump()
{
	if (JumpCount >= MaxJumpCount)
		return;

	// 给网格体施加一个向上的冲量
	Mesh->AddImpulse(FVector(0, 0, JumpImpulse));
	++JumpCount;
}

void ARollABallPlayer::MoveForward(const FInputActionValue &Value)
{
	float AxisValue = Value.Get<float>();
	const FVector Forward = Camera->GetForwardVector() * MoveForce * AxisValue;
	// 给网格体施加一个向前的力
	Mesh->AddForce(Forward);
}

void ARollABallPlayer::MoveRight(const FInputActionValue &Value)
{
	float AxisValue = Value.Get<float>();
	const FVector Right = Camera->GetRightVector() * MoveForce * AxisValue;
	// 给网格体施加一个向右的力
	Mesh->AddForce(Right);
}

void ARollABallPlayer::OnHit(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit)
{
	float HitDirection = Hit.Normal.Z; // 获取碰撞法线的 Z 分量
	// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("HitDirection: %f"), HitDirection));

	if (HitDirection > 0.0f)
	{
		// 碰撞法线的 Z 分量大于 0 ，说明碰撞平面在物体的下方
		// 碰撞法线的 Z 分量小于 0 ，说明碰撞平面在物体的上方
		// 碰撞法线的 Z 分量最大为 1 ，说明碰撞平面在物体的正下方
		JumpCount = 0;
	}
}
