//	CMultiverseModel.cpp
//
//	CMultiverseModel Class
//	Copyright (c) 2012 by Kronosaur Productions, LLC. All Rights Reserved.
//
//	USAGE
//
//	This object keeps track of our state with respect to the online Multiverse.
//	The CHexarcService object does the actual communication, but this object is
//	responsible for caching state (such as the user's collection).
//
//	This object is thread-safe; it may be called from either the UI or the
//	background task (e.g., CHexarcService).
//
//	1.	Call OnUserSignedIn and OnUserSignedOut in response to Hexarc
//		notifications. [OK to call multiple times--call with the current
//		state.]
//
//	2.	To load a collection follow this pattern:
//
//		i.		Call IsLoadCollectionNeeded to figure out if Multiverse wants
//					to load. If not, then the collection is valid (though it may
//					be empty). Call GetCollection.
//		ii.		To load, call OnCollectionLoading
//		iii.	If successful, call SetCollection
//		iv.		If failed, call OnCollectionLoadFailed
//		v.		This resets after a new sign-in (we require reloading the
//				collection.

#include "PreComp.h"

#define FIELD_DOWNLOAD_URL						CONSTLIT("downloadURL")
#define FIELD_FILE_VERSION						CONSTLIT("fileVersion")
#define FIELD_TYPE								CONSTLIT("type")
#define FIELD_UNID								CONSTLIT("unid")

#define TYPE_GAME_ENGINE						CONSTLIT("gameEngine")

static constexpr int ENTRY_ICON_HEIGHT =		150;
static constexpr int ENTRY_ICON_WIDTH =			300;

CMultiverseModel::CMultiverseModel (void) :
		m_fUserSignedIn(false),
		m_fCollectionLoaded(false),
		m_fDisabled(false),
		m_fLoadingCollection(false),
		m_fNewsLoaded(false)

//	CMultiverseModel constructor

	{
	}

void CMultiverseModel::AddResources (const CMultiverseCatalogEntry &Entry)

//	AddResources
//
//	Adds resources from the given entry to the m_Resources array.

	{
	int i;

	for (i = 0; i < Entry.GetResourceCount(); i++)
		{
		const CMultiverseFileRef &FileRef = Entry.GetResourceRef(i);

		SResourceDesc *pNewDesc = m_Resources.SetAt(FileRef.GetOriginalFilename());
		pNewDesc->sFilename = FileRef.GetOriginalFilename();
		pNewDesc->sFilePath = FileRef.GetFilePath();

		pNewDesc->pEntry = &Entry;
		pNewDesc->iIndex = i;
		}
	}

void CMultiverseModel::DeleteCollection (void)

//	DeleteCollection
//
//	Deletes the collection structure (after this we need to reload the 
//	collection from the service).
//
//	We assume that our caller has locked.

	{
	m_Collection.DeleteAll();
	m_Resources.DeleteAll();
	m_fCollectionLoaded = false;
	}

bool CMultiverseModel::FindEntry (DWORD dwUNID, CMultiverseCatalogEntry *retEntry) const

//	FindEntry
//
//	Finds the given entry.

	{
	CSmartLock Lock(m_cs);

	//	Look for the first entry in the catalog with the given UNID

	for (int i = 0; i < m_Collection.GetCount(); i++)
		{
		if (m_Collection.GetEntry(i)->GetUNID() == dwUNID)
			{
			if (retEntry)
				*retEntry = *m_Collection.GetEntry(i);

			return true;
			}
		}

	return false;
	}

CMultiverseCatalogEntry *CMultiverseModel::FindEntryActual (DWORD dwUNID) const

//	FindEntryActual
//
//	Returns a pointer to the given catalog entry (or NULL). This must be called
//	from inside a lock.

	{
	for (int i = 0; i < m_Collection.GetCount(); i++)
		{
		if (m_Collection.GetEntry(i)->GetUNID() == dwUNID)
			return m_Collection.GetEntry(i);
		}

	return NULL;
	}

TArray<CMultiverseCatalogEntry> CMultiverseModel::GetCollection (void) const

//	GetCollection
//
//	Returns the current list of catalog entries.

	{
	CSmartLock Lock(m_cs);

	//	Make a copy of the current collection

	TArray<CMultiverseCatalogEntry> Collection;
	Collection.InsertEmpty(m_Collection.GetCount());
	for (int i = 0; i < m_Collection.GetCount(); i++)
		Collection[i] = *m_Collection.GetEntry(i);

	//	Done

	return Collection;
	}

