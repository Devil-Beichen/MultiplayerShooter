// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

/** 菜单
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public: //公共部分

	/** 菜单设置
	 * @param NumberOfPublicConnections		 	玩家数量
	 * @param TypeOfMatch					 	匹配类型
	 * @param LobbyPath							大厅地址
	 */
	UFUNCTION(BlueprintCallable, meta=(Keywords = "菜单设置", DisplayName = "菜单设置"), Category="会话相关")
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForALL")),FString LobbyPath = FString(TEXT("/Game/ThirdPerson/Maps/Lobby")));

protected: //受保护的部分

	//初始化
	virtual bool Initialize() override;

	//从关卡中移除
	virtual void NativeDestruct() override;

	//
	//多人在线会话子系统上自定义委托的回调 
	//

	/** 创建会话
	 * @param bWasSuccessful	 	是成功的
	 */
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);

	//查找会话
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);

	//加入会话
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);

	//摧毁会话
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

	//开始会话
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

private: //私有部分

	//创建会话按钮
	UPROPERTY(meta=(BindWidget))
	class UButton* HostButton;

	//加入会话按钮
	UPROPERTY(meta=(BindWidget))
	UButton* JoinButton;

	//退出游戏按钮
	UPROPERTY(meta=(BindWidget))
	UButton* QuitButton;

	//创建会话
	UFUNCTION()
	void HostButtonClicked();

	//加入会话
	UFUNCTION()
	void JoinButtonClicked();

	//退出游戏
	UFUNCTION()
	void QuitButtonClicked();

	//菜单移除
	void MenuTearDown();

	//处理所有在线会话功能的子系统
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	//玩家数量
	int32 NumPublicConnections{4};

	//匹配类型
	FString MatchType{TEXT("FreeForALL")};

	//大厅地址
	FString PathToLobby{TEXT("")};
};
