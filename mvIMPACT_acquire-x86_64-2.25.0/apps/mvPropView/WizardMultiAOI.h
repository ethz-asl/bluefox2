//-----------------------------------------------------------------------------
#ifndef WizardMultiAOIH
#define WizardMultiAOIH WizardMultiAOIH
//-----------------------------------------------------------------------------
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#include <mvIMPACT_CPP/mvIMPACT_acquire_GenICam.h>
#include "PropViewFrame.h"
#include "ValuesFromUserDlg.h"

class ImageCanvas;
class wxSpinCtrlDbl;

//-----------------------------------------------------------------------------
class WizardMultiAOI : public OkAndCancelDlg
//-----------------------------------------------------------------------------
{
public:
    explicit WizardMultiAOI( PropViewFrame* pParent, const wxString& title, mvIMPACT::acquire::Device* pDev, ImageCanvas* pImageCanvas, bool boAcquisitionStateOnCancel, mvIMPACT::acquire::FunctionInterface* pFI );
    virtual ~WizardMultiAOI();

    void ShowImageTimeoutPopup( void );
    void UpdateControlsFromAOI( const AOI* p );
private:
    //-----------------------------------------------------------------------------
    /// \brief GUI elements for a single sequencer set
    struct AOIControls
    //-----------------------------------------------------------------------------
    {
        wxPanel* pSetPanel;
        wxFlexGridSizer* pGridSizer;
        wxSpinCtrlDbl* pSCAOIx;
        wxSpinCtrlDbl* pSCAOIy;
        wxSpinCtrlDbl* pSCAOIw;
        wxSpinCtrlDbl* pSCAOIh;
    };
    typedef std::map<int64_type, AOIControls*> AOINrToControlsMap;
    typedef std::map<const AOI*, AOIControls*> AOIToControlsMap;

    //-----------------------------------------------------------------------------
    enum TWidgetIDs_MultiAOIWizard
    //-----------------------------------------------------------------------------
    {
        widMainFrame = wxID_HIGHEST,
        widAddAnotherAOIButton,
        widRemoveSelectedAOIButton,
        widCBShowAOILabels,
        widStaticBitmapWarning,
        widLAST
    };

    //-----------------------------------------------------------------------------
    enum TWidgetIDs_MultiAOIWizard_AOIControls
    //-----------------------------------------------------------------------------
    {
        widSCAOIx,
        widSCAOIy,
        widSCAOIw,
        widSCAOIh
    };

    //-----------------------------------------------------------------------------
    enum TMultiAOIWizardError
    //-----------------------------------------------------------------------------
    {
        mrweAOIOverlap,
        mrweAOIOutOfBounds
    };

    //-----------------------------------------------------------------------------
    /// \brief Necessary Information for AOI Management Errors
    struct AOIErrorInfo
    //-----------------------------------------------------------------------------
    {

        int64_type AOINumber;
        int64_type overlappingAOINumber;
        TMultiAOIWizardError errorType;
        explicit AOIErrorInfo( int64_type nr, TMultiAOIWizardError type ) : AOINumber( nr ), overlappingAOINumber( -1LL ), errorType( type ) {}
        explicit AOIErrorInfo( int64_type nr, int64_type overlappingNr, TMultiAOIWizardError type ) : AOINumber( nr ), overlappingAOINumber( overlappingNr ), errorType( type ) {}
        bool operator==( const AOIErrorInfo& theOther )
        {
            return ( AOINumber == theOther.AOINumber ) &&
                   ( overlappingAOINumber == theOther.overlappingAOINumber ) &&
                   ( errorType == theOther.errorType );
        }
    };

    PropViewFrame*                      pParent_;
    Device*                             pDev_;
    FunctionInterface*                  pFI_;
    GenICam::ImageFormatControl         ifc_;
    ImageCanvas*                        pImageCanvas_;
    std::vector<AOIErrorInfo>           currentErrors_;
    AOINrToControlsMap                  configuredAreas_;
    AOIToControlsMap                    AOI2ControlsMap_;
    std::vector<std::pair<std::string, int64_type> > supportedAreas_;
    std::string                         mvMultiAreaMode_;
    bool                                boGUICreationComplete_;
    bool                                boHandleChangedMessages_;
    bool                                boAcquisitionStateOnCancel_;
    std::vector<AOI*>                   AOIsForOverlay_;
    wxRect                              rectSensor_;
    int64_type                          offsetXIncSensor_;
    int64_type                          offsetYIncSensor_;
    int64_type                          widthIncSensor_;
    int64_type                          heightIncSensor_;
    int64_type                          widthMinSensor_;
    int64_type                          heightMinSensor_;

    wxCheckBox*                         pCBShowAOILabels_;
    wxNotebook*                         pAOISettingsNotebook_;
    wxStaticBitmap*                     pStaticBitmapWarning_;
    wxStaticText*                       pInfoText_;

    static const wxString               s_AreaStringPrefix_;

    void                                AddError( const AOIErrorInfo& errorInfo );
    void                                CloseDlg( int result );
    void                                ComposeAreaPanel( int64_type value, bool boInsertOnFirstFreePosition );
    wxSpinCtrlDbl*                      CreateAOISpinControl( wxWindow* pParent, wxWindowID id, int64_type min, int64_type max, int64_type value, int64_type inc );
    bool                                HasCurrentConfigurationCriticalError( void ) const;
    void                                RemoveError( const AOIErrorInfo& errorInfo );
    void                                RemoveAreaPanel( void );
    void                                UpdateError( const wxString& msg, const wxBitmap& icon ) const;
    void                                UpdateErrors( void ) const;
    void                                ValidateAOIData( void );

    //----------------------------------GUI Functions----------------------------------
    void                                OnAOIChanged( wxSpinEvent& )
    {
        if( boHandleChangedMessages_ )
        {
            ValidateAOIData();
        }
    }
#ifdef BUILD_WITH_TEXT_EVENTS_FOR_SPINCTRL // Unfortunately on Linux wxWidgets 2.6.x - ??? handling these messages will cause problems, while on Windows not doing so will not always update the GUI as desired :-(
    void                                OnAOITextChanged( wxCommandEvent& )
    {
        if( boHandleChangedMessages_ )
        {
            ValidateAOIData();
        }
    }
#endif // #ifdef BUILD_WITH_TEXT_EVENTS_FOR_SPINCTRL
    void                                OnBtnAddAnotherAOI( wxCommandEvent& e );
    virtual void                        OnBtnCancel( wxCommandEvent& )
    {
        CloseDlg( wxID_CANCEL );
    }
    virtual void                        OnBtnOk( wxCommandEvent& );
    void                                OnBtnRemoveSelectedAOI( wxCommandEvent& e );
    void                                OnCBShowAOILabels( wxCommandEvent& )
    {
        ValidateAOIData();
    }
    virtual void                        OnClose( wxCloseEvent& );

    DECLARE_EVENT_TABLE()
};

#endif // WizardMultiAOIH
