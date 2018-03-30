//-----------------------------------------------------------------------------
#ifndef ImageCanvasH
#define ImageCanvasH ImageCanvasH
//-----------------------------------------------------------------------------
#include "DrawingCanvas.h"
#include <set>
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#include <vector>
#include <wx/colordlg.h>
#include <wx/dcclient.h>

typedef std::set<AOI*> AOIContainer;

struct ImageCanvasImpl;
class PlotCanvasImageAnalysis;

//-----------------------------------------------------------------------------
class ImageCanvas : public DrawingCanvas
//-----------------------------------------------------------------------------
{
public:
    explicit ImageCanvas() {}
    explicit ImageCanvas( wxWindow* pApp, wxWindow* parent, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                          long style = wxBORDER_NONE, const wxString& name = wxT( "ImageCanvas" ), bool boActive = true );
    virtual ~ImageCanvas();

    void                                    BeginAOIAccess( void )
    {
        m_critSect.Enter();
    }
    void                                    EndAOIAccess( void )
    {
        m_critSect.Leave();
    }
    wxString                                GetCurrentPixelDataAsString( void ) const;
    wxRect                                  GetVisiblePartOfImage( void );
    bool                                    IsScaled( void ) const
    {
        return m_boScaleToClientSize;
    }
    bool                                    IsFullScreen( void ) const;
    void                                    SetFullScreenMode( bool boActive );
    bool                                    InfoOverlayActive( void ) const
    {
        return m_boShowInfoOverlay;
    }
    void                                    RefreshScrollbars( bool boMoveToMousePos = false );
    void                                    RegisterAOI( AOI* pAOI );
    void                                    RegisterAOIs( const std::vector<AOI*>& AOIs );
    bool                                    UnregisterAOI( AOI* pAOI );
    void                                    UnregisterAllAOIs( void );
    bool                                    UpdateAOI( AOI* pAOI, int x, int y, int w, int h );
    bool                                    RegisterMonitorDisplay( ImageCanvas* pMonitorDisplay );
    ImageCanvas*                            GetMonitorDisplay( void ) const
    {
        return m_pMonitorDisplay;
    }

    //-----------------------------------------------------------------------------
    enum TSaveResult
    //-----------------------------------------------------------------------------
    {
        srOK,
        srNoImage,
        srFailedToSave
    };
    //-----------------------------------------------------------------------------
    enum TScalingMode
    //-----------------------------------------------------------------------------
    {
        smNearestNeighbour,
        smLinear,
        smCubic
    };
    void                                    IncreaseSkippedCounter( size_t count );
    void                                    HandleMouseAndKeyboardEvents( bool boHandle );
    void                                    DisableDoubleClickAndPrunePopupMenu( bool boDisable );
    TSaveResult                             SaveCurrentImage( const wxString& filenameAndPath, const wxString& extension ) const;
    void                                    SetActiveAnalysisPlot( const PlotCanvasImageAnalysis* pPlot );
    const PlotCanvasImageAnalysis*          GetActiveAnalysisPlot( void ) const
    {
        return m_pActiveAnalysisPlot;
    }
    bool                                    SetImage( const mvIMPACT::acquire::ImageBuffer* pIB, unsigned int bufferPartIndex, bool boMustRefresh = true );
    const mvIMPACT::acquire::ImageBuffer*   GetImage( void ) const
    {
        return m_pIB;
    }
    void                                    SetScaling( bool boOn );
    void                                    SetInfoOverlay( const std::vector<wxString>& infoStrings );
    void                                    SetInfoOverlayMode( bool boOn );
    void                                    ResetRequestInProgressFlag( void );
    void                                    ResetSkippedImagesCounter( void );
    void                                    SetImageModificationWarningOutput( bool boOn );
    bool                                    GetImageModificationWarningOutput( void ) const
    {
        return m_boShowImageModificationWarning;
    }
    void                                    SetPerformanceWarningOutput( bool boOn );
    bool                                    GetPerformanceWarningOutput( void ) const
    {
        return m_boShowPerformanceWarnings;
    }
    void                                    SetUserData( int userData )
    {
        m_userData = userData;
    }
    double                                  GetZoomFactor( void ) const
    {
        return m_currentZoomFactor;
    }
    void                                    SetZoomFactor( double zoomFactor )
    {
        m_currentZoomFactor = zoomFactor;
    }
    int                                     GetUserData( void ) const
    {
        return m_userData;
    }
    void                                    SetScalingMode( TScalingMode mode );
    TScalingMode                            GetScalingMode( void ) const
    {
        return m_scalingMode;
    }
    bool                                    SupportsDifferentScalingModes( void ) const
    {
        return m_boSupportsDifferentScalingModes;
    }
private:
    //-----------------------------------------------------------------------------
    enum
    //-----------------------------------------------------------------------------
    {
        TEXT_X_OFFSET = 10,
        IMAGE_MODIFICATIONS_Y_OFFSET = 20,
        PERFORMANCE_WARNINGS_Y_OFFSET = 40,
        SKIPPED_IMAGE_MESSAGE_Y_OFFSET = 60,
        BUFFER_PART_SELECTION_Y_OFFSET = 80,
        INFO_Y_OFFSET = 100,
        AOI_SIZING_ZONE_WIDTH = 3
    };
    //-----------------------------------------------------------------------------
    enum TZoomIncrementMode
    //-----------------------------------------------------------------------------
    {
        zimMultiply,
        zimDivide,
        zimFixedValue
    };
    //-----------------------------------------------------------------------------
    // IDs for the controls and the menu commands
    enum TMenuItem
    //-----------------------------------------------------------------------------
    {
        miPopUpFitToScreen = 1,
        miPopUpOneToOneDisplay,
        miPopUpFullScreen,
        miPopUpScalerMode_NearestNeighbour,
        miPopUpScalerMode_Linear,
        miPopUpScalerMode_Cubic,
        miPopUpSetShiftValue,
        miPopUpShowRequestInfoOverlay,
        miPopUpSelectRequestInfoOverlayColor,
        miPopUpShowPerformanceWarnings,
        miPopUpShowImageModificationsWarning
    };
    //-----------------------------------------------------------------------------
    enum TAOISizingPoint
    //-----------------------------------------------------------------------------
    {
        spTopLeft,
        spTopMiddle,
        spTopRight,
        spMiddleLeft,
        spMiddleRight,
        spBottomLeft,
        spBottomMiddle,
        spBottomRight
    };
    ImageCanvasImpl*                        m_pImpl;
    AOIContainer                            m_aoiContainer;
    typedef std::map<TAOISizingPoint, AOI*> SizingPointContainer;
    SizingPointContainer                    m_sizingPoints;
    unsigned int                            m_bufferPartIndexDisplayed;
    bool                                    m_boMouseIsWithinAnAOIsSizingZone;
    bool                                    m_boDoubleClickDisabledAndPopupMenuPruned;
    bool                                    m_boHandleMouseAndKeyboardEvents;
    bool                                    m_boRefreshInProgress;
    bool                                    m_boSupportsFullScreenMode;
    bool                                    m_boSupportsDifferentScalingModes;
    const PlotCanvasImageAnalysis*          m_pActiveAnalysisPlot;
    bool                                    m_boScaleToClientSize;
    bool                                    m_boShowInfoOverlay;
    wxColour                                m_InfoOverlayColor;
    std::vector<wxString>                   m_infoStringsOverlay;
    bool                                    m_boShowImageModificationWarning;
    bool                                    m_boShowPerformanceWarnings;
    wxWindow*                               m_pApp;
    TScalingMode                            m_scalingMode;
    size_t                                  m_skippedImages;
    size_t                                  m_skippedPaintEvents;
    const mvIMPACT::acquire::ImageBuffer*   m_pIB;
    double                                  m_currentZoomFactor;
    double                                  m_zoomFactor_Max;
    static const double                     s_zoomFactor_Min;
    wxPoint                                 m_lastLeftMouseDownPos;
    wxPoint                                 m_lastViewStart;
    wxPoint                                 m_lastRightMouseDownPos;
    wxPoint                                 m_lastRightMouseDownPos_Unscaled;
    AOI                                     m_AOIAtLeftMouseDown;
    AOI*                                    m_pDraggedAOI;
    AOI*                                    m_pAOICurrentlyBeingResized;
    SizingPointContainer::const_iterator    m_grabbedSizingPoint;
    wxPoint                                 m_lastMousePos;
    wxPoint                                 m_lastStartPoint;
    double                                  m_lastScaleFactor;
    ImageCanvas*                            m_pMonitorDisplay;
    AOI*                                    m_pVisiblePartOfImage;
    int                                     m_userData;

    wxPoint                                 AlignMouseMovementToAOIIncrementConstraints( const AOI* const pAOI ) const
    {
        return AlignPointToAOIIncrementConstraints( pAOI, m_lastMousePos - m_lastLeftMouseDownPos );
    }
    wxPoint                                 AlignMousePositionToAOIIncrementConstraints( const AOI* const pAOI ) const
    {
        return AlignPointToAOIIncrementConstraints( pAOI, m_lastMousePos );
    }
    wxPoint                                 AlignPointToAOIIncrementConstraints( const AOI* const pAOI, const wxPoint& unalignedPoint ) const;
    template<typename _Ty> void             AppendYUV411_UYYVYYDataPixelInfo( wxPoint pixel, wxString& pixelInfo, const ImageBuffer* pIB ) const;
    template<typename _Ty> void             AppendYUV422DataPixelInfo( wxPoint pixel, wxString& pixelInfo, const ImageBuffer* pIB ) const;
    template<typename _Ty> void             AppendYUV444DataPixelInfo( wxPoint pixel, wxString& pixelInfo, const ImageBuffer* pIB, const int order[3] ) const;
    template<typename _Ty> void             AppendUYVDataPixelInfo( wxPoint pixel, wxString& pixelInfo, const ImageBuffer* pIB ) const;
    void                                    BlitAOI( wxPaintDC& dc, double scaleFactor, int bmpXOff, int bmpYOff, int bmpW, int bmpH, const AOI* pAOI, bool boDrawOnTop ) const;
    void                                    BlitAOIs( wxPaintDC& dc, double scaleFactor, int bmpXOff, int bmpYOff, int bmpW, int bmpH ) const;
    void                                    BlitAOIs( wxPaintDC& dc, double scaleFactor, int bmpXOff, int bmpYOff, int bmpW, int bmpH, bool boDrawOnTop ) const;
    void                                    BlitSizingPoints( wxPaintDC& dc, double scaleFactor, int bmpXOff, int bmpYOff ) const;
    void                                    BlitInfoStrings( wxPaintDC& dc, double scaleFactor, int bmpScaledViewXOff, int bmpScaledViewVOff, int bmpXOff, int bmpYOff, int bmpW, int bmpH );
    void                                    BlitPerformanceMessages( wxPaintDC& dc, int bmpXOff, int bmpYOff, TImageBufferPixelFormat pixelFormat );
    void                                    ClipAOI( wxRect& rect, bool boForDragging ) const;
    void                                    DecreaseShiftValue( void );
    void                                    DragImageDisplay( void );
    int                                     GetAppliedShiftValue( void ) const;
    static int                              GetChannelBitDepth( TImageBufferPixelFormat format );
    wxPoint                                 GetScaledMousePos( int mouseXPos, int mouseYPos ) const;
    int                                     GetShiftValue( void ) const;
    wxCoord                                 GetSizingZoneWidth( void ) const
    {
        return static_cast<wxCoord>( AOI_SIZING_ZONE_WIDTH / ( ( m_lastScaleFactor < 1.0 ) ? m_lastScaleFactor : 1.0 ) );
    }
    void                                    HandleAOIResize( void );
    void                                    Init( const wxWindow* const pApp );
    void                                    IncreaseShiftValue( void );
    bool                                    IsPointWithinAOISizingZone( const wxPoint& p, const wxRect& r ) const;
    void                                    OnKeyDown( wxKeyEvent& e );
    void                                    OnLeftDblClick( wxMouseEvent& );
    void                                    OnLeftDown( wxMouseEvent& );
    void                                    OnLeftUp( wxMouseEvent& );
    void                                    OnMotion( wxMouseEvent& );
    void                                    OnMouseWheel( wxMouseEvent& );
    void                                    OnPaint( wxPaintEvent& e );
    void                                    OnPopUp_ScalingMode_Changed( wxCommandEvent& e );
    void                                    OnPopUpFitToScreen( wxCommandEvent& e );
    void                                    OnPopUpFullScreen( wxCommandEvent& e );
    void                                    OnPopUpOneToOneDisplay( wxCommandEvent& e );
    void                                    OnPopUpSelectRequestInfoOverlayColor( wxCommandEvent& )
    {
        m_InfoOverlayColor = wxGetColourFromUser( this );
    }
    void                                    OnPopUpSetShiftValue( wxCommandEvent& e );
    void                                    OnPopUpShowImageModificationsWarning( wxCommandEvent& e );
    void                                    OnPopUpShowPerformanceWarnings( wxCommandEvent& e );
    void                                    OnPopUpShowRequestInfoOverlay( wxCommandEvent& e );
    void                                    OnRightDown( wxMouseEvent& );
    void                                    OnRightUp( wxMouseEvent& );
    void                                    SendRefreshAOIControlsEvent( AOI* pAOI ) const;
    void                                    SetShiftValue( int value );
    void                                    SetMaxZoomFactor( const mvIMPACT::acquire::ImageBuffer* pIB, int oldMaxDim );
    void                                    Shutdown( void );
    TSaveResult                             StoreImage( const wxImage& img, const wxString& filenameAndPath, const wxString& extension ) const;
    bool                                    SupportsFullScreenMode( void ) const
    {
        return m_boSupportsFullScreenMode;
    }
    void                                    UpdateAOISizingPoint( AOI* pAOI, const wxPen& pen, int centerX, int centerY );
    void                                    UpdateZoomFactor( TZoomIncrementMode zim, double value );

    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
class AOIAccessScope
//-----------------------------------------------------------------------------
{
    ImageCanvas& canvas_;
public:
    explicit AOIAccessScope( ImageCanvas& canvas ) : canvas_( canvas )
    {
        canvas_.BeginAOIAccess();
    }
    ~AOIAccessScope()
    {
        canvas_.EndAOIAccess();
    }
};

#endif // ImageCanvasH
