//	CDockScreen.cpp
//
//	CDockScreen class

#include "PreComp.h"
#include "Transcendence.h"

const int SCREEN_PADDING_LEFT =		8;
const int SCREEN_PADDING_RIGHT =	8;
//const int MAX_SCREEN_WIDTH =		1280 + SCREEN_PADDING_LEFT + SCREEN_PADDING_RIGHT;
const int MAX_SCREEN_WIDTH =		1024 + SCREEN_PADDING_LEFT + SCREEN_PADDING_RIGHT;
const int MIN_DESC_PANE_WIDTH =		408;

const int MAX_BACKGROUND_WIDTH =	1500;
const int MAX_BACKGROUND_HEIGHT =	648;
const int EXTRA_BACKGROUND_IMAGE =	128;

const int STATUS_BAR_HEIGHT	=		20;
const int g_cyTitle =				72;
const int g_cxActionsRegion =		400;

const int g_cyItemTitle =			32;
const int g_cxItemMargin =			132;
const int g_cxItemImage =			96;
const int g_cyItemImage =			96;

const int g_cxStats =				400;
const int g_cyStats =				30;
const int g_cxCargoStats =			200;
const int g_cxCargoStatsLabel =		100;

#define CANVAS_TAG					CONSTLIT("Canvas")
#define DISPLAY_TAG					CONSTLIT("Display")
#define GROUP_TAG					CONSTLIT("Group")
#define IMAGE_TAG					CONSTLIT("Image")
#define INITIAL_PANE_TAG			CONSTLIT("InitialPane")
#define INITIALIZE_TAG				CONSTLIT("Initialize")
#define NEUROHACK_TAG				CONSTLIT("Neurohack")
#define ON_DISPLAY_INIT_TAG			CONSTLIT("OnDisplayInit")
#define ON_INIT_TAG					CONSTLIT("OnInit")
#define ON_PANE_INIT_TAG			CONSTLIT("OnPaneInit")
#define ON_SCREEN_INIT_TAG			CONSTLIT("OnScreenInit")
#define ON_SCREEN_UPDATE_TAG		CONSTLIT("OnScreenUpdate")
#define PANES_TAG					CONSTLIT("Panes")
#define TEXT_TAG					CONSTLIT("Text")

#define ALIGN_ATTRIB				CONSTLIT("align")
#define ANIMATE_ATTRIB				CONSTLIT("animate")
#define BACKGROUND_ID_ATTRIB		CONSTLIT("backgroundID")
#define BOTTOM_ATTRIB				CONSTLIT("bottom")
#define CENTER_ATTRIB				CONSTLIT("center")
#define COLOR_ATTRIB				CONSTLIT("color")
#define DESC_ATTRIB					CONSTLIT("desc")
#define FONT_ATTRIB					CONSTLIT("font")
#define HEIGHT_ATTRIB				CONSTLIT("height")
#define ID_ATTRIB					CONSTLIT("id")
#define LEFT_ATTRIB					CONSTLIT("left")
#define NAME_ATTRIB					CONSTLIT("name")
#define NAME_ID_ATTRIB				CONSTLIT("nameID")
#define NO_LIST_NAVIGATION_ATTRIB	CONSTLIT("noListNavigation")
#define PANE_ATTRIB					CONSTLIT("pane")
#define RIGHT_ATTRIB				CONSTLIT("right")
#define SHOW_TEXT_INPUT_ATTRIB		CONSTLIT("showTextInput")
#define TOP_ATTRIB					CONSTLIT("top")
#define TRANSPARENT_ATTRIB			CONSTLIT("transparent")
#define TYPE_ATTRIB					CONSTLIT("type")
#define VALIGN_ATTRIB				CONSTLIT("valign")
#define VCENTER_ATTRIB				CONSTLIT("vcenter")
#define WIDTH_ATTRIB				CONSTLIT("width")

#define PROPERTY_COUNTER			CONSTLIT("counter")
#define PROPERTY_DESCRIPTION		CONSTLIT("description")
#define PROPERTY_IN_FIRST_ON_INIT	CONSTLIT("inFirstOnInit")
#define PROPERTY_INPUT				CONSTLIT("input")

#define SCREEN_TYPE_ARMOR_SELECTOR		CONSTLIT("armorSelector")
#define SCREEN_TYPE_CANVAS				CONSTLIT("canvas")
#define SCREEN_TYPE_CAROUSEL_SELECTOR	CONSTLIT("carouselSelector")
#define SCREEN_TYPE_CUSTOM_PICKER		CONSTLIT("customPicker")
#define SCREEN_TYPE_CUSTOM_ITEM_PICKER	CONSTLIT("customItemPicker")
#define SCREEN_TYPE_DETAILS_PANE		CONSTLIT("detailsPane")
#define SCREEN_TYPE_DEVICE_SELECTOR		CONSTLIT("deviceSelector")
#define SCREEN_TYPE_ITEM_PICKER			CONSTLIT("itemPicker")
#define SCREEN_TYPE_MISC_SELECTOR		CONSTLIT("miscSelector")
#define SCREEN_TYPE_SUBJUGATE_MINIGAME	CONSTLIT("subjugateMinigame")
#define SCREEN_TYPE_WEAPONS_SELECTOR	CONSTLIT("weaponsSelector")

#define ALIGN_CENTER				CONSTLIT("center")
#define ALIGN_RIGHT					CONSTLIT("right")
#define ALIGN_LEFT					CONSTLIT("left")
#define ALIGN_BOTTOM				CONSTLIT("bottom")
#define ALIGN_TOP					CONSTLIT("top")
#define ALIGN_MIDDLE				CONSTLIT("middle")

#define BAR_COLOR							CG32bitPixel(0, 2, 10)

CDockScreen::CDockScreen (CGameSession &Session) : 
        m_Session(Session)

//	CDockScreen constructor

	{
	}

CDockScreen::~CDockScreen (void)

//	CDockScreen destructor

	{
	CleanUpScreen();
	}

void CDockScreen::Action (DWORD dwTag, DWORD dwData)

//	Action
//
//	Button pressed

	{
	switch (dwTag)
		{
		//	Handle tab control events

		case TAB_AREA_ID:
			{
			CString sTabID = m_pTabs->GetTabID(dwData);
			SelectTab(sTabID);
			break;
			}

		//	Otherwise, let the display handle it.

		default:
			{
			IDockScreenDisplay::EResults iResult = m_pDisplay->HandleAction(dwTag, dwData);
	
			switch (iResult)
				{
				//	If handled, then we're done

				case IDockScreenDisplay::resultHandled:
					break;

				//	If we need to reshow the pane, do it.

				case IDockScreenDisplay::resultShowPane:
					m_CurrentPane.ExecuteShowPane(EvalInitialPane());
					break;

				//	Otherwise, invoke the button

				default:
					m_CurrentPane.HandleAction(dwTag, dwData);
					break;
				}
			}
		}
	}

void CDockScreen::AddDisplayControl (CXMLElement *pDesc, 
									 AGScreen *pScreen, 
									 SDisplayControl *pParent,
									 const RECT &rcFrame, 
									 SDisplayControl **retpDControl)

