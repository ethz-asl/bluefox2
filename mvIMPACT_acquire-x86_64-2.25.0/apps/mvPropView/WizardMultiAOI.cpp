#include <algorithm>
#include <apps/Common/wxAbstraction.h>
#include <common/STLHelper.h>
#include "DataConversion.h"
#include "GlobalDataStorage.h"
#include "ImageCanvas.h"
#include "spinctld.h"
#include "WizardMultiAOI.h"
#include <wx/platinfo.h>
#include <wx/spinctrl.h>

using namespace std;

//=============================================================================
//============== Implementation MultiAreaModeScopeWithStoppedAcquisition ======
//=============================================================================
//-----------------------------------------------------------------------------
class MultiAreaModeScopeWithStoppedAcquisition
//-----------------------------------------------------------------------------
{
    PropViewFrame* pWindow_;
    mvIMPACT::acquire::GenICam::ImageFormatControl& ifc_;
public:
    explicit MultiAreaModeScopeWithStoppedAcquisition( PropViewFrame* pWindow, mvIMPACT::acquire::GenICam::ImageFormatControl& ifc, const std::string& mode ) :
        pWindow_( pWindow ), ifc_( ifc )
    {
        pWindow_->EnsureAcquisitionState( false );
        ifc_.mvMultiAreaMode.writeS( mode );
    }
    ~MultiAreaModeScopeWithStoppedAcquisition()
    {
        ifc_.mvMultiAreaMode.writeS( "mvOff" );
        pWindow_->EnsureAcquisitionState( true );
    }
};

//=============================================================================
//============== Implementation WizardMultiAOI ================================
//=============================================================================
BEGIN_EVENT_TABLE( WizardMultiAOI, OkAndCancelDlg )
    EVT_BUTTON( widAddAnotherAOIButton, WizardMultiAOI::OnBtnAddAnotherAOI )
    EVT_BUTTON( widRemoveSelectedAOIButton, WizardMultiAOI::OnBtnRemoveSelectedAOI )
    EVT_CHECKBOX( widCBShowAOILabels, WizardMultiAOI::OnCBShowAOILabels )
    EVT_CLOSE( WizardMultiAOI::OnClose )
END_EVENT_TABLE()

const wxString WizardMultiAOI::s_AreaStringPrefix_( wxT( "mvArea" ) );

//-----------------------------------------------------------------------------
WizardMultiAOI::WizardMultiAOI( PropViewFrame* pParent, const wxString& title, mvIMPACT::acquire::Device* pDev, ImageCanvas* pImageCanvas, bool boAcquisitionStateOnCancel, FunctionInterface* pFI )
    : OkAndCancelDlg( pParent, widMainFrame, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxMINIMIZE_BOX ),
      pParent_( pParent ), pDev_( pDev ), pFI_( pFI ), ifc_( pDev ), pImageCanvas_( pImageCanvas ), currentErrors_(), configuredAreas_(), AOI2ControlsMap_(),
      supportedAreas_(), mvMultiAreaMode_( "mvMultiAreasCombined" ), boGUICreationComplete_( false ), boHandleChangedMessages_( true ), boAcquisitionStateOnCancel_( boAcquisitionStateOnCancel ),
      AOIsForOverlay_(), rectSensor_(), offsetXIncSensor_( -1LL ), offsetYIncSensor_( -1LL ), widthIncSensor_( -1LL ), heightIncSensor_( -1LL ),
      widthMinSensor_( -1LL ), heightMinSensor_( -1LL ), pAOISettingsNotebook_( 0 ), pStaticBitmapWarning_( 0 ), pInfoText_( 0 )
