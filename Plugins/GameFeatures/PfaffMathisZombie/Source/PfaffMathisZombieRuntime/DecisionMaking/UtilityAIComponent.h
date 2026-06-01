#pragma once

#include "UtilityAction.h"
#include <memory>
#include <vector>

class ASurvivorPawn;

class UtilityAIComponent
{
public:
	template<typename T>
	T* AddAction(std::unique_ptr<T> Action)
	{
		T* Raw = Action.get();
		Actions.push_back(std::move(Action));
		return Raw;
	}

	void Update(ASurvivorPawn& Agent, float DeltaTime)
	{
		IUtilityAction* Best      = nullptr;
		float           BestScore = -1.f;

		for (auto& Action : Actions)
		{
			float S = Action->Score();
			if (S > BestScore)
			{
				BestScore = S;
				Best      = Action.get();
			}
		}

		if (Best && Best != CurrentAction)
		{
			if (CurrentAction) CurrentAction->OnExit(Agent);
			Best->OnEnter(Agent);
			CurrentAction = Best;
		}

		if (CurrentAction)
			CurrentAction->Execute(Agent, DeltaTime);
	}

	// Returns the active steering behavior so the controller can call CalculateSteering
	ISteeringBehavior* GetActiveBehavior() const
	{
		return CurrentAction ? CurrentAction->GetActiveBehavior() : nullptr;
	}

	const char* GetCurrentActionName() const
	{
		return CurrentAction ? CurrentAction->Name.c_str() : "None";
	}

private:
	std::vector<std::unique_ptr<IUtilityAction>> Actions;
	IUtilityAction* CurrentAction = nullptr;
};