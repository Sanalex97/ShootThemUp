// Shoot ThemUp Game. All Rights Reserved


#include "Weapon/STURifleWeapon.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Weapon/Components/STUWeaponFXComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

ASTURifleWeapon::ASTURifleWeapon()
{
    WeaponFXComponent = CreateDefaultSubobject<USTUWeaponFXComponent>("WeaponFXComponent");
}

void ASTURifleWeapon::BeginPlay()
{
    Super::BeginPlay();
    check(WeaponFXComponent);
}

void ASTURifleWeapon::StartFire()
{
    InitMuzzleFX();
    GetWorldTimerManager().SetTimer(ShotTimerHandle, this, &ASTURifleWeapon::MakeShot, TimeBetweenShots, true);
    MakeShot();
}


void ASTURifleWeapon::StopFire()
{
    GetWorldTimerManager().ClearTimer(ShotTimerHandle);
    SetMuzzleFXVisibility(false);
}

void ASTURifleWeapon::MakeShot()
{
    if (!GetWorld() || IsAmmoEmpty())
    {
        StopFire();
        return;
    }

    FVector TraceStart;
    FVector TraceEnd;
    if (!GetTraceData(TraceStart, TraceEnd))
    {
        StopFire();
        return;
    }

    FHitResult HitResult; // переменная для определения всей информации по пересечениям
    MakeHit(HitResult, TraceStart, TraceEnd);
    FVector TraceFXEnd = TraceEnd;
    if (HitResult.bBlockingHit)
    {
        TraceFXEnd = HitResult.ImpactPoint;
        MakeDamage(HitResult);
        // DrawDebugLine(GetWorld(), GetMuzzleWorldLocation(), HitResult.ImpactPoint, FColor::Red, false, 3.0f, 0, 3.0f);
        // DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 10.0f, 24, FColor::Red, false, 5.0f);
        WeaponFXComponent->PlayImpactFX(HitResult);

    }
    SpawnTraceFX(GetMuzzleWorldLocation(), TraceFXEnd);
    DecreaseAmmo();
}

bool ASTURifleWeapon::GetTraceData(FVector& TraceStart, FVector& TraceEnd) const
{
    FVector ViewLocation;
    FRotator ViewRotation;
    if (!GetPlayerViewPoint(ViewLocation, ViewRotation)) return false;

    TraceStart = ViewLocation; //SocketTransform.GetLocation();                        // точка начала выстрела
    const auto HalfRad = FMath::DegreesToRadians(BulletSpread); //перевод в радианы половины угла конуса
    const FVector ShootDirection = FMath::VRandCone(ViewRotation.Vector(), HalfRad);
    //SocketTransform.GetRotation().GetForwardVector(); // направление стрельбы c отклонением по конусу
    TraceEnd = TraceStart + ShootDirection * TraceMaxDistance;
    return true;
}

void ASTURifleWeapon::MakeDamage(const FHitResult& HitResult)
{
    const auto DamageActor = HitResult.GetActor();
    if (!DamageActor) return;

    DamageActor->TakeDamage(DamageAmount, FDamageEvent{}, GetPlayerController(), this);

}

void ASTURifleWeapon::InitMuzzleFX()
{
    if (!MuzzleFxComponent)
    {
        MuzzleFxComponent = SpawnMuzzleFX();
    }

    SetMuzzleFXVisibility(true);
}

void ASTURifleWeapon::SetMuzzleFXVisibility(bool Visible)
{
    if (MuzzleFxComponent)
    {
        MuzzleFxComponent->SetPaused(!Visible);
        MuzzleFxComponent->SetVisibility(Visible, true);
    }
}

void ASTURifleWeapon::SpawnTraceFX(const FVector& TraceStart, const FVector& TraceEnd)
{
    const auto TraceFxComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TraceFX, TraceStart);
    if (TraceFxComponent)
    {
        TraceFxComponent->SetNiagaraVariableVec3(TraceTargetName, TraceEnd);
    }
}