//-----------------------------------------------------------------------------
{
    /*
        |-------------------------------------|
        | pTopDownSizer                       |
        |                spacer               |
        | |---------------------------------| |
        | | pSettingsSizer                  | |
        | | |--------| |------------------| | |
        | | | Global | |   Area-Specific  | | |
        | | |        | |                  | | |
        | | |Settings| |     Settings     | | |
        | | |--------| |------------------| | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pButtonSizer                    | |
        | |---------------------------------| |
        |-------------------------------------|
    */

    wxPanel* pMainPanel = new wxPanel( this );
    pStaticBitmapWarning_ = new wxStaticBitmap( pMainPanel, widStaticBitmapWarning, *GlobalDataStorage::Instance()->GetBitmap( GlobalDataStorage::bIcon_Empty ), wxDefaultPosition, wxDefaultSize, 0, wxT( "" ) );
    pInfoText_ = new wxStaticText( pMainPanel, wxID_ANY, wxT( "" ), wxDefaultPosition, wxSize( 360, WXGRID_DEFAULT_ROW_HEIGHT ), wxST_ELLIPSIZE_MIDDLE );

    pImageCanvas_->DisableDoubleClickAndPrunePopupMenu( true );
    rectSensor_ = wxRect( 0, 0, static_cast<int>( ifc_.widthMax.read() ), static_cast<int>( ifc_.heightMax.read() ) );
    ifc_.mvMultiAreaMode.writeS( mvMultiAreaMode_ );

    pAOISettingsNotebook_ = new wxNotebook( pMainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP );
    ifc_.mvAreaSelector.getTranslationDict( supportedAreas_ );
    const vector<pair<string, int64_type> >::size_type supportedAreaCount = supportedAreas_.size();
    for( vector<pair<string, int64_type> >::size_type i = 0; i < supportedAreaCount; i++ )
    {
        ifc_.mvAreaSelector.write( supportedAreas_[i].second );
        if( ifc_.mvAreaEnable.read() == bTrue )
        {
            ComposeAreaPanel( supportedAreas_[i].second, false );
        }
    }

    offsetXIncSensor_ = ifc_.offsetX.getStepWidth();
    offsetYIncSensor_ = ifc_.offsetY.getStepWidth();
    widthIncSensor_ = ifc_.width.getStepWidth();
    heightIncSensor_ = ifc_.height.getStepWidth();
    widthMinSensor_ = ifc_.width.getMinValue();
    heightMinSensor_ = ifc_.height.getMinValue();

    if( supportedAreaCount >= 2 )
    {
        // make sure we have at least 2 AOIs as otherwise the whole operation is rather pointless...
        while( configuredAreas_.size() < 2 )
        {
            for( vector<pair<string, int64_type> >::size_type newSetCandidate = 0; newSetCandidate < supportedAreaCount; newSetCandidate++ )
            {
                AOINrToControlsMap::iterator it = configuredAreas_.find( static_cast<int64_type>( newSetCandidate ) );
                if( it == configuredAreas_.end() )
                {
                    ComposeAreaPanel( newSetCandidate, true );
                    break;
                }
            }
        }
    }

    pBtnOk_ = new wxButton( pMainPanel, widBtnOk, wxT( "&Ok" ) );
    pBtnOk_->SetToolTip( wxT( "Closes this dialog, applies all the AOI settings and starts the acquisition" ) );
    pBtnCancel_ = new wxButton( pMainPanel, widBtnCancel, wxT( "&Cancel" ) );
    pBtnCancel_->SetToolTip( wxT( "Just closes this dialog. AOI settings will be discarded" ) );

    // putting it all together
    const int minSpace = ( ( wxPlatformInfo().GetOperatingSystemId() & wxOS_WINDOWS ) != 0 ) ? 2 : 1;

    pCBShowAOILabels_ = new wxCheckBox( pMainPanel, widCBShowAOILabels, wxT( "Show AOI Labels" ) );
    pCBShowAOILabels_->SetValue( true );

    wxStaticBoxSizer* pSetManagementSizer = new wxStaticBoxSizer( wxVERTICAL, pMainPanel, wxT( "AOI Management" ) );
    pSetManagementSizer->Add( new wxButton( pMainPanel, widAddAnotherAOIButton, wxT( "Add Another AOI" ) ), wxSizerFlags().Expand().Border( wxALL, 2 ) );
    pSetManagementSizer->Add( new wxButton( pMainPanel, widRemoveSelectedAOIButton, wxT( "Remove Selected AOI" ) ), wxSizerFlags().Expand().Border( wxALL, 2 ) );
    pSetManagementSizer->AddSpacer( 5 );
    pSetManagementSizer->Add( pCBShowAOILabels_ );

    wxBoxSizer* pSettingsSizer = new wxBoxSizer( wxHORIZONTAL );
    pSettingsSizer->AddSpacer( 5 );
    pSettingsSizer->Add( pSetManagementSizer, wxSizerFlags( 2 ) );
    pSettingsSizer->AddSpacer( 2 * minSpace );
    pSettingsSizer->Add( pAOISettingsNotebook_, wxSizerFlags( 5 ).Expand() );
    pSettingsSizer->AddSpacer( 2 * minSpace );

    // customizing the last line of buttons
    wxBoxSizer* pButtonSizer = new wxBoxSizer( wxHORIZONTAL );
    pButtonSizer->Add( pStaticBitmapWarning_, wxSizerFlags().Border( wxALL, 7 ) );
    wxBoxSizer* pSmallInfoTextAlignmentSizer = new wxBoxSizer( wxVERTICAL );
    pSmallInfoTextAlignmentSizer->AddSpacer( 15 );
    wxFont font = pInfoText_->GetFont();
    font.SetWeight( wxFONTWEIGHT_BOLD );
    pInfoText_->SetFont( font );
    pSmallInfoTextAlignmentSizer->Add( pInfoText_ );
    pButtonSizer->Add( pSmallInfoTextAlignmentSizer );
    pButtonSizer->AddStretchSpacer( 10 );
    pButtonSizer->Add( pBtnOk_, wxSizerFlags().Border( wxALL, 7 ) );
    pButtonSizer->Add( pBtnCancel_, wxSizerFlags().Border( wxALL, 7 ) );

    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    pTopDownSizer->AddSpacer( 15 );
    wxStaticText* pAOIDragNote = new wxStaticText( pMainPanel, wxID_ANY, wxT( "You can either drag and resize the AOIs on top of the live image or use the slider below!\nAOIs in red, dotted style are merged from the green AOIs in the same row and column!" ) );
    pAOIDragNote->SetFont( font );
    pTopDownSizer->Add( pAOIDragNote, wxSizerFlags().Align( wxALIGN_CENTER_HORIZONTAL ) );
    pTopDownSizer->AddSpacer( 15 );
    pTopDownSizer->Add( pSettingsSizer, wxSizerFlags( 10 ).Expand() );
    pTopDownSizer->AddSpacer( 10 );
    pTopDownSizer->Add( pButtonSizer, wxSizerFlags().Expand() );

    pMainPanel->SetSizer( pTopDownSizer );
    pTopDownSizer->SetSizeHints( this );
    SetClientSize( pTopDownSizer->GetMinSize() );
    SetSizeHints( GetSize() );
    Center();

    ifc_.mvMultiAreaMode.writeS( "mvOff" );
    pParent->EnsureAcquisitionState( true );
    boGUICreationComplete_ = true;
    ValidateAOIData();
}

