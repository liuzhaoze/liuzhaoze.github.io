// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RollABallItemBase.generated.h"

UCLASS()
class ROLLABALL_API ARollABallItemBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARollABallItemBase();

	// 声明 Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent *Mesh;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 声明回调函数
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

	// 声明蓝图函数
	UFUNCTION(BlueprintNativeEvent)
	void Collected();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
