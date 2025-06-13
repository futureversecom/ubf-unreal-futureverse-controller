// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.


#include "Sylo/FutureverseSyloIntegrationSubsystem.h"

#include "FuturepassSubsystem.h"
#include "SyloSubsystem.h"
#include "Sylo/FuturepassSyloAccessSource.h"
#include "Sylo/FutureverseSyloIntegrationSettings.h"

void UFutureverseSyloIntegrationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency(UFuturepassSubsystem::StaticClass());

	if (UFuturepassSubsystem* FuturepassSubsystem = GetGameInstance()->GetSubsystem<UFuturepassSubsystem>())
	{
		FuturepassSubsystem->OnFuturepassLoginComplete.AddUniqueDynamic(this, &ThisClass::OnLoginComplete);
		FuturepassSubsystem->OnFuturepassLogout.AddUniqueDynamic(this, &ThisClass::OnLogout);
	}
}

void UFutureverseSyloIntegrationSubsystem::Deinitialize()
{
	if (UFuturepassSubsystem* FuturepassSubsystem = GetGameInstance()->GetSubsystem<UFuturepassSubsystem>())
	{
		FuturepassSubsystem->OnFuturepassLoginComplete.RemoveDynamic(this, &ThisClass::OnLoginComplete);
		FuturepassSubsystem->OnFuturepassLogout.RemoveDynamic(this, &ThisClass::OnLogout);
	}
	
	Super::Deinitialize();
}

void UFutureverseSyloIntegrationSubsystem::OverrideCurrentSource(UFuturepassUser* User)
{
	if (CurrentAccessSource.IsValid() && User == CurrentAccessSource->GetTargetUser()) return;

	SetUserAsSource(User);
}

UFuturepassUser* UFutureverseSyloIntegrationSubsystem::GetCurrentUserSource() const
{
	return CurrentAccessSource.IsValid() ? CurrentAccessSource->GetTargetUser() : nullptr;
}

void UFutureverseSyloIntegrationSubsystem::FindAndSetBestUserAsSource()
{
	if (UFuturepassSubsystem* FuturepassSubsystem = GetGameInstance()->GetSubsystem<UFuturepassSubsystem>())
	{
		TArray<UFuturepassUser*> Users;
		FuturepassSubsystem->GetAllValidUsers(Users);

		for (auto User : Users)
		{
			// We have no conditions for being the 'best user' yet so just set the first
			SetUserAsSource(User);
			break;
		}
	}
}

void UFutureverseSyloIntegrationSubsystem::OnLoginComplete(UFuturepassUser* User)
{
	if (!GetDefault<UFutureverseSyloIntegrationSettings>()->bAutomaticallyHandleSyloAccessSource) return;
	
	if (HasSyloAccessSource()) return;

	FindAndSetBestUserAsSource();
}

void UFutureverseSyloIntegrationSubsystem::OnLogout(UFuturepassUser* User)
{
	if (!GetDefault<UFutureverseSyloIntegrationSettings>()->bAutomaticallyHandleSyloAccessSource) return;

	if (!CurrentAccessSource.IsValid()) return;

	if (User == CurrentAccessSource->GetTargetUser())
	{
		ClearCurrentAccessSource();
		FindAndSetBestUserAsSource();
	}
}

void UFutureverseSyloIntegrationSubsystem::SetUserAsSource(UFuturepassUser* User)
{
	ClearCurrentAccessSource();
	
	CurrentAccessSource = MakeShared<FFuturepassSyloAccessSource>(User);

	if (USyloSubsystem* SyloSubsystem = GetGameInstance()->GetSubsystem<USyloSubsystem>())
	{
		for (const FString& TargetSyloResolverID : GetDefault<UFutureverseSyloIntegrationSettings>()->TargetSyloResolverIDs)
		{
			SyloSubsystem->SetSyloAccessSource(TargetSyloResolverID, CurrentAccessSource);
		}
	}
}

void UFutureverseSyloIntegrationSubsystem::ClearCurrentAccessSource()
{
	if (!CurrentAccessSource.IsValid()) return;
	
	if (USyloSubsystem* SyloSubsystem = GetGameInstance()->GetSubsystem<USyloSubsystem>())
	{
		for (auto TargetSyloResolverID : GetDefault<UFutureverseSyloIntegrationSettings>()->TargetSyloResolverIDs)
		{
			SyloSubsystem->SetSyloAccessSource(TargetSyloResolverID, nullptr);
		}
	}
	
	CurrentAccessSource = nullptr;
}

bool UFutureverseSyloIntegrationSubsystem::HasSyloAccessSource() const
{
	return CurrentAccessSource.IsValid() && CurrentAccessSource->IsTargetUserIsValid();
}
