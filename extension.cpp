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

#include "extension.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

CGlobalVars *gpGlobals = NULL;

Outputinfo g_Outputinfo;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_Outputinfo);

#include <isaverestore.h>
#include <variant_t.h>

#define EVENT_FIRE_ALWAYS	-1

class CEventAction
{
public:
	CEventAction( const char *ActionData = NULL );

	string_t m_iTarget; // name of the entity(s) to cause the action in
	string_t m_iTargetInput; // the name of the action to fire
	string_t m_iParameter; // parameter to send, 0 if none
	float m_flDelay; // the number of seconds to wait before firing the action
	int m_nTimesToFire; // The number of times to fire this event, or EVENT_FIRE_ALWAYS.

	int m_iIDStamp;	// unique identifier stamp

	static int s_iNextIDStamp;

	CEventAction *m_pNext;
/*
	// allocates memory from engine.MPool/g_EntityListPool
	static void *operator new( size_t stAllocateBlock );
	static void *operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine );
	static void operator delete( void *pMem );
	static void operator delete( void *pMem , int nBlockUse, const char *pFileName, int nLine ) { operator delete(pMem); }
*/
	DECLARE_SIMPLE_DATADESC();

};

class CBaseEntityOutput
{
public:

	~CBaseEntityOutput();

	void ParseEventAction( const char *EventData );
	void AddEventAction( CEventAction *pEventAction );

	int Save( ISave &save );
	int Restore( IRestore &restore, int elementCount );

	int NumberOfElements( void );

	float GetMaxDelay( void );

	fieldtype_t ValueFieldType() { return m_Value.FieldType(); }

	void FireOutput( variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0 );
/*
	/// Delete every single action in the action list.
	void DeleteAllElements( void ) ;
*/
public:
	variant_t m_Value;
	CEventAction *m_ActionList;
	DECLARE_SIMPLE_DATADESC();

	CBaseEntityOutput() {} // this class cannot be created, only it's children

private:
	CBaseEntityOutput( CBaseEntityOutput& ); // protect from accidental copying
};

BEGIN_SIMPLE_DATADESC( CEventAction )
	DEFINE_FIELD( m_iTarget, FIELD_STRING ),
	DEFINE_FIELD( m_iTargetInput, FIELD_STRING ),
	DEFINE_FIELD( m_iParameter, FIELD_STRING ),
	DEFINE_FIELD( m_flDelay, FIELD_FLOAT ),
	DEFINE_FIELD( m_nTimesToFire, FIELD_INTEGER ),
	DEFINE_FIELD( m_iIDStamp, FIELD_INTEGER ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( CBaseEntityOutput )
	//DEFINE_CUSTOM_FIELD( m_Value, variantFuncs ),
END_DATADESC()

int CBaseEntityOutput::NumberOfElements(void)
{
	int count = 0;

	if (m_ActionList == NULL)
		return -1;

	for (CEventAction *ev = m_ActionList; ev != NULL; ev = ev->m_pNext)
		count++;

	return (count);
}

inline CBaseEntity *GetCBaseEntity(int Num)
{
#if SOURCE_ENGINE >= SE_LEFT4DEAD
	edict_t *pEdict = (edict_t *)(gpGlobals->pEdicts + Num);
#else
	edict_t *pEdict = engine->PEntityOfEntIndex(Num);
#endif
	if(!pEdict || pEdict->IsFree())
		return NULL;

	IServerUnknown *pUnk;
	if((pUnk = pEdict->GetUnknown()) == NULL)
		return NULL;

	return pUnk->GetBaseEntity();
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

	CBaseEntity *pEntity = GetCBaseEntity(params[1]);
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL)
		return 0;

	return pEntityOutput->NumberOfElements();
}

cell_t GetOutputTarget(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = GetCBaseEntity(params[1]);
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if(pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pActionList = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if(pActionList->m_pNext == NULL)
			return 0;

		pActionList = pActionList->m_pNext;
	}

	int Len = strlen(pActionList->m_iTarget.ToCStr());

	pContext->StringToLocal(params[4], Len + 1, pActionList->m_iTarget.ToCStr());

	return Len;
}

cell_t GetOutputTargetInput(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = GetCBaseEntity(params[1]);
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pActionList = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if(pActionList->m_pNext == NULL)
			return 0;

		pActionList = pActionList->m_pNext;
	}

	int Len = strlen(pActionList->m_iTargetInput.ToCStr());

	pContext->StringToLocal(params[4], Len + 1, pActionList->m_iTargetInput.ToCStr());

	return Len;
}

cell_t GetOutputParameter(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = GetCBaseEntity(params[1]);
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return 0;

	CEventAction *pActionList = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if(pActionList->m_pNext == NULL)
			return 0;

		pActionList = pActionList->m_pNext;
	}

	int Len = strlen(pActionList->m_iParameter.ToCStr());

	pContext->StringToLocal(params[4], Len + 1, pActionList->m_iParameter.ToCStr());

	return Len;
}

cell_t GetOutputDelay(IPluginContext *pContext, const cell_t *params)
{
	char *pOutput;
	pContext->LocalToString(params[2], &pOutput);

	CBaseEntity *pEntity = GetCBaseEntity(params[1]);
	CBaseEntityOutput *pEntityOutput = GetOutput(pEntity, pOutput);

	if (pEntityOutput == NULL || pEntityOutput->m_ActionList == NULL)
		return -1;

	CEventAction *pActionList = pEntityOutput->m_ActionList;
	for(int i = 0; i < params[3]; i++)
	{
		if(pActionList->m_pNext == NULL)
			return -1;

		pActionList = pActionList->m_pNext;
	}

	return *(cell_t *)&pActionList->m_flDelay;
}


const sp_nativeinfo_t MyNatives[] =
{
	{ "GetOutputCount", GetOutputCount },
	{ "GetOutputTarget", GetOutputTarget },
	{ "GetOutputTargetInput", GetOutputTargetInput },
	{ "GetOutputParameter", GetOutputParameter },
	{ "GetOutputDelay", GetOutputDelay },
	{ NULL, NULL },
};

void Outputinfo::SDK_OnAllLoaded()
{
	sharesys->AddNatives(myself, MyNatives);
}

bool Outputinfo::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late)
{
	gpGlobals = ismm->GetCGlobals();
	return true;
}
