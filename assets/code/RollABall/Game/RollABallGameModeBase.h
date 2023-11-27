// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RollABall/UHD/RollABallWidget.h"
#include "RollABallGameModeBase.generated.h"

/**
 *
 */
UCLASS()
class ROLLABALL_API ARollABallGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

protected:
	int32 ItemsCollected = 0;
	int32 ItemsInLevel = 0;

	// 用于创建 UHD
	UPROPERTY(EditAnywhere, Category = "Widgets")
	TSubclassOf<class UUserWidget> GameWidgetClass; // 暴露给虚幻编辑器选择 UHD 的接口
	UPROPERTY()
	URollABallWidget *GameWidget; // UHD 的实例

	// 在游戏开始时初始化关卡内的 Items 总数：ItemsInLevel
	virtual void BeginPlay() override;
	// 用于更新 UHD
	void UpdateItemText();

public:
	// 在 Item 被收集时调用，用于更新 ItemsCollected
	void ItemCollected();
};