//	AddDisplayControl
//
//	Adds a control from XML element

	{
	int i;
	SDisplayControl *pDControl = m_Controls.Insert();

	//	Set basic attribs

	pDControl->sID = pDesc->GetAttribute(ID_ATTRIB);

	//	Propage animation from parent, unless overridden.
	//	Note: we default to always animate

	if (!pDesc->FindAttributeBool(ANIMATE_ATTRIB, &pDControl->bAnimate))
		pDControl->bAnimate = (pParent ? pParent->bAnimate : true);

	//	Control rect (relative to pScreen)

	RECT rcRect;
	InitDisplayControlRect(pDesc, rcFrame, &rcRect);

	//	Get the font

	const CG16bitFont *pControlFont = &m_pFonts->Large;
	CString sFontName;
	if (pDesc->FindAttribute(FONT_ATTRIB, &sFontName))
		pControlFont = &GetFontByName(*m_pFonts, sFontName);

	CG32bitPixel rgbControlColor;
	CString sColorName;
	if (pDesc->FindAttribute(COLOR_ATTRIB, &sColorName))
		rgbControlColor = ::LoadRGBColor(sColorName);
	else
		rgbControlColor = CG32bitPixel(255, 255, 255);

	//	Create the control based on the type

	if (strEquals(pDesc->GetTag(), TEXT_TAG))
		{
		pDControl->iType = ctrlText;

		CGTextArea *pControl = new CGTextArea;
		pControl->SetFont(pControlFont);
		pControl->SetColor(rgbControlColor);
		pControl->SetFontTable(&g_pHI->GetVisuals());

		CString sAlign = pDesc->GetAttribute(ALIGN_ATTRIB);
		if (strEquals(sAlign, ALIGN_CENTER))
			pControl->SetStyles(alignCenter);
		else if (strEquals(sAlign, ALIGN_RIGHT))
			pControl->SetStyles(alignRight);

		pScreen->AddArea(pControl, rcRect, 0);
		pDControl->pArea = pControl;

		//	Load the text code

		const CString &sCode = pDesc->GetContentText(0);
		pDControl->pCode = (!sCode.IsBlank() ? CCodeChain::Link(sCode) : NULL);
		}
	else if (strEquals(pDesc->GetTag(), IMAGE_TAG))
		{
		pDControl->iType = ctrlImage;

		CGImageArea *pControl = new CGImageArea;
		pControl->SetTransBackground(pDesc->GetAttributeBool(TRANSPARENT_ATTRIB));

		DWORD dwStyles = 0;
		CString sAlign = pDesc->GetAttribute(ALIGN_ATTRIB);
		if (strEquals(sAlign, ALIGN_CENTER))
			dwStyles |= alignCenter;
		else if (strEquals(sAlign, ALIGN_RIGHT))
			dwStyles |= alignRight;
		else
			dwStyles |= alignLeft;

		sAlign = pDesc->GetAttribute(VALIGN_ATTRIB);
		if (strEquals(sAlign, ALIGN_CENTER))
			dwStyles |= alignMiddle;
		else if (strEquals(sAlign, ALIGN_BOTTOM))
			dwStyles |= alignBottom;
		else
			dwStyles |= alignTop;

		pControl->SetStyles(dwStyles);

		pScreen->AddArea(pControl, rcRect, 0);
		pDControl->pArea = pControl;

		//	Load the code that returns the image

		const CString &sCode = pDesc->GetContentText(0);
		pDControl->pCode = (!sCode.IsBlank() ? CCodeChain::Link(sCode) : NULL);
		}
	else if (strEquals(pDesc->GetTag(), CANVAS_TAG))
		{
		pDControl->iType = ctrlCanvas;

		CGDrawArea *pControl = new CGDrawArea;
		pScreen->AddArea(pControl, rcRect, 0);
		pDControl->pArea = pControl;

		//	Load the draw code

		const CString &sCode = pDesc->GetContentText(0);
		pDControl->pCode = (!sCode.IsBlank() ? CCodeChain::Link(sCode) : NULL);
		}
	else if (strEquals(pDesc->GetTag(), GROUP_TAG))
		{
		for (i = 0; i < pDesc->GetContentElementCount(); i++)
			{
			CXMLElement *pSubDesc = pDesc->GetContentElement(i);
			AddDisplayControl(pSubDesc, pScreen, pDControl, rcRect);
			}
		}
	else if (strEquals(pDesc->GetTag(), NEUROHACK_TAG))
		{
		pDControl->iType = ctrlNeurohack;

		CGNeurohackArea *pControl = new CGNeurohackArea;
		pScreen->AddArea(pControl, rcRect, 0);
		pDControl->pArea = pControl;

		//	Load the text code

		const CString &sCode = pDesc->GetContentText(0);
		pDControl->pCode = (!sCode.IsBlank() ? CCodeChain::Link(sCode) : NULL);
		}

	//	Done

	if (retpDControl)
		*retpDControl = pDControl;
	}

void CDockScreen::AddListFilter (const CString &sID, const CString &sLabel, const CItemCriteria &Filter)

//	AddListFilter
//
//	Adds a new user-selectable filter to the current list.

	{
	m_pDisplay->AddListFilter(sID, sLabel, Filter);
	}

void CDockScreen::BltSystemBackground (CSystem *pSystem, const RECT &rcRect)

//	BltSystemBackground
//
//	Blts the system space background on the docks screen background image
//	(If we have one.)

	{
	CSystemType *pSystemType = pSystem->GetType();
	DWORD dwSpaceID = pSystemType->GetBackgroundUNID();
	CG32bitImage *pSpaceImage;
	if (dwSpaceID
			&& (pSpaceImage = g_pUniverse->GetLibraryBitmap(dwSpaceID)))
		BltToBackgroundImage(rcRect, pSpaceImage, 0, 0, RectWidth(rcRect), RectHeight(rcRect));
	}

void CDockScreen::BltToBackgroundImage (const RECT &rcRect, CG32bitImage *pImage, int xSrc, int ySrc, int cxSrc, int cySrc)

//	BltToBackgroundImage
//
//	Blts the image to m_pBackgroundImage

	{
	//	EXTRA_BACKGROUND_IMAGE is the amount of image to show to the
	//	right of the center-line.

	int cxAvail = (RectWidth(rcRect) / 2) + EXTRA_BACKGROUND_IMAGE;
	int xImage = -Max(0, cxSrc - cxAvail);

    CG32bitImage &ScreenMask = GetVisuals().GetContentMask().GetImage(CONSTLIT("ShowScreen"));
	if (!ScreenMask.IsEmpty())
		{
		//	Center the mask and align it with the position of the background.

		int xAlpha = Max(0, (ScreenMask.GetWidth() - m_pBackgroundImage->GetWidth()) / 2);

		//	If the image is too small, then slide the mask over to the right
		//	so that the fade-out part aligns with the right edge of the image.

		if (pImage->GetWidth() < cxAvail)
			xAlpha += (cxAvail - pImage->GetWidth());

		//	Generate a new bitmap containing the mask at the exact position of
		//	the image we want to blt.

		CG32bitImage Mask;
        CG32bitImage *pScreenMask;
        if (xAlpha != 0)
            {
            Mask.Create(pImage->GetWidth(), pImage->GetHeight(), CG32bitImage::alpha8);
            Mask.CopyChannel(channelAlpha, xAlpha, 0, pImage->GetWidth(), pImage->GetHeight(), ScreenMask, 0, 0);
            pScreenMask = &Mask;
            }
        else
            pScreenMask = &ScreenMask;

		m_pBackgroundImage->BltMask(xSrc, ySrc, cxSrc, cySrc, *pScreenMask, *pImage, xImage, 0);
		}
	else
		m_pBackgroundImage->Blt(xImage, 0, cxSrc, cySrc, *pImage, xSrc, ySrc);
	}

void CDockScreen::CleanUpBackgroundImage (void)

//	CleanUpBackgroundImage
//
//	Cleans up the image

	{
	if (m_pBackgroundImage)
		{
		if (m_bFreeBackgroundImage)
			delete m_pBackgroundImage;

		m_pBackgroundImage = NULL;
		}
	}

void CDockScreen::CleanUpScreen (void)

//	CleanUpScreen
//
//	Called to bring the screen down after InitScreen

	{
	DEBUG_TRY

	if (m_pScreen)
		{
		delete m_pScreen;
		m_pScreen = NULL;
		}

	CleanUpBackgroundImage();

	if (m_pDisplay)
		{
		delete m_pDisplay;
		m_pDisplay = NULL;
		}

	m_Controls.DeleteAll();
	m_pDisplayInitialize = NULL;
	m_pData = NULL;
	m_pTabs = NULL;

	m_CurrentPane.CleanUp();
	m_CurrentPane.ClearDescriptionError();

	if (m_pOnScreenUpdate)
		{
		m_pOnScreenUpdate->Discard();
		m_pOnScreenUpdate = NULL;
		}

	DEBUG_CATCH
	}

ALERROR CDockScreen::CreateBackgroundImage (const IDockScreenDisplay::SBackgroundDesc &Desc, const RECT &rcRect, int xOffset)

