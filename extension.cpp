/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include <amtl/am-string.h>
#include "extension.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

Outputinfo g_Outputinfo;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_Outputinfo);

IGameConfig *g_pGameConf = NULL;

#include <isaverestore.h>
#include <variant_t.h>

class CEventAction
{
public:
	CEventAction( const char *ActionData = NULL );

	string_t m_iTarget; // name of the entity(s) to cause the action in
	string_t m_iTargetInput; // the name of the action to fire
	string_t m_iParameter; // parameter to send, 0 if none
	float m_flDelay; // the number of seconds to wait before firing the action
	int m_nTimesToFire; // The number of times to fire this event, or EVENT_FIRE_ALWAYS (-1).

	int m_iIDStamp;	// unique identifier stamp

	static int s_iNextIDStamp;

	CEventAction *m_pNext;
	DECLARE_SIMPLE_DATADESC();

	static void (*s_pOperatorDeleteFunc)(void *pMem);
	static void operator delete(void *pMem);
};
void (*CEventAction::s_pOperatorDeleteFunc)(void *pMem);

void CEventAction::operator delete(void *pMem)
{
	s_pOperatorDeleteFunc(pMem);
}

class CBaseEntityOutput
{
public:
	variant_t m_Value;
	CEventAction *m_ActionList;
	DECLARE_SIMPLE_DATADESC();

	int NumberOfElements(void);
	CEventAction *GetElement(int Index);
	int DeleteElement(int Index);
	int DeleteAllElements(void);
};

int CBaseEntityOutput::NumberOfElements(void)
{
	int Count = 0;
	for(CEventAction *ev = m_ActionList; ev != NULL; ev = ev->m_pNext)
		Count++;

	return Count;
}

CEventAction *CBaseEntityOutput::GetElement(int Index)
{
	int Count = 0;
	for(CEventAction *ev = m_ActionList; ev != NULL; ev = ev->m_pNext)
	{
		if(Count == Index)
			return ev;

		Count++;
	}

	return NULL;
}

int CBaseEntityOutput::DeleteElement(int Index)
{
	CEventAction *pPrevEvent = NULL;
	CEventAction *pEvent = NULL;

	int Count = 0;
	for(CEventAction *ev = m_ActionList; ev != NULL; ev = ev->m_pNext)
	{
		if(Count == Index)
		{
			pEvent = ev;
			break;
		}
		pPrevEvent = ev;
		Count++;
	}

	if(pEvent == NULL)
		return 0;

	if(pPrevEvent != NULL)
		pPrevEvent->m_pNext = pEvent->m_pNext;
	else
		m_ActionList = pEvent->m_pNext;

	delete pEvent;
	return 1;
}

int CBaseEntityOutput::DeleteAllElements(void)
{
	// walk front to back, deleting as we go. We needn't fix up pointers because
	// EVERYTHING will die.

	int Count = 0;
	CEventAction *pNext = m_ActionList;
	// wipe out the head
	m_ActionList = NULL;
	while(pNext)
	{
		CEventAction *pStrikeThis = pNext;
		pNext = pNext->m_pNext;
		delete pStrikeThis;
		Count++;
	}

	return Count;
}

inline int GetDataMapOffset(CBaseEntity *pEnt, const char *pName)
{
	datamap_t *pMap = gamehelpers->GetDataMap(pEnt);
	if(!pMap)
		return -1;

	typedescription_t *pTypeDesc = gamehelpers->FindInDataMap(pMap, pName);

	if(pTypeDesc == NULL)
		return -1;

#if SOURCE_ENGINE >= SE_LEFT4DEAD
	return pTypeDesc->fieldOffset;
#else
	return pTypeDesc->fieldOffset[TD_OFFSET_NORMAL];
#endif
}

inline CBaseEntityOutput *GetOutput(CBaseEntity *pEntity, const char *pOutput)
{
	int Offset = GetDataMapOffset(pEntity, pOutput);

	if(Offset == -1)
		return NULL;

	return (CBaseEntityOutput *)((intptr_t)pEntity + Offset);
}

cell_t GetOutputCount(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL)
		return -1;

	return pEntityOutput->NumberOfElements();
}

cell_t GetOutputTarget(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->GetElement(params[3]);
	if(!pAction)
		return 0;

	size_t Length;
	pContext->StringToLocalUTF8(params[4], params[5], pAction->m_iTarget.ToCStr(), &Length);

	return Length;
}

