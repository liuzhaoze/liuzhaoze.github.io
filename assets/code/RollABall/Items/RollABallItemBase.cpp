// Fill out your copyright notice in the Description page of Project Settings.

#include "RollABallItemBase.h"
#include "RollABall/Game/RollABallPlayer.h"
#include "RollABall/Game/RollABallGameModeBase.h"

// Sets default values
ARollABallItemBase::ARollABallItemBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// 创建 Components
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");

	RootComponent = Mesh;

	// 回调函数
	Mesh->OnComponentBeginOverlap.AddDynamic(this, &ARollABallItemBase::OnBeginOverlap);
}

// Called when the game starts or when spawned
void ARollABallItemBase::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ARollABallItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ARollABallItemBase::OnBeginOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	if (Cast<ARollABallPlayer>(OtherActor) != nullptr)
	{
		// Overlap 的 Actor 是 RollABallPlayer 类
		Collected(); // 调用蓝图函数
	}
}

void ARollABallItemBase::Collected_Implementation()
{
	ARollABallGameModeBase *GameMode = Cast<ARollABallGameModeBase>(GetWorld()->GetAuthGameMode()); // 获得当前关卡的 Game Mode
	if (GameMode)
	{
		GameMode->ItemCollected(); // 调用 Game Mode 的 ItemCollected() 函数
	}
}
