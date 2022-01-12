/***********************************************************************
    created:    Tue Jul 5 2005
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2006 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/
#include "CEGUI/WindowRendererSets/Core/StaticText.h"
#include "CEGUI/falagard/WidgetLookFeel.h"
#include "CEGUI/falagard/XMLEnumHelper.h"
#include "CEGUI/widgets/Scrollbar.h"
#include "CEGUI/Font.h"
#include "CEGUI/LeftAlignedRenderedString.h"
#include "CEGUI/RightAlignedRenderedString.h"
#include "CEGUI/CentredRenderedString.h"
#include "CEGUI/RenderedStringWordWrapper.h"
#include "CEGUI/TplWindowRendererProperty.h"
#include "CEGUI/CoordConverter.h"

namespace CEGUI
{
const String FalagardStaticText::TypeName("Core/StaticText");
const String FalagardStaticText::VertScrollbarName( "__auto_vscrollbar__" );
const String FalagardStaticText::HorzScrollbarName( "__auto_hscrollbar__" );
const String PropertyHelper<FalagardStaticText::NumOfTextLinesToShow>::s_autoString("Auto");

//----------------------------------------------------------------------------//
FalagardStaticText::FalagardStaticText(const String& type)
    : FalagardStatic(type)
{
    CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY(FalagardStaticText, ColourRect,
        "TextColours", "Property to get/set the text colours for the FalagardStaticText widget."
        "  Value is \"tl:[aarrggbb] tr:[aarrggbb] bl:[aarrggbb] br:[aarrggbb]\".",
        &FalagardStaticText::setTextColours, &FalagardStaticText::getTextColours,
        ColourRect(0xFFFFFFFF));

    CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY(FalagardStaticText, HorizontalTextFormatting,
        "HorzFormatting", "Property to get/set the horizontal formatting mode."
        "  Value is one of the HorzFormatting strings.",
        &FalagardStaticText::setHorizontalFormatting, &FalagardStaticText::getHorizontalFormatting,
        HorizontalTextFormatting::LeftAligned);

    CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY(FalagardStaticText, VerticalTextFormatting,
        "VertFormatting", "Property to get/set the vertical formatting mode."
        "  Value is one of the VertFormatting strings.",
        &FalagardStaticText::setVerticalFormatting, &FalagardStaticText::getVerticalFormatting,
        VerticalTextFormatting::CentreAligned);

    CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY(FalagardStaticText, bool,
        "VertScrollbar", "Property to get/set the setting for the vertical scroll bar."
        "  Value is either \"true\" or \"false\".",
        &FalagardStaticText::setVerticalScrollbarEnabled, &FalagardStaticText::isVerticalScrollbarEnabled,
        false);

    CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY(FalagardStaticText, bool,
        "HorzScrollbar", "Property to get/set the setting for the horizontal scroll bar."
        "  Value is either \"true\" or \"false\".",
        &FalagardStaticText::setHorizontalScrollbarEnabled, &FalagardStaticText::isHorizontalScrollbarEnabled,
        false);

    CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY(FalagardStaticText, float,
        "HorzExtent", "Property to get the current horizontal extent of the formatted text string."
        "  Value is a float indicating the pixel extent.",
        nullptr, &FalagardStaticText::getHorizontalTextExtent,
        0);

    CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY(FalagardStaticText, float,
        "VertExtent", "Property to get the current vertical extent of the formatted text string."
        "  Value is a float indicating the pixel extent.",
        nullptr, &FalagardStaticText::getVerticalTextExtent,
        0);

    CEGUI_DEFINE_WINDOW_RENDERER_PROPERTY(FalagardStaticText, NumOfTextLinesToShow,
        "NumOfTextLinesToShow", "Property to get/set the number of text lines to use to compute the content height"
        "of the widget. Useful in conjunction with \"AdjustHeightToContent\".",
        &FalagardStaticText::setNumOfTextLinesToShow, &FalagardStaticText::getNumOfTextLinesToShow,
        FalagardStaticText::NumOfTextLinesToShow());
}

//----------------------------------------------------------------------------//
FalagardStaticText::~FalagardStaticText() = default;

//----------------------------------------------------------------------------//
void FalagardStaticText::createRenderGeometry()
{
    // Create common geometry for Static
    FalagardStatic::createRenderGeometry();

    updateFormatting();

    // get destination area for the text.
    const Rectf clipper(getTextRenderArea());
    Rectf absarea(clipper);

    // see if we may need to adjust horizontal position
    const Scrollbar* const horzScrollbar = getHorzScrollbar();
    if (horzScrollbar->isEffectiveVisible())
    {
        const float range = horzScrollbar->getDocumentSize() -
            horzScrollbar->getPageSize();

        switch (d_actualHorzFormatting)
        {
            case HorizontalTextFormatting::LeftAligned:
            case HorizontalTextFormatting::WordWrapLeftAligned:
            case HorizontalTextFormatting::Justified:
            case HorizontalTextFormatting::WordWraperJustified:
                absarea.offset(glm::vec2(-horzScrollbar->getScrollPosition(), 0));
                break;

            case HorizontalTextFormatting::CentreAligned:
            case HorizontalTextFormatting::WordWrapCentreAligned:
                absarea.setWidth(horzScrollbar->getDocumentSize());
                absarea.offset(glm::vec2(range / 2 - horzScrollbar->getScrollPosition(), 0));
                break;

            case HorizontalTextFormatting::RightAligned:
            case HorizontalTextFormatting::WordWrapRightAligned:
                absarea.offset(glm::vec2(range - horzScrollbar->getScrollPosition(), 0));
                break;
            default:
                throw InvalidRequestException("Invalid actual horizontal text formatting.");
        }
    }

    // adjust y positioning according to formatting option
    float textHeight = d_formatter->getVerticalExtent(d_window);
    const Scrollbar* const vertScrollbar = getVertScrollbar();
    const float vertScrollPosition = vertScrollbar->getScrollPosition();
    // if scroll bar is in use, position according to that.
    if (vertScrollbar->isEffectiveVisible())
        absarea.d_min.y -= vertScrollPosition;
    // no scrollbar, so adjust position according to formatting set.
    else
        switch (getActualVerticalFormatting())
        {
            case VerticalTextFormatting::CentreAligned:
                absarea.d_min.y += CoordConverter::alignToPixels((absarea.getHeight() - textHeight) * 0.5f);
                break;
            case VerticalTextFormatting::BottomAligned:
                absarea.d_min.y = absarea.d_max.y - textHeight;
                break;
            case VerticalTextFormatting::TopAligned:
                break;
            default:
                throw InvalidRequestException("Invalid actual vertical text formatting.");
        }

    // calculate final colours
    const ColourRect final_cols(d_textCols);
    // cache the text for rendering.
    std::vector<GeometryBuffer*> geomBuffers = d_formatter->createRenderGeometry(
        d_window,
        absarea.getPosition(),
        &final_cols, &clipper);

    d_window->appendGeometryBuffers(geomBuffers);
}

//----------------------------------------------------------------------------//
void FalagardStaticText::onIsFrameEnabledChanged()
{
    FalagardStatic::onIsFrameEnabledChanged();
    invalidateFormatting();
    d_window->adjustSizeToContent();
}

//----------------------------------------------------------------------------//
bool FalagardStaticText::isWordWrapOn() const
{
    switch (d_horzFormatting)
    {
        case HorizontalTextFormatting::LeftAligned:
        case HorizontalTextFormatting::RightAligned:
        case HorizontalTextFormatting::CentreAligned:
        case HorizontalTextFormatting::Justified:
            return false;
        case HorizontalTextFormatting::WordWrapLeftAligned:
        case HorizontalTextFormatting::WordWrapRightAligned:
        case HorizontalTextFormatting::WordWrapCentreAligned:
        case HorizontalTextFormatting::WordWraperJustified:
            return true;
        default:
            throw InvalidRequestException("Invalid horizontal formatting.");
    }
}

//----------------------------------------------------------------------------//
std::size_t FalagardStaticText::getNumOfOriginalTextLines() const
{
    return d_formatter->getNumOfOriginalTextLines();
}

//----------------------------------------------------------------------------//
std::size_t FalagardStaticText::getNumOfFormattedTextLines() const
{
    return d_formatter->getNumOfFormattedTextLines();
}

//----------------------------------------------------------------------------//
float FalagardStaticText::getContentWidth() const
{
    return d_formatter->getHorizontalExtent(d_window);
}

//----------------------------------------------------------------------------//
float FalagardStaticText::getContentHeight() const
{
    if (d_numOfTextLinesToShow.isAuto())
        return d_formatter->getVerticalExtent(d_window) + 1.f;
    if (d_numOfTextLinesToShow <= 1.f)
        return getLineHeight() * d_numOfTextLinesToShow;
    return getLineHeight() + (d_numOfTextLinesToShow - 1.f) * getVerticalAdvance();
}

//----------------------------------------------------------------------------//
UDim FalagardStaticText::getWidthOfAreaReservedForContentLowerBoundAsFuncOfWindowWidth() const
{
    return getTextComponentArea().getWidthLowerBoundAsFuncOfWindowWidth(*d_window);
}

//----------------------------------------------------------------------------//
UDim FalagardStaticText::getHeightOfAreaReservedForContentLowerBoundAsFuncOfWindowHeight() const
{
    return getTextComponentArea().getHeightLowerBoundAsFuncOfWindowHeight(*d_window);
}

//----------------------------------------------------------------------------//
void FalagardStaticText::adjustSizeToContent()
{
    const float epsilon = d_window->adjustSizeToContent_getEpsilon();
    getHorzScrollbarWithoutUpdate()->hide();
    getVertScrollbarWithoutUpdate()->hide();
    if (isWordWrapOn())
    {
        const LeftAlignedRenderedString orig_str(d_formatter->getRenderedString());
        USize sizeFunc(
            d_window->getElementWidthLowerBoundAsFuncOfWidthOfAreaReservedForContent(),
            d_window->getElementHeightLowerBoundAsFuncOfHeightOfAreaReservedForContent());
        float contentMaxWidth(orig_str.getHorizontalExtent(d_window));
        float windowMaxWidth((contentMaxWidth + epsilon) * sizeFunc.d_width.d_scale + sizeFunc.d_width.d_offset);
        if (isSizeAdjustedToContentKeepingAspectRatio())
        {
            adjustSizeToContent_wordWrap_keepingAspectRatio(orig_str, sizeFunc, contentMaxWidth, windowMaxWidth, epsilon);
            return;
        }
        if (d_window->isWidthAdjustedToContent())
        {
            adjustSizeToContent_wordWrap_notKeepingAspectRatio(sizeFunc, contentMaxWidth, windowMaxWidth, epsilon);
            return;
        }
    }
    adjustSizeToContent_direct();
}

//----------------------------------------------------------------------------//
bool FalagardStaticText::isSizeAdjustedToContentKeepingAspectRatio() const
{
    if (!(d_window->isSizeAdjustedToContent() && isWordWrapOn()))
        return false;
    if (d_numOfTextLinesToShow.isAuto())
        return (d_window->isWidthAdjustedToContent() &&
            d_window->isHeightAdjustedToContent())  ||
               (d_window->getAspectMode() != AspectMode::Ignore);
    return d_window->isWidthAdjustedToContent()  &&
           !d_window->isHeightAdjustedToContent()  &&
           (d_window->getAspectMode() != AspectMode::Ignore);
}

//----------------------------------------------------------------------------//
bool FalagardStaticText::contentFitsForSpecifiedWindowSize(const Sizef& window_size) const
{
    return d_window->contentFitsForSpecifiedElementSize_tryByResizing(window_size);
}

//----------------------------------------------------------------------------//
bool FalagardStaticText::contentFits() const
{
    // Scrollbars are configured inside
    const Sizef contentSize(getDocumentSize());

    if (getHorzScrollbar()->isVisible() || getVertScrollbar()->isVisible())
        return false;

    const Rectf area(getTextRenderArea());
    return !d_formatter->wasWordSplit() &&
      contentSize.d_width <= area.getWidth() &&
      contentSize.d_height <= area.getHeight();
}

//------------------------------------------------------------------------//
void FalagardStaticText::invalidateFormatting()
{
    d_formatValid = false;
    d_window->invalidate();
}

//------------------------------------------------------------------------//
Scrollbar* FalagardStaticText::getVertScrollbar() const
{
    updateFormatting();
    return getVertScrollbarWithoutUpdate();
}

//------------------------------------------------------------------------//
Scrollbar* FalagardStaticText::getHorzScrollbar() const
{
    updateFormatting();
    return getHorzScrollbarWithoutUpdate();
}

//------------------------------------------------------------------------//
Rectf FalagardStaticText::getTextRenderArea() const
{
    updateFormatting();
    return getTextRenderAreaWithoutUpdate();
}

//----------------------------------------------------------------------------//
const ComponentArea& FalagardStaticText::getTextComponentArea() const
{
    updateFormatting();
    return getTextComponentAreaWithoutUpdate();
}

//------------------------------------------------------------------------//
Sizef FalagardStaticText::getDocumentSize() const
{
    updateFormatting();
    return getDocumentSizeWithoutUpdate();
}

//------------------------------------------------------------------------//
void FalagardStaticText::setTextColours(const ColourRect& colours)
{
    d_textCols = colours;
    d_window->invalidate();
}

//------------------------------------------------------------------------//
void FalagardStaticText::setHorizontalFormatting(HorizontalTextFormatting h_fmt)
{
    if (h_fmt == d_horzFormatting)
        return;

    d_horzFormatting = h_fmt;
    invalidateFormatting();
    d_window->adjustSizeToContent();
}

//------------------------------------------------------------------------//
void FalagardStaticText::setVerticalFormatting(VerticalTextFormatting v_fmt)
{
    if (d_vertFormatting == v_fmt)
        return;

    d_vertFormatting = v_fmt;
    invalidateFormatting();
    d_window->adjustSizeToContent();
}

//----------------------------------------------------------------------------//
void FalagardStaticText::setNumOfTextLinesToShow(NumOfTextLinesToShow newValue)
{
    if (d_numOfTextLinesToShow == newValue)
        return;

    d_numOfTextLinesToShow = newValue;
    invalidateFormatting();
    d_window->adjustSizeToContent();
}

//----------------------------------------------------------------------------//
void FalagardStaticText::setVerticalScrollbarEnabled(bool setting)
{
    if (d_enableVertScrollbar == setting)
        return;

    d_enableVertScrollbar = setting;
    invalidateFormatting();
    d_window->adjustSizeToContent();
}

//----------------------------------------------------------------------------//
void FalagardStaticText::setHorizontalScrollbarEnabled(bool setting)
{
    if (d_enableHorzScrollbar == setting)
        return;

    d_enableHorzScrollbar = setting;
    invalidateFormatting();
    d_window->adjustSizeToContent();
}

//----------------------------------------------------------------------------//
// Update string formatting and horizontal and vertical scrollbar visibility.
// This may require repeating a process several times because showing one of
// the scrollbars shrinks the area reserved for the text, and thus may require
// reformatting of the string, as well as cause the 2nd scrollbar to also be required.
void FalagardStaticText::configureScrollbars() const
{
    Scrollbar* vertScrollbar = getVertScrollbarWithoutUpdate();
    Scrollbar* horzScrollbar = getHorzScrollbarWithoutUpdate();
    vertScrollbar->hide();
    horzScrollbar->hide();

    Rectf renderArea(getTextRenderAreaWithoutUpdate());
    Sizef renderAreaSize(renderArea.getSize());
    d_formatter->format(d_window, renderAreaSize);
    Sizef documentSize(getDocumentSizeWithoutUpdate());

    bool showVert = d_enableVertScrollbar && (documentSize.d_height > renderAreaSize.d_height);
    bool showHorz = d_enableHorzScrollbar && (documentSize.d_width > renderAreaSize.d_width);
    vertScrollbar->setVisible(showVert);
    horzScrollbar->setVisible(showHorz);

    Rectf updatedRenderArea = getTextRenderAreaWithoutUpdate();
    if (renderArea != updatedRenderArea)
    {
        renderArea = updatedRenderArea;
        renderAreaSize = renderArea.getSize();
        d_formatter->format(d_window, renderAreaSize);
        documentSize = getDocumentSizeWithoutUpdate();

        showVert = d_enableVertScrollbar && (documentSize.d_height > renderAreaSize.d_height);
        showHorz = d_enableHorzScrollbar && (documentSize.d_width > renderAreaSize.d_width);
        vertScrollbar->setVisible(showVert);
        horzScrollbar->setVisible(showHorz);

        updatedRenderArea = getTextRenderAreaWithoutUpdate();
        if (renderArea != updatedRenderArea)
        {
            renderArea = updatedRenderArea;
            renderAreaSize = renderArea.getSize();
            d_formatter->format(d_window, renderAreaSize);
            documentSize = getDocumentSizeWithoutUpdate();
        }
    }

    d_window->performChildLayout(false, false);

    vertScrollbar->setDocumentSize(documentSize.d_height);
    vertScrollbar->setPageSize(renderAreaSize.d_height);
    vertScrollbar->setStepSize(std::max(1.0f, renderAreaSize.d_height / 10.0f));
    horzScrollbar->setDocumentSize(documentSize.d_width);
    horzScrollbar->setPageSize(renderAreaSize.d_width);
    horzScrollbar->setStepSize(std::max(1.0f, renderAreaSize.d_width / 10.0f));
}

//----------------------------------------------------------------------------//
bool FalagardStaticText::onTextChanged(const EventArgs&)
{
    invalidateFormatting();
    d_window->adjustSizeToContent();
    return true;
}

//----------------------------------------------------------------------------//
bool FalagardStaticText::onSized(const EventArgs&)
{
    invalidateFormatting();
    return true;
}

//----------------------------------------------------------------------------//
bool FalagardStaticText::onFontChanged(const EventArgs&)
{
    invalidateFormatting();
    d_window->adjustSizeToContent();
    return true;
}

//----------------------------------------------------------------------------//
bool FalagardStaticText::onScroll(const EventArgs& event)
{
    const CursorInputEventArgs& e = static_cast<const CursorInputEventArgs&>(event);

    Scrollbar* vertScrollbar = getVertScrollbar();
    Scrollbar* horzScrollbar = getHorzScrollbar();

    const bool vertScrollbarVisible = vertScrollbar->isEffectiveVisible();
    const bool horzScrollbarVisible = horzScrollbar->isEffectiveVisible();

    if (vertScrollbarVisible && (vertScrollbar->getDocumentSize() > vertScrollbar->getPageSize()))
        vertScrollbar->setScrollPosition(vertScrollbar->getScrollPosition() + vertScrollbar->getStepSize() * -e.scroll);
    else if (horzScrollbarVisible && (horzScrollbar->getDocumentSize() > horzScrollbar->getPageSize()))
        horzScrollbar->setScrollPosition(horzScrollbar->getScrollPosition() + horzScrollbar->getStepSize() * -e.scroll);

    return vertScrollbarVisible || horzScrollbarVisible;
}

//----------------------------------------------------------------------------//
bool FalagardStaticText::onIsSizeAdjustedToContentChanged(const EventArgs&)
{
    invalidateFormatting();
    return true;
}

//----------------------------------------------------------------------------//
bool FalagardStaticText::handleScrollbarChange(const EventArgs&)
{
    d_window->invalidate();
    return true;
}

//----------------------------------------------------------------------------//
void FalagardStaticText::onLookNFeelAssigned()
{
    // do initial scrollbar setup
    Scrollbar* vertScrollbar = getVertScrollbarWithoutUpdate();
    Scrollbar* horzScrollbar = getHorzScrollbarWithoutUpdate();

    vertScrollbar->hide();
    horzScrollbar->hide();

    // scrollbar events
    vertScrollbar->subscribeEvent(Scrollbar::EventScrollPositionChanged,
        Event::Subscriber(&FalagardStaticText::handleScrollbarChange, this));
    horzScrollbar->subscribeEvent(Scrollbar::EventScrollPositionChanged,
        Event::Subscriber(&FalagardStaticText::handleScrollbarChange, this));

    d_connections.clear();

    // events that scrollbars should react to
    d_connections.push_back(
        d_window->subscribeEvent(Window::EventTextChanged,
            Event::Subscriber(&FalagardStaticText::onTextChanged, this)));

    d_connections.push_back(
        d_window->subscribeEvent(Window::EventSized,
            Event::Subscriber(&FalagardStaticText::onSized, this)));

    d_connections.push_back(
        d_window->subscribeEvent(Window::EventFontChanged,
            Event::Subscriber(&FalagardStaticText::onFontChanged, this)));

    d_connections.push_back(
        d_window->subscribeEvent(Window::EventScroll,
            Event::Subscriber(&FalagardStaticText::onScroll, this)));

    d_connections.push_back(
        d_window->subscribeEvent(Window::EventIsSizeAdjustedToContentChanged,
            Event::Subscriber(&FalagardStaticText::onIsSizeAdjustedToContentChanged, this)));

    invalidateFormatting();
}

//----------------------------------------------------------------------------//
void FalagardStaticText::onLookNFeelUnassigned()
{
    // clean up connections that rely on widgets created by the look and feel
    d_connections.clear();
}

//----------------------------------------------------------------------------//
float FalagardStaticText::getHorizontalTextExtent() const
{
    updateFormatting();
    return d_formatter->getHorizontalExtent(d_window);
}

//----------------------------------------------------------------------------//
float FalagardStaticText::getVerticalTextExtent() const
{
    updateFormatting();
    return d_formatter->getVerticalExtent(d_window);
}

//----------------------------------------------------------------------------//
float FalagardStaticText::getHorizontalScrollPosition() const
{
    return getHorzScrollbar()->getScrollPosition();
}

//----------------------------------------------------------------------------//
float FalagardStaticText::getVerticalScrollPosition() const
{
    return getVertScrollbar()->getScrollPosition();
}

//----------------------------------------------------------------------------//
float FalagardStaticText::getUnitIntervalHorizontalScrollPosition() const
{
    return getHorzScrollbar()->getUnitIntervalScrollPosition();
}

//----------------------------------------------------------------------------//
float FalagardStaticText::getUnitIntervalVerticalScrollPosition() const
{
    return getVertScrollbar()->getUnitIntervalScrollPosition();
}

//----------------------------------------------------------------------------//
void FalagardStaticText::setHorizontalScrollPosition(float position)
{
    getHorzScrollbar()->setScrollPosition(position);
}

//----------------------------------------------------------------------------//
void FalagardStaticText::setVerticalScrollPosition(float position)
{
    getVertScrollbar()->setScrollPosition(position);
}

//----------------------------------------------------------------------------//
void FalagardStaticText::setUnitIntervalHorizontalScrollPosition(float position)
{
    getHorzScrollbar()->setUnitIntervalScrollPosition(position);
}

//----------------------------------------------------------------------------//
void FalagardStaticText::setUnitIntervalVerticalScrollPosition(float position)
{
    getVertScrollbar()->setUnitIntervalScrollPosition(position);
}

//----------------------------------------------------------------------------//
float FalagardStaticText::getLineHeight() const
{
    return d_window->getActualFont()->getFontHeight();
}

//----------------------------------------------------------------------------//
float FalagardStaticText::getVerticalAdvance() const
{
    return d_window->getActualFont()->getFontHeight();
}

//----------------------------------------------------------------------------//
void FalagardStaticText::updateFormatting() const
{
    if (d_formatValid)
        return;

    if (!d_formatter || d_actualHorzFormatting != d_horzFormatting)
    {
        d_actualHorzFormatting = d_horzFormatting;
        setupStringFormatter();
    }

    // "Touch" the window's rendered string to ensure it's re-parsed if needed.
    d_window->getRenderedString();

    d_actualVertFormatting = d_vertFormatting;

    configureScrollbars();

    if (d_window->isSizeAdjustedToContent() && !isSizeAdjustedToContentKeepingAspectRatio())
    {
        const auto lineCount = getNumOfFormattedTextLines();

        if (d_window->isWidthAdjustedToContent() && lineCount == 1)
        {
            d_actualHorzFormatting = isWordWrapOn() ? HorizontalTextFormatting::WordWrapCentreAligned : HorizontalTextFormatting::CentreAligned;
            setupStringFormatter();
            d_formatter->format(d_window, getTextRenderAreaWithoutUpdate().getSize());
        }

        if (d_window->isHeightAdjustedToContent() && (d_numOfTextLinesToShow.isAuto() || d_numOfTextLinesToShow <= lineCount))
            d_actualVertFormatting = VerticalTextFormatting::CentreAligned;
    }

    d_formatValid = true;
}

//----------------------------------------------------------------------------//
void FalagardStaticText::setupStringFormatter() const
{
    const RenderedString& renderedString = d_window->getRenderedString();
    switch (d_actualHorzFormatting)
    {
        case HorizontalTextFormatting::LeftAligned:
            d_formatter.reset(new LeftAlignedRenderedString(renderedString));
            break;

        case HorizontalTextFormatting::RightAligned:
            d_formatter.reset(new RightAlignedRenderedString(renderedString));
            break;

        case HorizontalTextFormatting::CentreAligned:
            d_formatter.reset(new CentredRenderedString(renderedString));
            break;

        case HorizontalTextFormatting::Justified:
            d_formatter.reset(new JustifiedRenderedString(renderedString));
            break;

        case HorizontalTextFormatting::WordWrapLeftAligned:
            d_formatter.reset(new RenderedStringWordWrapper<LeftAlignedRenderedString>(renderedString));
            break;

        case HorizontalTextFormatting::WordWrapRightAligned:
            d_formatter.reset(new RenderedStringWordWrapper<RightAlignedRenderedString>(renderedString));
            break;

        case HorizontalTextFormatting::WordWrapCentreAligned:
            d_formatter.reset(new RenderedStringWordWrapper<CentredRenderedString>(renderedString));
            break;

        case HorizontalTextFormatting::WordWraperJustified:
            d_formatter.reset(new RenderedStringWordWrapper<JustifiedRenderedString>(renderedString));
            break;

        default:
            d_formatter.reset();
            break;
    }
}

//----------------------------------------------------------------------------//
bool FalagardStaticText::handleFontRenderSizeChange(const Font* const font)
{
    const bool res = WindowRenderer::handleFontRenderSizeChange(font);

    if (d_window->getActualFont() == font)
    {
        invalidateFormatting();
        d_window->adjustSizeToContent();
        return true;
    }

    return res;
}

//----------------------------------------------------------------------------//
Scrollbar* FalagardStaticText::getVertScrollbarWithoutUpdate() const
{
    // return component created by look'n'feel assignment.
    return static_cast<Scrollbar*>(d_window->getChild(VertScrollbarName));
}

//----------------------------------------------------------------------------//
Scrollbar* FalagardStaticText::getHorzScrollbarWithoutUpdate() const
{
    // return component created by look'n'feel assignment.
    return static_cast<Scrollbar*>(d_window->getChild(HorzScrollbarName));
}

//----------------------------------------------------------------------------//
Rectf FalagardStaticText::getTextRenderAreaWithoutUpdate() const
{
    return getTextComponentAreaWithoutUpdate().getPixelRect(*d_window);
}

//----------------------------------------------------------------------------//
const ComponentArea& FalagardStaticText::getTextComponentAreaWithoutUpdate() const
{
    const bool v_visible = getVertScrollbarWithoutUpdate()->isVisible();
    const bool h_visible = getHorzScrollbarWithoutUpdate()->isVisible();

    // get WidgetLookFeel for the assigned look.
    const WidgetLookFeel& wlf = getLookNFeel();

    String area_name(d_frameEnabled ? "WithFrameTextRenderArea" : "NoFrameTextRenderArea");

    // if either of the scrollbars are visible, we might want to use a special rendering area
    if (v_visible || h_visible)
    {
        if (h_visible)
            area_name += 'H';
        if (v_visible)
            area_name += 'V';
        area_name += "Scroll";
    }

    if (wlf.isNamedAreaPresent(area_name))
        return wlf.getNamedArea(area_name).getArea();

    // default to plain WithFrameTextRenderArea
    return wlf.getNamedArea("WithFrameTextRenderArea").getArea();
}

//----------------------------------------------------------------------------//
Sizef FalagardStaticText::getDocumentSizeWithoutUpdate() const
{
    //!!!TODO TEXT: make one function!
    return Sizef(d_formatter->getHorizontalExtent(d_window),
                    d_formatter->getVerticalExtent(d_window));
}

/*----------------------------------------------------------------------------//
    An implementation of "adjustSizeToContent" where we adjust both the window
    width and the window height simultaneously, keeping the window's aspect
    ratio according to "d_window->getAspectRatio()".

    We do that by try-and-error, using bisection.
------------------------------------------------------------------------------*/
void FalagardStaticText::adjustSizeToContent_wordWrap_keepingAspectRatio(
    const LeftAlignedRenderedString& orig_str, USize& sizeFunc,
    float contentMaxWidth, float windowMaxWidth, float epsilon)
{
    // Start by trying height that can fit 0 text lines.
    Sizef window_size(0.f, sizeFunc.d_height.d_scale*epsilon + sizeFunc.d_height.d_offset);
    window_size.d_width = window_size.d_height * d_window->getAspectRatio();
    if (d_window->contentFitsForSpecifiedElementSize(window_size))
    {
        // It fits - so we go for that size.
        d_window->setSize(USize(UDim(0.f, window_size.d_width), UDim(0.f, window_size.d_height)));
        return;
    }

    /* It doesn't fit - so we try a positive integer number of text lines. Here
       we use try-and-error, using bisection.
       We only try heights in which we can fit exactly an integer number of text
       lines, as there's no logic in trying differently. */
    UDim height_sequence_precise(sizeFunc.d_height.d_scale*getVerticalAdvance() + sizeFunc.d_height.d_offset,
                                 sizeFunc.d_height.d_scale*getLineHeight() + sizeFunc.d_height.d_offset);
    UDim height_sequence(sizeFunc.d_height.d_scale*getVerticalAdvance() + sizeFunc.d_height.d_offset,
                         sizeFunc.d_height.d_scale*(getLineHeight()+epsilon) + sizeFunc.d_height.d_offset);
    float max_num_of_lines(std::max(
      static_cast<float>(d_formatter->getNumOfOriginalTextLines() -1),
      (windowMaxWidth / d_window->getAspectRatio() - height_sequence_precise.d_offset)
        / height_sequence_precise.d_scale));
    window_size = d_window->getSizeAdjustedToContent_bisection(
      USize(height_sequence *d_window->getAspectRatio(), height_sequence), -1.f, max_num_of_lines);
    d_window->setSize(USize(UDim(0.f, window_size.d_width), UDim(0.f, window_size.d_height)), false);

    /* It's possible that due to a too low "d_window->getMaxSize().d_height",
       we're unable to make the whole text fit without the need for a vertical
       scrollbar. In that case, we need to redo the computations, with the
       vertical scrollbar visible. We go for the maximal size that makes sense,
       which is the size of "orig_str", expanded to keep the aspect ratio
       "d_window->getAspectRatio()". */
    updateFormatting();
    if (getVertScrollbar()->isVisible())
    {
        sizeFunc.d_width = d_window->getElementWidthLowerBoundAsFuncOfWidthOfAreaReservedForContent();
        sizeFunc.d_height = d_window->getElementHeightLowerBoundAsFuncOfHeightOfAreaReservedForContent();
        window_size.d_width = (contentMaxWidth+epsilon)*sizeFunc.d_width.d_scale + sizeFunc.d_width.d_offset;
        window_size.d_height = (orig_str.getVerticalExtent(d_window)+epsilon)*sizeFunc.d_height.d_scale +
                                sizeFunc.d_height.d_offset;
        window_size.scaleToAspect(AspectMode::Expand, d_window->getAspectRatio());
        d_window->setSize(USize(UDim(0.f, window_size.d_width), UDim(0.f, window_size.d_height)), false);
    }
}

