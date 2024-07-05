// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"

//菜单设置
void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	AddToViewport(); //添加到窗口
	SetVisibility(ESlateVisibility::Visible); //设置默认可见
	SetIsFocusable(true); //将此标志设置为true，允许此小部件在单击或导航到时接受焦点


	//获取世界并判度
	if (const UWorld* World = GetWorld())
	{
		//获取第一个玩家控制器
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			// 用于设置只允许UI响应用户输入的输入模式的数据结构  
			FInputModeUIOnly InputModeUIOnly;
			//聚焦于小部件
			InputModeUIOnly.SetWidgetToFocus(TakeWidget());
			//不锁定鼠标光标到视口
			InputModeUIOnly.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			//设置输入模式
			PlayerController->SetInputMode(InputModeUIOnly);
			//设置显示鼠标光标
			PlayerController->SetShowMouseCursor(true);
		}
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		//设置指定的子系统
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubsystem)
	{
		//在线会话子系统 动态多播绑定 创建会话
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		//在线会话子系统 查找会话 添加一个基于Uobject的成员函数委托
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		//在线会话子系统 加入会话 添加一个基于Uobject的成员函数委托
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
		//在线会话子系统 动态多播绑定 删除会话
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.
		                              AddDynamic(this, &ThisClass::OnDestroySession);
		//在线会话子系统 动态多播绑定 开始会话
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

//初始化
bool UMenu::Initialize()
{
	if (!Super::Initialize())
		return false;
	//主机按钮有效，绑定点击函数
	if (HostButton)
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	//加入按钮有效，绑定加入函数
	if (JoinButton)
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	//退出按钮有效，绑定退出函数
	if (QuitButton)
		QuitButton->OnClicked.AddDynamic(this, &ThisClass::QuitButtonClicked);

	return true;
}

//从关卡中移除
void UMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

//创建会话
void UMenu::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				1,
				15.f,
				FColor::Green,
				FString::Printf(TEXT("会话创建成功"))
			);
		}
		if (UWorld* World = GetWorld())
		{
			//服务器的旅行
			World->ServerTravel(PathToLobby);
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				1,
				15.f,
				FColor::Red,
				FString::Printf(TEXT("会话创建失败！！！"))
			);
		}
		HostButton->SetIsEnabled(true);
	}
}

//查找会话
void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	if (MultiplayerSessionsSubsystem == nullptr)
	{
		return;
	}

	//判断是否查找成功或者查找到的会话的结果是空的
	if (!bWasSuccessful || SessionResults.Num() == 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				1,
				15.f,
				FColor::Red,
				FString::Printf(TEXT("会话查找失败！！！"))
			);
		}
		JoinButton->SetIsEnabled(true);
	}

	for (auto Result : SessionResults)
	{
		const FString ID = Result.GetSessionIdStr(); //会话结果的ID
		const FString User = Result.Session.OwningUserName; //会话拥有者的名字
		//设置值
		FString SettingsValue;
		Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
		if (SettingsValue == MatchType) //查找到的与匹配类型同一的话
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Cyan,
					FString::Printf(TEXT("查找到 ID: %s, 名字： %s, 创建的会话"), *ID, *User)
				);
			}
			//加入会话
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(
				1,
				2.f,
				FColor::Red,
				FString::Printf(TEXT("未能找到对应的匹配类型！！！"))
			);
		}
	}
}

//加入会话
void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	//联机子系统获取
	if (const IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		if (const IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface()) //设置会话接口指针
		{
			FString Address;
			//解析连接字符串
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			//获取游戏实例的第一个控制器
			if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						-1,
						15.f,
						FColor::Yellow,
						FString::Printf(TEXT("正在连接地址： %s"), *Address)
					);
				}
				//进入绝对URL IP地址的地图
				PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
			}
		}
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
		JoinButton->SetIsEnabled(true);
}

//摧毁会话
void UMenu::OnDestroySession(bool bWasSuccessful)
{
}

//开始会话
void UMenu::OnStartSession(bool bWasSuccessful)
{
}

//主机按钮点击
void UMenu::HostButtonClicked()
{
	HostButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		//通过子系统创建会话
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

//加入按钮点击
void UMenu::JoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(10000); //多人子系统是否有效，有效就开始查找
	}
}

//退出游戏
void UMenu::QuitButtonClicked()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

//菜单移除
void UMenu::MenuTearDown()
{
	//移除视口
	RemoveFromParent();
	//获取世界并判度
	if (const UWorld* World = GetWorld())
	{
		//获取第一个玩家控制器
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			//用于设置只允许玩家输入/玩家控制器响应用户输入的输入模式的数据结构  
			FInputModeGameOnly InputModeGameData;
			//设置输入模式
			PlayerController->SetInputMode(InputModeGameData);
			//设置隐藏鼠标光标
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
