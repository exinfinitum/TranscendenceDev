//	CCTransDataLibrary.cpp
//
//	Functions and placeholders defined by TransData
//	Copyright (c) 2018 Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"

static constexpr DWORD FN_TSE_PARSE_INTEGER_RANGE =		1;
static constexpr DWORD FN_TSE_PATTERN =					2;
static constexpr DWORD FN_TSE_SET_SYSTEM =				3;
static constexpr DWORD FN_TSE_UPDATE_SYSTEM =			4;
static constexpr DWORD FN_TSE_AFFINITY_CRITERIA =		5;
static constexpr DWORD FN_TSE_SET_PLAYER_SHIP =			6;
static constexpr DWORD FN_TSE_REGEN_DESC =				7;

ICCItem *fnNil (CEvalContext *pEvalCtx, ICCItem *pArgs, DWORD dwData);
ICCItem *fnTransEngine (CEvalContext *pEvalCtx, ICCItem *pArgs, DWORD dwData);

static PRIMITIVEPROCDEF g_Library[] =
	{
		//	The following are placeholder functions that are normally 
		//	implemented in Transcendence (above TSE).

		{	"scrTranslate",					fnNil,		0,
			"PLACEHOLDER",
			"iv*",	0,	},

		//	These are engine diagnostic functions

		{	"diagSetPlayerShip",			fnTransEngine,		FN_TSE_SET_PLAYER_SHIP,
			"(diagSetPlayerShip obj) -> True/error",
			"i",	PPFLAG_SIDEEFFECTS,	},

		{	"diagSetSystem",				fnTransEngine,		FN_TSE_SET_SYSTEM,
			"(diagSetSystem nodeID) -> True/error",
			"s",	PPFLAG_SIDEEFFECTS,	},

		{	"diagUpdateSystem",				fnTransEngine,		FN_TSE_UPDATE_SYSTEM,
			"(diagUpdateSystem [updates]) -> True/error",
			"*",	PPFLAG_SIDEEFFECTS,	},

		{	"tseAffinityCriteria",			fnTransEngine,		FN_TSE_AFFINITY_CRITERIA,
			"(tseAffinityCriteria criteria attribs [freq]) -> result",
			"ss*",	0,	},

		{	"tseParseIntegerRange",			fnTransEngine,		FN_TSE_PARSE_INTEGER_RANGE,
			"(tseParseIntegerRange criteria) -> result",
			"s",	0,	},

		{	"tsePattern",					fnTransEngine,		FN_TSE_PATTERN,
			"(tsePattern pattern ...) -> string",
			"s*",	0,	},

		{	"tseRegenDesc",					fnTransEngine,		FN_TSE_REGEN_DESC,
			"(tseRegenDesc regen [ticksPerCycle]) -> desc of hp to regen",
			"n*",	0,	},
	};

static constexpr int FUNCTION_COUNT	=	((sizeof g_Library) / (sizeof g_Library[0]));

void GetCCTransDataLibrary (SPrimitiveDefTable *retpTable)

//	GetCCTransDataLibrary
//
//	Registers extensions

	{
	retpTable->pTable = g_Library;
	retpTable->iCount = FUNCTION_COUNT;
	}

ICCItem *fnNil (CEvalContext *pEvalCtx, ICCItem *pArgs, DWORD dwData)

//	fnNil
//
//	Always returns Nil

	{
	CCodeChain *pCC = pEvalCtx->pCC;
	return pCC->CreateNil();
	}

ICCItem *fnTransEngine (CEvalContext *pEvalCtx, ICCItem *pArgs, DWORD dwData)