//	CreateBackgroundImage
//
//	Creates the background image to use for the dock screen. Initializes
//	m_pBackgroundImage and m_bFreeBackgroundImage

	{
    const CDockScreenVisuals &DockScreenVisuals = GetVisuals();

	int cxBackground = m_cxImageBackground;
	int cyBackground = m_cyImageBackground;

	//	Load the image

	CG32bitImage *pImage = NULL;
	if (Desc.iType == IDockScreenDisplay::backgroundImage && Desc.dwImageID)
		pImage = g_pUniverse->GetLibraryBitmap(Desc.dwImageID);

	//	Sometimes (like in the case of item lists) the image is larger than normal

	int cyExtra = 0;
	if (pImage)
		cyExtra = Max(pImage->GetHeight() - cyBackground, 0);

	//	Create a new image for the background

	CleanUpBackgroundImage();

	m_pBackgroundImage = new CG32bitImage;
	m_bFreeBackgroundImage = true;
	m_pBackgroundImage->Create(cxBackground, cyBackground + cyExtra, CG32bitImage::alpha8);

	if (cyExtra)
		m_pBackgroundImage->Fill(0, cyBackground, cxBackground, cyExtra, 0);

	//	Load and blt the dock screen background based on the player ship class

    CG32bitImage &ScreenImage = DockScreenVisuals.GetBackground().GetImage(CONSTLIT("ShowScreen"));
	if (!ScreenImage.IsEmpty())
		{
		//	Right-align the image on the screen
		int xOffset = cxBackground - ScreenImage.GetWidth();
		m_pBackgroundImage->Copy(0, 0, ScreenImage.GetWidth(), ScreenImage.GetHeight(), ScreenImage, xOffset, 0);
		}

	//	If not image, then we're done

	if (Desc.iType == IDockScreenDisplay::backgroundNone)
		;

	//	Paint hero object

	else if (Desc.iType == IDockScreenDisplay::backgroundObjHeroImage)
		{
		//	If we have a hero image, then use that

		const CObjectImageArray &HeroImage = Desc.pObj->GetHeroImage();
		if (!HeroImage.IsEmpty())
			{
			//	Paint the hero image on top of the system space background.

			BltSystemBackground(Desc.pObj->GetSystem(), rcRect);
			HeroImage.PaintImage(*m_pBackgroundImage,
					xOffset + m_xBackgroundFocus,
					m_yBackgroundFocus,
					0,
					0);
			}

		//	Otherwise we draw the object

		else
			{
            //  Our image is only as wide as the space allotted for it (which is
            //  less than the entire background).

            int cxObjImage = (RectWidth(rcRect) / 2) + EXTRA_BACKGROUND_IMAGE;
            int cyObjImage = cyBackground + cyExtra;

            //  Create a temporary image

            CG32bitImage ObjImage;
	        ObjImage.Create(cxObjImage, cyObjImage);

			//	Paint the system background

	        CSystemType *pSystemType = Desc.pObj->GetSystem()->GetType();
	        DWORD dwSpaceID = pSystemType->GetBackgroundUNID();
	        CG32bitImage *pSpaceImage;
	        if (dwSpaceID
			        && (pSpaceImage = g_pUniverse->GetLibraryBitmap(dwSpaceID)))
        		ObjImage.Blt(0, 0, ObjImage.GetWidth(), ObjImage.GetHeight(), *pSpaceImage, 0, 0);

            //  Prepare to paint the object

			SViewportPaintCtx Ctx;
            Ctx.pCenter = Desc.pObj;
            Ctx.vCenterPos = Desc.pObj->GetPos();
            Ctx.xCenter = xOffset + m_xBackgroundFocus;
            Ctx.yCenter = m_yBackgroundFocus;
			Ctx.fNoSelection = true;
            Ctx.fNoDockedShips = true;
            Ctx.fShowSatellites = true;

	        Ctx.XForm = ViewportTransform(Ctx.vCenterPos, g_KlicksPerPixel, Ctx.xCenter, Ctx.yCenter);
	        Ctx.XFormRel = Ctx.XForm;

			//	If we've docked with a satellite of a composite object, then we 
			//	figure out the parent so that we can paint the entire composite.

			CSpaceObject *pBase = Desc.pObj->GetBase();
			int xPaint, yPaint;
			if (pBase && Desc.pObj->IsSatelliteSegmentOf(pBase))
				{
				Ctx.pObj = pBase;
				Ctx.XForm.Transform(pBase->GetPos(), &xPaint, &yPaint);
				}
			else
				{
				Ctx.pObj = Desc.pObj;
				xPaint = Ctx.xCenter;
				yPaint = Ctx.yCenter;
				}

            //  Now paint the object

			Ctx.pObj->Paint(ObjImage,
					xPaint,
					yPaint,
					Ctx);

            //  Blt using the appropriate mask

    		BltToBackgroundImage(rcRect, &ObjImage, 0, 0, ObjImage.GetWidth(), ObjImage.GetHeight());
			}
		}

    //  Paint a schematic (top-down) image

    else if (Desc.iType == IDockScreenDisplay::backgroundObjSchematicImage)
        {
        //  LATER: We should call a separate method and allow the developer
        //  to specify separate images.

        const CObjectImageArray &HeroImage = Desc.pObj->GetHeroImage();
		if (!HeroImage.IsEmpty())
			{
			HeroImage.PaintImage(*m_pBackgroundImage,
					xOffset + m_xBackgroundFocus,
					m_yBackgroundFocus,
					0,
					0);
			}
        }

	//	If we have an image with a mask, just blt the masked image

	else if (pImage && pImage->GetAlphaType() != CG32bitImage::alphaNone)
		m_pBackgroundImage->Blt(0, 0, pImage->GetWidth(), pImage->GetHeight(), 255, *pImage, xOffset, 0);

	//	If we have an image with no mask, then we need to create our own mask.
	//	If the image is larger than the space, then it is flush right with 
	//	the center line. Otherwise, it is flush left.

	else if (pImage)
		BltToBackgroundImage(rcRect, pImage, 0, 0, pImage->GetWidth(), pImage->GetHeight());

	return NOERROR;
	}

void CDockScreen::CreateScreenSetTabs (const IDockScreenDisplay::SInitCtx &Ctx, const IDockScreenDisplay::SDisplayOptions &Options, const TArray<SScreenSetTab> &ScreenSet, const CString &sCurTab)

//	CreateScreenSetTabs
//
//	Creates tabs for the screenset.

	{
	const CVisualPalette &VI = g_pHI->GetVisuals();
    const CDockScreenVisuals &DockScreenVisuals = GetVisuals();

	RECT rcRect = Ctx.rcRect;
	rcRect.left += Options.rcControl.left;
	rcRect.right = rcRect.left + RectWidth(Options.rcControl);
	rcRect.top += Options.rcControl.top;
	rcRect.bottom = rcRect.top + DockScreenVisuals.GetTabHeight();

	m_pTabs = new CGTabArea(VI);
    m_pTabs->SetColor(DockScreenVisuals.GetTitleTextColor());
    m_pTabs->SetBackColor(DockScreenVisuals.GetTextBackgroundColor());

	for (int i = 0; i < ScreenSet.GetCount(); i++)
		m_pTabs->AddTab(ScreenSet[i].sID, ScreenSet[i].sName);

	//	Select the tab, if necessary.

	if (!sCurTab.IsBlank())
		m_pTabs->SetCurTab(sCurTab);

	//	Create. NOTE: Once we add it to the screen, it takes ownership of it. 
	//	We do not have to free it.

	Ctx.pScreen->AddArea(m_pTabs, rcRect, TAB_AREA_ID);
	}

ALERROR CDockScreen::CreateTitleArea (CXMLElement *pDesc, AGScreen *pScreen, const RECT &rcRect, const RECT &rcInner)

//	CreateTitleArea
//
//	Creates the title and status bar

	{
	DEBUG_TRY

	const CVisualPalette &VI = g_pHI->GetVisuals();
    const CDockScreenVisuals &DockScreenVisuals = GetVisuals();

	int yTop = m_yDisplay;

	//	Add a background bar to the title part

	CGImageArea *pImage = new CGImageArea;
	pImage->SetBackColor(DockScreenVisuals.GetTitleBackgroundColor());
	RECT rcArea;
	rcArea.left = rcRect.left;
	rcArea.top = yTop - g_cyTitle;
	rcArea.right = rcRect.right;
	rcArea.bottom = yTop - STATUS_BAR_HEIGHT;
	pScreen->AddArea(pImage, rcArea, 0);

	pImage = new CGImageArea;
	pImage->SetBackColor(CG32bitPixel::Darken(DockScreenVisuals.GetTitleBackgroundColor(), 200));
	rcArea.left = rcRect.left;
	rcArea.top = yTop - STATUS_BAR_HEIGHT;
	rcArea.right = rcRect.right;
	rcArea.bottom = yTop;
	pScreen->AddArea(pImage, rcArea, 0);

	//	Get the name of this location

	CString sName = GetScreenName(pDesc);

	//	Add the name as a title to the screen

	CGTextArea *pText = new CGTextArea;
	pText->SetText(sName);
	pText->SetFont(&m_pFonts->Title);
	pText->SetColor(DockScreenVisuals.GetTitleTextColor());
	pText->AddShadowEffect();
	rcArea.left = rcRect.left + 8;
	rcArea.top = yTop - g_cyTitle;
	rcArea.right = rcRect.right;
	rcArea.bottom = yTop;
	pScreen->AddArea(pText, rcArea, 0);

	//	Add the money area

	int cyOffset = (STATUS_BAR_HEIGHT - m_pFonts->MediumHeavyBold.GetHeight()) / 2;

	m_pCredits = new CGTextArea;
	m_pCredits->SetFont(&m_pFonts->MediumHeavyBold);
	m_pCredits->SetColor(DockScreenVisuals.GetTitleTextColor());

	rcArea.left = rcInner.right - g_cxStats;
	rcArea.top = yTop - STATUS_BAR_HEIGHT + cyOffset;
	rcArea.right = rcInner.right;
	rcArea.bottom = rcArea.top + g_cyStats;
	pScreen->AddArea(m_pCredits, rcArea, 0);

	//	Add the cargo space label

	pText = new CGTextArea;
	pText->SetText(CONSTLIT("Cargo Space:"));
	pText->SetFont(&m_pFonts->MediumHeavyBold);
	pText->SetColor(DockScreenVisuals.GetTitleTextColor());

	rcArea.left = rcInner.right - g_cxCargoStats;
	rcArea.top = yTop - STATUS_BAR_HEIGHT + cyOffset;
	rcArea.right = rcInner.right;
	rcArea.bottom = rcArea.top + g_cyStats;
	pScreen->AddArea(pText, rcArea, 0);

	//	Add the cargo space area

	m_pCargoSpace = new CGTextArea;
	m_pCargoSpace->SetFont(&m_pFonts->MediumHeavyBold);
	m_pCargoSpace->SetColor(DockScreenVisuals.GetTitleTextColor());

	rcArea.left = rcInner.right - g_cxCargoStats + g_cxCargoStatsLabel;
	rcArea.top = yTop - STATUS_BAR_HEIGHT + cyOffset;
	rcArea.right = rcInner.right;
	rcArea.bottom = rcArea.top + g_cyStats;
	pScreen->AddArea(m_pCargoSpace, rcArea, 0);

	UpdateCredits();

	return NOERROR;

	DEBUG_CATCH
	}

ICCItem *CDockScreen::GetCurrentListEntry (void)