CMultiverseNewsEntry *CMultiverseModel::GetNextNewsEntry (void)

//	GetNextNewsEntry
//
//	Returns the next news entry to display.
//
//	NOTE: We return a copy which the callers are responsible for freeing.
	
	{
	CSmartLock Lock(m_cs);
	int i;

	//	First make a list of all available news entries

	TArray<CMultiverseNewsEntry *> Available;
	for (i = 0; i < m_News.GetCount(); i++)
		{
		CMultiverseNewsEntry *pEntry = m_News.GetEntry(i);

		//	If we've already shown this entry, skip it.

		if (pEntry->IsShown())
			continue;

		//	If this entry does not match the collection criteria, then skip it.

		if (m_Collection.HasAnyUNID(pEntry->GetExcludedUNIDs()))
			continue;

		if (!m_Collection.HasAllUNIDs(pEntry->GetRequiredUNIDs()))
			continue;

		Available.Insert(pEntry);
		}

	//	If none available, nothing

	if (Available.GetCount() == 0)
		return NULL;

	//	Pick a random entry

	CMultiverseNewsEntry *pEntry = Available[mathRandom(0, Available.GetCount() - 1)];

	//	Mark the entry as having been shown

	m_News.ShowNews(pEntry);

	//	return this entry

	return new CMultiverseNewsEntry(*pEntry);
	}

CMultiverseModel::EOnlineStates CMultiverseModel::GetOnlineState (CString *retsUsername, CString *retsDesc) const

//	GetOnlineState
//
//	Returns the current online state (and optionally the username)

	{
	CSmartLock Lock(m_cs);

	//	Figure out our state

	if (m_fDisabled)
		{
		if (retsUsername) *retsUsername = CONSTLIT("Offline");
#ifdef STEAM_BUILD
		if (retsDesc) *retsDesc = CONSTLIT("Steam client is not running");
#else
		if (retsDesc) *retsDesc = CONSTLIT("Multiverse disabled");
#endif
		return stateDisabled;
		}
	else if (m_sUsername.IsBlank())
		{
		if (retsUsername) *retsUsername = CONSTLIT("Offline");
		if (retsDesc) *retsDesc = CONSTLIT("Click to register a new account or sign in");
		return stateNoUser;
		}
	else if (!m_fUserSignedIn)
		{
		if (retsUsername) *retsUsername = m_sUsername;
		if (retsDesc) *retsDesc = CONSTLIT("Click to sign in");
		return stateOffline;
		}
	else
		{
		if (retsUsername) *retsUsername = m_sUsername;
#ifdef STEAM_BUILD
		if (retsDesc) *retsDesc = CONSTLIT("Connected to Steam");
#else
		if (retsDesc) *retsDesc = CONSTLIT("Signed in to the Multiverse");
#endif
		return stateOnline;
		}
	}

bool CMultiverseModel::GetResourceFileRefs (const TArray<CString> &Filespecs, TArray<CMultiverseFileRef> *retFileRefs) const

//	GetResourceFilePaths
//
//	Returns a filepath for each of the files

	{
	CSmartLock Lock(m_cs);
	int i;

	retFileRefs->DeleteAll();
	for (i = 0; i < Filespecs.GetCount(); i++)
		{
		//	We index by filename

		CString sFilename = pathGetFilename(Filespecs[i]);

		//	Look up the resource

		const SResourceDesc *pDesc = m_Resources.GetAt(sFilename);
		if (pDesc == NULL)
			{
			::kernelDebugLogPattern("Unable to find TDB resource file: %s.", sFilename);
			continue;
			}

		//	Return a copy of it

		CMultiverseFileRef *pNewRef = retFileRefs->Insert();
		*pNewRef = pDesc->pEntry->GetResourceRef(pDesc->iIndex);

		//	Set the local filespec that we expect.

		pNewRef->SetFilespec(Filespecs[i]);
		}

	//	Done

	return (retFileRefs->GetCount() > 0);
	}

bool CMultiverseModel::IsLoadCollectionNeeded (void) const

