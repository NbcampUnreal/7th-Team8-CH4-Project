#include "Character/HeistCharacter.h"

#include "Components/HeistPawnExtensionComponent.h"
#include "Components/HeistPlayerComponent.h"
#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Core/HeistPlayerState.h"
#include "Data/HeistPawnData.h"
#include "Input/HeistInputComponent.h"
#include "Character/HeistTags_State.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/HeistHitReactionComponent.h"
#include "Core/HeistMatchGameState.h"
#include "GameFramework/CharacterMovementComponent.h"

AHeistCharacter::AHeistCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// 커서 방향 회전은 CMC가 보간 처리 — UpdateCursorRotation()이 SetControlRotation()으로 목표 Yaw를 세팅하면
	// CMC가 RotationRate 속도로 보간하여 메시를 회전시킨다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	PawnExtensionComponent = CreateDefaultSubobject<UHeistPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PlayerComponent        = CreateDefaultSubobject<UHeistPlayerComponent>(TEXT("PlayerComponent"));
	InteractionComponent   = CreateDefaultSubobject<UHeistInteractionComponent>(TEXT("InteractionComponent"));

	// 탑다운 카메라
	CameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	CameraSpringArm->SetupAttachment(RootComponent);
	CameraSpringArm->TargetArmLength = 800.0f;
	CameraSpringArm->SetRelativeRotation(FRotator(-65.0f, 0.0f, 0.0f));
	CameraSpringArm->bInheritPitch    = false;
	CameraSpringArm->bInheritYaw      = false;
	CameraSpringArm->bInheritRoll     = false;
	CameraSpringArm->bDoCollisionTest = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraSpringArm, USpringArmComponent::SocketName);

	HitReactionComponent = CreateDefaultSubobject<UHeistHitReactionComponent>(TEXT("HitReactionComponent"));
}

UAbilitySystemComponent* AHeistCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UHeistAbilitySystemComponent* AHeistCharacter::GetHeistAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AHeistCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(DefaultPawnData))
	{
		PawnExtensionComponent->SetPawnData(DefaultPawnData);
	}
}

void AHeistCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	UE_LOG(LogTemp, Log, TEXT("[PawnReady] PossessedBy: Pawn=%s Controller=%s"),
		*GetNameSafe(this),
		*GetNameSafe(NewController));
	InitializeGameplayAbilitySystem();
	RefreshClientPawnReadyState();
}

void AHeistCharacter::UnPossessed()
{
	Super::UnPossessed();
	PawnExtensionComponent->UninitializeAbilitySystem();
}

void AHeistCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	UE_LOG(LogTemp, Log, TEXT("[PawnReady] OnRep_PlayerState: Pawn=%s PS=%s"),
		*GetNameSafe(this),
		*GetNameSafe(GetPlayerState()));
	InitializeGameplayAbilitySystem();
	RefreshClientPawnReadyState();
}

void AHeistCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	UE_LOG(LogTemp, Log, TEXT("[PawnReady] OnRep_Controller: Pawn=%s Controller=%s"),
		*GetNameSafe(this),
		*GetNameSafe(GetController()));
	InitializeGameplayAbilitySystem();
	RefreshClientPawnReadyState();
}

void AHeistCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	ensureMsgf(IsValid(Cast<UHeistInputComponent>(PlayerInputComponent)),
		TEXT("UHeistInputComponent가 필요합니다. 프로젝트 세팅에서 Input Component Class를 설정하세요."));

	UHeistPlayerComponent* PlayerComp = UHeistPlayerComponent::FindPlayerComponent(this);
	if (IsValid(PlayerComp))
	{
		PlayerComp->OnPawnInputComponentReady(PlayerInputComponent);
	}
}

void AHeistCharacter::InitializeGameplayAbilitySystem()
{
	AHeistPlayerState* HeistPS = GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS)) return;

	UHeistAbilitySystemComponent* NewASC = HeistPS->GetHeistAbilitySystemComponent();
	if (!IsValid(NewASC)) return;

	if (AbilitySystemComponent == NewASC) return;

	AbilitySystemComponent = NewASC;

	// Owner = PlayerState, Avatar = Character
	PawnExtensionComponent->InitializeAbilitySystem(AbilitySystemComponent, HeistPS);

	OnPlayerStateInitialized.Broadcast(HeistPS);
}

bool AHeistCharacter::IsInMatchPhaseContext() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const AHeistMatchGameState* MatchGameState = World->GetGameState<AHeistMatchGameState>();
	if (!IsValid(MatchGameState))
	{
		return false;
	}

	return MatchGameState->IsBriefingPhase() || MatchGameState->IsExecutionPhase();
}

void AHeistCharacter::RefreshClientPawnReadyState()
{
	if (!IsInMatchPhaseContext())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[PawnReady] Skip: not in match phase Pawn=%s"), *GetNameSafe(this));
		return;
	}

	AHeistPlayerState* HeistPS = GetPlayerState<AHeistPlayerState>();
	if (!IsValid(HeistPS))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[PawnReady] Defer: PlayerState invalid Pawn=%s"), *GetNameSafe(this));
		return;
	}

	AController* OwnerController = GetController();
	if (!IsValid(OwnerController))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[PawnReady] Defer: Controller invalid Pawn=%s"), *GetNameSafe(this));
		return;
	}

	// ASC/입력 수렴은 PossessedBy / OnRep_PlayerState / OnRep_Controller 에서 phase와 무관하게 먼저 시도한다.
	// 여기서는 그 이후의 매치 전용 후처리(예: transparency trigger 재평가)만 담당한다.
	UE_LOG(LogTemp, Log, TEXT("[PawnReady] Ready: Pawn=%s PS=%s Controller=%s Team=%d"),
		*GetNameSafe(this),
		*HeistPS->GetName(),
		*OwnerController->GetName(),
		static_cast<int32>(HeistPS->GetAssignedTeam()));

	if (IsValid(PlayerComponent))
	{
		PlayerComponent->NotifyOverlappingTransparencyTriggers();
	}
}
