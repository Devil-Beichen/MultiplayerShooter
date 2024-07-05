// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionsSubsystem.generated.h"

//
// 我们为自己的菜单定义委托，也绑定回调
// 

/**  声明动态 多人游戏创建会话创建 多播委托 一个参数
 * @param bWasSuccessful   是成功的
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);

/** 声明组播 多人游戏中找到会话完成 委托 两个参数
 * @param SessionResults	 	会话结果
 * @param bWasSuccessful	 	是成功的
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete,
                                     const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)

/**声明组播 多人游戏中加入会话完成 委托 一个参数
 * @param Result	 	结果
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result)

/**  声明动态 多人模式会话摧毁完成 多播委托 一个参数
 * @param bWasSuccessful   是成功的
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);

/**  声明动态 多人游戏开始会话完成 多播委托 一个参数
 * @param bWasSuccessful   是成功的
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);


/** 多人在线会话子系统
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public: //公共部分

	//构造函数
	UMultiplayerSessionsSubsystem();

	//
	// 处理会话功能， 菜单类将调用这些  
	//

	/** 创建会话
	 * @param NumPublicConnections	 	玩家数量
	 * @param MatchType				 	匹配类型
	 */
	void CreateSession(int32 NumPublicConnections, FString MatchType);

	/** 查找会话
	 * @param MaxSearchResults		 	最大的搜索结果
	 */
	void FindSessions(int32 MaxSearchResults);

	/** 加入会话
	 * @param SessionResult			 	会话结果
	 */
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);

	//销毁会话
	void DestroySession();

	//开始会话
	void StartSession();

	//
	//	自己定义的代理，为菜单类回调绑定自定义委托
	//

	//多人游戏创建会话完成
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	//多人游戏查找会话完成
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionsComplete;
	//多人游戏加入会话完成
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	//多人游戏摧毁会话完成
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	//多人游戏开始会话完成
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

protected: //受保护的部分

	//
	// 我们将添加在线会话接口委托列表中的内部回调
	// 这些不需要在这个类之外调用 
	//

	/** 创建会话完成
	 * @param SessionName		 	会话名称
	 * @param bWasSuccessful	 	是成功的
	 */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	/** 找到会话完成
	 * @param bWasSuccessful		是成功的
	 */
	void OnFindSessionsComplete(bool bWasSuccessful);

	/** 加入会话完成
	 * @param SessionName			会话名称
	 * @param Result				结果
	 */
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	/** 销毁会话完成
	 * @param SessionName		 	会话名称
	 * @param bWasSuccessful	 	是成功的
	 */
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	/** 开始会话完成
	 * @param SessionName		 	会话名称
	 * @param bWasSuccessful	 	是成功的
	 */
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private: //私有部分

	//在线会话接口指针
	IOnlineSessionPtr SessionInterface;

	//单个会话的所有设置
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	//最后一个会话搜索
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	//
	// 添加到网上会话接口委托列表
	// 我们将绑定多人游戏会话子系统，并对这些内部回调
	//

	//在线会话创建请求完成时的委托
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;

	//创建会话完成委托句柄。  
	FDelegateHandle CreateSessionCompleteDelegateHandle;

	//在线会话搜索完成时的委托
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;

	//查找会话完成委托句柄
	FDelegateHandle FindSessionCompleteDelegateHandle;

	//在线会话加入过程完成时的委托
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;

	//加入会话完成委托句柄
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	//在线会话被摧毁时的委托
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;

	//销毁会话完成委托句柄
	FDelegateHandle DestroySessionCompleteDelegateHandle;

	//在线会话变成开始的委托
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;

	//启动会话完成委托句柄
	FDelegateHandle StartSessionCompleteDelegateHandle;

	//创建了会话
	bool bCreateSessionOnDestroy{false};

	//上一个公共玩家数量
	int32 LastNumPublicConnections = 0;

	//上一个匹配类型
	FString LastMatchType = FString("None");
};