cell_t GetOutputTargetInput(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->GetElement(params[3]);
	if(!pAction)
		return 0;

	size_t Length;
	pContext->StringToLocalUTF8(params[4], params[5], pAction->m_iTargetInput.ToCStr(), &Length);

	return Length;
}

cell_t GetOutputParameter(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->GetElement(params[3]);
	if(!pAction)
		return 0;

	size_t Length;
	pContext->StringToLocalUTF8(params[4], params[5], pAction->m_iParameter.ToCStr(), &Length);

	return Length;
}

cell_t GetOutputDelay(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return -1;

	CEventAction *pAction = pEntityOutput->GetElement(params[3]);
	if(!pAction)
		return -1;

	return *(cell_t *)&pAction->m_flDelay;
}

cell_t GetOutputFormatted(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pAction = pEntityOutput->GetElement(params[3]);
	if(!pAction)
		return 0;

	char aBuffer[1024];
	ke::SafeSprintf(aBuffer, sizeof(aBuffer), "%s,%s,%s,%g,%d",
		pAction->m_iTarget.ToCStr(),
		pAction->m_iTargetInput.ToCStr(),
		pAction->m_iParameter.ToCStr(),
		pAction->m_flDelay,
		pAction->m_nTimesToFire);

	size_t Length;
	pContext->StringToLocalUTF8(params[4], params[5], aBuffer, &Length);

	return Length;
}

cell_t FindOutput(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return -1;

	char *piTarget;
	pContext->LocalToStringNULL(params[4], &piTarget);
	char *piTargetInput;
	pContext->LocalToStringNULL(params[5], &piTargetInput);
	char *piParameter;
	pContext->LocalToStringNULL(params[6], &piParameter);
	float flDelay = *(float *)&params[7];
	cell_t nTimesToFire = params[8];

	int StartCount = params[3];
	int Count = 0;
	for(CEventAction *ev = pEntityOutput->m_ActionList; ev != NULL; ev = ev->m_pNext)
	{
		Count++;
		if(StartCount > 0)
		{
			StartCount--;
			continue;
		}

		if(piTarget != NULL && strcmp(ev->m_iTarget.ToCStr(), piTarget) != 0)
			continue;

		if(piTargetInput != NULL && strcmp(ev->m_iTargetInput.ToCStr(), piTargetInput) != 0)
			continue;

		if(piParameter != NULL && strcmp(ev->m_iParameter.ToCStr(), piParameter) != 0)
			continue;

		if(flDelay >= 0 && flDelay != ev->m_flDelay)
			continue;

		if(nTimesToFire != 0 && nTimesToFire != ev->m_nTimesToFire)
			continue;

		return Count - 1;
	}

	return -1;
}

cell_t DeleteOutput(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return -1;

	return pEntityOutput->DeleteElement(params[3]);
}

cell_t DeleteAllOutputs(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(gamehelpers->IndexToReference(params[1]));
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return -1;

	return pEntityOutput->DeleteAllElements();
}


const sp_nativeinfo_t MyNatives[] =
{
	{ "GetOutputCount", GetOutputCount },
	{ "GetOutputTarget", GetOutputTarget },
	{ "GetOutputTargetInput", GetOutputTargetInput },
	{ "GetOutputParameter", GetOutputParameter },
	{ "GetOutputDelay", GetOutputDelay },
	{ "GetOutputFormatted", GetOutputFormatted },
	{ "FindOutput", FindOutput },
	{ "DeleteOutput", DeleteOutput },
	{ "DeleteAllOutputs", DeleteAllOutputs },
	{ NULL, NULL },
};

bool Outputinfo::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
	char conf_error[255] = "";
	if(!gameconfs->LoadGameConfigFile("outputinfo", &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if(conf_error[0])
		{
			snprintf(error, maxlen, "Could not read outputinfo.txt: %s\n", conf_error);
		}
		return false;
	}

	if(!g_pGameConf->GetMemSig("CEventAction__operator_delete", (void **)(&CEventAction::s_pOperatorDeleteFunc)) || !CEventAction::s_pOperatorDeleteFunc)
	{
		snprintf(error, maxlen, "Failed to find CEventAction__operator_delete function.\n");
		return false;
	}

	return true;
}

void Outputinfo::SDK_OnUnload()
{
	gameconfs->CloseGameConfigFile(g_pGameConf);
}

void Outputinfo::SDK_OnAllLoaded()
{
	sharesys->AddNatives(myself, MyNatives);
}
