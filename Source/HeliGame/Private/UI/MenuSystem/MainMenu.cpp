// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "MainMenu.h"
#include "HostMenu.h"
#include "FindServersMenu.h"
#include "OptionsMenu.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"

UMainMenu::UMainMenu(const FObjectInitializer & ObjectInitializer)
{
}

bool UMainMenu::Initialize()
{
	bool Success = Super::Initialize();
	if (!Success) return false;

	if (!ensure(HostButton != nullptr)) return false;
	HostButton->OnClicked.AddDynamic(this, &UMainMenu::OpenHostMenu);

	if (!ensure(ServersButton != nullptr)) return false;
	ServersButton->OnClicked.AddDynamic(this, &UMainMenu::OpenServersMenu);

	if (!ensure(OptionsButton != nullptr)) return false;
	OptionsButton->OnClicked.AddDynamic(this, &UMainMenu::OpenOptionsMenu);

	if (!ensure(AboutButton != nullptr)) return false;
	AboutButton->OnClicked.AddDynamic(this, &UMainMenu::OpenAboutMenu);

	if (!ensure(QuitButton != nullptr)) return false;
	QuitButton->OnClicked.AddDynamic(this, &UMainMenu::QuitGame);

	if (!ensure(BackToMainMenuButton != nullptr)) return false;
	BackToMainMenuButton->OnClicked.AddDynamic(this, &UMainMenu::OpenMainMenu);
	BackToMainMenuButton->SetVisibility(ESlateVisibility::Hidden);

	if (!ensure(HostMenu != nullptr)) return false;

	return true;
}

void UMainMenu::OpenMainMenu()
{
	if (!ensure(MenuSwitcher != nullptr)) return;
	if (!ensure(MainMenu != nullptr)) return;
	
	BackToMainMenuButton->SetVisibility(ESlateVisibility::Hidden);
	MenuSwitcher->SetActiveWidget(MainMenu);
	QuitButton->SetVisibility(ESlateVisibility::Visible);
}

void UMainMenu::OpenHostMenu()
{
	if (!ensure(MenuSwitcher != nullptr)) return;
	if (!ensure(HostMenu != nullptr)) return;
	HostMenu->SetMenuInterface(this->GetMenuInterface());
	MenuSwitcher->SetActiveWidget(HostMenu);

	BackToMainMenuButton->SetVisibility(ESlateVisibility::Visible);
	QuitButton->SetVisibility(ESlateVisibility::Hidden);
}

void UMainMenu::OpenServersMenu()
{
	if (!ensure(MenuSwitcher != nullptr)) return;
	if (!ensure(FindServersMenu != nullptr)) return;
	
	FindServersMenu->SetMenuInterface(this->GetMenuInterface());
	MenuSwitcher->SetActiveWidget(FindServersMenu);

	BackToMainMenuButton->SetVisibility(ESlateVisibility::Visible);
	QuitButton->SetVisibility(ESlateVisibility::Hidden);
}

void UMainMenu::OpenOptionsMenu()
{
	if (!ensure(MenuSwitcher != nullptr)) return;
	if (!ensure(OptionsMenu != nullptr)) return;

	MenuSwitcher->SetActiveWidget(OptionsMenu);

	BackToMainMenuButton->SetVisibility(ESlateVisibility::Visible);
	QuitButton->SetVisibility(ESlateVisibility::Hidden);
}

void UMainMenu::OpenAboutMenu()
{
	if (!ensure(MenuSwitcher != nullptr)) return;
	if (!ensure(AboutMenu != nullptr)) return;
	MenuSwitcher->SetActiveWidget(AboutMenu);

	BackToMainMenuButton->SetVisibility(ESlateVisibility::Visible);
	QuitButton->SetVisibility(ESlateVisibility::Hidden);
}

void UMainMenu::QuitGame()
{
	UWorld* World = GetWorld();
	if (!ensure(World != nullptr)) return;

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!ensure(PlayerController != nullptr)) return;

	PlayerController->ConsoleCommand("Quit");
}

void UMainMenu::SetAvailableServerList(TArray<FServerData> AvailableServersData)
{
	if (!ensure(FindServersMenu != nullptr)) return;
	FindServersMenu->SetAvailableServerList(AvailableServersData);
}