//	GetCurrentListEntry
//
//	Returns the current list entry

	{
	if (m_pDisplay == NULL)
		return CCodeChain::CreateNil();

	ICCItem *pResult = m_pDisplay->GetCurrentListEntry();
	if (pResult == NULL)
		return CCodeChain::CreateNil();

	return pResult;
	}

CG32bitImage *CDockScreen::GetDisplayCanvas (const CString &sID)

//	GetDisplayCanvas
//
//	Returns the given canvas (NULL if none found)

	{
	SDisplayControl *pControl = FindDisplayControl(sID);
	if (pControl == NULL || pControl->pArea == NULL)
		return NULL;

	if (pControl->iType != ctrlCanvas)
		return NULL;

	CGDrawArea *pCanvasControl = (CGDrawArea *)pControl->pArea;
	return &pCanvasControl->GetCanvas();
	}

ICCItemPtr CDockScreen::GetProperty (const CString &sProperty) const

//	GetProperty
//
//	Returns the given screen property.

	{
	//	See if this is a generic property.

	if (strEquals(sProperty, PROPERTY_COUNTER))
		return ICCItemPtr(GetCounter());

	else if (strEquals(sProperty, PROPERTY_DESCRIPTION))
		return ICCItemPtr(GetDescription());

	else if (strEquals(sProperty, PROPERTY_IN_FIRST_ON_INIT))
		return ICCItemPtr(IsFirstOnInit());

	else if (strEquals(sProperty, PROPERTY_INPUT))
		return ICCItemPtr(GetTextInput());

	//	Otherwise, ask the display

	else if (m_pDisplay)
		return m_pDisplay->GetProperty(sProperty);

	//	Otherwise, Nil

	else
		return ICCItemPtr(ICCItem::Nil);
	}

bool CDockScreen::SetProperty (const CString &sProperty, ICCItem &Value)

//	SetProperty
//
//	Sets the given screen property.

	{
	if (strEquals(sProperty, PROPERTY_COUNTER))
		SetCounter(Value.GetIntegerValue());

	else if (m_pDisplay)
		return m_pDisplay->SetProperty(sProperty, Value);
	else
		return false;

	return true;
	}

CDesignType *CDockScreen::GetResolvedRoot (CString *retsResolveScreen) const

//	GetResolvedRoot
//
//	Returns the screen root for the current stack frame.

	{
	SDockFrame CurFrame;
	g_pTrans->GetModel().GetScreenSession(&CurFrame);

	if (retsResolveScreen)
		*retsResolveScreen = CurFrame.sResolvedScreen;

	return CurFrame.pResolvedRoot;
	}

const CDockScreenVisuals &CDockScreen::GetVisuals (void) const

//  GetVisuals
//
//  Returns the visuals for this dock screen (from the ship class)
    
    {
    ASSERT(m_pPlayer);
    return m_pPlayer->GetShip()->GetClass()->GetPlayerSettings()->GetDockScreenVisuals();
    }

bool CDockScreen::EvalBool (const CString &sCode)

//	EvalBool
//
//	Evaluates the given CodeChain code.

	{
	CCodeChainCtx Ctx(GetUniverse());
	Ctx.SetScreen(this);
	Ctx.DefineContainingType(m_pRoot);
	Ctx.SaveAndDefineSourceVar(m_pLocation);
	Ctx.SaveAndDefineDataVar(m_pData);

	CCodeChain::SLinkOptions Options;
	Options.iOffset = 1;

	ICCItemPtr pExp = Ctx.LinkCode(sCode, Options);
	ICCItemPtr pResult = Ctx.RunCode(pExp);	//	LATER:Event

	if (pResult->IsError())
		ReportError(pResult->GetStringValue());

	return !pResult->IsNil();
	}

CString CDockScreen::EvalInitialPane (void)

//	EvalInitialPane
//
//	Invokes <InitialPane>

	{
	SDockFrame CurFrame;
	g_pTrans->GetModel().GetScreenSession(&CurFrame);
	return EvalInitialPane(CurFrame.pLocation, CurFrame.pInitialData);
	}

CString CDockScreen::EvalInitialPane (CSpaceObject *pSource, ICCItem *pData)

//	EvalInitialPane
//
//	Invokes <InitialPane> code and returns the result (or "Default")

	{
	DEBUG_TRY

	CXMLElement *pInitialPane = m_pDesc->GetContentElementByTag(INITIAL_PANE_TAG);
	if (pInitialPane)
		{
		CString sPane;
		CString sCode = pInitialPane->GetContentText(0);

		//	Execute

		CCodeChainCtx Ctx(GetUniverse());
		Ctx.SetScreen(this);
		Ctx.DefineContainingType(m_pRoot);
		Ctx.SaveAndDefineSourceVar(pSource);
		Ctx.SaveAndDefineDataVar(pData);

		ICCItemPtr pExp = Ctx.LinkCode(sCode);
		ICCItemPtr pResult = Ctx.RunCode(pExp);	//	LATER:Event

		if (pResult->IsError())
			ReportError(pResult->GetStringValue());
		else
			sPane = pResult->GetStringValue();

		return sPane;
		}
	else
		return CONSTLIT("Default");

	DEBUG_CATCH
	}

bool CDockScreen::EvalString (const CString &sString, ICCItem *pData, bool bPlain, ECodeChainEvents iEvent, CString *retsResult)

//	EvalString
//
//	Evaluates a string using CodeChain. A string that being with an equal sign
//	indicates a CodeChain expression.
//
//	Returns TRUE if successful. Otherwise, retsResult is an error.

	{
	CCodeChainCtx Ctx(GetUniverse());
	Ctx.SetEvent(iEvent);
	Ctx.SetScreen(this);
	Ctx.DefineContainingType(m_pRoot);
	Ctx.SaveAndDefineSourceVar(m_pLocation);
	Ctx.SaveAndDefineDataVar(pData);

	return Ctx.RunEvalString(sString, bPlain, retsResult);
	}

CDockScreen::SDisplayControl *CDockScreen::FindDisplayControl (const CString &sID)

//	FindDisplayControl
//
//	Returns the display control with the given ID (or NULL if not found)

	{
	int i;

	for (i = 0; i < m_Controls.GetCount(); i++)
		{
		if (strEquals(sID, m_Controls[i].sID))
			return &m_Controls[i];
		}

	return NULL;
	}

ALERROR CDockScreen::FireOnScreenInit (CSpaceObject *pSource, ICCItem *pData, CString *retsError)

//	FireOnScreenInit
//
//	Fire screen OnScreenInit

	{
	DEBUG_TRY

	//	We accept either OnScreenInit or OnInit

	CXMLElement *pOnInit = m_pDesc->GetContentElementByTag(ON_SCREEN_INIT_TAG);
	if (pOnInit == NULL)
		pOnInit = m_pDesc->GetContentElementByTag(ON_INIT_TAG);

	//	See if we have it

	if (pOnInit)
		{
		m_bInOnInit = true;

		CString sCode = pOnInit->GetContentText(0);

		//	Execute

		CCodeChainCtx Ctx(GetUniverse());
		Ctx.SetScreen(this);
		Ctx.DefineContainingType(m_pRoot);
		Ctx.SaveAndDefineSourceVar(pSource);
		Ctx.SaveAndDefineDataVar(pData);

		ICCItemPtr pExp = Ctx.LinkCode(sCode);
		ICCItemPtr pResult = Ctx.RunCode(pExp);	//	LATER:Event

		if (pResult->IsError())
			{
			*retsError = pResult->GetStringValue();

			m_bInOnInit = false;
			return ERR_FAIL;
			}

		m_bInOnInit = false;
		}

	m_bFirstOnInit = false;

	return NOERROR;

	DEBUG_CATCH
	}

CString CDockScreen::GetScreenName (CXMLElement *pDesc)

//	GetScreenName
//
//	Computes the screen name.

	{
	CString sValue;
	ICCItemPtr pTitle;
	CString sName;

	//	If we have a "core.name" language element, then use that.

	if (Translate(CONSTLIT("core.name"), m_pData, pTitle))
		return pTitle->GetStringValue();

	//	If we have nameID= then use that as a translation ID.

	else if (pDesc->FindAttribute(NAME_ID_ATTRIB, &sValue))
		{
		if (!Translate(sValue, m_pData, pTitle))
			return strPatternSubst(CONSTLIT("Unknown language ID: %s"), sValue);

		return pTitle->GetStringValue();
		}

	//	Otherwise, see if we have a name= attribute.

	else if (pDesc->FindAttribute(NAME_ATTRIB, &sValue))
		{
		if (!EvalString(sValue, m_pData, false, eventNone, &sName))
			return strPatternSubst(CONSTLIT("Error evaluating screen title: %s"), sName);

		return sName;
		}

	//	Otherwise, ask the location

	else
		return m_pLocation->GetNounPhrase(nounTitleCapitalize);
	}

void CDockScreen::HandleChar (char chChar)

//	HandleChar
//
//	Handle char events

	{
	m_CurrentPane.HandleChar(chChar);
	}

void CDockScreen::HandleKeyDown (int iVirtKey)

