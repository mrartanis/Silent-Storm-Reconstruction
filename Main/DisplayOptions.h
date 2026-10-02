#pragma once
namespace NGScene {
void BeginDisplayChange();
void ConfirmDisplayChange();
void RevertDisplayChange();
void PollDisplayChange();
int DisplayChangeSeconds();
}
