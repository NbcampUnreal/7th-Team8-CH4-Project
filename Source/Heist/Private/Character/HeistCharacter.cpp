#include "Character/HeistCharacter.h"

#include "Components/HeistPawnExtensionComponent.h"
#include "Components/HeistPlayerComponent.h"
#include "AbilitySystem/HeistAbilitySystemComponent.h"
#include "Data/HeistPawnData.h"
#include "Input/HeistInputComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AHeistCharacter::AHeistCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// 컨트롤러 회전 미사용 — PlayerComponent에서 커서 방향으로 직접 처리
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;

	AbilitySystemComponent = CreateDefaultSubobject<UHeistAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	PawnExtensionComponent = CreateDefaultSubobject<UHeistPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PlayerComponent        = CreateDefaultSubobject<UHeistPlayerComponent>(TEXT("PlayerComponent"));

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
	InitializeGameplayAbilitySystem();
}

void AHeistCharacter::UnPossessed()
{
	Super::UnPossessed();
	PawnExtensionComponent->UninitializeAbilitySystem();
}

void AHeistCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeGameplayAbilitySystem();
}

void AHeistCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// UHeistInputComponent가 설정되어 있어야 입력이 동작한다.
	// 프로젝트 세팅 > Default Classes > Input Component Class = UHeistInputComponent
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
	// TODO: PlayerState로 ASC 이동 시 Owner = PlayerState, Avatar = this 로 변경
	PawnExtensionComponent->InitializeAbilitySystem(AbilitySystemComponent, this);
}