//	HandleKeyDown
//
//	Handle key down events

	{
	//	If we have a tab control, see if it handles it.

	if (m_pTabs && iVirtKey == VK_TAB)
		{
		if (uiIsShiftDown())
			SelectTab(m_pTabs->GetPrevTabID());
		else
			SelectTab(m_pTabs->GetNextTabID());
		}

	//	Otherwise, let the display handle it.

	else
		{
		IDockScreenDisplay::EResults iResult = m_pDisplay->HandleKeyDown(iVirtKey);

		switch (iResult)
			{
			//	If handled, then we're done

			case IDockScreenDisplay::resultHandled:
				break;

			//	If we need to reshow the pane, do it.

			case IDockScreenDisplay::resultShowPane:
				m_CurrentPane.ExecuteShowPane(EvalInitialPane());
				break;

			//	Otherwise, the pane handles it.

			default:
				m_CurrentPane.HandleKeyDown(iVirtKey);
				break;
			}
		}
	}

ALERROR CDockScreen::InitCodeChain (CTranscendenceWnd *pTrans, CSpaceObject *pStation)

//	InitCodeChain
//
//	Initializes CodeChain language
//	LATER: We should define variables inside of Eval...

	{
	CCodeChain &CC = GetUniverse().GetCC();

	//	Define some globals

	CC.DefineGlobalInteger(CONSTLIT("gSource"), (int)pStation);
	CC.DefineGlobalInteger(CONSTLIT("gScreen"), (int)this);

	return NOERROR;
	}

ALERROR CDockScreen::InitDisplay (CXMLElement *pDisplayDesc, AGScreen *pScreen, const RECT &rcScreen)

//	InitDisplay
//
//	Initializes display controls

	{
	DEBUG_TRY

	int i;

	ASSERT(m_Controls.GetCount() == 0);

	//	Init

	m_pDisplayInitialize = NULL;
	if (!pDisplayDesc->FindAttributeBool(ANIMATE_ATTRIB, &m_bDisplayAnimate))
		m_bDisplayAnimate = true;

	//	Allocate the controls

	int iControlCount = pDisplayDesc->GetContentElementCount();
	if (iControlCount == 0)
		return NOERROR;

	//	Compute the canvas rect for the controls (relative to pScreen)

	RECT rcCanvas;
	rcCanvas.left = rcScreen.left;
	rcCanvas.top = m_yDisplay;		//	Just under the title bar
	rcCanvas.right = rcCanvas.left + m_cxDisplay;
	rcCanvas.bottom = rcScreen.bottom;

	//	Create each control

	for (i = 0; i < iControlCount; i++)
		{
		CXMLElement *pControlDesc = pDisplayDesc->GetContentElement(i);

		//	If this is the initialize tag, remember it

		if (strEquals(pControlDesc->GetTag(), INITIALIZE_TAG)
				|| strEquals(pControlDesc->GetTag(), ON_DISPLAY_INIT_TAG))
			m_pDisplayInitialize = pControlDesc;

		//	Otherwise, Add the control

		else
			AddDisplayControl(pControlDesc, pScreen, NULL, rcCanvas);
		}

	return NOERROR;

	DEBUG_CATCH
	}

void CDockScreen::InitDisplayControlRect (CXMLElement *pDesc, const RECT &rcFrame, RECT *retrcRect)

//	InitDisplayControlRect
//
//	Initializes the RECT for a display control

	{
	RECT rcRect;

	bool bValidLeft = pDesc->FindAttributeInteger(LEFT_ATTRIB, (int *)&rcRect.left);
	if (!bValidLeft)
		rcRect.left = 0;

	bool bValidTop = pDesc->FindAttributeInteger(TOP_ATTRIB, (int *)&rcRect.top);
	if (!bValidTop)
		rcRect.top = 0;

	bool bValidRight = pDesc->FindAttributeInteger(RIGHT_ATTRIB, (int *)&rcRect.right);
	if (!bValidRight)
		rcRect.right = 0;

	bool bValidBottom = pDesc->FindAttributeInteger(BOTTOM_ATTRIB, (int *)&rcRect.bottom);
	if (!bValidBottom)
		rcRect.bottom = 0;

	int xCenter;
	bool bXCenter = pDesc->FindAttributeInteger(CENTER_ATTRIB, &xCenter);

	int yCenter;
	bool bYCenter = pDesc->FindAttributeInteger(VCENTER_ATTRIB, &yCenter);

	if (bValidRight && rcRect.right <= 0)
		rcRect.right = RectWidth(rcFrame) + rcRect.right;

	if (bValidBottom && rcRect.bottom <= 0)
		rcRect.bottom = RectHeight(rcFrame) + rcRect.bottom;

	int cxWidth = pDesc->GetAttributeInteger(WIDTH_ATTRIB);
	if (cxWidth)
		{
		if (bXCenter)
			{
			rcRect.left = xCenter + ((RectWidth(rcFrame) - cxWidth) / 2);
			rcRect.right = rcRect.left + cxWidth;
			}
		else if (!bValidRight)
			rcRect.right = rcRect.left + cxWidth;
		else if (!bValidLeft)
			rcRect.left = rcRect.right - cxWidth;
		}
	else
		{
		if (rcRect.right < rcRect.left)
			rcRect.right = RectWidth(rcFrame);
		}

	int cyHeight = pDesc->GetAttributeInteger(HEIGHT_ATTRIB);
	if (cyHeight)
		{
		if (bYCenter)
			{
			rcRect.top = yCenter + ((RectHeight(rcFrame) - cyHeight) / 2);
			rcRect.bottom = rcRect.top + cyHeight;
			}
		else if (!bValidBottom)
			rcRect.bottom = rcRect.top + cyHeight;
		else if (!bValidTop)
			rcRect.top = rcRect.bottom - cyHeight;
		}
	else
		{
		if (rcRect.bottom < rcRect.top)
			rcRect.bottom = RectHeight(rcFrame);
		}

	//	If we're still 0-size, then we take up the whole frame

	if (rcRect.left == 0 && rcRect.top == 0
			&& rcRect.top == 0 && rcRect.bottom == 0)
		{
		rcRect.right = RectWidth(rcFrame);
		rcRect.bottom = RectHeight(rcFrame);
		}

	//	Otherwise, make sure we don't exceed the canvas

	else
		{
		rcRect.left = Min(RectWidth(rcFrame), Max(0, (int)rcRect.left));
		rcRect.right = Min(RectWidth(rcFrame), (int)Max(rcRect.left, rcRect.right));
		rcRect.top = Min(RectHeight(rcFrame), Max(0, (int)rcRect.top));
		rcRect.bottom = Min(RectHeight(rcFrame), (int)Max(rcRect.top, rcRect.bottom));
		}

	//	RECT is in absolute coordinates

	rcRect.left += rcFrame.left;
	rcRect.right += rcFrame.left;
	rcRect.top += rcFrame.top;
	rcRect.bottom += rcFrame.top;

	//	Done

	*retrcRect = rcRect;
	}

ALERROR CDockScreen::InitScreen (HWND hWnd, 
								 RECT &rcRect, 
								 CDockScreenStack &FrameStack,
								 CExtension *pExtension,
								 CXMLElement *pDesc, 
								 const CString &sPane,
								 ICCItem *pData,
								 AGScreen **retpScreen,
								 CString *retsError)