//	IsLoadCollectionNeeded
//
//	Returns TRUE if we need to load the collection.

	{
	CSmartLock Lock(m_cs);

	//	If we're not signed in...
	//	If we've already got the collection loaded...
	//	If we're in the middle of loading the collection...
	//
	//	...then don't load.

	if (!m_fUserSignedIn 
			|| m_fCollectionLoaded
			|| m_fLoadingCollection)
		return false;

	//	Need to load the collection

	return true;
	}

bool CMultiverseModel::IsLoadNewsNeeded (void) const

//	IsLoadNewsNeeded
//
//	Returns TRUE if we need to load the news

	{
	CSmartLock Lock(m_cs);

	return !m_fNewsLoaded;
	}

bool CMultiverseModel::LockCollection (void) const

//	LockCollection
//
//	Attempts to lock the collection and returns TRUE if successful.

	{
	//	If we're in the middle of getting the collection then wait a little bit

	bool bSuccess = false;
	for (int i = 0; i < 30; i++)
		{
		m_cs.Lock();
		if (!m_fLoadingCollection && m_fCollectionLoaded)
			{
			bSuccess = true;
			break;
			}
		m_cs.Unlock();

		::Sleep(100);
		}

	//	Done. If we return TRUE, it is the caller's responsibility to unlock.

	return bSuccess;
	}

void CMultiverseModel::OnCollectionLoadFailed (void)

//	OnCollectionLoadFailed
//
//	Load failed.

	{
	CSmartLock Lock(m_cs);

	m_fLoadingCollection = false;

	//	Collection loaded but empty

	m_fCollectionLoaded = true;
	}

void CMultiverseModel::OnCollectionLoading (void)

//	OnCollectionLoading
//
//	Sets state. Caller must follow up with either SetCollection or
//	OnCollectionLoadFailed.

	{
	CSmartLock Lock(m_cs);

	m_fLoadingCollection = true;
	}

void CMultiverseModel::OnUserSignedIn (const CString &sUsername)

//	OnUserSignedIn
//
//	The given user has signed in. The given username is the human-readable
//	username (not the username key).

	{
	CSmartLock Lock(m_cs);

	ASSERT(!sUsername.IsBlank());

	if (!strEquals(sUsername, m_sUsername) || !m_fUserSignedIn)
		{
		m_sUsername = sUsername;

		//	Clear out the collection so that we are forced to reload it.

		DeleteCollection();

		//	We're signed in

		m_fUserSignedIn = true;
		}
	}

void CMultiverseModel::OnUserSignedOut (void)

//	OnUserSignedOut
//
//	The current user has signed out. Note that we do not necessarily clear 
//	our cache, since we want to support offline mode.

	{
	CSmartLock Lock(m_cs);

	m_fUserSignedIn = false;
	}

ALERROR CMultiverseModel::SetCollection (const TArray<CMultiverseCatalogEntry *> &NewCollection)

//	SetCollection
//
//	Sets the collection from a list of entries.
//	NOTE: We take ownership of the catalog entries.

	{
	CSmartLock Lock(m_cs);
	int i;

	//	Replace our collection.

	DeleteCollection();
	for (i = 0; i < NewCollection.GetCount(); i++)
		{
		m_Collection.Insert(NewCollection[i]);

		//	Get the resources in this entry and add them to our list

		AddResources(*NewCollection[i]);
		}

	//	Done

	m_fCollectionLoaded = true;
	m_fLoadingCollection = false;

	return NOERROR;
	}

ALERROR CMultiverseModel::SetCollection (const CJSONValue &Data, CExtensionCollection &Extensions, CString *retsResult)

