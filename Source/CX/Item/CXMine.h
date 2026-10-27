#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXMine.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;
class USoundBase;
class ACXPawn;

UCLASS()
class CX_API ACXMine : public AActor
{
	GENERATED_BODY()

public:
	ACXMine();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<UStaticMeshComponent> MineMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<UParticleSystem> ExplosionParticle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mine")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(ReplicatedUsing = OnRep_Exploded)
	bool bExploded = false;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_Exploded();

	void Explode(ACXPawn* Vehicle);
	bool bExplosionPlayed = false;
};