//	InitScreen
//
//	Initializes the docking screen. Returns an AGScreen object
//	that has been initialized appropriately.

	{
	DEBUG_TRY

	ALERROR error;
	int i;

	//	Make sure we clean up first

	CleanUpScreen();
	m_pFonts = &g_pTrans->GetFonts();

	//	Init some variables

	SDockFrame Frame = FrameStack.GetCurrent();
	m_pLocation = Frame.pLocation;
	m_pRoot = Frame.pRoot;
	m_sScreen = Frame.sScreen;
	m_pData = pData;
	m_pPlayer = g_pTrans->GetPlayer();
	m_pExtension = pExtension;
	m_pDesc = pDesc;

	//	Initialize CodeChain processor

	if (error = InitCodeChain(g_pTrans, m_pLocation))
		{
		if (retsError) *retsError = CONSTLIT("Unable to initialize CodeChain.");
		return error;
		}

	//	Call OnScreenInit

	CString sError;
	if (error = FireOnScreenInit(m_pLocation, pData, &sError))
		{
		kernelDebugLogString(sError);
		//	We do not fail because otherwise the screen would be invalid.
		}

	//	If we've already got a screen set up then we don't need to
	//	continue (OnScreenInit has navigated to a different screen).

	if (m_pScreen)
		return NOERROR;

	//	Create a new screen

	m_pScreen = new AGScreen(hWnd, rcRect);
	m_pScreen->SetController(this);

	if (g_pUniverse->GetSFXOptions().IsDockScreenTransparent())
		m_pScreen->SetBackgroundColor(CG32bitPixel::Null());
	else
		m_pScreen->SetBackgroundColor(GetVisuals().GetWindowBackgroundColor());

	//	Compute RECTs.
	//
	//	rRect is the screen-relative area where we will draw the dock screen.
	//
	//	m_rcBackground is the full RECT of the background including the title
	//	bar. This RECT shrinks to fit the background image, and expands up to
	//	the hard-coded limit.
	//
	//	m_rcScreen is RECT limits for the controls.

    CG32bitImage &ScreenImage = GetVisuals().GetBackground().GetImage(CONSTLIT("ShowScreen"));
	int cxMaxBackground = (ScreenImage.IsEmpty() ? MAX_BACKGROUND_WIDTH : ScreenImage.GetWidth());
	int cyMaxBackground = (ScreenImage.IsEmpty() ? MAX_BACKGROUND_HEIGHT : ScreenImage.GetHeight() + g_cyTitle);

	int cxBackground = Min(cxMaxBackground, RectWidth(rcRect));
	int cyBackground = Min(cyMaxBackground, RectHeight(rcRect));
	m_rcBackground.left = (RectWidth(rcRect) - cxBackground) / 2;
	m_rcBackground.right = m_rcBackground.left + cxBackground;

	//	We center the screen on the display area (below the title)

	m_rcBackground.top = ((RectHeight(rcRect) - (cyBackground - g_cyTitle)) / 2) - g_cyTitle;
	m_rcBackground.bottom = m_rcBackground.top + cyBackground;

	m_cxImageBackground = cxBackground;
	m_cyImageBackground = cyBackground - g_cyTitle;

	int cxScreen = Min(MAX_SCREEN_WIDTH, RectWidth(m_rcBackground));
	int cyScreen = RectHeight(m_rcBackground);
	m_rcScreen.left = (RectWidth(rcRect) - cxScreen) / 2;
	m_rcScreen.top = m_rcBackground.top;
	m_rcScreen.right = m_rcScreen.left + cxScreen;
	m_rcScreen.bottom = m_rcScreen.top + cyScreen;

	//	Compute the width of the display and the panes

	int cxRightPane = Max(MIN_DESC_PANE_WIDTH, cxScreen / 3);

	//	The main display is starts below the title bar

	m_yDisplay = m_rcScreen.top + g_cyTitle;
	m_cxDisplay = cxScreen - cxRightPane;

	//	Compute the center of the display

	m_xBackgroundFocus = (cxScreen - cxRightPane) / 2;
	m_yBackgroundFocus = m_cyImageBackground / 2;

	//	Prepare a display context

	IDockScreenDisplay::SInitCtx DisplayCtx;
	DisplayCtx.pPlayer = m_pPlayer;
	DisplayCtx.dwFirstID = DISPLAY_ID;
	DisplayCtx.pData = pData;
	DisplayCtx.pDesc = m_pDesc;
	DisplayCtx.pDisplayDesc = m_pDesc->GetContentElementByTag(DISPLAY_TAG);
	DisplayCtx.pDockScreen = this;
	DisplayCtx.pRoot = m_pRoot;
	DisplayCtx.pVI = &g_pHI->GetVisuals();
	DisplayCtx.pFontTable = m_pFonts;
	DisplayCtx.pLocation = m_pLocation;
	DisplayCtx.pScreen = m_pScreen;

	DisplayCtx.rcScreen = m_rcScreen;
	DisplayCtx.rcScreen.top = m_yDisplay;

	DisplayCtx.rcRect.left = m_rcScreen.left + SCREEN_PADDING_LEFT;
	DisplayCtx.rcRect.top = m_yDisplay;
	DisplayCtx.rcRect.right = DisplayCtx.rcRect.left + RectWidth(m_rcScreen) - (cxRightPane + SCREEN_PADDING_LEFT + SCREEN_PADDING_RIGHT);
	DisplayCtx.rcRect.bottom = DisplayCtx.rcScreen.bottom;

	//	Get any display options (we need to do this first because it may specify
	//	a background image.

	IDockScreenDisplay::SDisplayOptions DisplayOptions;
	if (!IDockScreenDisplay::GetDisplayOptions(DisplayCtx, &DisplayOptions, retsError))
		return ERR_FAIL;

	//	If we have a deferred background setting, then use that (and reset it
	//	so that we don't use it again).

	if (m_DeferredBackground.iType != IDockScreenDisplay::backgroundDefault)
		{
		DisplayOptions.BackgroundDesc = m_DeferredBackground;
		m_DeferredBackground.iType = IDockScreenDisplay::backgroundDefault;
		}

	//	Creates the title area

	if (error = CreateTitleArea(m_pDesc, m_pScreen, m_rcBackground, m_rcScreen))
		{
		if (retsError) *retsError = CONSTLIT("Unable to create title area.");
		return error;
		}

	//	If a screenset is defined, then create the tabs now. Note that we get
	//	the latest frame because we could have added a screenset in 
	//	OnScreenInit above.

	if (FrameStack.GetCurrent().ScreenSet.GetCount() > 0)
		{
		const SDockFrame &CurFrame = FrameStack.GetCurrent();
		CreateScreenSetTabs(DisplayCtx, DisplayOptions, CurFrame.ScreenSet, CurFrame.sCurrentTab);

		DisplayOptions.cyTabRegion = GetVisuals().GetTabHeight();
		}

	//	Get the list of panes for this screen

	m_pPanes = m_pDesc->GetContentElementByTag(PANES_TAG);
	if (m_pPanes == NULL)
		{
		if (retsError) *retsError = CONSTLIT("ERROR: No <Panes> tag defined.");
		return ERR_FAIL;
		}

	//	Create the main display object based on the type parameter.

	if (DisplayOptions.sType.IsBlank() || strEquals(DisplayOptions.sType, SCREEN_TYPE_CANVAS))
		m_pDisplay = new CDockScreenNullDisplay(*this);

	else if (strEquals(DisplayOptions.sType, SCREEN_TYPE_ITEM_PICKER))
		m_pDisplay = new CDockScreenItemList(*this);

	else if (strEquals(DisplayOptions.sType, SCREEN_TYPE_CAROUSEL_SELECTOR))
		m_pDisplay = new CDockScreenCarousel(*this);

	else if (strEquals(DisplayOptions.sType, SCREEN_TYPE_CUSTOM_PICKER))
		m_pDisplay = new CDockScreenCustomList(*this);

	else if (strEquals(DisplayOptions.sType, SCREEN_TYPE_CUSTOM_ITEM_PICKER))
		m_pDisplay = new CDockScreenCustomItemList(*this);

	else if (strEquals(DisplayOptions.sType, SCREEN_TYPE_DETAILS_PANE))
		m_pDisplay = new CDockScreenDetailsPane(*this);

	else if (strEquals(DisplayOptions.sType, SCREEN_TYPE_ARMOR_SELECTOR))
		m_pDisplay = new CDockScreenSelector(*this, CGSelectorArea::configArmor);

	else if (strEquals(DisplayOptions.sType, SCREEN_TYPE_DEVICE_SELECTOR))
		m_pDisplay = new CDockScreenSelector(*this, CGSelectorArea::configDevices);

	else if (strEquals(DisplayOptions.sType, SCREEN_TYPE_MISC_SELECTOR))
		m_pDisplay = new CDockScreenSelector(*this, CGSelectorArea::configMiscDevices);

	else if (strEquals(DisplayOptions.sType, SCREEN_TYPE_SUBJUGATE_MINIGAME))
		m_pDisplay = new CDockScreenSubjugate(*this);

	else if (strEquals(DisplayOptions.sType, SCREEN_TYPE_WEAPONS_SELECTOR))
		m_pDisplay = new CDockScreenSelector(*this, CGSelectorArea::configWeapons);
	
	else
		{
		if (retsError) *retsError = strPatternSubst(CONSTLIT("ERROR: Invalid display type: %s."), DisplayOptions.sType);
		return ERR_FAIL;
		}

	//	Initialize

	if (error = m_pDisplay->Init(DisplayCtx, DisplayOptions, &sError))
		{
		ReportError(sError);

		//	Continue
		}

	//	If we have a display element, then load the display controls
	//	LATER: Move this to its own display class.

	if (DisplayCtx.pDisplayDesc
			&& (DisplayOptions.sType.IsBlank() || strEquals(DisplayOptions.sType, SCREEN_TYPE_CANVAS)))
		{
		if (error = InitDisplay(DisplayCtx.pDisplayDesc, m_pScreen, m_rcScreen))
			{
			if (retsError) *retsError = CONSTLIT("Unable to initialize display.");
			return error;
			}

		//	Set any deferred text

		for (i = 0; i < m_DeferredDisplayText.GetCount(); i++)
			SetDisplayText(m_DeferredDisplayText.GetKey(i), m_DeferredDisplayText[i]);

		m_DeferredDisplayText.DeleteAll();
		}

	//	If the screen did not define a background image, then ask the display
	//	to do so.

	IDockScreenDisplay::SBackgroundDesc BackgroundDesc = DisplayOptions.BackgroundDesc;
	if (BackgroundDesc.iType == IDockScreenDisplay::backgroundDefault)
		{
		m_pDisplay->GetDefaultBackground(&BackgroundDesc);

		//	It's OK if m_pDisplay leaves it as a default. SetBackground (below)
		//	will handle the default properly.
		}

	//	Create the background area. We do this after the display init because we
	//	may need to refer to it to come up with a suitable background.

	SetBackground(BackgroundDesc);

	//	Cache the screen's OnUpdate

	CXMLElement *pOnUpdate = m_pDesc->GetContentElementByTag(ON_SCREEN_UPDATE_TAG);
	if (pOnUpdate)
		{
		CCodeChainCtx Ctx(GetUniverse());
		ICCItemPtr pCode = Ctx.LinkCode(pOnUpdate->GetContentText(0));
		if (pCode->IsError())
			{
			kernelDebugLogPattern("Unable to parse OnScreenUpdate: %s.", pCode->GetStringValue());
			m_pOnScreenUpdate = NULL;
			}
		else
			{
			m_pOnScreenUpdate = pCode->Reference();
			}
		}

	//	Show the pane

	if (!sPane.IsBlank())
		{
		ShowPane(sPane);
		}
	else
		{
		CString sPane = EvalInitialPane(m_pLocation, pData);

		//	If evaluation fails due to an error, then show default

		if (sPane.IsBlank())
			sPane = CONSTLIT("Default");

		//	Set the pane for the current frame stack, now that we've figured it out.

		FrameStack.SetCurrentPane(sPane);

		//	Show it

		ShowPane(sPane);
		}

	//	Done

	*retpScreen = m_pScreen;

	return NOERROR;

	DEBUG_CATCH
	}

