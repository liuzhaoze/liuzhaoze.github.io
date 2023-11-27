---
title: UE5 & C++ 开发游戏 Roll A Ball
date: 2023-11-27 10:00:00 +0800
categories: [软件]
tags: [ue5]    # TAG names should always be lowercase
---

## 0 前言

使用驼峰命名法。

设置英文界面 `culture=en` 。

点击虚幻编辑器右下角的编译按钮 ![编译按钮](/assets/img/RollABall/编译按钮.png) 可以实时编译被修改的源文件。  
可以代替下文中 **按 <kbd>F5</kbd> 编译打开 UE5 编辑器** 的步骤。

源代码目录结构：

```text
.../Unreal Projects/RollABall/Source
│  RollABall.Target.cs
│  RollABallEditor.Target.cs
│
└─RollABall
    │  RollABall.Build.cs
    │  RollABall.cpp
    │  RollABall.h
    │
    ├─Game
    │      RollABallGameModeBase.cpp
    │      RollABallGameModeBase.h
    │      RollABallPlayer.cpp
    │      RollABallPlayer.h
    │
    ├─Items
    │      RollABallItemBase.cpp
    │      RollABallItemBase.h
    │
    └─UHD
            RollABallWidget.cpp
            RollABallWidget.h
```

* [RollABall.Build.cs](/assets/code/RollABall/RollABall.Build.cs)
* [RollABallGameModeBase.cpp](/assets/code/RollABall/Game/RollABallGameModeBase.cpp)
* [RollABallGameModeBase.h](/assets/code/RollABall/Game/RollABallGameModeBase.h)
* [RollABallPlayer.cpp](/assets/code/RollABall/Game/RollABallPlayer.cpp)
* [RollABallPlayer.h](/assets/code/RollABall/Game/RollABallPlayer.h)
* [RollABallItemBase.cpp](/assets/code/RollABall/Items/RollABallItemBase.cpp)
* [RollABallItemBase.h](/assets/code/RollABall/Items/RollABallItemBase.h)
* [RollABallWidget.cpp](/assets/code/RollABall/UHD/RollABallWidget.cpp)
* [RollABallWidget.h](/assets/code/RollABall/UHD/RollABallWidget.h)

## 1 创建项目

1. 游戏 - 空白
2. 项目默认设置：蓝图
3. 项目名称：RollABall
4. 点击创建

在创建项目时应该选择**蓝图**而非 C++ 。因为 C++ 项目会默认生成一个 game mode 和初始 sln 工程，不利于项目文件的管理。

Editor Preference 设置（Edit - Editor Preferences...）：

1. General - Appearance
   1. User Interface - Asset Editor Open Location: Main Window 资产编辑窗口以选项卡的形式在主窗口打开
2. General - Source Code
   1. Accessor - Source Code Editor: Visual Studio Code

## 1 创建 Player Class

虽然 Character Class 提供了很多实用的功能，可以更方便地创建玩家。但是为了学习更底层的知识，此处选择创建 Pawn Class 。

1. Tools - New C++ Class...
2. Add C++ Class 窗口 - 选择 Pawn - Next
3. Name: RollABallPlayer
4. Path: `/Unreal Projects/RollABall/Source/RollABall/Game` （将与游戏有关的类放在 Game 文件夹下）
5. 点击 Create Class
6. 下面的提示框选择 OK

   ```text
   Project now includes sources, please close the editor and build from your IDE.
   ```

7. 下面的提示框选择 No

   ```text
   Successfully added class 'RollABallPlayer', however you must recompile the 'RollABall' module before it will appear in the Content Browser.

   Would you like to edit the code now?
   ```

8. 关闭 UE5 编辑器
9. 在项目路径 `/Unreal Projects/RollABall` 下打开 code-workspace 文件
10. 将 `RollABallPlayer.cpp` 中的 `#include "Game/RollABallPlayer.h"` 改为 `#include "RollABallPlayer.h"` 以解决路径错误
11. 在运行和调试（<kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>D</kbd>）中选择 `Launch RollABallEditor (Development)`
12. 以后通过 <kbd>F5</kbd> 编译并打开 UE5 编辑器

