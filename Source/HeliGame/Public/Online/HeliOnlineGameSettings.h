// Copyright 2017 Andrey Bicalho Santos. All Rights Reserved.

#include "OnlineSessionSettings.h"

#pragma once

/**
 * General session settings for a Shooter game
 */
class FHeliOnlineSessionSettings : public FOnlineSessionSettings
{
public:

	FHeliOnlineSessionSettings(bool bIsLAN = false, bool bIsPresence = false, int32 MaxNumPlayers = 10);

	virtual ~FHeliOnlineSessionSettings() {}

	void SetMaxNumPlayers(int32 NewMaxNumPlayers);
};

/**
 * General search setting for a Shooter game
 */
class FHeliOnlineSearchSettings : public FOnlineSessionSearch
{
public:
	FHeliOnlineSearchSettings(bool bSearchingLAN = true, bool bSearchingPresence = true);

	virtual ~FHeliOnlineSearchSettings() {}
};

/**
 * Search settings for an empty dedicated server to host a match
 */
class FHeliOnlineSearchSettingsEmptyDedicated : public FHeliOnlineSearchSettings
{
public:
	FHeliOnlineSearchSettingsEmptyDedicated(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FHeliOnlineSearchSettingsEmptyDedicated() {}
};