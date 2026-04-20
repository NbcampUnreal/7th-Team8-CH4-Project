#include "HeistLoadingScreenSubsystem.h"

#include "HeistLoadingScreenWidget.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"

void UHeistLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UHeistLoadingScreenSubsystem::OnPreLoadMap);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UHeistLoadingScreenSubsystem::OnPostLoadMapWithWorld);

	UHeistMessageSubsystem& MessageSubsystem = *GetGameInstance()->GetSubsystem<UHeistMessageSubsystem>();

	SeamlessStartHandle = MessageSubsystem.RegisterListener<FHeistTravelSeamlessStartMessage>(
		HeistMessageTags::Message_Travel_SeamlessStart,
		[this](FGameplayTag Channel, const FHeistTravelSeamlessStartMessage& Msg)
		{
			OnSeamlessStart(Channel, Msg);
		});

	SeamlessEndHandle = MessageSubsystem.RegisterListener<FHeistTravelSeamlessEndMessage>(
		HeistMessageTags::Message_Travel_SeamlessEnd,
		[this](FGameplayTag Channel, const FHeistTravelSeamlessEndMessage& Msg)
		{
			OnSeamlessEnd(Channel, Msg);
		});
}

void UHeistLoadingScreenSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	SeamlessStartHandle.Unregister();
	SeamlessEndHandle.Unregister();

	HideLoadingScreen();

	Super::Deinitialize();
}

void UHeistLoadingScreenSubsystem::OnPreLoadMap(const FString& MapName)
{
	if (!IsValid(GetGameInstance()->GetWorld())) return;

	ShowLoadingScreen(false);
}

void UHeistLoadingScreenSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (IsTransitionMap(LoadedWorld)) return;

	TriggerFillAnimation();
}

void UHeistLoadingScreenSubsystem::OnSeamlessStart(FGameplayTag Channel, const FHeistTravelSeamlessStartMessage& Msg)
{
	ShowLoadingScreen(true);
}

void UHeistLoadingScreenSubsystem::OnSeamlessEnd(FGameplayTag Channel, const FHeistTravelSeamlessEndMessage& Msg)
{
	TriggerFillAnimation();
}

void UHeistLoadingScreenSubsystem::ShowLoadingScreen(bool bIsSeamless)
{
	if (IsValid(LoadingScreenInstance)) return;

	if (LoadingScreenClass.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoadingScreen] LoadingScreenClass is null. DefaultGame.ini 경로를 확인하세요."));
		return;
	}

	UClass* WidgetClass = LoadingScreenClass.LoadSynchronous();
	if (WidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LoadingScreen] WidgetClass 로드 실패. 에셋 경로를 확인하세요."));
		return;
	}

	// UGameInstance는 레벨 전환에 걸쳐 유지되므로 PlayerController 대신 사용.
	UGameInstance* GameInstance = GetGameInstance();
	LoadingScreenInstance = CreateWidget<UHeistLoadingScreenWidget>(GameInstance, WidgetClass);
	if (!IsValid(LoadingScreenInstance)) return;

	LoadingScreenInstance->AddToViewport(LoadingScreenZOrder);
	LoadingScreenInstance->OnLoadingStarted(bIsSeamless);

	bFillAnimationStarted = false;
}

void UHeistLoadingScreenSubsystem::TriggerFillAnimation()
{
	if (!IsValid(LoadingScreenInstance)) return;
	if (bFillAnimationStarted) return;

	bFillAnimationStarted = true;

	LoadingScreenInstance->OnFillComplete.AddUniqueDynamic(this, &UHeistLoadingScreenSubsystem::HideLoadingScreen);
	LoadingScreenInstance->StartFillAnimation();

	UWorld* World = GetGameInstance()->GetWorld();
	if (!IsValid(World)) return;

	World->GetTimerManager().SetTimer(
		FallbackTimerHandle,
		this,
		&UHeistLoadingScreenSubsystem::HideLoadingScreen,
		FallbackTimeout,
		false);
}

void UHeistLoadingScreenSubsystem::HideLoadingScreen()
{
	if (!IsValid(LoadingScreenInstance)) return;

	UWorld* World = GetGameInstance()->GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(FallbackTimerHandle);
	}

	LoadingScreenInstance->OnFillComplete.RemoveDynamic(this, &UHeistLoadingScreenSubsystem::HideLoadingScreen);
	LoadingScreenInstance->RemoveFromParent();
	LoadingScreenInstance = nullptr;
	bFillAnimationStarted = false;
}

bool UHeistLoadingScreenSubsystem::IsTransitionMap(UWorld* World) const
{
	if (!IsValid(World)) return false;

	return World->GetMapName().Contains(TEXT("L_Transition"));
}