ALERROR CDockScreen::ReportError (const CString &sError)

//	ReportError
//
//	Reports an error while evaluating some function in the dock screen.

	{
	CString sNewError = strPatternSubst(CONSTLIT("%08x%s: %s"),
				(m_pRoot ? m_pRoot->GetUNID() : 0),
				(m_sScreen.IsBlank() ? NULL_STR : strPatternSubst(CONSTLIT("/%s"), m_sScreen)),
				sError);

	//	Set the error on the pane description.

	m_CurrentPane.SetDescriptionError(sNewError);
	::kernelDebugLogString(sNewError);

	return ERR_FAIL;
	}

void CDockScreen::OnModifyItemBegin (SModifyItemCtx &Ctx, CSpaceObject *pSource, const CItem &Item)

//	OnModifyItemBegin
//
//	An item on the source is about to be modified.

	{
	if (m_pDisplay)
		m_pDisplay->OnModifyItemBegin(Ctx, pSource, Item);
	}

void CDockScreen::OnModifyItemComplete (SModifyItemCtx &Ctx, CSpaceObject *pSource, const CItem &Result)

//	OnModifyItemComplete
//
//	And item on the source has been modified.

	{
	if (m_pDisplay
			&& m_pDisplay->OnModifyItemComplete(Ctx, pSource, Result) == IDockScreenDisplay::resultShowPane)
		{
		const SDockFrame &CurFrame = g_pUniverse->GetDockSession().GetCurrentFrame();

		//	NOTE: We defer the actual recalc of the pane until after any action
		//	is done. We need to do this because we don't want to execute
		//	<OnPaneInit> in the middle of processing an action (since that might
		//	change state which the action is relying on.

		if (CurFrame.sPane.IsBlank())
			m_CurrentPane.ExecuteShowPane(EvalInitialPane(), true);
		else
			m_CurrentPane.ExecuteShowPane(CurFrame.sPane, true);
		}
	}

void CDockScreen::ShowDisplay (bool bAnimateOnly)

//	ShowDisplay
//
//	Updates the controls on the display

	{
	int i;

	//	Run initialize

	if (m_pDisplayInitialize)
		{
		CString sCode = m_pDisplayInitialize->GetContentText(0);
		CString sError;
		if (!EvalString(sCode, m_pData, true, eventNone, &sError))
			ReportError(strPatternSubst(CONSTLIT("Error evaluating <OnDisplayInit>: %s"), sError));
		}

	//	Set controls

	for (i = 0; i < m_Controls.GetCount(); i++)
		{
		//	If we're animating, skip controls that don't need animation

		if (bAnimateOnly && !m_Controls[i].bAnimate)
			continue;

		//	Update control based on type

		switch (m_Controls[i].iType)
			{
			case ctrlImage:
				{
				CGImageArea *pControl = (CGImageArea *)m_Controls[i].pArea;

				if (m_Controls[i].pCode)
					{
					CCodeChainCtx Ctx(GetUniverse());
					Ctx.SetScreen(this);
					Ctx.DefineContainingType(m_pRoot);
					Ctx.SaveAndDefineSourceVar(m_pLocation);
					Ctx.SaveAndDefineDataVar(m_pData);

					ICCItem *pResult = Ctx.Run(m_Controls[i].pCode);	//	LATER:Event

					//	If we have an error, report it

					if (pResult->IsError())
						{
						if (!bAnimateOnly)
							{
							kernelDebugLogString(pResult->GetStringValue());
							}
						Ctx.Discard(pResult);
						break;
						}

					//	The result is the image descriptor

					CG32bitImage *pImage;
					RECT rcImage;
					GetImageDescFromList(pResult, &pImage, &rcImage);
					if (pImage)
						pControl->SetImage(pImage, rcImage);

					//	Done

					Ctx.Discard(pResult);
					}

				break;
				}

			case ctrlCanvas:
				{
				CGDrawArea *pControl = (CGDrawArea *)m_Controls[i].pArea;

				if (m_Controls[i].pCode)
					{
					CCodeChainCtx Ctx(GetUniverse());
					Ctx.SetScreen(this);
					Ctx.DefineContainingType(m_pRoot);
					Ctx.SaveAndDefineSourceVar(m_pLocation);
					Ctx.SaveAndDefineDataVar(m_pData);
					CG32bitImage *pCanvas = &pControl->GetCanvas();
					Ctx.SetCanvas(pCanvas);

					//	Erase to full transparency

					pCanvas->Set(CG32bitPixel::Null());

					//	Run the code to paint on the canvas

					ICCItem *pResult = Ctx.Run(m_Controls[i].pCode);	//	LATER:Event

					//	If we have an error, report it

					if (pResult->IsError())
						{
						if (!bAnimateOnly)
							{
							kernelDebugLogString(pResult->GetStringValue());
							}

						Ctx.Discard(pResult);
						break;
						}

					//	Done

					Ctx.Discard(pResult);
					}

				break;
				}

			case ctrlNeurohack:
				{
				CGNeurohackArea *pControl = (CGNeurohackArea *)m_Controls[i].pArea;

				if (m_Controls[i].pCode)
					{
					CCodeChainCtx Ctx(GetUniverse());
					Ctx.SetScreen(this);
					Ctx.DefineContainingType(m_pRoot);
					Ctx.SaveAndDefineSourceVar(m_pLocation);
					Ctx.SaveAndDefineDataVar(m_pData);

					ICCItem *pResult = Ctx.Run(m_Controls[i].pCode);	//	LATER:Event

					//	If we have an error, report it

					if (pResult->IsError())
						{
						if (!bAnimateOnly)
							{
							kernelDebugLogString(pResult->GetStringValue());
							}

						Ctx.Discard(pResult);
						break;
						}

					//	The result is the data descriptor in this order:
					//
					//	Willpower
					//	Damage

					int iWillpower = (pResult->GetCount() > 0 ? pResult->GetElement(0)->GetIntegerValue() : 0);
					int iDamage = (pResult->GetCount() > 1 ? pResult->GetElement(1)->GetIntegerValue() : 0);
					pControl->SetData(iWillpower, iDamage);

					//	Done

					Ctx.Discard(pResult);
					}

				break;
				}

			case ctrlText:
				{
				CGTextArea *pControl = (CGTextArea *)m_Controls[i].pArea;

				if (m_Controls[i].pCode)
					{
					CCodeChainCtx Ctx(GetUniverse());
					Ctx.SetScreen(this);
					Ctx.DefineContainingType(m_pRoot);
					Ctx.SaveAndDefineSourceVar(m_pLocation);
					Ctx.SaveAndDefineDataVar(m_pData);

					ICCItem *pResult = Ctx.Run(m_Controls[i].pCode);	//	LATER:Event

					//	The result is the text for the control

					CUIHelper UIHelper(*g_pHI);
					CString sRTF;
					UIHelper.GenerateDockScreenRTF(pResult->GetStringValue(), &sRTF);
					pControl->SetRichText(sRTF);

					//	If we have an error, report it as well

					if (pResult->IsError())
						{
						if (!bAnimateOnly)
							{
							kernelDebugLogString(pResult->GetStringValue());
							}
						}

					//	Done

					Ctx.Discard(pResult);
					}

				break;
				}
			}
		}
	}

void CDockScreen::SetBackground (const IDockScreenDisplay::SBackgroundDesc &Desc)

//	SetBackground
//
//	Sets the dock screen background

	{
	DEBUG_TRY

	//	If we haven't yet initialized the screen (e.g., we're inside of
	//	OnScreenInit) then we need to defer this.

	if (m_pScreen == NULL)
		{
		m_DeferredBackground = Desc;
		return;
		}

	//	Use a default, if necessary

	if (Desc.iType == IDockScreenDisplay::backgroundDefault)
		{
		IDockScreenDisplay::SBackgroundDesc DefaultDesc;

		if (DefaultDesc.dwImageID = m_pLocation->GetDefaultBkgnd())
			DefaultDesc.iType = IDockScreenDisplay::backgroundImage;
		else
			{
			DefaultDesc.pObj = m_pLocation;
            if (m_pLocation->IsPlayer())
			    DefaultDesc.iType = IDockScreenDisplay::backgroundObjSchematicImage;
            else
			    DefaultDesc.iType = IDockScreenDisplay::backgroundObjHeroImage;
			}

		CreateBackgroundImage(DefaultDesc, m_rcBackground, m_rcScreen.left - m_rcBackground.left);
		}

	//	Otherwise, create the image with the given descriptor

	else
		CreateBackgroundImage(Desc, m_rcBackground, m_rcScreen.left - m_rcBackground.left);

	//	Create the area

	if (m_pBackgroundImage)
		{
		//	Delete any previous image area, if necessary

		m_pScreen->DestroyArea(IMAGE_AREA_ID);

		//	Add the background

		RECT rcBackArea;
		CGImageArea *pImage = new CGImageArea;

		RECT rcImage;
		rcImage.left = 0;
		rcImage.top = 0;
		rcImage.right = m_pBackgroundImage->GetWidth();
		rcImage.bottom = m_pBackgroundImage->GetHeight();
		pImage->SetImage(m_pBackgroundImage, rcImage);
        pImage->SetTransBackground(true);

		rcBackArea.left = m_rcBackground.left;
		rcBackArea.top = m_yDisplay;
		rcBackArea.right = rcBackArea.left + RectWidth(m_rcBackground);
		rcBackArea.bottom = rcBackArea.top + m_pBackgroundImage->GetHeight();

		//	bSendToBack = true because we may have created other areas before 
		//	this and we need the background to be in back.

		m_pScreen->AddArea(pImage, rcBackArea, IMAGE_AREA_ID, true);
		}

	DEBUG_CATCH
	}