//-----------------------------------------------------------------------------
WizardMultiAOI::~WizardMultiAOI()
//-----------------------------------------------------------------------------
{
    for_each( configuredAreas_.begin(), configuredAreas_.end(), ptr_fun( DeleteSecond<const int64_type, AOIControls*> ) );
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::AddError( const AOIErrorInfo& errorInfo )
//-----------------------------------------------------------------------------
{
    RemoveError( errorInfo );
    currentErrors_.push_back( errorInfo );
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::CloseDlg( int result )
//-----------------------------------------------------------------------------
{
    if( result != wxID_OK )
    {
        pParent_->EnsureAcquisitionState( false );
    }
    pParent_->RemoveCaptureSettingFromStack( pDev_, pFI_, result != wxID_OK );
    pImageCanvas_->UnregisterAllAOIs();
    pImageCanvas_->DisableDoubleClickAndPrunePopupMenu( false );
    for_each( AOIsForOverlay_.begin(), AOIsForOverlay_.end(), ptr_fun( DeleteElement<AOI*> ) );
    pParent_->RestoreGUIStateAfterWizard( true, true );
    pParent_->EnsureAcquisitionState( ( result == wxID_OK ) ? true : boAcquisitionStateOnCancel_ );
    pParent_->OnBeforeMultiAOIDialogDestruction();
    Destroy();
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::ComposeAreaPanel( int64_type value, bool boInsertOnFirstFreePosition )
//-----------------------------------------------------------------------------
{
    ifc_.mvAreaSelector.write( value );

    AOIControls* pAOIControls = new AOIControls();
    pAOIControls->pSetPanel = new wxPanel( pAOISettingsNotebook_, widStaticBitmapWarning + value + 1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, ConvertedString( ifc_.mvAreaSelector.readS() ) );

    // workaround for correct notebook background colors on all platforms
    const wxColour defaultCol = pAOISettingsNotebook_->GetThemeBackgroundColour();
    if( defaultCol.IsOk() )
    {
        pAOIControls->pSetPanel->SetBackgroundColour( defaultCol );
    }

    pAOIControls->pGridSizer = new wxFlexGridSizer( 2 );
    pAOIControls->pGridSizer->AddGrowableCol( 1, 2 );

    const int x = ifc_.mvAreaOffsetX.read();
    const int y = ifc_.mvAreaOffsetY.read();
    const int w = ifc_.mvAreaWidth.read();
    const int h = ifc_.mvAreaHeight.read();

    const wxWindowID x_id = widLAST + value * widSCAOIx;
    const wxWindowID y_id = widLAST + value * widSCAOIy;
    const wxWindowID w_id = widLAST + value * widSCAOIw;
    const wxWindowID h_id = widLAST + value * widSCAOIh;

    pAOIControls->pGridSizer->Add( new wxStaticText( pAOIControls->pSetPanel, wxID_ANY, wxT( "AOI X-Offset:" ) ), wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL ) );
    pAOIControls->pSCAOIx = CreateAOISpinControl( pAOIControls->pSetPanel, x_id, 0, ifc_.widthMax.read(), x, ifc_.offsetX.getStepWidth() );
    pAOIControls->pGridSizer->Add( pAOIControls->pSCAOIx, wxSizerFlags().Expand() );
    pAOIControls->pGridSizer->Add( new wxStaticText( pAOIControls->pSetPanel, wxID_ANY, wxT( "AOI Y-Offset:" ) ), wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL ) );
    pAOIControls->pSCAOIy = CreateAOISpinControl( pAOIControls->pSetPanel, y_id, 0, ifc_.heightMax.read(), y, ifc_.offsetY.getStepWidth() );
    pAOIControls->pGridSizer->Add( pAOIControls->pSCAOIy, wxSizerFlags().Expand() );
    pAOIControls->pGridSizer->Add( new wxStaticText( pAOIControls->pSetPanel, wxID_ANY, wxT( "AOI Width:" ) ), wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL ) );
    pAOIControls->pSCAOIw = CreateAOISpinControl( pAOIControls->pSetPanel, w_id, ifc_.width.getMinValue(), ifc_.widthMax.read(), w, ifc_.width.getStepWidth() );
    pAOIControls->pGridSizer->Add( pAOIControls->pSCAOIw, wxSizerFlags().Expand() );
    pAOIControls->pGridSizer->Add( new wxStaticText( pAOIControls->pSetPanel, wxID_ANY, wxT( "AOI Height:" ) ), wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL ) );
    pAOIControls->pSCAOIh = CreateAOISpinControl( pAOIControls->pSetPanel, h_id, ifc_.height.getMinValue(), ifc_.heightMax.read(), h, ifc_.height.getStepWidth() );
    pAOIControls->pGridSizer->Add( pAOIControls->pSCAOIh, wxSizerFlags().Expand() );
    pAOIControls->pSetPanel->SetSizer( pAOIControls->pGridSizer );

    Bind( wxEVT_SPINCTRL, &WizardMultiAOI::OnAOIChanged, this, x_id );
    Bind( wxEVT_SPINCTRL, &WizardMultiAOI::OnAOIChanged, this, y_id );
    Bind( wxEVT_SPINCTRL, &WizardMultiAOI::OnAOIChanged, this, w_id );
    Bind( wxEVT_SPINCTRL, &WizardMultiAOI::OnAOIChanged, this, h_id );