/*----------------------------------------------------------------------------//
    An implementation of "adjustSizeToContent" where we do the following:

    1) If "d_window->isHeightAdjustedToContent()" is true, adjust the height
       of the window so that the text fits in without the need for a vertical
       scrollbar. This case only happens when
       "d_numOfTextLinesToShow.isAuto()" is false, which means we know
       exactly how many text lines we want to reserve space for, regardless of
       word-wrapping.
    2) Adjust the window width by try-and-error, using bisection.
------------------------------------------------------------------------------*/
void FalagardStaticText::adjustSizeToContent_wordWrap_notKeepingAspectRatio(
    USize& sizeFunc, float contentMaxWidth, float windowMaxWidth, float epsilon)
{
    float height(d_window->isHeightAdjustedToContent()  ?
      sizeFunc.d_height.d_scale*(getContentHeight()+epsilon) + sizeFunc.d_height.d_offset  :
      d_window->getPixelSize().d_height);
    UDim height_as_u_dim(d_window->isHeightAdjustedToContent()  ?  UDim(0.f, height) : d_window->getHeight());
    float window_width(d_window->getSizeAdjustedToContent_bisection(
                         USize(UDim(1.f, 0.f), UDim(0.f, height)), -1.f, windowMaxWidth)
                       .d_width);
    d_window->setSize(USize(UDim(0.f, window_width), height_as_u_dim), false);

     /* It's possible that due to a too low height we're unable to make the
        whole text fit without the need for a vertical scrollbar. In that case,
        we need to redo the computations, with the vertical scrollbar visible.
        We go for the maximal width that makes sense, which is the width of the
        original string (i.e. not divided to lines by word-wrapping). */
    updateFormatting();
    if (getVertScrollbar()->isVisible())
    {
        sizeFunc.d_width = d_window->getElementWidthLowerBoundAsFuncOfWidthOfAreaReservedForContent();
        windowMaxWidth = (contentMaxWidth+epsilon)*sizeFunc.d_width.d_scale + sizeFunc.d_width.d_offset;
        d_window->setSize(USize(UDim(0.f, std::ceil(windowMaxWidth)), height_as_u_dim));
    }
}

/*----------------------------------------------------------------------------//
    An implementation of "adjustSizeToContent" where we do the following:

    Adjust the size of the window to the text by adjusting the width and the
    height independently of each other, and then, if necessary, fix it to comply
    with "d_window->getAspectMode()".
------------------------------------------------------------------------------*/
void FalagardStaticText::adjustSizeToContent_direct()
{
    updateFormatting();
    d_window->adjustSizeToContent_direct();

    /* The process may have to be repeated, because if, for example, word wrap
       is on, and "d_window->isHeightAdjustedToContent()" is true, adjusting
       the height might make the vertical scrollbar visible, in which case the
       word wrapping must be recomputed and then the height adjusted again. */
    if ((getVertScrollbar()->isVisible() || getHorzScrollbar()->isVisible())  &&
        (isWordWrapOn() ||
          (d_window->isWidthAdjustedToContent() && d_window->isHeightAdjustedToContent())))
    {
        updateFormatting();
        d_window->adjustSizeToContent_direct();
    }
}

}
