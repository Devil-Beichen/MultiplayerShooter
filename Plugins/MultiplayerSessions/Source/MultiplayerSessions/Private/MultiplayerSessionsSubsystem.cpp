// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"

//构造函数
UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	CreateSessionCompleteDelegate(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)), //创建基于UObject的函数委托
	FindSessionsCompleteDelegate(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(
		FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))

{
	//联机子系统获取
	if (const IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		SessionInterface = Subsystem->GetSessionInterface(); //设置会话接口指针
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Green,
				FString::Printf(TEXT("发现 %s 的子系统"), *Subsystem->GetSubsystemName().ToString())
			);
		}
	}
}

//创建会话
void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1,
			15.f,
			FColor::Yellow,
			FString::Printf(TEXT("开始创建"))
		);
	}

	//获得指定会话名称
	const auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	//如果指定的会话不为空指针就删除
	if (ExistingSession != nullptr)
	{
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;
		DestroySession();
	}

	//将创建会话委托句柄存储起来，这样我们就可以从委托列表中删除它  （会话请求完成时，将触发委托函数）
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		CreateSessionCompleteDelegate);
	//设置单个会话的所有设置
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	//设置游戏是联网模式
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	//设置玩家数量
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	//允许加入正在运行的游戏
	LastSessionSettings->bAllowJoinInProgress = true;
	//允许通过玩家的身份加入
	LastSessionSettings->bAllowJoinViaPresence = true;
	// 该匹配在服务上公开
	LastSessionSettings->bShouldAdvertise = true;
	//显示用户信息状态
	LastSessionSettings->bUsesPresence = true;
	//如果平台支持可以搜索 Lobby API
	LastSessionSettings->bUseLobbiesIfAvailable = true;
	//设置关键字，值和在线广告
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	//防止不同版本在搜索中看到
	LastSessionSettings->BuildUniqueId = 1;

	//通过控制器获取本地第一个有效的玩家
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	//根据指定设置创建在线会话
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession,
	                                     *LastSessionSettings))
	{
		//创建会话失败清除委托
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		//广播自定义的委托
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}

//查找会话
void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1,
			15.f,
			FColor::Yellow,
			FString::Printf(TEXT("开始查找"))
		);
	}
	//查找会话完成委托句柄
	FindSessionCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FindSessionsCompleteDelegate);

	//会话搜索初始化
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	//设置最大查询次数
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	//是否查询LAN匹配
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	//查找匹配的服务器
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	//通过控制器获取本地第一个有效的玩家
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	//查询符合设置的会话
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		//查询会话失败清除委托
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionCompleteDelegateHandle);
		//查询会话失败广播自定义 多人游戏中找到会话 代理
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

//加入会话
void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString::Printf(TEXT("加入会话失败！！！发生了未知错误"))
			);
		}

		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1,
			15.f,
			FColor::Yellow,
			FString::Printf(TEXT("正在尝试加入会话..."))
		);
	}

	//设置加入会话完成委托句柄
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		JoinSessionCompleteDelegate);

	//通过控制器获取本地第一个有效的玩家
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	//加入指定对话
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		//加入会话失败清除委托
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		//加入会话失败 广播自定义的问题并告知未知错误
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString::Printf(TEXT("加入会话失败！！！"))
			);
		}
	}
}

//销毁会话
void UMultiplayerSessionsSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		//广播多人模式会话摧毁完成
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	//摧毁会话委托句柄
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		DestroySessionCompleteDelegate);

	//删除指定的对话
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		//清除委托
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		//广播多人模式会话摧毁
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}

//开始会话
void UMultiplayerSessionsSubsystem::StartSession()
{
}

//创建会话完成
void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		//创建会话完成清除委托
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	//广播自定义的委托
	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

//找到会话完成
void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface)
	{
		//查询会话成功清除委托
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionCompleteDelegateHandle);
	}
	//当没有搜索到会话数组时
	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		//查询会话失败广播自定义 多人游戏中找到会话 代理
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}
	//查询会话成功广播自定义 多人游戏中找到会话 代理
	MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

//加入会话完成
void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface)
	{
		//加入会话成功清除委托
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1,
			15.f,
			FColor::Green,
			FString::Printf(TEXT("加入 %s 的会话成功"), *SessionName.ToString())
		);
	}

	//加入会话成功 广播自定义的问题并告知未知错误
	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

//销毁会话完成
void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;

		//创建会话
		CreateSession(LastNumPublicConnections, LastMatchType);
	}
	//广播多人模式会话摧毁
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

//开始会话完成
void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}