### 1.1 编辑 RollABallPlayer.h

#### 声明组件

```cpp
// 声明 Components
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
UStaticMeshComponent *Mesh; // 静态网格体
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
USpringArmComponent *SpringArm; // 弹簧臂
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
UCameraComponent *Camera; // 摄像机
```

相当于创建 [Components 选项卡](https://liuzhaoze.github.io/posts/UE5%E5%AD%A6%E4%B9%A0%E7%AC%94%E8%AE%B0-1-%E5%85%A5%E9%97%A8%E7%AF%87/#3-%E7%8E%A9%E5%AE%B6%E8%A7%92%E8%89%B2%E7%BC%96%E8%BE%91)中的组件。

所需的头文件

```cpp
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
```

> 所有的 `#include` 语句都应该在 `#include "XXX.generated.h"` 之前。

#### 定义变量

```cpp
// 定义变量
UPROPERTY(EditAnywhere, BlueprintReadWrite)
float MoveForce = 1000.0f;
UPROPERTY(EditAnywhere, BlueprintReadWrite)
float JumpImpulse = 800.0f;
UPROPERTY(EditAnywhere, BlueprintReadWrite)
int32 MaxJumpCount = 1;
```

相当于在 [My Blueprint 选项卡](https://liuzhaoze.github.io/posts/UE5%E5%AD%A6%E4%B9%A0%E7%AC%94%E8%AE%B0-1-%E5%85%A5%E9%97%A8%E7%AF%87/#demo5-%E4%BA%BA%E7%89%A9%E5%87%BA%E7%94%9F%E6%97%B6%E6%89%93%E5%8D%B0%E5%AD%97%E7%AC%A6%E4%B8%B2)中添加新的变量。

#### 在类内使用的变量

```cpp
// 当前跳跃的次数
int32 JumpCount = 0;
```

### 1.2 编辑 RollABallPlayer.cpp

#### 设置每帧是否调用 Tick 函数

```cpp
PrimaryActorTick.bCanEverTick = false;
```

#### 设置 Components 的默认值

```cpp
// 设置 Components 的默认值；字符串是 Component 的名字
Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
Camera = CreateDefaultSubobject<UCameraComponent>("Camera");

// 设置 Components 的依赖关系
RootComponent = Mesh;
SpringArm->SetupAttachment(Mesh);
Camera->SetupAttachment(SpringArm);
```

这里的依赖关系相当于在 [Components 选项卡](https://liuzhaoze.github.io/posts/UE5%E5%AD%A6%E4%B9%A0%E7%AC%94%E8%AE%B0-1-%E5%85%A5%E9%97%A8%E7%AF%87/#demo15-0-%E5%88%9B%E5%BB%BA%E7%81%AB%E7%84%B0%E5%8F%B0)中设置的父子关系。

## 2 从 Player Class C++ 类创建蓝图

### 2.1 创建蓝图

1. 在 Content 文件夹下创建 `_RollABall/Blueprints` 文件夹
2. 右键 `C++ Classes/RollABall/Game` 中的 `RollABallPlayer` C++ 类 - Create Blueprint class based on RollABallPlayer
3. Name: BP_RollABallPlayer
4. Path 选择 `_RollABall/Blueprints`
5. 点击 Create Blueprint Class

### 2.2 设置蓝图

1. 选中 Mesh 组件 - Details 选项卡 - Static Mesh - Static Mesh - 下拉菜单 - 点击齿轮图标 - 勾选 Show Engine Content - 选择 100x100x100 的 Sphere
2. 选中 SpringArm 组件 - Details 选项卡 -
   1. Camera - Target Arm Length: 1000
   2. Transform - Rotation: 0, -45, 0
3. 编译保存

## 3 绑定输入

### 3.1 修改编译选项

1. 打开 `Source/RollABall` 中的 `RollABall.Build.cs`
2. 在 `PublicDependencyModuleNames` 中添加 `EnhancedInput`

```cs
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
```

### 3.2 声明输入映射上下文和输入动作

1. 在 `RollABallPlayer.h` 中添加头文件

   ```cpp
   #include "EnhancedInputComponent.h"
   #include "EnhancedInputSubsystems.h"
   ```

2. 声明变量

   ```cpp
   // 声明角色的输入映射上下文和输入动作
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
   UInputMappingContext *DefaultIMC;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
   UInputAction *JumpAction;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
   UInputAction *MoveForwardAction;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
   UInputAction *MoveRightAction;
   ```

3. 按 <kbd>F5</kbd> 编译打开 UE5 编辑器
4. Tools - Refresh Visual Studio Project
5. 声明输入动作触发的回调函数

   ```cpp
   // 声明输入触发回调函数
   void MoveForward(const FInputActionValue &Value);
   void MoveRight(const FInputActionValue &Value);
   void Jump();
   ```

### 3.3 绑定动作

修改 `RollABallPlayer.cpp` 中的 `ARollABallPlayer::BeginPlay` 函数：

```cpp
void ARollABallPlayer::BeginPlay()
{
    Super::BeginPlay();

    // 在启动时添加 Input Mapping Context 文件
    if (APlayerController *PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultIMC, 0);
        }
    }
}
```

修改 `RollABallPlayer.cpp` 中的 `ARollABallPlayer::SetupPlayerInputComponent` 函数：

```cpp
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
```

定义输入动作的回调函数：

```cpp
void ARollABallPlayer::Jump()
{
    // 给网格体施加一个向上的冲量
    Mesh->AddImpulse(FVector(0, 0, JumpImpulse));
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
```

### 3.4 创建输入映射上下文和输入动作

参考[设置按键映射](https://liuzhaoze.github.io/posts/UE5%E5%AD%A6%E4%B9%A0%E7%AC%94%E8%AE%B0-1-%E5%85%A5%E9%97%A8%E7%AF%87/#115-%E8%AE%BE%E7%BD%AE%E6%8C%89%E9%94%AE%E6%98%A0%E5%B0%84)在 `_RollABall/Input` 文件夹下创建 IMC 文件，在 `_RollABall/Input/Actions` 文件夹下创建 IA 文件。

* `IA_Jump` Value Type: Digital (bool)
* `IA_MoveForward` Value Type: Axis 1D (float)
* `IA_MoveRight` Value Type: Axis 1D (float)

* `IMC_Default` Mappings
  * `IA_Jump`
    * Space Bar
    * Gamepad Face Button Bottom
  * `IA_MoveForward`
    * W
    * S - Modifiers: Negative
    * Gamepad Left Thumbstick Y-Axis
  * `IA_MoveRight`
    * D
    * A - Modifiers: Negative
    * Gamepad Left Thumbstick X-Axis

### 3.5 在蓝图类中设置 IMC 和 IA 文件

1. 打开 `BP_RollABallPlayer` 蓝图
2. 分别搜索 `DefaultIMC`、`JumpAction`、`MoveForwardAction`、`MoveRightAction` （在 `RollABallPlayer.h` 中声明的变量），将其设置为对应的 IMC 和 IA 文件
3. Physics - Mesh -
   1. Simulate Physics: true
   2. Mass (kg): 10
4. Camera Settings - Spring Arm - （不让摄像机跟随角色旋转）
   1. Inherit Pitch: false
   2. Inherit Yaw: false
   3. Inherit Roll: false
5. 编译保存
6. 将 `BP_RollABallPlayer` 蓝图拖入场景中
7. Pawn - Auto Possess Player: Player 0

## 4 设置默认值

1. 在构造函数 `ARollABallPlayer::ARollABallPlayer` 中设置默认值

   ```cpp
   // 设置默认值
   Mesh->SetSimulatePhysics(true);
   ```

2. 在关卡开始时 `ARollABallPlayer::BeginPlay` 设置给网格体施加的力

   ```cpp
   // 根据质量设置施加给网格体的力
   MoveForce *= Mesh->GetMass();
   JumpImpulse *= Mesh->GetMass();
   ```

## 5 限制跳跃次数

在 `ARollABallPlayer::Jump` 中进行跳跃计数

```cpp
void ARollABallPlayer::Jump()
{
    if (JumpCount >= MaxJumpCount)
        return;

    // 给网格体施加一个向上的冲量
    Mesh->AddImpulse(FVector(0, 0, JumpImpulse));
    ++JumpCount;
}
```

在构造函数 `ARollABallPlayer::ARollABallPlayer` 中设置 Mesh 发生碰撞时的回调函数

```cpp
Mesh->OnComponentHit.AddDynamic(this, &ARollABallPlayer::OnHit);
```

获取 `OnComponentHit` 的类型定义：右键 `OnComponentHit` - 转到类型定义

```cpp
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FiveParams( FComponentHitSignature, UPrimitiveComponent, OnComponentHit, UPrimitiveComponent*, HitComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, FVector, NormalImpulse, const FHitResult&, Hit );
```

按照上述类型签名声明回调函数

```cpp
// 声明碰撞回调函数
UFUNCTION()
void OnHit(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit);
```

定义回调函数

```cpp
void ARollABallPlayer::OnHit(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit)
{
    float HitDirection = Hit.Normal.Z; // 获取碰撞法线的 Z 分量

    if (HitDirection > 0.0f)
    {
        // 碰撞法线的 Z 分量大于 0 ，说明碰撞平面在物体的下方
        // 碰撞法线的 Z 分量小于 0 ，说明碰撞平面在物体的上方
        // 碰撞法线的 Z 分量最大为 1 ，说明碰撞平面在物体的正下方
        JumpCount = 0;
    }
}
```

1. 打开 `BP_RollABallPlayer` 蓝图
2. Collision - Mesh - Simulation Generates Hit Events: true
3. 编译保存

## 6 输出 DEBUG 信息的方法

```cpp
GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("HitDirection: %f"), HitDirection));
```

## 7 创建 Item Class

1. Tools - New C++ Class...
2. Add C++ Class 窗口 - 选择 Actor - Next
3. Name: RollABallItemBase
4. Path: `/Unreal Projects/RollABall/Source/RollABall/Items` （将与物品有关的类放在 Items 文件夹下）
5. 点击 Create Class
6. 忽略编译错误
7. 关闭 UE5 编辑器
8. 将 `RollABallItemBase.cpp` 中的 `#include "Items/RollABallItemBase.h"` 改为 `#include "RollABallItemBase.h"` 以解决路径错误
9. 按 <kbd>F5</kbd> 编译打开 UE5 编辑器
10. Tools - Refresh Visual Studio Project

在 `RollABallItemBase.h` 中声明静态网格体：

```cpp
// 声明 Components
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
UStaticMeshComponent *Mesh;
```

### 7.1 定义并绑定 OnBeginOverlap

在构造函数 `ARollABallItemBase::ARollABallItemBase` 中绑定回调函数

```cpp
Mesh->OnComponentBeginOverlap.AddDynamic(this, &ARollABallItemBase::OnBeginOverlap);
```

获取 `OnComponentBeginOverlap` 的类型定义：右键 `OnComponentBeginOverlap` - 转到类型定义

```cpp
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_SixParams( FComponentBeginOverlapSignature, UPrimitiveComponent, OnComponentBeginOverlap, UPrimitiveComponent*, OverlappedComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex, bool, bFromSweep, const FHitResult &, SweepResult);
```

在 `RollABallItemBase.h` 中声明回调函数

```cpp
// 声明回调函数
UFUNCTION()
void OnBeginOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);
```

在 `RollABallItemBase.cpp` 中定义回调函数

```cpp
#include "RollABall/Game/RollABallPlayer.h"

void ARollABallItemBase::OnBeginOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
    if (Cast<ARollABallPlayer>(OtherActor) != nullptr)
    {
        // Overlap 的 Actor 是 RollABallPlayer 类
        Collected(); // 调用蓝图函数
    }
}
```

### 7.2 定义蓝图函数 Collected

在 `RollABallItemBase.h` 中声明蓝图函数

```cpp
// 声明蓝图函数
UFUNCTION(BlueprintNativeEvent)
void Collected();
```

> Collected 函数使用蓝图实现

在 `RollABallItemBase.cpp` 中定义蓝图函数的 C++ 部分（要添加 `_Implementation` 后缀）

```cpp
void ARollABallItemBase::Collected_Implementation()
{
    // TODO: Do Game Mode Stuff
}
```

编译后 Implementation 版本的函数不会出现报错。

## 8 从 Item Class C++ 类创建蓝图

### 8.1 创建蓝图

1. 右键 `C++ Classes/RollABall/Items` 中的 `RollABallItemBase` C++ 类 - Create Blueprint class based on RollABallItemBase
2. Name: BP_ItemBase
3. Path 选择 `_RollABall/Blueprints`
4. 点击 Create Blueprint Class

### 8.2 设置蓝图

1. 选中 Mesh 组件 - Details 选项卡 - Collision - Collision Presets: OverlapAll
2. 选中 Mesh 组件 - Details 选项卡 - Static Mesh - Static Mesh - 下拉菜单 - 点击齿轮图标 - 勾选 Show Engine Content - 选择 SM_Cube_01
3. Components 选项卡 - `+ Add` - Rotating Movement - Details 选项卡 - Rotating Component - Rotation Rate: 100, 0, 100
4. My Blueprint 选项卡 - FUNCTIONS - 右侧 Override 下拉菜单 - 选择 Collected
5. 编译保存
6. 将 `BP_ItemBase` 蓝图拖入场景中

点击 Simulation 按钮 ![Simulation 按钮](/assets/img/RollABall/Simulation按钮.png) 可以预览旋转效果。

### 8.3 通过蓝图实现 Collected 函数

右键 `Event Collected` - Add Call to Parent Function  
该节点用于调用 C++ 中的 `ARollABallItemBase::Collected_Implementation` 函数。

![Collected 函数](/assets/img/RollABall/Collected函数.png)

## 9 创建 Game Mode Class

> Game Mode 记录当前关卡的状态。在 Roll A Ball 中，Game Mode 记录了关卡中的 Item 总数，以及玩家收集的 Item 数量。  
> 同时，Game Mode 还可以允许设置玩家 Pawn ，详见[下文](#102-设置蓝图)。

1. Tools - New C++ Class...
2. Add C++ Class 窗口 - 选择 Game Mode Base - Next
3. Name: RollABallGameModeBase
4. Path: `/Unreal Projects/RollABall/Source/RollABall/Game` （将与游戏有关的类放在 Game 文件夹下）
5. 点击 Create Class
6. 忽略编译错误
7. 关闭 UE5 编辑器
8. 将 `RollABallGameModeBase.cpp` 中的 `#include "Game/RollABallGameModeBase.h"` 改为 `#include "RollABallGameModeBase.h"` 以解决路径错误
9. 按 <kbd>F5</kbd> 编译打开 UE5 编辑器
10. Tools - Refresh Visual Studio Project

### 9.1 编辑 RollABallGameModeBase.h

声明变量和函数：

```cpp
protected:
    int32 ItemsCollected = 0;
    int32 ItemsInLevel = 0;

    // 在游戏开始时初始化关卡内的 Items 总数：ItemsInLevel
    virtual void BeginPlay() override;
    // 用于更新 UHD
    void UpdateItemText();

public:
    // 在 Item 被收集时调用，用于更新 ItemsCollected
    void ItemCollected();
```

### 9.2 编辑 RollABallGameModeBase.cpp

定义函数：

```cpp
void ARollABallGameModeBase::BeginPlay()
{
    TArray<AActor *> Items;                                                                      // 储存 Items 的数组
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARollABallItemBase::StaticClass(), Items); // 将 Level 中的所有 ARollABallItemBase 类型的 Actor 储存到 Items 数组中
    ItemsInLevel = Items.Num();                                                                  // 获取 Items 数组的长度
}

void ARollABallGameModeBase::ItemCollected()
{
    ++ItemsCollected;
}
```

## 10 从 Game Mode Class C++ 类创建蓝图

### 10.1 创建蓝图

1. 按 <kbd>F5</kbd> 编译打开 UE5 编辑器
2. 在 `_RollABall/Blueprints` 下新建 `Game` 文件夹
3. 右键 `C++ Classes/RollABall/Game` 中的 `RollABallGameModeBase` C++ 类 - Create Blueprint class based on RollABallGameModeBase
4. Name: BP_GameModeBase
5. Path 选择 `_RollABall/Blueprints/Game`
6. 点击 Create Blueprint Class
7. 编译保存

### 10.2 设置蓝图

1. 在 `BP_GameModeBase` 中 - Components 选项卡 - 选中根节点 (Self)
2. Details 选项卡 - Classes - Default Pawn Class: BP_RollABallPlayer
3. 编译保存

### 10.3 应用蓝图

1. Edit - Project Settings... - Maps & Modes - Default Modes - Default GameMode: BP_GameModeBase
2. 关闭 Project Settings 面板
3. 删除场景中的 `BP_RollABallPlayer`
4. Place Actors 选项卡 - Basic - 将 Player Start 拖入场景中
5. 选中 Player Start - Details 选项卡 - Transform - Location - 设置位置

> 在 Edit - Project Settings... - Maps & Modes - Default Modes - Selected GameMode - Default Pawn Class 中的设置会与 `BP_GameModeBase` 蓝图内的 Default Pawn Class 同步修改

## 11 创建 User Widget Class

1. Tools - New C++ Class...
2. Add C++ Class 窗口 - 选择 All Classes - 搜索并选中 UserWidget - Next
3. Name: RollABallWidget
4. Path: `/Unreal Projects/RollABall/Source/RollABall/UHD`
5. 点击 Create Class
6. 忽略编译错误
7. 关闭 UE5 编辑器
8. 将 `RollABallWidget.cpp` 中的 `#include "UHD/RollABallWidget.h"` 改为 `#include "RollABallWidget.h"` 以解决路径错误
9. 按 <kbd>F5</kbd> 编译打开 UE5 编辑器
10. Tools - Refresh Visual Studio Project

在 `RollABallWidget.h` 中声明函数：

```cpp
UFUNCTION(BlueprintImplementableEvent)
// 用于更新 UHD ；功能通过蓝图实现
void SetItemText(int32 ItemsCollected, int32 ItemsInLevel);
```

## 12 从 User Widget Class C++ 类创建蓝图

> 使用上文的[方法](#2-从-player-class-c-类创建蓝图)（同[这个方法](#8-从-item-class-c-类创建蓝图)和[这个方法](#10-从-game-mode-class-c-类创建蓝图)）创建 Widget 蓝图无法获得可视化编辑界面  
> 因此此处需要使用另一种方法创建继承 C++ 类的 Widget 蓝图

1. 在 `_RollABall/Blueprints` 中右键 - User Interface - Widget Blueprint
2. 在 Pick Parent Class for New Widget Blueprint 窗口中的 ALL CLASSES 下拉菜单中搜索并选中 RollABallWidget （[上一步](#11-创建-user-widget-class)创建的 C++ 类）
3. 点击 Select
4. 命名为 `WBP_RollABallWidget`
5. 双击进入编辑界面
6. 点击右上角的 Graph 按钮 ![Graph 按钮](/assets/img/RollABall/Graph按钮.png)
7. My Blueprint 选项卡 - FUNCTIONS - 右侧 Override 下拉菜单 - 选择 Set Item Text

### 12.1 设计 UHD

1. 点击右上角 Designer 按钮
2. 将 Palette 选项卡中的 Common - Text 拖入界面中
3. Details 选项卡
   * Is Variable: true
   * 变量名改为 `TextCollected`
   * Content - Text: Items Collected: 0 / 0
   * Appearance - Color and Opacity: 黑色
   * Appearance - Font - Size: 72
   * Appearance - Justification: Center
   * Appearance - Margin - Top: 100

### 12.2 编写蓝图函数 Set Item Text 修改变量 TextCollected

1. 点击右上角 Graph 按钮
2. 将 My Blueprint 选项卡中的 TextCollected 拖入 Graph 中 - 拖动输出端口 - 添加 SetText (Text) 节点
3. 右键空白区域 - 添加 Utilities/String 中的 Append 节点 - 点击 Add pin 添加到 4 个端口
4. 如图所示连接节点
   ![SetText](/assets/img/RollABall/SetText.png)
5. 编译保存

## 13 完善 Game Mode

### 方法一：通过蓝图实现（未验证）

在 `BP_GameModeBase` 的 Event Graph 中：  
Event BeginPlay 连接 Create Widget 节点；然后将 Create Widget 节点的返回值连接到 Add to Viewport 节点。

### 方法二：通过 C++ 实现

#### 修改代码

在 `RollABallGameModeBase.h` 中声明 Widget 变量：

```cpp
#include "RollABall/UHD/RollABallWidget.h"

// 用于创建 UHD
UPROPERTY(EditAnywhere, Category = "Widgets")
TSubclassOf<class UUserWidget> GameWidgetClass; // 暴露给虚幻编辑器选择 UHD 的接口
UPROPERTY()
URollABallWidget *GameWidget; // UHD 的实例
```

在 `RollABallGameModeBase.cpp` 中的 `ARollABallGameModeBase::BeginPlay` 中添加 Widget：

```cpp
if (GameWidgetClass)
{
    GameWidget = Cast<URollABallWidget>(CreateWidget(GetWorld(), GameWidgetClass)); // 创建 UHD
    if (GameWidget)
    {
        GameWidget->AddToViewport(); // 将 UHD 添加到视口
        UpdateItemText();            // 更新 UHD
    }
}
```

定义 `ARollABallGameModeBase::UpdateItemText` 函数：

```cpp
void ARollABallGameModeBase::UpdateItemText()
{
    if (GameWidget)
    {
        // RollABallWidget.h 中的 SetItemText() 函数通过蓝图实现
        GameWidget->SetItemText(ItemsCollected, ItemsInLevel); // 更新 UHD 的文本内容
    }
}
```

完善 `ARollABallGameModeBase::ItemCollected` 函数：

```cpp
void ARollABallGameModeBase::ItemCollected()
{
    ++ItemsCollected; // 计数
    UpdateItemText(); // 更新 UHD
}
```

#### 设置 Game Mode

1. 按 <kbd>F5</kbd> 编译打开 UE5 编辑器
2. 进入 `BP_GameModeBase` 蓝图 - Widgets - Game Widget Class: WBP_RollABallWidget
3. 编译保存

## 14 完善 Item Class

完成 `ARollABallItemBase::Collected_Implementation` 的定义：

```cpp
void ARollABallItemBase::Collected_Implementation()
{
    ARollABallGameModeBase *GameMode = Cast<ARollABallGameModeBase>(GetWorld()->GetAuthGameMode()); // 获得当前关卡的 Game Mode
    if (GameMode)
    {
        GameMode->ItemCollected(); // 调用 Game Mode 的 ItemCollected() 函数
    }
}
```

## Finished
