// Fill out your copyright notice in the Description page of Project Settings.

#include "RollABallGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "RollABall/Items/RollABallItemBase.h"

void ARollABallGameModeBase::BeginPlay()
{
    TArray<AActor *> Items;                                                                      // 储存 Items 的数组
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARollABallItemBase::StaticClass(), Items); // 将 Level 中的所有 ARollABallItemBase 类型的 Actor 储存到 Items 数组中
    ItemsInLevel = Items.Num();                                                                  // 获取 Items 数组的长度

    if (GameWidgetClass)
    {
        GameWidget = Cast<URollABallWidget>(CreateWidget(GetWorld(), GameWidgetClass)); // 创建 UHD
        if (GameWidget)
        {
            GameWidget->AddToViewport(); // 将 UHD 添加到视口
            UpdateItemText();            // 更新 UHD
        }
    }
}

void ARollABallGameModeBase::UpdateItemText()
{
    if (GameWidget)
    {
        // RollABallWidget.h 中的 SetItemText() 函数通过蓝图实现
        GameWidget->SetItemText(ItemsCollected, ItemsInLevel); // 更新 UHD 的文本内容
    }
}

void ARollABallGameModeBase::ItemCollected()
{
    ++ItemsCollected; // 计数
    UpdateItemText(); // 更新 UHD
}
