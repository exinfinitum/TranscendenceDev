//	CHoverDescriptionPainter.cpp
//
//	CHoverDescriptionPainter class
//	Copyright (c) 2016 by Kronosaur Productions, LLC. All Rights Reserved.

#include "stdafx.h"

const int DEFAULT_BORDER_RADIUS =					4;
const int DEFAULT_LINE_SPACING =					0;
const int DEFAULT_PADDING_BOTTOM =					8;
const int DEFAULT_PADDING_LEFT =					8;
const int DEFAULT_PADDING_TOP =						8;
const int DEFAULT_PADDING_RIGHT =					8;

CHoverDescriptionPainter::CHoverDescriptionPainter (const CVisualPalette &VI) :
		m_VI(VI),
		m_cxWidth(0),
		m_rcRect({ 0, 0, 0, 0 }),
		m_rgbBack(VI.GetColor(colorAreaDialog)),
		m_rgbTitle(VI.GetColor(colorTextHighlight)),
		m_rgbDescription(VI.GetColor(colorTextNormal))

	//	CHoverDescriptionPainter constructor

	{
	}

void CHoverDescriptionPainter::FormatText (void) const

//	FormatText
//
//	Make sure text is formatted. Initializes:
//
//		m_DescriptionRTF
//		m_rcRect
//		m_rcText

	{
	//	If we're already valid, or we're invisible, then exit

	if (!IsInvalid()
			|| !IsVisible())
		return;

	//	Compute the inner width
	
	int cxContent = m_cxWidth - (DEFAULT_PADDING_LEFT + DEFAULT_PADDING_RIGHT);

	//	Format the RTF description

	if (!m_sDescription.IsBlank())
		{
		SBlockFormatDesc BlockFormat;

		BlockFormat.cxWidth = cxContent;
		BlockFormat.cyHeight = -1;
		BlockFormat.iHorzAlign = alignLeft;
		BlockFormat.iVertAlign = alignTop;
		BlockFormat.iExtraLineSpacing = DEFAULT_LINE_SPACING;

		BlockFormat.DefaultFormat.rgbColor = m_rgbDescription;
		BlockFormat.DefaultFormat.pFont = &m_VI.GetFont(fontMedium);

		m_DescriptionRTF.InitFromRTF(m_sDescription, m_VI, BlockFormat);
		}
	else
		m_DescriptionRTF.DeleteAll();

	//	Compute the height of the content

	int cyContent = 0;

	//	Title

	if (!m_sTitle.IsBlank())
		cyContent += m_VI.GetFont(fontHeader).GetHeight();

	//	Description

	if (!m_DescriptionRTF.IsEmpty())
		{
		RECT rcBounds;
		m_DescriptionRTF.GetBounds(&rcBounds);
		cyContent += RectHeight(rcBounds);
		}

	//	Figure out the position of the outer and inner rects based on our width
	//	and height.

	InitRects(m_cxWidth, DEFAULT_PADDING_TOP + cyContent + DEFAULT_PADDING_BOTTOM);
	}

void CHoverDescriptionPainter::InitRects (int cxWidth, int cyHeight) const

//	InitRects
//
//	Initializes m_rcRect and m_rcText based on the given metrics. We make sure 
//	that the rect is inside m_rcContainer.

	{
	//	Figure out the position of the rect. Start at the desired position.

	m_rcRect.left = m_xPos;
	m_rcRect.top = m_yPos;

	//	Make sure we fit horizontally

	if (m_rcRect.left < m_rcContainer.left)
		m_rcRect.left = m_rcContainer.left;
	else if (m_rcRect.left + cxWidth > m_rcContainer.right)
		m_rcRect.left = Max(m_rcContainer.left, m_rcContainer.right - cxWidth);

	//	If we don't fit below the position, and we can fit above it, adjust

	if (m_rcRect.top + cyHeight > m_rcContainer.bottom
			&& m_rcRect.top - cyHeight >= m_rcContainer.top)
		m_rcRect.top = m_rcRect.top - cyHeight;

	//	Complete the rect

	m_rcRect.right = m_rcRect.left + cxWidth;
	m_rcRect.bottom = m_rcRect.top + cyHeight;

	//	Now initialize the inner text rect

	m_rcText.left = m_rcRect.left + DEFAULT_PADDING_LEFT;
	m_rcText.top = m_rcRect.top + DEFAULT_PADDING_TOP;
	m_rcText.right = m_rcRect.right - DEFAULT_PADDING_RIGHT;
	m_rcText.bottom = m_rcRect.bottom - DEFAULT_PADDING_BOTTOM;
	}

void CHoverDescriptionPainter::Paint (CG32bitImage &Dest) const

//	Paint
//
//	Paint the description

	{
	if (!IsVisible())
		return;

	//	Paint based on our type

	if (!m_Item.IsEmpty())
		PaintItem(Dest);
	else
		PaintText(Dest);
	}

void CHoverDescriptionPainter::PaintItem (CG32bitImage &Dest) const

//	PaintItem
//
//	Paint an item description

	{
	CItemPainter Painter(g_pHI->GetVisuals());

	CItemPainter::SOptions Options;
	Options.bNoIcon = true;
	Options.bTitle = true;
	Options.bNoPadding = true;

	Painter.Init(m_Item, m_cxWidth - (DEFAULT_PADDING_LEFT + DEFAULT_PADDING_RIGHT), Options);

	//	Figure out the position of the outer and inner rects based on our width
	//	and height.

	InitRects(m_cxWidth, DEFAULT_PADDING_TOP + Painter.GetHeight() + DEFAULT_PADDING_BOTTOM);

	//	Paint the background

	CGDraw::RoundedRect(Dest, m_rcRect.left, m_rcRect.top, RectWidth(m_rcRect), RectHeight(m_rcRect), DEFAULT_BORDER_RADIUS, m_rgbBack);

	//	Paint the item

	Painter.Paint(Dest, m_rcText.left, m_rcText.top, m_rgbTitle);
	}

void CHoverDescriptionPainter::PaintText (CG32bitImage &Dest) const

//	PaintText
//
//	Paint the description

	{
	//	Format, if necessary

	FormatText();

	//	Paint the background

	CGDraw::RoundedRect(Dest, m_rcRect.left, m_rcRect.top, RectWidth(m_rcRect), RectHeight(m_rcRect), DEFAULT_BORDER_RADIUS, m_rgbBack);

	//	Paint the title

	int y = m_rcText.top;
	if (!m_sTitle.IsBlank())
		{
		m_VI.GetFont(fontHeader).DrawText(Dest, m_rcText, m_rgbTitle, m_sTitle, 0, CG16bitFont::TruncateLine);

		y += m_VI.GetFont(fontHeader).GetHeight();
		}

	//	Paint the description

	m_DescriptionRTF.Paint(Dest, m_rcText.left, y);
	}

void CHoverDescriptionPainter::SetDescription (const CString &sValue)

//	SetDescription
//
//	Sets the description (which may be RTF).

	{
	CUIHelper UIHelper(*g_pHI);
	UIHelper.GenerateDockScreenRTF(sValue, &m_sDescription);

	m_Item = CItem();
	Invalidate();
	}

void CHoverDescriptionPainter::Show (int x, int y, int cxWidth, const RECT &rcContainer)

//	Show
//
//	Show the description with the given settings.

	{
	//	Width must not be zero.

	if (cxWidth <= (DEFAULT_PADDING_LEFT + DEFAULT_PADDING_RIGHT))
		return;

	m_xPos = x;
	m_yPos = y;
	m_cxWidth = cxWidth;
	m_rcContainer = rcContainer;

	Invalidate();
	}