ALERROR CDockScreen::SetDisplayText (const CString &sID, const CString &sText)

//	SetDisplayText
//
//	Sets the text for a display control

	{
	//	If the screen is not yet initialized, then we need to defer the setting
	//	until later.

	if (m_pScreen == NULL)
		{
		m_DeferredDisplayText.SetAt(sID, sText);
		return NOERROR;
		}

	//	Set the value

	SDisplayControl *pControl = FindDisplayControl(sID);
	if (pControl == NULL || pControl->pArea == NULL)
		return ERR_FAIL;

	if (pControl->iType != ctrlText)
		return ERR_FAIL;

	CGTextArea *pTextControl = (CGTextArea *)pControl->pArea;

	CUIHelper UIHelper(*g_pHI);
	CString sRTF;
	UIHelper.GenerateDockScreenRTF(CLanguage::Compose(sText, m_pData), &sRTF);
	pTextControl->SetRichText(sRTF);

	//	If we're explicitly setting the text, then we cannot animate

	pControl->bAnimate = false;

	return NOERROR;
	}

void CDockScreen::SetListCursor (int iCursor)

//	SetListCursor
//
//	Sets the list cursor

	{
	if (m_pDisplay->SetListCursor(iCursor) == IDockScreenDisplay::resultShowPane)
		m_CurrentPane.ExecuteShowPane(EvalInitialPane());
	}

void CDockScreen::SetListFilter (const CItemCriteria &Filter)

//	SetListFilter
//
//	Filters the list given the criteria

	{
	if (m_pDisplay->SetListFilter(Filter) == IDockScreenDisplay::resultShowPane)
		m_CurrentPane.ExecuteShowPane(EvalInitialPane());
	}

void CDockScreen::SetLocation (CSpaceObject *pLocation)

//	SetLocation
//
//	Sets the location

	{
	m_pLocation = pLocation;
	if (m_pDisplay)
		{
		if (m_pDisplay->SetLocation(pLocation) == IDockScreenDisplay::resultShowPane)
			m_CurrentPane.ExecuteShowPane(EvalInitialPane());
		}
	}

void CDockScreen::ShowPane (const CString &sName)

//	ShowPane
//
//	Shows the pane of the given name

	{
	DEBUG_TRY

	const CVisualPalette &VI = g_pHI->GetVisuals();

#ifdef DEBUG_STRING_LEAKS
	CString::DebugMark();
#endif

	//	If sName is blank, then it is likely due to EvalInitialPane failing

	if (sName.IsBlank())
		return;

	//	Update the display. We need to do this BEFORE we call OnPaneInit
	//	because we want OnPaneInit to be able to set values on the display.

	if (m_Controls.GetCount() > 0)
		ShowDisplay();

	//	Find the pane named

	CXMLElement *pNewPane;

	//	If our root is a dockscreen, then we ask it for a pane (this will also
	//	check for a pane in an ancestor).

	CDockScreenType *pRootDockScreen = CDockScreenType::AsType(m_pRoot);
	if (pRootDockScreen)
		pNewPane = pRootDockScreen->GetPane(sName);

	//	Otherwise, we ask our local pane list

	else
		pNewPane = m_pPanes->GetContentElementByTag(sName);

	//	Error if we did not find a pane.

	if (pNewPane == NULL)
		{
		ReportError(strPatternSubst(CONSTLIT("Unable to find pane: %s"), sName));
		return;
		}

	//	Update the source list before we initialize

	CSpaceObject *pLocation = m_pDisplay->GetSource();
	if (pLocation)
		pLocation->UpdateArmorItems();

	//	Initialize the pane based on the pane descriptor

	RECT rcPane;
	rcPane.left = m_rcScreen.left;
	rcPane.right = m_rcScreen.right;
	rcPane.top = m_yDisplay;
	rcPane.bottom = m_rcScreen.bottom;

	m_CurrentPane.InitPane(this, pNewPane, rcPane);

	//	Update screen
	//	Show the currently selected item

	m_pDisplay->ShowPane(pNewPane->GetAttributeBool(NO_LIST_NAVIGATION_ATTRIB));

	UpdateCredits();

#ifdef DEBUG_STRING_LEAKS
	CString::DebugOutputLeakedStrings();
#endif

	DEBUG_CATCH
	}

void CDockScreen::SelectNextItem (bool *retbMore)

//	SelectNextItem
//
//	Selects the next item in the list

	{
	bool bMore = m_pDisplay->SelectNextItem();
	if (retbMore)
		*retbMore = bMore;
	}

void CDockScreen::SelectPrevItem (bool *retbMore)

//	SelectPrevItem
//
//	Selects the previous item in the list

	{
	bool bMore = m_pDisplay->SelectPrevItem();
	if (retbMore)
		*retbMore = bMore;
	}

bool CDockScreen::SelectTab (const CString &sID)

//	SelectTab
//
//	Selects the given tab.

	{
	const SScreenSetTab *pTabInfo = g_pTrans->GetModel().GetScreenStack().FindTab(sID);
	if (pTabInfo == NULL)
		return false;

	CString sError;
	SShowScreenCtx Ctx;
	Ctx.sScreen = pTabInfo->sScreen;
	Ctx.sPane = pTabInfo->sPane;
	Ctx.pData = pTabInfo->pData;
	Ctx.sTab = sID;

	if (g_pTrans->GetModel().ShowScreen(Ctx, &sError) != NOERROR)
		{
		ReportError(sError);
		return false;
		}

	return true;
	}

bool CDockScreen::Translate (const CString &sTextID, ICCItem *pData, ICCItemPtr &pResult)

//	Translate
//
//	Translate text

	{
	return g_pTrans->GetModel().ScreenTranslate(sTextID, pData, pResult);
	}

void CDockScreen::Update (int iTick)

//	Update
//
//	Updates the display

	{
	DEBUG_TRY

	if (m_pScreen)
		m_pScreen->Update();

	if (m_bDisplayAnimate && (iTick % 10) == 0)
		{
		ShowDisplay(true);
		}

	//	Call OnScreenUpdate every 15 ticks

	if (m_pOnScreenUpdate
			&& ((iTick % ON_SCREEN_UPDATE_CYCLE) == 0))
		{
		//	Add a reference to m_pOnScreenUpdate because we might
		//	reinitialize the screen inside the call.

		ICCItem *pCode = m_pOnScreenUpdate->Reference();

		//	Execute

		CCodeChainCtx Ctx(GetUniverse());
		Ctx.SetScreen(this);
		Ctx.DefineContainingType(m_pRoot);
		Ctx.SaveAndDefineSourceVar(m_pLocation);
		Ctx.SaveAndDefineDataVar(m_pData);

		ICCItem *pResult = Ctx.Run(pCode);

		if (pResult->IsError())
			kernelDebugLogPattern("<OnScreenUpdate>: %s", pResult->GetStringValue());

		Ctx.Discard(pResult);
		Ctx.Discard(pCode);
		}

	DEBUG_CATCH
	}

void CDockScreen::UpdateCredits (void)

//	UpdateCredits
//
//	Updates the display of credits

	{
	//	Money

	const CEconomyType *pEconomy = m_pLocation->GetDefaultEconomy();
	m_pCredits->SetText(strPatternSubst(CONSTLIT("%s: %s"), 
			strCapitalize(pEconomy->GetCurrencyNamePlural()),
			strFormatInteger((int)m_pPlayer->GetCredits(pEconomy->GetUNID()), -1, FORMAT_THOUSAND_SEPARATOR)
			));

	//	Cargo space
	Metric rCargoSpace = m_pPlayer->GetShip()->GetCargoSpaceLeft();
	if(rCargoSpace == 1.0)
		m_pCargoSpace->SetText(CONSTLIT("1 ton"));
	else
		{
		int iCargoTons = (int) rCargoSpace;
		int iCargoKg = (int) ((rCargoSpace - iCargoTons) * 1000);		//The kg left after taking the tons
		if(iCargoTons > 0)
			m_pCargoSpace->SetText(strPatternSubst("%d.%d tons", iCargoTons, iCargoKg / 100));			//Truncate kg to one decimal
		else
			m_pCargoSpace->SetText(strPatternSubst("%d kg", iCargoKg));
		}
	}