//	fnTransEngine

	{
	CCodeChain *pCC = pEvalCtx->pCC;
	CCodeChainCtx *pCtx = (CCodeChainCtx *)pEvalCtx->pExternalCtx;
	CUniverse &Universe = pCtx->GetUniverse();

	switch (dwData)
		{
		case FN_TSE_AFFINITY_CRITERIA:
			{
			CString sError;
			CAffinityCriteria Criteria;
			if (Criteria.Parse(pArgs->GetElement(0)->GetStringValue(), &sError) != NOERROR)
				return pCC->CreateError(sError, pArgs->GetElement(0));

			CString sAttribList = pArgs->GetElement(1)->GetStringValue();
			int iFreq = (pArgs->GetCount() > 2 ? pArgs->GetElement(2)->GetIntegerValue() : -1);

			std::function<bool(const CString &)> fnHasAttrib = [sAttribList](const CString &sAttrib) { return ::HasModifier(sAttribList, sAttrib); };
			std::function<int(const CString &)> fnGetFreq = NULL;
			if (iFreq != -1)
				fnGetFreq = [iFreq](const CString &sAttrib) { return iFreq; };

			ICCItemPtr pResult(ICCItem::SymbolTable);
			pResult->SetStringAt(CONSTLIT("criteria"), Criteria.AsString());

			bool bMatch = Criteria.Matches(fnHasAttrib);
			pResult->SetBooleanAt(CONSTLIT("match"), bMatch);

			int iWeight = Criteria.CalcWeight(fnHasAttrib, NULL, fnGetFreq);
			pResult->SetIntegerAt(CONSTLIT("weight"), iWeight);

			return pResult->Reference();
			}

		case FN_TSE_PARSE_INTEGER_RANGE:
			{
			CIntegerRangeCriteria Criteria;
			CString sCriteria = pArgs->GetElement(0)->GetStringValue();

			const char *pPos = sCriteria.GetASCIIZPointer();
			char chModifier;
			if (!Criteria.Parse(pPos, &pPos, &chModifier))
				return pCC->CreateError(CONSTLIT("Invalid criteria"), pArgs->GetElement(0));

			if (Criteria.IsEmpty())
				return pCC->CreateNil();

			ICCItemPtr pResult(ICCItem::SymbolTable);
			if (Criteria.GetEqualToValue() != -1)
				pResult->SetIntegerAt(CONSTLIT("equalTo"), Criteria.GetEqualToValue());

			if (Criteria.GetGreaterThanValue() != -1)
				pResult->SetIntegerAt(CONSTLIT("greaterThan"), Criteria.GetGreaterThanValue());

			if (Criteria.GetLessThanValue() != -1)
				pResult->SetIntegerAt(CONSTLIT("lessThan"), Criteria.GetLessThanValue());

			if (chModifier != '\0')
				pResult->SetStringAt(CONSTLIT("modifier"), CString(&chModifier, 1));

			return pResult->Reference();
			}

		case FN_TSE_PATTERN:
			{
			static constexpr int MAX_ARG_BLOCK_SIZE = 4000;

			CString sPattern = pArgs->GetElement(0)->GetStringValue();

			//	Generate a block holding pointers to actual arguments. We also
			//	keep a list of strings that we allocated.

			TArray<CString> Strings;
			Strings.InsertEmpty(pArgs->GetCount());
			CMemoryWriteStream ArgBlock(MAX_ARG_BLOCK_SIZE);
			if (ArgBlock.Create() != NOERROR)
				return pCC->CreateNil();

			//	Add all arguments.

			for (int i = 1; i < pArgs->GetCount(); i++)
				{
				ICCItem *pArg = pArgs->GetElement(i);
				switch (pArg->GetValueType())
					{
					case ICCItem::String:
						{
						CString *pSrcString = &Strings[i];
						*pSrcString = pArg->GetStringValue();
						ArgBlock.Write((char *)pSrcString, sizeof(CString));
						break;
						}

					case ICCItem::Integer:
						{
						int iSrcInt = pArg->GetIntegerValue();
						ArgBlock.Write((char *)&iSrcInt, sizeof(int));
						break;
						}

					case ICCItem::Double:
						{
						double rSrcDouble = pArg->GetDoubleValue();
						ArgBlock.Write((char *)&rSrcDouble, sizeof(double));
						break;
						}

					default:
						return pCC->CreateError(CONSTLIT("Unsupported type"), pArg);
					}
				}

			//	Execute

			CString sResult;
			try
				{
				sResult = strPattern(sPattern, (LPVOID *)ArgBlock.GetPointer());
				}
			catch (...)
				{
				return pCC->CreateError(CONSTLIT("Crash in strPattern"));
				}

			return pCC->CreateString(sResult);
			}

		case FN_TSE_SET_PLAYER_SHIP:
			{
			CSpaceObject *pObj = CreateObjFromItem(pArgs->GetElement(0));
			if (pObj)
				{
				//	Setting this will make the object return IsPlayer() -> true.
				//	This will clear gPlayerShip when the object is destroyed.

				pObj->SetAISettingString(CONSTLIT("isPlayer"), CONSTLIT("true"));

				Universe.SetPlayerShip(pObj);
				return pCC->CreateTrue();
				}
			else
				{
				Universe.SetPlayerShip(NULL);
				return pCC->CreateNil();
				}
			}

		case FN_TSE_REGEN_DESC:
			{
			constexpr int TOTAL_TICKS = 360;
			constexpr int TICKS180 = 180;
			Metric rRegen = pArgs->GetElement(0)->GetDoubleValue();
			int iTicksPerCycle = (pArgs->GetCount() > 1 ? pArgs->GetElement(1)->GetIntegerValue() : 1);
			CRegenDesc Regen;
			Regen.InitFromRegen(rRegen, iTicksPerCycle);

			ICCItemPtr pHPList(ICCItem::List);
			int iResultCount = TOTAL_TICKS / iTicksPerCycle;
			int iTick = 0;
			int iTotalHP = 0;
			for (int i = 0; i < iResultCount; i++)
				{
				int iHP = Regen.GetRegen(iTick, iTicksPerCycle);
				pHPList->Append(ICCItemPtr(iHP));

				iTick += iTicksPerCycle;
				iTotalHP += iHP;
				}

			ICCItemPtr pResult(ICCItem::SymbolTable);
			pResult->SetIntegerAt(CONSTLIT("actualRegen"), mathRound((double)iTotalHP / (TOTAL_TICKS / (double)TICKS180)));
			pResult->SetDoubleAt(CONSTLIT("regen"), Regen.GetHPPer180(iTicksPerCycle));

			pResult->SetIntegerAt(CONSTLIT("cyclesPerBurst"), Regen.GetCyclesPerBurst());
			pResult->SetIntegerAt(CONSTLIT("hpPerCycle"), Regen.GetHPPerCycle());
			pResult->SetIntegerAt(CONSTLIT("hpPerEraRemainder"), Regen.GetHPPerEraRemainder());
			pResult->SetAt(CONSTLIT("hp"), pHPList);

			return pResult->Reference();
			}

		case FN_TSE_SET_SYSTEM:
			{
			CString sNodeID = pArgs->GetElement(0)->GetStringValue();
			CTopologyNode *pNode = Universe.FindTopologyNode(sNodeID);
			if (pNode == NULL)
				return pCC->CreateError("Unknown nodeID", pArgs->GetElement(0));

			//	If system is not loaded, then we fail.

			CSystem *pSystem = pNode->GetSystem();
			if (pSystem == NULL)
				return pCC->CreateError("System not loaded", pArgs->GetElement(0));

			//	Set as current system.

			Universe.SetNewSystem(*pSystem);
			return pCC->CreateTrue();
			}

		case FN_TSE_UPDATE_SYSTEM:
			{
			int iUpdates;
			if (pArgs->GetCount() > 0)
				iUpdates = pArgs->GetElement(0)->GetIntegerValue();
			else
				iUpdates = 1;

			if (iUpdates <= 0)
				return pCC->CreateNil();

			SSystemUpdateCtx UpdateCtx;
			UpdateCtx.bForceEventFiring = true;
			UpdateCtx.bForcePainted = true;

			for (int i = 0; i < iUpdates; i++)
				{
				Universe.Update(UpdateCtx);
				}

			return pCC->CreateTrue();
			}

		default:
			ASSERT(false);
			return pCC->CreateNil();
		}
	}
