// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RollABallWidget.generated.h"

/**
 *
 */
UCLASS()
class ROLLABALL_API URollABallWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	// 用于更新 UHD ；功能通过蓝图实现
	void SetItemText(int32 ItemsCollected, int32 ItemsInLevel);
};