#ifdef BUILD_WITH_TEXT_EVENTS_FOR_SPINCTRL // Unfortunately on Linux wxWidgets 2.6.x - ??? handling these messages will cause problems, while on Windows not doing so will not always update the GUI as desired :-(
    Bind( wxEVT_TEXT, &WizardMultiAOI::OnAOITextChanged, this, x_id );
    Bind( wxEVT_TEXT, &WizardMultiAOI::OnAOITextChanged, this, y_id );
    Bind( wxEVT_TEXT, &WizardMultiAOI::OnAOITextChanged, this, w_id );
    Bind( wxEVT_TEXT, &WizardMultiAOI::OnAOITextChanged, this, h_id );
#endif // #ifdef BUILD_WITH_TEXT_EVENTS_FOR_SPINCTRL

    configuredAreas_.insert( make_pair( value, pAOIControls ) );
    if( boInsertOnFirstFreePosition )
    {
        // determine the position in which the new Tab will be inserted.
        int64_type position = -1;
        for( AOINrToControlsMap::iterator it = configuredAreas_.begin(); it != configuredAreas_.end(); it++ )
        {
            position++;
            if( it->first == value )
            {
                break;
            }
        }
        pAOISettingsNotebook_->InsertPage( position, pAOIControls->pSetPanel, pAOIControls->pSetPanel->GetName(), true );
    }
    else
    {
        pAOISettingsNotebook_->AddPage( pAOIControls->pSetPanel, pAOIControls->pSetPanel->GetName(), false );
    }
}

