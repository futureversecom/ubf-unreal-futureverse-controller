// Copyright (c) 2025, Futureverse Corporation Limited. All rights reserved.

#pragma once
#include "Dynamic.h"
#include "UBFAPIControllerLog.h"

class FContextData : public TSharedFromThis<FContextData>
{
public:
	FContextData(const FString& BlueprintID, const TMap<FString, UBF::FDynamicHandle>& Traits) : BlueprintID(BlueprintID), Traits(Traits) {}

	FString GetBlueprintID() const {return BlueprintID;}
	const TMap<FString, TSharedPtr<FContextData>>& GetChildren() const {return Children;}
	const TMap<FString, UBF::FDynamicHandle>& GetTraits() const {return Traits;}

	FContextData* AddChild(const TSharedPtr<FContextData>& Child, const FString& Relationship)
	{
		if (!ensure(Child != nullptr)) return this;
		
		if (Children.Contains(Relationship))
		{
			UE_LOG(LogUBFAPIController, Warning, TEXT("Relationship '%s' already exists between Parent (ID: %s) and Child (ID: %s)."), 
				*Relationship, *GetBlueprintID(), *Child->GetBlueprintID());
			return this;
		}
		
		Children.Add(Relationship, Child);
		
		return this;
	}

	FContextData* AddParent(const TSharedPtr<FContextData>& Parent, const FString& Relationship)
	{
		if (!ensure(Parent != nullptr)) return this;
		return Parent->AddChild(SharedThis(this), Relationship);
	}

	void AddTraits(const TMap<FString, UBF::FDynamicHandle>& NewTraits)
	{
		Traits.Append(NewTraits);
	}

private:
	FString BlueprintID;
	TMap<FString, TSharedPtr<FContextData>> Children;
	TMap<FString, UBF::FDynamicHandle> Traits;
};

class FContextTree
{
public:
	void SetRoot(const TSharedPtr<FContextData>& Data)
	{
		Marker = Data;
	}

	TSharedPtr<FContextData> AddItem(const FString& BlueprintID,
		const TMap<FString, UBF::FDynamicHandle>& Traits = TMap<FString, UBF::FDynamicHandle>(), 
		const TSharedPtr<FContextData>& Parent = nullptr, 
		const FString& RelationshipName = FString())
	{
		TSharedPtr<FContextData> NewData = MakeShared<FContextData>(BlueprintID, Traits);

		AllContextData.Add(NewData);

		if (Parent.IsValid())
		{
			NewData->AddParent(Parent, RelationshipName);
		}

		return NewData;
	}
	
	bool TryMoveMarker(const FString& Relationship, TSharedPtr<FContextData>& NewMarker)
	{
		if (!Marker.IsValid()) return false;
		
		TSharedPtr<FContextData> Current = Marker;
		
		if (Current->GetChildren().Contains(Relationship))
		{
			Marker = Current->GetChildren()[Relationship];
			NewMarker = Marker;
			return true;
		}

		while (Current.IsValid())
		{
			TSharedPtr<FContextData>* Parent = AllContextData.FindByPredicate([Current](const TSharedPtr<FContextData>& Context)
			{
				// Check if any of the children have a value equal to Current
				return Algo::FindByPredicate(Context->GetChildren(), [Current](const TPair<FString, TSharedPtr<FContextData>>& Pair)
				{
					return Pair.Value == Current;
				}) != nullptr;
			});
			
			if (Parent == nullptr)
			{
				NewMarker = Marker;
				return false;
			}

			if ((*Parent)->GetChildren().Contains(Relationship))
			{
				Marker = (*Parent)->GetChildren()[Relationship];
				NewMarker = Marker;
				return true;
			}

			Current = *Parent;
		}

		NewMarker = nullptr;
		return false;
	}

private:
	TArray<TSharedPtr<FContextData>> AllContextData;
	TSharedPtr<FContextData> Marker;
};