//	SetCollection
//
//	Sets the collection from a JSON value. Caller should have called
//	OnCollectionLoading.

	{
	CSmartLock Lock(m_cs);
	int i;

	//	Try to load the collection into a temporary array. If we get any errors
	//	then we abort without damage.

	bool bErrors = false;
	TArray<CMultiverseCatalogEntry *> NewCollection;
	for (i = 0; i < Data.GetCount(); i++)
		{
		const CJSONValue &Entry = Data.GetElement(i);

		//	If this is a game engine entry then see if it tells us to upgrade
		//	our engine.

		if (strEquals(TYPE_GAME_ENGINE, Entry.GetElement(FIELD_TYPE).AsString()))
			{
			SetUpgradeVersion(Entry);
			continue;
			}

		//	Create a catalog entry and add to our collection

		CMultiverseCatalogEntry *pNewEntry;
		if (CMultiverseCatalogEntry::CreateFromJSON(Entry, &pNewEntry, retsResult) != NOERROR)
			{
			bErrors = true;
			continue;
			}

		//	If this entry is not valid (perhaps because it is still in
		//	development) then ignore it.

		if (!pNewEntry->IsValid())
			{
			delete pNewEntry;
			continue;
			}

		//	Add to the new collection

		NewCollection.Insert(pNewEntry);
		}

	//	If we had errors while loading, and we didn't load anything then abort.
	//	Otherwise we assume that only a couple of entries were bad (possibly
	//	because they are under development).

	if (bErrors && NewCollection.GetCount() == 0)
		{
		m_fLoadingCollection = false;
		return ERR_FAIL;
		}

	//	Verify the signatures

	Extensions.UpdateRegistrationStatus(NewCollection);

	//	Otherwise, replace our collection.

	return SetCollection(NewCollection);
	}

void CMultiverseModel::SetDisabled (void)

//	SetDisabled
//
//	Disables access to the Multiverse. This happens if we don't have the proper
//	cloud service (or if the user has disabled cloud connections).

	{
	CSmartLock Lock(m_cs);

	m_fDisabled = true;
	m_sUsername = NULL_STR;
	m_fUserSignedIn = false;
	DeleteCollection();
	}

void CMultiverseModel::SetEntryDownloadRequested (DWORD dwUNID, bool bValue)

//	SetEntryDownloadRequested
//
//	Sets the download requested flag.

	{
	CSmartLock Lock(m_cs);

	CMultiverseCatalogEntry *pEntry = FindEntryActual(dwUNID);
	if (pEntry == NULL)
		return;

	pEntry->SetDownloadRequested(bValue);
	}

ALERROR CMultiverseModel::SetNews (const CJSONValue &Data, const CString &sCacheFilespec, TSortMap<CString, CString> *retDownloads, CString *retsResult)

//	SetNews
//
//	Sets the list of news articles from the Multiverse.

	{
	CSmartLock Lock(m_cs);
	m_fNewsLoaded = true;
	return m_News.SetNews(Data, sCacheFilespec, retDownloads, retsResult);
	}

void CMultiverseModel::SetUpgradeVersion (const CJSONValue &Entry)

//	SetUpgradeVersion
//
//	Sets the engine version available on the Multiverse.

	{
	//	If this is not for our engine, then ignore it.

	if (!strEquals(Entry.GetElement(FIELD_UNID).AsString(), UPGRADE_ENTRY_UNID))
		return;

	//	Get the upgrade URL

	m_sUpgradeURL = Entry.GetElement(FIELD_DOWNLOAD_URL).AsString();
	if (m_sUpgradeURL.IsBlank())
		{
		::kernelDebugLogPattern("Missing download URL in upgrade entry.");
		return;
		}

	//	Parse the fileVersion

	m_UpgradeVersion.sProductVersion = Entry.GetElement(FIELD_FILE_VERSION).AsString();

	TArray<CString> Parts;
	if (::strDelimit(m_UpgradeVersion.sProductVersion, '.', 0, &Parts) != NOERROR
			|| Parts.GetCount() != 4)
		{
		::kernelDebugLogPattern("Invalid upgrade entry fileVersion: %s", m_UpgradeVersion.sProductVersion);
		return;
		}

	m_UpgradeVersion.dwProductVersion = (((ULONG64)strToInt(Parts[0], 0)) << 48)
			| (((ULONG64)strToInt(Parts[1], 0)) << 32)
			| (((ULONG64)strToInt(Parts[2], 0)) << 16)
			| (((ULONG64)strToInt(Parts[3], 0)));
	}

void CMultiverseModel::SetUsername (const CString &sUsername)

//	SetUsername
//
//	Sets a cached username.

	{
	CSmartLock Lock(m_cs);

	//	Not valid if we're already signed in (use OnUserSignedIn instead)

	if (m_fUserSignedIn)
		return;

	if (!strEquals(m_sUsername, sUsername))
		{
		m_sUsername = sUsername;

		//	Clear out the collection since we've got a different user.

		DeleteCollection();
		}
	}