//-----------------------------------------------------------------------------
wxSpinCtrlDbl* WizardMultiAOI::CreateAOISpinControl( wxWindow* pParent, wxWindowID id, int64_type min, int64_type max, int64_type value, int64_type inc )
//-----------------------------------------------------------------------------
{
    wxSpinCtrlDbl* pSpinCtrl = new wxSpinCtrlDbl();
    pSpinCtrl->SetMode( mInt64 );
    pSpinCtrl->Create( pParent, id, wxEmptyString, wxDefaultPosition, wxSize( 80, -1 ), wxSP_ARROW_KEYS, min, max, value, inc, wxSPINCTRLDBL_AUTODIGITS, wxT( MY_FMT_I64 ), wxT( "wxSpinCtrlDbl" ), true );
    return pSpinCtrl;
}

//-----------------------------------------------------------------------------
bool WizardMultiAOI::HasCurrentConfigurationCriticalError( void ) const
//-----------------------------------------------------------------------------
{
    const vector<AOIErrorInfo>::size_type errorCnt = currentErrors_.size();
    for( vector<AOIErrorInfo>::size_type i = 0; i < errorCnt; i++ )
    {
        switch( currentErrors_[i].errorType )
        {
        case mrweAOIOutOfBounds:
            return true;
        case mrweAOIOverlap:
            break;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::OnBtnAddAnotherAOI( wxCommandEvent& )
//-----------------------------------------------------------------------------
{
    const AOINrToControlsMap::size_type currentAreaCount = configuredAreas_.size();
    const vector<pair<string, int64_type> >::size_type supportedAreaCount = supportedAreas_.size();
    if( currentAreaCount >= supportedAreaCount )
    {
        wxMessageBox( wxT( "Maximum Number Of Supported AOIs Reached!" ), wxT( "Cannot Add Another AOI!" ), wxICON_EXCLAMATION );
        return;
    }

    MultiAreaModeScopeWithStoppedAcquisition scope( pParent_, ifc_, mvMultiAreaMode_ );
    vector<pair<string, int64_type> > supportedAreas_;
    ifc_.mvAreaSelector.getTranslationDict( supportedAreas_ );

    for( vector<pair<string, int64_type> >::size_type newSetCandidate = 0; newSetCandidate < supportedAreaCount; newSetCandidate++ )
    {
        AOINrToControlsMap::iterator it = configuredAreas_.find( static_cast<int64_type>( newSetCandidate ) );
        if( it == configuredAreas_.end() )
        {
            ComposeAreaPanel( newSetCandidate, true );
            ValidateAOIData();
            break;
        }
    }
    currentErrors_.clear();
    Refresh();
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::OnBtnOk( wxCommandEvent& )
//-----------------------------------------------------------------------------
{
    try
    {
        pParent_->EnsureAcquisitionState( false );
        ifc_.mvMultiAreaMode.writeS( mvMultiAreaMode_ );

        // first switch on the areas which are configured and apply their values...
        AOINrToControlsMap::const_iterator it = configuredAreas_.begin();
        const AOINrToControlsMap::const_iterator itEND = configuredAreas_.end();
        while( it != itEND )
        {
            ifc_.mvAreaSelector.write( it->first );
            ifc_.mvAreaEnable.write( bTrue );
            ifc_.mvAreaOffsetX.write( ifc_.mvAreaOffsetX.getMinValue() );
            ifc_.mvAreaOffsetY.write( ifc_.mvAreaOffsetY.getMinValue() );
            ifc_.mvAreaWidth.write( it->second->pSCAOIw->GetValue() );
            ifc_.mvAreaHeight.write( it->second->pSCAOIh->GetValue() );
            ifc_.mvAreaOffsetX.write( it->second->pSCAOIx->GetValue() );
            ifc_.mvAreaOffsetY.write( it->second->pSCAOIy->GetValue() );
            ++it;
        }

        // ... and then switch off all other areas
        const vector<pair<string, int64_type> >::size_type supportedAreaCount = supportedAreas_.size();
        for( vector<pair<string, int64_type> >::size_type i = 0; i < supportedAreaCount; i++ )
        {
            if( configuredAreas_.find( supportedAreas_[i].second ) == itEND )
            {
                ifc_.mvAreaSelector.write( supportedAreas_[i].second );
                ifc_.mvAreaEnable.write( bFalse );
            }
        }
    }
    catch( const ImpactAcquireException& e )
    {
        const wxString msg = wxString::Format( wxT( "Failed to set up multiple AOIs for device %s(%s)!\n\nAPI error reported: %s(%s)" ),
                                               ConvertedString( pDev_->serial.read() ).c_str(),
                                               ConvertedString( pDev_->product.read() ).c_str(),
                                               ConvertedString( e.getErrorString() ).c_str(),
                                               ConvertedString( e.getErrorCodeAsString() ).c_str() );
        wxMessageBox( msg, wxT( "Multi-AOI Setup Failed" ), wxOK | wxICON_EXCLAMATION, this );
    }
    CloseDlg( wxID_OK );
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::OnBtnRemoveSelectedAOI( wxCommandEvent& )
//-----------------------------------------------------------------------------
{
    if( configuredAreas_.size() <= 1 )
    {
        wxMessageBox( wxT( "To set up multiple AOIs at least 1 AOI should be used!" ), wxT( "Cannot Remove AOI!" ), wxICON_EXCLAMATION );
        return;
    }
    RemoveAreaPanel();
    currentErrors_.clear();
    Refresh();
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::OnClose( wxCloseEvent& e )
//-----------------------------------------------------------------------------
{
    CloseDlg( wxID_CLOSE );
    if( e.CanVeto() )
    {
        e.Veto();
    }
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::RemoveError( const AOIErrorInfo& errorInfo )
//-----------------------------------------------------------------------------
{
    const vector<AOIErrorInfo>::iterator it = find( currentErrors_.begin(), currentErrors_.end(), errorInfo );
    if( it != currentErrors_.end() )
    {
        currentErrors_.erase( it );
    }
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::RemoveAreaPanel( void )
//-----------------------------------------------------------------------------
{
    const int64_type currentPosition = pAOISettingsNotebook_->FindPage( pAOISettingsNotebook_->GetCurrentPage() );
    const int64_type AOIIndex = wxAtoi( pAOISettingsNotebook_->GetCurrentPage()->GetName().substr( s_AreaStringPrefix_.length() ) );
    const AOINrToControlsMap::iterator it = configuredAreas_.find( AOIIndex );

    const wxWindowID x_id = widLAST + AOIIndex * widSCAOIx;
    const wxWindowID y_id = widLAST + AOIIndex * widSCAOIy;
    const wxWindowID w_id = widLAST + AOIIndex * widSCAOIw;
    const wxWindowID h_id = widLAST + AOIIndex * widSCAOIh;
    Unbind( wxEVT_SPINCTRL, &WizardMultiAOI::OnAOIChanged, this, x_id );
    Unbind( wxEVT_SPINCTRL, &WizardMultiAOI::OnAOIChanged, this, y_id );
    Unbind( wxEVT_SPINCTRL, &WizardMultiAOI::OnAOIChanged, this, w_id );
    Unbind( wxEVT_SPINCTRL, &WizardMultiAOI::OnAOIChanged, this, h_id );
#ifdef BUILD_WITH_TEXT_EVENTS_FOR_SPINCTRL // Unfortunately on Linux wxWidgets 2.6.x - ??? handling these messages will cause problems, while on Windows not doing so will not always update the GUI as desired :-(
    Unbind( wxEVT_TEXT, &WizardMultiAOI::OnAOITextChanged, this, x_id );
    Unbind( wxEVT_TEXT, &WizardMultiAOI::OnAOITextChanged, this, y_id );
    Unbind( wxEVT_TEXT, &WizardMultiAOI::OnAOITextChanged, this, w_id );
    Unbind( wxEVT_TEXT, &WizardMultiAOI::OnAOITextChanged, this, h_id );
#endif // #ifdef BUILD_WITH_TEXT_EVENTS_FOR_SPINCTRL

    pAOISettingsNotebook_->DeletePage( currentPosition );
    configuredAreas_.erase( it );
    ValidateAOIData();
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::UpdateError( const wxString& msg, const wxBitmap& icon ) const
//-----------------------------------------------------------------------------
{
    pStaticBitmapWarning_->SetBitmap( icon );
    pInfoText_->SetForegroundColour( HasCurrentConfigurationCriticalError() ? wxColour( 255, 0, 0 ) : wxColour( 0, 0, 0 ) );
    pInfoText_->SetLabel( msg );
    pInfoText_->SetToolTip( msg );
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::UpdateErrors( void ) const
//-----------------------------------------------------------------------------
{
    if( currentErrors_.empty() )
    {
        UpdateError( wxString::Format( wxT( "" ) ), *GlobalDataStorage::Instance()->GetBitmap( GlobalDataStorage::bIcon_Empty ) );
    }
    else
    {
        const AOIErrorInfo& lastError = currentErrors_.back();
        switch( lastError.errorType )
        {
        case mrweAOIOverlap:
            UpdateError( wxT( "Overlapping AOIs will be merged by the device!" ), *GlobalDataStorage::Instance()->GetBitmap( GlobalDataStorage::bIcon_Warning ) );
            // In case you want detailed information use the next line instead
            //UpdateError( wxString::Format( wxT( "AOI %d does overlap with AOI %d!" ), static_cast<int>( lastError.AOINumber ), static_cast<int>( lastError.overlappingAOINumber ) ), *GlobalDataStorage::Instance()->GetBitmap( GlobalDataStorage::bIcon_Warning ) );
            break;
        case mrweAOIOutOfBounds:
            UpdateError( wxString::Format( wxT( "AOI %d does not fit into the sensor area!" ), static_cast<int>( lastError.AOINumber ) ), *GlobalDataStorage::Instance()->GetBitmap( GlobalDataStorage::bIcon_Warning ) );
            break;
        }
    }
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::ShowImageTimeoutPopup( void )
//-----------------------------------------------------------------------------
{
    wxMessageBox( wxT( "Image request timeout\n\nThe last Image Request returned with an Image Timeout. This means that the camera cannot stream data and indicates a problem with the current configuration.\n\nPlease close the Multi AOI setup, fix the configuration and then continue with the setup of multiple AOIs." ), wxT( "Image Timeout!" ), wxOK | wxICON_INFORMATION, this );
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::UpdateControlsFromAOI( const AOI* p )
//-----------------------------------------------------------------------------
{
    const AOIToControlsMap::const_iterator it = AOI2ControlsMap_.find( p );
    if( it != AOI2ControlsMap_.end() )
    {
        boHandleChangedMessages_ = false;
        it->second->pSCAOIx->SetValue( p->m_rect.GetX() );
        it->second->pSCAOIy->SetValue( p->m_rect.GetY() );
        it->second->pSCAOIw->SetValue( p->m_rect.GetWidth() );
        it->second->pSCAOIh->SetValue( p->m_rect.GetHeight() );
        boHandleChangedMessages_ = true;
        ValidateAOIData();
    }
}

//-----------------------------------------------------------------------------
void WizardMultiAOI::ValidateAOIData( void )
//-----------------------------------------------------------------------------
{
    if( !boGUICreationComplete_ )
    {
        return;
    }

    // first collect all AOIs configured with the Wizards notebook pages
    vector<pair<AOI, AOIControls*> > AOIs;
    const bool boShowAOILabels = pCBShowAOILabels_->IsChecked();
    AOINrToControlsMap::const_iterator it = configuredAreas_.begin();
    const AOINrToControlsMap::const_iterator itEND = configuredAreas_.end();
    while( it != itEND )
    {
        const wxRect rect1( it->second->pSCAOIx->GetValue(), it->second->pSCAOIy->GetValue(), it->second->pSCAOIw->GetValue(), it->second->pSCAOIh->GetValue() );
        AOIs.push_back( make_pair( AOI( rect1, wxPen( wxColour( 0, 255, 0 ), 2 ), true, true,
                                        boShowAOILabels ? wxString::Format( wxT( "%s%d" ), s_AreaStringPrefix_, static_cast<int>( it->first ) ) : wxString( wxEmptyString ),
                                        static_cast<int>( offsetXIncSensor_ ), static_cast<int>( offsetYIncSensor_ ), static_cast<int>( widthIncSensor_ ), static_cast<int>( heightIncSensor_ ),
                                        static_cast<int>( widthMinSensor_ ), static_cast<int>( heightMinSensor_ ) ), it->second ) );
        // check if an AOI does not fit into the full sensor resolution and add an error if so
        if( rectSensor_.Contains( rect1 ) )
        {
            RemoveError( AOIErrorInfo( it->first, mrweAOIOutOfBounds ) );
        }
        else
        {
            AddError( AOIErrorInfo( it->first, mrweAOIOutOfBounds ) );
        }
        ++it;
    }

    // now build a list of ALL AOIs. These are the ones from above plus all the AOIs resulting from them according the
    // behaviour of the multi AOI mode of the sensors this wizard is meant for
    const AOINrToControlsMap::size_type configuredAreaCount = configuredAreas_.size();
    const vector<AOI*>::size_type currentAOICount = AOIsForOverlay_.size();
    const vector<AOI*>::size_type neededAOICount = configuredAreaCount * configuredAreaCount;
    const bool boAOICountDidChange = currentAOICount != neededAOICount;

    if( boAOICountDidChange )
    {
        pImageCanvas_->UnregisterAllAOIs();
        AOI2ControlsMap_.clear();
    }

    if( currentAOICount < neededAOICount )
    {
        for( vector<AOI*>::size_type i = currentAOICount; i < neededAOICount; i++ )
        {
            AOIsForOverlay_.push_back( new AOI() );
        }
    }
    else if( currentAOICount > neededAOICount )
    {
        while( AOIsForOverlay_.size() > neededAOICount )
        {
            delete AOIsForOverlay_.back();
            AOIsForOverlay_.pop_back();
        }
    }

    AOIAccessScope aoiAccessScope( *pImageCanvas_ );
    for( vector<AOI>::size_type x = 0; x < configuredAreaCount; x++ )
    {
        for( vector<AOI>::size_type y = 0; y < configuredAreaCount; y++ )
        {
            if( x == y )
            {
                // the main diagonal of AOIs is formed by the AOIs configured by the user
                *AOIsForOverlay_[y * configuredAreaCount + x] = AOIs[x].first;
                AOI2ControlsMap_.insert( make_pair( AOIsForOverlay_[y * configuredAreaCount + x], AOIs[x].second ) );
            }
            else
            {
                // all other AOIs result from the configured ones. This can better be seen in the dialog then it can be explained here...
                const wxRect r( AOIs[y].first.m_rect.GetX(), AOIs[x].first.m_rect.GetY(), AOIs[y].first.m_rect.GetWidth(), AOIs[x].first.m_rect.GetHeight() );
                *AOIsForOverlay_[y * configuredAreaCount + x] = AOI( r, wxPen( wxColour( 255, 0, 0 ), 1, wxPENSTYLE_DOT ), false, false, ( boShowAOILabels ? wxT( "Merged" ) : wxEmptyString ),
                        static_cast<int>( offsetXIncSensor_ ), static_cast<int>( offsetYIncSensor_ ), static_cast<int>( widthIncSensor_ ), static_cast<int>( heightIncSensor_ ),
                        static_cast<int>( widthMinSensor_ ), static_cast<int>( heightMinSensor_ ) );
            }
        }
    }

    for( vector<AOI>::size_type i = 0; i < neededAOICount; i++ )
    {
        if( boShowAOILabels )
        {
            AOIsForOverlay_[i]->m_description.Append( wxString::Format( wxT( "\n(index: %d)" ), static_cast<int>( i ) ) );
        }
        // check for overlapping AOIs
        for( vector<AOI>::size_type j = i + 1; j < neededAOICount; j++ )
        {
            if( AOIsForOverlay_[i]->m_rect.Intersects( AOIsForOverlay_[j]->m_rect ) )
            {
                AddError( AOIErrorInfo( i, j, mrweAOIOverlap ) );
            }
            else
            {
                RemoveError( AOIErrorInfo( i, j, mrweAOIOverlap ) );
            }
        }
    }

    if( boAOICountDidChange )
    {
        pImageCanvas_->RegisterAOIs( AOIsForOverlay_ );
    }
    UpdateErrors();

    pBtnOk_->Enable( !HasCurrentConfigurationCriticalError() );
}
