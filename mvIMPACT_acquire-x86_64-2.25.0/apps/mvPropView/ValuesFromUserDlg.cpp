//-----------------------------------------------------------------------------
#include <apps/Common/wxAbstraction.h>
#include "GlobalDataStorage.h"
#include "icons.h"
#include "PropData.h"
#include "PropViewFrame.h"
#include "ValuesFromUserDlg.h"
#include <wx/arrstr.h>
#include <wx/checklst.h>
#include <wx/file.h>
#include <wx/listbox.h>

using namespace std;
using namespace mvIMPACT::acquire::GenICam;

//-----------------------------------------------------------------------------
string BinaryDataFromString( const string& value )
//-----------------------------------------------------------------------------
{
    const string::size_type valueLength = value.size();
    const string::size_type bufSize = ( valueLength + 1 ) / 2;
    string binaryValue;
    binaryValue.resize( bufSize );
    for( string::size_type i = 0; i < bufSize; i++ )
    {
        string token( value.substr( i * 2, ( ( ( i * 2 ) + 2 ) > valueLength ) ? 1 : 2 ) );
        unsigned int tmp;
#if defined(_MSC_VER) && (_MSC_VER >= 1400) // is at least VC 2005 compiler?
        sscanf_s( token.c_str(), "%x", &tmp );
#else
        sscanf( token.c_str(), "%x", &tmp );
#endif // #if defined(_MSC_VER) && (_MSC_VER >= 1400) // is at least VC 2005 compiler?
        binaryValue[i] = static_cast<string::value_type>( tmp );
    }
    return binaryValue;
}

//-----------------------------------------------------------------------------
string BinaryDataToString( const string& value )
//-----------------------------------------------------------------------------
{
    const string::size_type len = value.size();
    string result;
    result.resize( len * 2 );
    const size_t BUF_SIZE = 3;
    char buf[BUF_SIZE];
    for( string::size_type i = 0; i < len; i++ )
    {
#if defined(_MSC_VER) && (_MSC_VER >= 1400) // is at least VC 2005 compiler?
        sprintf_s( buf, 3, "%02x", static_cast<unsigned char>( value[i] ) );
#else
        sprintf( buf, "%02x", static_cast<unsigned char>( value[i] ) );
#endif // #if defined(_MSC_VER) && (_MSC_VER >= 1400) // is at least VC 2005 compiler?
        result[i * 2]   = buf[0];
        result[i * 2 + 1] = buf[1];
    }
    return result;
}

//-----------------------------------------------------------------------------
void WriteFile( const void* pBuf, size_t bufSize, const wxString& pathName, wxTextCtrl* pTextCtrl )
//-----------------------------------------------------------------------------
{
    wxFile file( pathName.c_str(), wxFile::write );
    if( !file.IsOpened() )
    {
        WriteToTextCtrl( pTextCtrl, wxString::Format( wxT( "Storing of %s failed. Could not open file.\n" ), pathName.c_str() ), wxTextAttr( *wxRED ) );
        return;
    }
    size_t bytesWritten = file.Write( pBuf, bufSize );
    if( bytesWritten < bufSize )
    {
        WriteToTextCtrl( pTextCtrl, wxString::Format( wxT( "Storing of %s failed. Could not write all data.\n" ), pathName.c_str() ), wxTextAttr( *wxRED ) );
        return;
    }
    WriteToTextCtrl( pTextCtrl, wxString::Format( wxT( "Storing of %s was successful.\n" ), pathName.c_str() ) );
}

//-----------------------------------------------------------------------------
void WriteToTextCtrl( wxTextCtrl* pTextCtrl, const wxString& msg, const wxTextAttr& style /* = wxTextAttr(*wxBLACK) */ )
//-----------------------------------------------------------------------------
{
    if( pTextCtrl )
    {
        // If you want the control to show the last line of text at the bottom, you can add "ScrollLines(1)"
        // right after the AppendText call. AppendText will ensure the new line is visible, and ScrollLines
        // will ensure the scroll bar is at the real end of the range, not further.
        long posBefore = pTextCtrl->GetLastPosition();
        pTextCtrl->AppendText( msg );
        long posAfter = pTextCtrl->GetLastPosition();
        pTextCtrl->SetStyle( posBefore, posAfter, style );
        pTextCtrl->ScrollLines( 1 );
        pTextCtrl->ShowPosition( pTextCtrl->GetLastPosition() ); // ensure that this position is really visible
    }
}

//=============================================================================
//================= Implementation CollectDeviceInformationThread =============
//=============================================================================
//------------------------------------------------------------------------------
class CollectDeviceInformationThread : public wxThread
//------------------------------------------------------------------------------
{
    const mvIMPACT::acquire::DeviceManager& devMgr_;
    mvIMPACT::acquire::GenICam::InterfaceModule im_;
    std::map<wxString, bool> deviceInformation_;
protected:
    void* Entry( void )
    {
        if( im_.deviceID.readS().length() > 0 ) // even if there is NO device, the deviceSelector will contain one 'empty' entry...
        {
            const unsigned int deviceCount = im_.deviceSelector.getMaxValue() + 1;
            for( unsigned int i = 0; i < deviceCount; i++ )
            {
                im_.deviceSelector.write( i );
                const string serial = im_.deviceSerialNumber.readS();
                const Device* const pDev = devMgr_.getDeviceBySerial( serial );
                const bool boInUse = pDev && pDev->isInUse();
                deviceInformation_.insert( make_pair( wxString::Format( wxT( "%s(%s)%s" ), ConvertedString( im_.deviceSerialNumber.readS() ).c_str(), ConvertedString( im_.deviceModelName.readS() ).c_str(), boInUse ? wxT( " - IN USE!" ) : wxT( "" ) ), boInUse ) );
            }
        }
        return 0;
    }
public:
    explicit CollectDeviceInformationThread( const mvIMPACT::acquire::DeviceManager& devMgr, unsigned int interfaceIndex ) : wxThread( wxTHREAD_JOINABLE ),
        devMgr_( devMgr ), im_( interfaceIndex ), deviceInformation_() {}
    const std::map<wxString, bool> GetResults( void ) const
    {
        return deviceInformation_;
    }
};

//=============================================================================
//================= Implementation HEXStringValidator =========================
//=============================================================================
//-----------------------------------------------------------------------------
HEXStringValidator::HEXStringValidator( wxString* valPtr /* = NULL */ ) : wxTextValidator( wxFILTER_INCLUDE_CHAR_LIST, valPtr )
//-----------------------------------------------------------------------------
{
    wxArrayString strings;
    strings.push_back( wxT( "a" ) );
    strings.push_back( wxT( "b" ) );
    strings.push_back( wxT( "c" ) );
    strings.push_back( wxT( "d" ) );
    strings.push_back( wxT( "e" ) );
    strings.push_back( wxT( "f" ) );
    strings.push_back( wxT( "A" ) );
    strings.push_back( wxT( "B" ) );
    strings.push_back( wxT( "C" ) );
    strings.push_back( wxT( "D" ) );
    strings.push_back( wxT( "E" ) );
    strings.push_back( wxT( "F" ) );
    for( unsigned int i = 0; i <= 9; i++ )
    {
        wxString s;
        s << i;
        strings.push_back( s );
    }
    SetIncludes( strings );
}

//=============================================================================
//============== Implementation OkAndCancelDlg ================================
//=============================================================================
BEGIN_EVENT_TABLE( OkAndCancelDlg, wxDialog )
    EVT_BUTTON( widBtnOk, OkAndCancelDlg::OnBtnOk )
    EVT_BUTTON( widBtnCancel, OkAndCancelDlg::OnBtnCancel )
    EVT_BUTTON( widBtnApply, OkAndCancelDlg::OnBtnApply )
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
OkAndCancelDlg::OkAndCancelDlg( wxWindow* pParent, wxWindowID id, const wxString& title, const wxPoint& pos /*= wxDefaultPosition*/,
                                const wxSize& size /*= wxDefaultSize*/, long style /*= wxDEFAULT_DIALOG_STYLE*/, const wxString& name /* = "OkAndCancelDlg" */ )
    : wxDialog( pParent, id, title, pos, size, style, name ), pBtnApply_( 0 ), pBtnCancel_( 0 ), pBtnOk_( 0 )
//-----------------------------------------------------------------------------
{

}

//-----------------------------------------------------------------------------
void OkAndCancelDlg::AddButtons( wxWindow* pWindow, wxSizer* pSizer, bool boCreateApplyButton /* = false */ )
//-----------------------------------------------------------------------------
{
    // lower line of buttons
    wxBoxSizer* pButtonSizer = new wxBoxSizer( wxHORIZONTAL );
    pButtonSizer->AddStretchSpacer( 100 );
    pBtnOk_ = new wxButton( pWindow, widBtnOk, wxT( "&Ok" ) );
    pButtonSizer->Add( pBtnOk_, wxSizerFlags().Border( wxALL, 7 ) );
    pBtnCancel_ = new wxButton( pWindow, widBtnCancel, wxT( "&Cancel" ) );
    pButtonSizer->Add( pBtnCancel_, wxSizerFlags().Border( wxALL, 7 ) );
    if( boCreateApplyButton )
    {
        pBtnApply_ = new wxButton( pWindow, widBtnApply, wxT( "&Apply" ) );
        pButtonSizer->Add( pBtnApply_, wxSizerFlags().Border( wxALL, 7 ) );
    }
    pSizer->AddSpacer( 10 );
    pSizer->Add( pButtonSizer, wxSizerFlags().Expand() );
}

//-----------------------------------------------------------------------------
void OkAndCancelDlg::FinalizeDlgCreation( wxWindow* pWindow, wxSizer* pSizer )
//-----------------------------------------------------------------------------
{
    pWindow->SetSizer( pSizer );
    pSizer->SetSizeHints( this );
    SetClientSize( pSizer->GetMinSize() );
    SetSizeHints( GetSize() );
}

//=============================================================================
//============== Implementation ValuesFromUserDlg =============================
//=============================================================================
//-----------------------------------------------------------------------------
ValuesFromUserDlg::ValuesFromUserDlg( wxWindow* pParent, const wxString& title, const vector<ValueData*>& inputData )
    : OkAndCancelDlg( pParent, wxID_ANY, title )
//-----------------------------------------------------------------------------
{
    /*
        |-------------------------------------|
        | pTopDownSizer                       |
        |                spacer               |
        | |---------------------------------| |
        | | pGridSizer                      | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pButtonSizer                    | |
        | |---------------------------------| |
        |-------------------------------------|
    */

    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    pTopDownSizer->AddSpacer( 10 );

    wxPanel* pPanel = new wxPanel( this );

    const int colCount = static_cast<int>( inputData.size() );
    userInputData_.resize( colCount );
    ctrls_.resize( colCount );
    wxFlexGridSizer* pGridSizer = new wxFlexGridSizer( 2 );
    for( int i = 0; i < colCount; i++ )
    {
        pGridSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxString( wxT( " " ) ) + inputData[i]->caption_ ) );
        wxControl* p = 0;
        if( dynamic_cast<ValueRangeData*>( inputData[i] ) )
        {
            ValueRangeData* pData = dynamic_cast<ValueRangeData*>( inputData[i] );
            p = new wxSpinCtrl( pPanel, widFirst + i, wxString::Format( wxT( "%d" ), pData->def_ ), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, pData->min_, pData->max_, pData->def_ );
        }
        else if( dynamic_cast<ValueChoiceData*>( inputData[i] ) )
        {
            ValueChoiceData* pData = dynamic_cast<ValueChoiceData*>( inputData[i] );
            p = new wxComboBox( pPanel, widFirst + i, pData->choices_[0], wxDefaultPosition, wxDefaultSize, pData->choices_, wxCB_DROPDOWN | wxCB_READONLY );
        }
        else
        {
            wxASSERT( !"Invalid data type detected!" );
        }
        pGridSizer->Add( p, wxSizerFlags().Expand() );
        ctrls_[i] = p;
    }
    pTopDownSizer->Add( pGridSizer );
    AddButtons( pPanel, pTopDownSizer );
    FinalizeDlgCreation( pPanel, pTopDownSizer );
}

//=============================================================================
//============== Implementation SettingHierarchyDlg ===========================
//=============================================================================
//-----------------------------------------------------------------------------
SettingHierarchyDlg::SettingHierarchyDlg( wxWindow* pParent, const wxString& title, const StringToStringMap& settingRelationships )
    : OkAndCancelDlg( pParent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX )
//-----------------------------------------------------------------------------
{
    /*
        |-------------------------------------|
        | pTopDownSizer                       |
        |                spacer               |
        | |---------------------------------| |
        | | pTreeCtrl                       | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pButtonSizer                    | |
        | |---------------------------------| |
        |-------------------------------------|
    */

    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    pTopDownSizer->AddSpacer( 10 );

    wxPanel* pPanel = new wxPanel( this );

    wxTreeCtrl* pTreeCtrl = new wxTreeCtrl( pPanel, wxID_ANY );
    wxTreeItemId rootId = pTreeCtrl->AddRoot( wxT( "Base" ) );
    PopulateTreeCtrl( pTreeCtrl, rootId, wxString( wxT( "Base" ) ), settingRelationships );
    ExpandAll( pTreeCtrl );
    pTopDownSizer->Add( pTreeCtrl, wxSizerFlags( 3 ).Expand() );
    AddButtons( pPanel, pTopDownSizer );
    FinalizeDlgCreation( pPanel, pTopDownSizer );
}

//-----------------------------------------------------------------------------
void SettingHierarchyDlg::ExpandAll( wxTreeCtrl* pTreeCtrl )
//-----------------------------------------------------------------------------
{
    ExpandAllChildren( pTreeCtrl, pTreeCtrl->GetRootItem() );
}

//-----------------------------------------------------------------------------
/// \brief this code is 'stolen' from the wxWidgets 2.8.0 source as this application
/// might be compiled with older versions of wxWidgets not supporting the wxTreeCtrl::ExpandAll function
void SettingHierarchyDlg::ExpandAllChildren( wxTreeCtrl* pTreeCtrl, const wxTreeItemId& item )
//-----------------------------------------------------------------------------
{
    // expand this item first, this might result in its children being added on
    // the fly
    pTreeCtrl->Expand( item );

    // then (recursively) expand all the children
    wxTreeItemIdValue cookie;
    for( wxTreeItemId idCurr = pTreeCtrl->GetFirstChild( item, cookie ); idCurr.IsOk(); idCurr = pTreeCtrl->GetNextChild( item, cookie ) )
    {
        ExpandAllChildren( pTreeCtrl, idCurr );
    }
}

//-----------------------------------------------------------------------------
void SettingHierarchyDlg::PopulateTreeCtrl( wxTreeCtrl* pTreeCtrl, wxTreeItemId currentItem, const wxString& currentItemName, const StringToStringMap& settingRelationships )
//-----------------------------------------------------------------------------
{
    StringToStringMap::const_iterator it = settingRelationships.begin();
    StringToStringMap::const_iterator itEND = settingRelationships.end();
    while( it != itEND )
    {
        if( it->second == currentItemName )
        {
            wxTreeItemId newItem = pTreeCtrl->AppendItem( currentItem, it->first );
            PopulateTreeCtrl( pTreeCtrl, newItem, it->first, settingRelationships );
        }
        ++it;
    }
}

//=============================================================================
//============== Implementation DetailedRequestInformationDlg =================
//=============================================================================

BEGIN_EVENT_TABLE( DetailedRequestInformationDlg, OkAndCancelDlg )
    EVT_SPINCTRL( widSCRequestSelector, DetailedRequestInformationDlg::OnSCRequestSelectorChanged )
#ifdef BUILD_WITH_TEXT_EVENTS_FOR_SPINCTRL // Unfortunately on Linux wxWidgets 2.6.x - ??? handling these messages will cause problems, while on Windows not doing so will not always update the GUI as desired :-(
    EVT_TEXT( widSCRequestSelector, DetailedRequestInformationDlg::OnSCRequestSelectorTextChanged )
#endif // #ifdef BUILD_WITH_TEXT_EVENTS_FOR_SPINCTRL
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
DetailedRequestInformationDlg::DetailedRequestInformationDlg( wxWindow* pParent, const wxString& title, mvIMPACT::acquire::FunctionInterface* pFI )
    : OkAndCancelDlg( pParent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX ),
      pFI_( pFI ), pTreeCtrl_( 0 )
//-----------------------------------------------------------------------------
{
    /*
        |-------------------------------------|
        | pTopDownSizer                       |
        |                spacer               |
        | |---------------------------------| |
        | | pLeftRightSizer                 | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | property display                | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pButtonSizer                    | |
        | |---------------------------------| |
        |-------------------------------------|
    */

    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    wxPanel* pPanel = new wxPanel( this );

    wxBoxSizer* pLeftRightSizer = new wxBoxSizer( wxHORIZONTAL );
    pSCRequestSelector_ = new wxSpinCtrl( pPanel, widSCRequestSelector, wxT( "0" ), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, pFI_->requestCount() - 1, 1 );
    pSCRequestSelector_->SetToolTip( wxT( "Can be used to quickly move within the requests currently allocated" ) );
    pLeftRightSizer->AddSpacer( 10 );
    pLeftRightSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( "Request Selector: " ) ), wxSizerFlags().Align( wxALIGN_CENTER_VERTICAL ) );
    pLeftRightSizer->AddSpacer( 5 );
    pLeftRightSizer->Add( pSCRequestSelector_, wxSizerFlags().Expand() );

    pTreeCtrl_ = new wxTreeCtrl( pPanel, wxID_ANY );
    PopulateTreeCtrl( pTreeCtrl_, 0 );
    ExpandAll( pTreeCtrl_ );
    pTopDownSizer->AddSpacer( 5 );
    pTopDownSizer->Add( pLeftRightSizer, wxSizerFlags().Expand() );
    pTopDownSizer->AddSpacer( 5 );
    pTopDownSizer->Add( pTreeCtrl_, wxSizerFlags( 3 ).Expand() );
    pTopDownSizer->AddSpacer( 10 );
    AddButtons( pPanel, pTopDownSizer );
    FinalizeDlgCreation( pPanel, pTopDownSizer );
    SetSize( 400, 700 );
}

//-----------------------------------------------------------------------------
void DetailedRequestInformationDlg::ExpandAll( wxTreeCtrl* pTreeCtrl )
//-----------------------------------------------------------------------------
{
    ExpandAllChildren( pTreeCtrl, pTreeCtrl->GetRootItem() );
}

//-----------------------------------------------------------------------------
/// \brief this code is 'stolen' from the wxWidgets 2.8.0 source as this application
/// might be compiled with older versions of wxWidgets not supporting the wxTreeCtrl::ExpandAll function
void DetailedRequestInformationDlg::ExpandAllChildren( wxTreeCtrl* pTreeCtrl, const wxTreeItemId& item )
//-----------------------------------------------------------------------------
{
    // expand this item first, this might result in its children being added on
    // the fly
    pTreeCtrl->Expand( item );

    // then (recursively) expand all the children
    wxTreeItemIdValue cookie;
    for( wxTreeItemId idCurr = pTreeCtrl->GetFirstChild( item, cookie ); idCurr.IsOk(); idCurr = pTreeCtrl->GetNextChild( item, cookie ) )
    {
        ExpandAllChildren( pTreeCtrl, idCurr );
    }
}

//-----------------------------------------------------------------------------
void DetailedRequestInformationDlg::PopulateTreeCtrl( wxTreeCtrl* pTreeCtrl, wxTreeItemId parent, Component itComponent )
//-----------------------------------------------------------------------------
{
    while( itComponent.isValid() )
    {
        if( itComponent.isList() )
        {
            wxTreeItemId newList = pTreeCtrl->AppendItem( parent, wxString::Format( wxT( "%s" ), ConvertedString( itComponent.name() ).c_str() ) );
            PopulateTreeCtrl( pTreeCtrl, newList, itComponent.firstChild() );
        }
        else if( itComponent.isProp() )
        {
            Property prop( itComponent );
            wxString data = wxString::Format( wxT( "%s%s: " ), ConvertedString( itComponent.name() ).c_str(),
                                              itComponent.isVisible() ? wxT( "" ) : wxT( " (currently invisible)" ) );
            const unsigned int valCount = prop.valCount();
            if( valCount > 1 )
            {
                data.Append( wxT( "[ " ) );
                for( unsigned int i = 0; i < valCount; i++ )
                {
                    data.Append( ConvertedString( prop.readS( static_cast<int>( i ) ) ).c_str() );
                    if( i < valCount - 1 )
                    {
                        data.Append( wxT( ", " ) );
                    }
                }
                data.Append( wxT( " ]" ) );
            }
            else
            {
                data.Append( ConvertedString( prop.readS() ).c_str() );
            }
            pTreeCtrl->AppendItem( parent, data );
        }
        else
        {
            pTreeCtrl->AppendItem( parent, wxString::Format( wxT( "%s" ), ConvertedString( itComponent.name() ).c_str() ) );
        }
        itComponent = itComponent.nextSibling();
    }
}

//-----------------------------------------------------------------------------
void DetailedRequestInformationDlg::PopulateTreeCtrl( wxTreeCtrl* pTreeCtrl, const int requestNr )
//-----------------------------------------------------------------------------
{
    Component itComponent( pFI_->getRequest( requestNr )->getComponentLocator().hObj() );
    wxTreeItemId rootId = pTreeCtrl->AddRoot( ConvertedString( itComponent.name() ) );
    itComponent = itComponent.firstChild();
    PopulateTreeCtrl( pTreeCtrl, rootId, itComponent );
}

//-----------------------------------------------------------------------------
void DetailedRequestInformationDlg::SelectRequest( const int requestNr )
//-----------------------------------------------------------------------------
{
    pTreeCtrl_->DeleteAllItems();
    PopulateTreeCtrl( pTreeCtrl_, requestNr );
    ExpandAll( pTreeCtrl_ );
}

//=============================================================================
//============== Implementation DriverInformationDlg ==========================
//=============================================================================
BEGIN_EVENT_TABLE( DriverInformationDlg, OkAndCancelDlg )
    EVT_TREE_STATE_IMAGE_CLICK( widTCtrl, DriverInformationDlg::OnItemStateClick )
    EVT_CLOSE( DriverInformationDlg::OnClose )
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
DriverInformationDlg::DriverInformationDlg( wxWindow* pParent, const wxString& title, ComponentIterator itDrivers,
        const DeviceManager& devMgr, const wxString& newestMVIAVersionAvailable, const wxString& newestMVIAVersionInstalled )
    : OkAndCancelDlg( pParent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER |
                      wxMAXIMIZE_BOX | wxMINIMIZE_BOX ), pSystemModule_( 0 ), pTreeCtrl_( 0 ), pIconList_( new wxImageList( 16, 16, true, 3 ) ), ignoredInterfacesInfo_(),
      ignoredInterfacesInfoOriginal_(), newestMVIAVersionAvailable_( newestMVIAVersionAvailable ), newestMVIAVersionInstalled_( newestMVIAVersionInstalled )
//-----------------------------------------------------------------------------
{
    /*
        |-------------------------------------|
        | pTopDownSizer                       |
        |                spacer               |
        | |---------------------------------| |
        | | pTreeCtrl                       | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pButtonSizer                    | |
        | |---------------------------------| |
        |-------------------------------------|
    */

    wxBusyCursor busyCursorScope;
    // The next command yields control to pending messages in the windowing system. It is necessary in order
    // to get a busy cursor in Linux systems while waiting for the dialog window to appear
    wxYield();
    try
    {
        pSystemModule_ = new mvIMPACT::acquire::GenICam::SystemModule();
    }
    catch( const ImpactAcquireException& )
    {
        // The GenICam/GenTL driver might not be installed on every system and then creating any of the objects above will raise an exception.
    }
    ReadIgnoredInterfacesInfoAndEnableAllInterfaces();
    UpdateDeviceListWithProgressMessage( this, devMgr );
    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    pTopDownSizer->AddSpacer( 10 );
    wxPanel* pPanel = new wxPanel( this );
    pTreeCtrl_ = new wxTreeCtrl( pPanel, widTCtrl );
    PopulateTreeCtrl( itDrivers, devMgr );
    // The next line is a workaround to prevent the buttons getting off-screen
    // in linux systems, when the number of interfaces is high.
    pTreeCtrl_->SetMaxSize( wxSize( 600, 700 ) );
    ExpandAll();
    pTopDownSizer->Add( pTreeCtrl_, wxSizerFlags( 1 ).Expand() );
    AddButtons( pPanel, pTopDownSizer );
    FinalizeDlgCreation( pPanel, pTopDownSizer );
    SetSize( 600, 700 );
    Center();
}

//-----------------------------------------------------------------------------
DriverInformationDlg::~DriverInformationDlg()
//-----------------------------------------------------------------------------
{
    pTreeCtrl_->SetStateImageList( 0 );
    delete pIconList_;
}

//-----------------------------------------------------------------------------
wxTreeItemId DriverInformationDlg::AddComponentListToList( wxTreeItemId parent, mvIMPACT::acquire::ComponentLocator locator, const char* pName )
//-----------------------------------------------------------------------------
{
    ComponentList list;
    locator.bindComponent( list, string( pName ) );
    return list.isValid() ? pTreeCtrl_->AppendItem( parent, ConvertedString( list.name() ) ) : wxTreeItemId();
}

//-----------------------------------------------------------------------------
void DriverInformationDlg::AddStringPropToList( wxTreeItemId parent, ComponentLocator locator, const char* pName )
//-----------------------------------------------------------------------------
{
    PropertyS prop;
    locator.bindComponent( prop, string( pName ) );
    if( prop.isValid() )
    {
        const wxString propName = ConvertedString( prop.name() );
        if( propName == wxString( "Version" ) )
        {
            const wxString propValue = ConvertedString( prop.read().substr( 0, prop.read().find_last_of( "." ) ) );
            if( VersionFromString( propValue ) < VersionFromString( newestMVIAVersionInstalled_ ) )
            {
                const wxTreeItemId treeItem = pTreeCtrl_->AppendItem( parent, wxString::Format( wxT( "%s: %s (It is recommended to update to at least version %s!!!)" ), propName.c_str(), propValue.c_str(), newestMVIAVersionInstalled_.c_str() ) );
                pTreeCtrl_->SetItemTextColour( treeItem, *wxRED );
                pTreeCtrl_->SetItemBackgroundColour( treeItem, acYellowPastel );
                pTreeCtrl_->SetItemTextColour( parent, *wxRED );
                return;
            }
            else if( VersionFromString( propValue ) < VersionFromString( newestMVIAVersionAvailable_ ) )
            {
                const wxTreeItemId treeItem = pTreeCtrl_->AppendItem( parent, wxString::Format( wxT( "%s: %s (Version %s is now available!!!)" ), propName.c_str(), propValue.c_str(), newestMVIAVersionAvailable_.c_str() ) );
                pTreeCtrl_->SetItemTextColour( treeItem, *wxBLUE );
                pTreeCtrl_->SetItemTextColour( parent, *wxBLUE );
                return;
            }
            else
            {
                pTreeCtrl_->AppendItem( parent, wxString::Format( wxT( "%s: %s" ), propName.c_str(), propValue.c_str() ) );
            }
        }
        else
        {
            const wxString propValue = ConvertedString( prop.read() );
            pTreeCtrl_->AppendItem( parent, wxString::Format( wxT( "%s: %s" ), propName.c_str(), propValue.c_str() ) );
        }
    }
}

//-----------------------------------------------------------------------------
void DriverInformationDlg::SetParentUndefinedStateIfConflictingChildrenStates( const wxTreeItemId& itemId )
//-----------------------------------------------------------------------------
{
    if( !itemId.IsOk() ||
        ( pTreeCtrl_->GetChildrenCount( itemId, false ) < 2 ) )
    {
        return;
    }

    bool boAtLeastOneEnumerate = false, boAtLEastOneIgnore = false;
    wxTreeItemIdValue childVal;
    wxTreeItemId childId = pTreeCtrl_->GetFirstChild( itemId, childVal );
    while( childId.IsOk() )
    {
        if( pTreeCtrl_->GetItemState( childId ) == lisChecked )
        {
            boAtLeastOneEnumerate = true;
        }
        else
        {
            boAtLEastOneIgnore = true;
        }

        if( boAtLeastOneEnumerate  && boAtLEastOneIgnore )
        {
            pTreeCtrl_->SetItemState( itemId, lisUndefined );
            return;
        }
        childId = pTreeCtrl_->GetNextChild( itemId, childVal );
    }
}

//-----------------------------------------------------------------------------
void DriverInformationDlg::ConfigureItemState( const wxTreeItemId& itemId ) const
//-----------------------------------------------------------------------------
{
    const TListItemStates state = ( pTreeCtrl_->GetItemState( itemId ) == lisChecked ) ? lisUnchecked : lisChecked;
    pTreeCtrl_->SetItemState( itemId, state );

    if( string( pTreeCtrl_->GetItemText( itemId ) ) == string( "GigEVision" ) ||
        string( pTreeCtrl_->GetItemText( itemId ) ) == string( "USB3Vision" ) )
    {
        // recursively apply the same state an all children(interfaces of the same type)
        wxTreeItemIdValue childVal;
        wxTreeItemId childId = pTreeCtrl_->GetFirstChild( itemId, childVal );
        while( childId.IsOk() )
        {
            pTreeCtrl_->SetItemState( childId, state );
            childId = pTreeCtrl_->GetNextChild( itemId, childVal );
        }
    }
    else
    {
        // set the state of the parent (interface type) to undefined or to item state, depending on the states of the other siblings.
        const wxTreeItemId parentId = pTreeCtrl_->GetItemParent( itemId );
        pTreeCtrl_->SetItemState( parentId, state );

        wxTreeItemIdValue siblingVal;
        wxTreeItemId siblingId = pTreeCtrl_->GetFirstChild( parentId, siblingVal );
        while( siblingId.IsOk() )
        {
            if( pTreeCtrl_->GetItemState( siblingId ) != state )
            {
                pTreeCtrl_->SetItemState( parentId, lisUndefined );
                break;
            }
            siblingId = pTreeCtrl_->GetNextChild( parentId, siblingVal );
        }
    }
}

//-----------------------------------------------------------------------------
void DriverInformationDlg::IgnoreInterfaceTechnologySetting( const string& technology, IgnoredInterfacesInfo& interfacesInfo )
//-----------------------------------------------------------------------------
{
    if( pSystemModule_ && pSystemModule_->mvInterfaceTechnologyToIgnoreSelector.isValid() && pSystemModule_->mvInterfaceTechnologyToIgnoreEnable.isValid() )
    {
        pSystemModule_->mvInterfaceTechnologyToIgnoreSelector.writeS( technology );
        if( interfacesInfo[string( technology )] == string( "1" ) )
        {
            pSystemModule_->mvInterfaceTechnologyToIgnoreEnable.write( bTrue );
        }
    }
}

//-----------------------------------------------------------------------------
vector<map<wxString, bool> > DriverInformationDlg::EnumerateGenICamDevices( unsigned int interfaceCount, const DeviceManager& devMgr )
//-----------------------------------------------------------------------------
{
    static const int MAX_TIME_MS = 10000;
    wxProgressDialog progressDialog( wxT( "Obtaining Device Information" ),
                                     wxString::Format( wxT( "Obtaining device information from %u interface%s...\n\nThis dialog will disappear automatically once this operation completes!" ), interfaceCount, ( ( interfaceCount > 1 ) ? wxT( "s" ) : wxT( "" ) ) ),
                                     MAX_TIME_MS, // range
                                     this,
                                     wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_ELAPSED_TIME );

    // create a thread for each interface
    vector<CollectDeviceInformationThread*> threads;
    for( unsigned int i = 0; i < interfaceCount; i++ )
    {
        threads.push_back( new CollectDeviceInformationThread( devMgr, i ) );
        threads.back()->Create();
        threads.back()->Run();
    }

    // if at least one thread is still running keep waiting
    for( unsigned int i = 0; i < interfaceCount; i++ )
    {
        if( threads[i]->IsRunning() )
        {
            wxMilliSleep( 100 );
            progressDialog.Pulse();
            i = 0;
        }
    }

    // collect the data and clean up
    vector<map<wxString, bool> > results;
    for( unsigned int i = 0; i < interfaceCount; i++ )
    {
        threads[i]->Wait();
        results.push_back( threads[i]->GetResults() );
        delete threads[i];
    }
    progressDialog.Update( MAX_TIME_MS );
    return results;
}

//-----------------------------------------------------------------------------
void DriverInformationDlg::ExpandAll()
//-----------------------------------------------------------------------------
{
    ExpandAllChildren( pTreeCtrl_->GetRootItem() );
}

//-----------------------------------------------------------------------------
/// \brief this code is 'stolen' from the wxWidgets 2.8.0 source as this application
/// might be compiled with older versions of wxWidgets not supporting the wxTreeCtrl::ExpandAll function
void DriverInformationDlg::ExpandAllChildren( const wxTreeItemId& item )
//-----------------------------------------------------------------------------
{
    // expand this item first, this might result in its children being added on
    // the fly
    pTreeCtrl_->Expand( item );

    // then (recursively) expand all the children
    wxTreeItemIdValue cookie;
    for( wxTreeItemId idCurr = pTreeCtrl_->GetFirstChild( item, cookie ); idCurr.IsOk(); idCurr = pTreeCtrl_->GetNextChild( item, cookie ) )
    {
        ExpandAllChildren( idCurr );
    }
}

//-----------------------------------------------------------------------------
void DriverInformationDlg::OnItemStateClick( wxTreeEvent& event )
//-----------------------------------------------------------------------------
{
    if( pSystemModule_->mvInterfaceTechnologyToIgnoreSelector.isValid() &&
        pSystemModule_->mvInterfaceTechnologyToIgnoreEnable.isValid() &&
        pSystemModule_->mvDeviceUpdateListBehaviour.isValid() )
    {
        wxTreeItemId itemId = event.GetItem();
        ConfigureItemState( itemId );
        UpdateIgnoredInterfacesInfo( itemId );
    }
}

//-----------------------------------------------------------------------------
void DriverInformationDlg::PopulateTreeCtrl( ComponentIterator itDrivers, const DeviceManager& devMgr )
//-----------------------------------------------------------------------------
{
    const wxImage checkboxChecked( wxBitmap::NewFromPNGData( checkbox_checked_png, sizeof( checkbox_checked_png ) ).ConvertToImage() );
    const wxImage checkboxUnchecked( wxBitmap::NewFromPNGData( checkbox_unchecked_png, sizeof( checkbox_unchecked_png ) ).ConvertToImage() );
    const wxImage checkboxUndefined( wxBitmap::NewFromPNGData( checkbox_undefined_png, sizeof( checkbox_undefined_png ) ).ConvertToImage() );

    pIconList_->Add( checkboxUndefined.Scale( 16, 16 ) );
    pIconList_->Add( checkboxChecked.Scale( 16, 16 ) );
    pIconList_->Add( checkboxUnchecked.Scale( 16, 16 ) );

    pTreeCtrl_->SetStateImageList( pIconList_ );
    const wxTreeItemId rootId = pTreeCtrl_->AddRoot( ConvertedString( itDrivers.name() ) );
    itDrivers = itDrivers.firstChild();
    while( itDrivers.isValid() )
    {
        wxString msg;
        ComponentList clDriver( itDrivers.hObj() );

        //If the driver is not installed then do not even list it!
        ComponentLocator locator( itDrivers.hObj() );
        PropertyS prop;
        locator.bindComponent( prop, string( "Version" ) );
        if( !prop.isValid() )
        {
            itDrivers = itDrivers.nextSibling();
            continue;
        }

        if( !clDriver.contentDescriptor().empty() )
        {
            msg = wxString::Format( wxT( " %s" ), ConvertedString( itDrivers.name() ).c_str() );
        }
        wxTreeItemId newItem = pTreeCtrl_->AppendItem( rootId, msg );
        if( !msg.IsEmpty() )
        {
            pTreeCtrl_->SetItemBold( newItem );
        }

        AddStringPropToList( newItem, locator, "FullPath" );
        AddStringPropToList( newItem, locator, "Version" );

        wxTreeItemId interfaceTypeGEV;
        wxTreeItemId interfaceTypeU3V;
        if( ( clDriver.name() == string( "mvGenTLConsumer" ) ) && pSystemModule_ )
        {
            const unsigned int interfaceCount = pSystemModule_->interfaceSelector.getMaxValue() + 1;
            const IgnoredInterfacesInfo::const_iterator itEND = ignoredInterfacesInfo_.end();
            unsigned int gevInterfaceCount = 0;
            unsigned int u3vInterfaceCount = 0;
            for( unsigned int i = 0; i < interfaceCount; i++ )
            {
                pSystemModule_->interfaceSelector.write( i );
                wxTreeItemId interfaceTreeItem;
                const string interfaceID = pSystemModule_->interfaceID.readS();
                const IgnoredInterfacesInfo::const_iterator it = ignoredInterfacesInfo_.find( interfaceID );
                assert( ( it != itEND ) && "At this point there should be an interfaceID entry for each interface in the ignored interfaces information map!" );
                if( it == itEND )
                {
                    continue;
                }
                if( pSystemModule_->interfaceType.readS() == string( "GEV" ) )
                {
                    if( gevInterfaceCount == 0 )
                    {
                        interfaceTypeGEV = pTreeCtrl_->AppendItem( newItem, "GigEVision" );
                        pTreeCtrl_->SetItemBold( interfaceTypeGEV );
                        if( pSystemModule_->mvInterfaceTechnologyToIgnoreSelector.isValid() )
                        {
                            pSystemModule_->mvInterfaceTechnologyToIgnoreSelector.writeS( "GEV" );
                        }
                        pTreeCtrl_->SetItemState( interfaceTypeGEV, ( ignoredInterfacesInfo_[string( "GEV" )] == string( "1" ) ) ? 2 : 1 );
                    }
                    gevInterfaceCount++;
                }
                else if( pSystemModule_->interfaceType.readS() == string( "U3V" ) )
                {
                    if( u3vInterfaceCount == 0 )
                    {
                        interfaceTypeU3V = pTreeCtrl_->AppendItem( newItem, "USB3Vision" );
                        pTreeCtrl_->SetItemBold( interfaceTypeU3V );
                        if( pSystemModule_->mvInterfaceTechnologyToIgnoreSelector.isValid() )
                        {
                            pSystemModule_->mvInterfaceTechnologyToIgnoreSelector.writeS( "U3V" );
                        }
                        pTreeCtrl_->SetItemState( interfaceTypeU3V, ( ignoredInterfacesInfo_[string( "U3V" )] == string( "1" ) ) ? 2 : 1 );
                    }
                    u3vInterfaceCount++;
                }
            }

            vector<map<wxString, bool> > deviceInformation = EnumerateGenICamDevices( interfaceCount, devMgr );
            for( unsigned int i = 0; i < interfaceCount; i++ )
            {
                pSystemModule_->interfaceSelector.write( i );
                wxTreeItemId interfaceTreeItem;
                const string interfaceID = pSystemModule_->interfaceID.readS();
                const IgnoredInterfacesInfo::const_iterator it = ignoredInterfacesInfo_.find( interfaceID );
                assert( ( it != itEND ) && "At this point there should be an interfaceID entry for each interface in the ignored interfaces information map!" );
                if( it == itEND )
                {
                    continue;
                }
                if( pSystemModule_->interfaceType.readS() == string( "GEV" ) )
                {
                    interfaceTreeItem = pTreeCtrl_->AppendItem( interfaceTypeGEV, wxString::Format( wxT( "%s(%s)" ), ConvertedString( interfaceID ).c_str(), ConvertedString( pSystemModule_->interfaceDisplayName.readS() ).c_str() ) );
                    pTreeCtrl_->SetItemState( interfaceTreeItem, ( it->second == string( "NotConfigured" ) ) ? pTreeCtrl_->GetItemState( interfaceTypeGEV ) : ( it->second == string( "ForceIgnore" ) ) ? 2 : 1 );
                }
                else if( pSystemModule_->interfaceType.readS() == string( "U3V" ) )
                {
                    interfaceTreeItem = pTreeCtrl_->AppendItem( interfaceTypeU3V, wxString::Format( wxT( "%s(%s)" ), ConvertedString( interfaceID ).c_str(), ConvertedString( pSystemModule_->interfaceDisplayName.readS() ).c_str() ) );
                    pTreeCtrl_->SetItemState( interfaceTreeItem, ( it->second == string( "NotConfigured" ) ) ? pTreeCtrl_->GetItemState( interfaceTypeU3V ) : ( it->second == string( "ForceIgnore" ) ) ? 2 : 1 );
                }
                if( !deviceInformation[i].empty() )
                {
                    map<wxString, bool>::const_iterator itDevices = deviceInformation[i].begin();
                    const map<wxString, bool>::const_iterator itDevicesEND = deviceInformation[i].end();
                    while( itDevices != itDevicesEND )
                    {
                        wxTreeItemId deviceTreeItem = pTreeCtrl_->AppendItem( interfaceTreeItem, itDevices->first );
                        if( itDevices->second )
                        {
                            pTreeCtrl_->SetItemTextColour( deviceTreeItem, acDarkGrey );
                        }
                        ++itDevices;
                    }
                }
            }
            SetParentUndefinedStateIfConflictingChildrenStates( interfaceTypeGEV );
            SetParentUndefinedStateIfConflictingChildrenStates( interfaceTypeU3V );
        }
        else
        {
            const wxTreeItemId devices = AddComponentListToList( newItem, locator, "Devices" );
            if( devices.IsOk() )
            {
                ComponentIterator itDev( locator.findComponent( "Devices" ) );
                itDev = itDev.firstChild();
                while( itDev.isValid() )
                {
                    const PropertyS serial( itDev );
                    const Device* const pDev = devMgr.getDeviceBySerial( serial.read() );
                    const bool boInUse = pDev && pDev->isInUse();
                    const wxTreeItemId deviceTreeItem = pTreeCtrl_->AppendItem( devices, wxString::Format( wxT( "%s(%s)%s" ), ConvertedString( serial.read() ).c_str(), ConvertedString( pDev->product.read() ).c_str(), boInUse ? wxT( " - IN USE!" ) : wxT( "" ) ) );
                    if( boInUse )
                    {
                        pTreeCtrl_->SetItemTextColour( deviceTreeItem, acDarkGrey );
                    }
                    itDev = itDev.nextSibling();
                }
            }
        }
        itDrivers = itDrivers.nextSibling();
    }
}

//-----------------------------------------------------------------------------
void DriverInformationDlg::ReadIgnoredInterfacesInfoAndEnableAllInterfaces()
//-----------------------------------------------------------------------------
{
    if( pSystemModule_ )
    {
        ReadIgnoredInterfaceTechnologyInfoAndEnableIt( "GEV" );
        ReadIgnoredInterfaceTechnologyInfoAndEnableIt( "U3V" );
        const unsigned int interfaceCount = pSystemModule_->interfaceSelector.getMaxValue() + 1;
        for( unsigned int i = 0; i < interfaceCount; i++ )
        {
            pSystemModule_->interfaceSelector.write( i );
            const string value( pSystemModule_->mvDeviceUpdateListBehaviour.isValid() ? pSystemModule_->mvDeviceUpdateListBehaviour.readS() : "NotConfigured" );
            ignoredInterfacesInfo_.insert( make_pair( pSystemModule_->interfaceID.readS(), value ) );
            ignoredInterfacesInfoOriginal_.insert( make_pair( pSystemModule_->interfaceID.readS(), value ) );
            if( pSystemModule_->mvDeviceUpdateListBehaviour.isValid() )
            {
                pSystemModule_->mvDeviceUpdateListBehaviour.writeS( "NotConfigured" );
            }
        }
    }
}

//-----------------------------------------------------------------------------
void DriverInformationDlg::ReadIgnoredInterfaceTechnologyInfoAndEnableIt( const string& technology )
//-----------------------------------------------------------------------------
{
    string value( "0" );
    if( pSystemModule_->mvInterfaceTechnologyToIgnoreSelector.isValid() && pSystemModule_->mvInterfaceTechnologyToIgnoreEnable.isValid() )
    {
        pSystemModule_->mvInterfaceTechnologyToIgnoreSelector.writeS( technology );
        value = ( pSystemModule_->mvInterfaceTechnologyToIgnoreEnable.read() == bTrue ) ? "1" : "0";
    }
    ignoredInterfacesInfo_.insert( make_pair( technology, value ) );
    ignoredInterfacesInfoOriginal_.insert( make_pair( technology, value ) );
    if( pSystemModule_->mvInterfaceTechnologyToIgnoreEnable.isValid() )
    {
        pSystemModule_->mvInterfaceTechnologyToIgnoreEnable.write( bFalse );
    }
}

//-----------------------------------------------------------------------------
void DriverInformationDlg::UpdateIgnoredInterfacesSettings( IgnoredInterfacesInfo& interfacesInfo )
//-----------------------------------------------------------------------------
{
    if( pSystemModule_ && pSystemModule_->mvDeviceUpdateListBehaviour.isValid() )
    {
        IgnoreInterfaceTechnologySetting( "GEV", interfacesInfo );
        IgnoreInterfaceTechnologySetting( "U3V", interfacesInfo );

        const IgnoredInterfacesInfo::const_iterator itGEV = interfacesInfo.find( "GEV" );
        const IgnoredInterfacesInfo::const_iterator itU3V = interfacesInfo.find( "U3V" );
        const IgnoredInterfacesInfo::const_iterator itEND = interfacesInfo.end();
        assert( ( itGEV != itEND ) && "At this point there should be an 'GEV' entry in the ignored interfaces information map!" );
        assert( ( itU3V != itEND ) && "At this point there should be an 'U3V' entry in the ignored interfaces information map!" );

        const unsigned int interfaceCount = pSystemModule_->interfaceSelector.getMaxValue() + 1;
        for( unsigned int i = 0; i < interfaceCount; i++ )
        {
            pSystemModule_->interfaceSelector.write( i );
            const string interfaceID( pSystemModule_->interfaceID.readS() );
            const IgnoredInterfacesInfo::const_iterator it = interfacesInfo.find( interfaceID );
            if( it == itEND )
            {
                continue;
            }
            if( it->second == string( "NotConfigured" ) )
            {
                pSystemModule_->mvDeviceUpdateListBehaviour.writeS( "NotConfigured" );
            }
            else if( it->second == string( "ForceEnumerate" ) )
            {
                if( ( ( pSystemModule_->interfaceType.readS() == string( "GEV" ) ) && ( itGEV->second == string( "1" ) ) ) ||
                    ( ( pSystemModule_->interfaceType.readS() == string( "U3V" ) ) && ( itU3V->second == string( "1" ) ) ) )
                {
                    pSystemModule_->mvDeviceUpdateListBehaviour.writeS( "ForceEnumerate" );
                }
                else
                {
                    pSystemModule_->mvDeviceUpdateListBehaviour.writeS( "NotConfigured" );
                }
            }
            else if( it->second == string( "ForceIgnore" ) )
            {
                if( ( ( pSystemModule_->interfaceType.readS() == string( "GEV" ) ) && ( itGEV->second == string( "0" ) ) ) ||
                    ( ( pSystemModule_->interfaceType.readS() == string( "U3V" ) ) && ( itU3V->second == string( "0" ) ) ) )
                {
                    pSystemModule_->mvDeviceUpdateListBehaviour.writeS( "ForceIgnore" );
                }
                else
                {
                    pSystemModule_->mvDeviceUpdateListBehaviour.writeS( "NotConfigured" );
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void DriverInformationDlg::UpdateIgnoredInterfacesInfo( const wxTreeItemId& itemId )
//-----------------------------------------------------------------------------
{
    if( string( pTreeCtrl_->GetItemText( itemId ) ) == string( "GigEVision" ) ||
        string( pTreeCtrl_->GetItemText( itemId ) ) == string( "USB3Vision" ) )
    {
        ignoredInterfacesInfo_[( string( pTreeCtrl_->GetItemText( itemId ) ) == string( "GigEVision" ) ) ? string( "GEV" ) : string( "U3V" )] = ( pTreeCtrl_->GetItemState( itemId ) == 2 ) ? string( "1" ) : string( "0" );

        // recursively update the settings of all children(interfaces of the same type)
        wxTreeItemIdValue childVal;
        wxTreeItemId childId = pTreeCtrl_->GetFirstChild( itemId, childVal );
        while( childId.IsOk() )
        {
            const string itemText( pTreeCtrl_->GetItemText( childId ) );
            IgnoredInterfacesInfo::iterator it = ignoredInterfacesInfo_.begin();
            const IgnoredInterfacesInfo::iterator itEND = ignoredInterfacesInfo_.end();
            while( it != itEND )
            {
                if( itemText.find( it->first ) != string::npos )
                {
                    it->second = string( "NotConfigured" );
                    break;
                }
                ++it;
            }
            childId = pTreeCtrl_->GetNextChild( itemId, childVal );
        }
    }
    else
    {
        // Depending on the parent setting the NotConfigured option should be used.
        const wxTreeItemId parentId = pTreeCtrl_->GetItemParent( itemId );

        // Since configuring a child can modify the parent state too, we have to update the ignoredInterfacesInfo_ to
        // reflect this change, before we do anything else.
        ignoredInterfacesInfo_[( string( pTreeCtrl_->GetItemText( parentId ) ) == string( "GigEVision" ) ) ? string( "GEV" ) : string( "U3V" )] = ( pTreeCtrl_->GetItemState( parentId ) == 2 ) ? string( "1" ) : string( "0" );

        const bool boParentIgnored = ignoredInterfacesInfo_[( string( pTreeCtrl_->GetItemText( parentId ) ) == string( "GigEVision" ) ) ? string( "GEV" ) : string( "U3V" )] == string( "1" );
        const string itemText( pTreeCtrl_->GetItemText( itemId ) );
        IgnoredInterfacesInfo::iterator it = ignoredInterfacesInfo_.begin();
        const IgnoredInterfacesInfo::iterator itEND = ignoredInterfacesInfo_.end();
        while( it != itEND )
        {
            if( itemText.find( it->first ) != string::npos )
            {
                if( pTreeCtrl_->GetItemState( itemId ) == lisUnchecked )
                {
                    it->second = boParentIgnored ? string( "NotConfigured" ) : string( "ForceIgnore" );
                }
                else if( pTreeCtrl_->GetItemState( itemId ) == lisChecked )
                {
                    it->second = boParentIgnored ? string( "ForceEnumerate" ) : string( "NotConfigured" );
                }
                break;
            }
            ++it;
        }
    }
}

//=============================================================================
//=================== Implementation UpdatesInformationDlg ====================
//=============================================================================
BEGIN_EVENT_TABLE( UpdatesInformationDlg, OkAndCancelDlg )
    EVT_CLOSE( UpdatesInformationDlg::OnClose )
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
UpdatesInformationDlg::UpdatesInformationDlg( wxWindow* pParent, const wxString& title, const StringToStringMap& olderDriverVersions,
        const wxString& currentVersion, const wxString& newestVersion, const wxString& dateReleased, const wxString& whatsNew ) :
    OkAndCancelDlg( pParent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize ), olderDriverVersions_( olderDriverVersions ),
    currentVersion_( currentVersion ), newestVersion_( newestVersion ), dateReleased_( dateReleased ), whatsNew_( whatsNew )
//-----------------------------------------------------------------------------
{
    /*
    |-------------------------------------|
    | pTopDownSizer                       |
    |                spacer               |
    | |---------------------------------| |
    | |          Release Info           | |
    | |---------------------------------| |
    |                spacer               |
    | |---------------------------------| |
    | |            WhatsNew             | |
    | |---------------------------------| |
    |-------------------------------------|
    */

    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    pTopDownSizer->AddSpacer( 20 );
    wxPanel* pPanel = new wxPanel( this );

    wxFont tmpFont = pPanel->GetFont();
    tmpFont.SetPointSize( pPanel->GetFont().GetPointSize() + 1 );

    wxStaticText* pVersionText = new wxStaticText( pPanel, wxID_ANY, ( VersionFromString( currentVersion_ ) < VersionFromString( newestVersion_ ) ) ? wxT( "A newer mvIMPACT Acquire version has been released:" ) : wxT( "The newest mvIMPACT Acquire version has already been installed on your system:" ) );
    pVersionText->SetFont( tmpFont );

    wxStaticText* pNewVersion = new wxStaticText( pPanel, wxID_ANY, ( VersionFromString( newestVersion_ ) >= VersionFromString( currentVersion_ ) ) ? newestVersion_ : currentVersion_ );
    wxStaticText* pDateText = new wxStaticText( pPanel, wxID_ANY, wxT( "Release date:" ) );
    wxStaticText* pDate = new wxStaticText( pPanel, wxID_ANY, ( VersionFromString( newestVersion_ ) >= VersionFromString( currentVersion_ ) ) ? dateReleased_ : wxString( wxT( "Not yet released" ) ) );
    wxStaticText* pWhatsNewText = new wxStaticText( pPanel, wxID_ANY, wxT( "What's new:" ) );

    wxTextCtrl* pWhatsNew = new wxTextCtrl( pPanel, wxID_ANY, whatsNew_, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxBORDER_NONE | wxTE_RICH | wxTE_READONLY | wxTE_DONTWRAP );
    wxFont* fixedWidthFont = new wxFont( 8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
    pWhatsNew->SetFont( *fixedWidthFont );
    pWhatsNew->ShowPosition( 0 );
    pTopDownSizer->Add( pVersionText, wxSizerFlags().Align( wxALIGN_CENTRE_HORIZONTAL ) );
    pTopDownSizer->Add( pNewVersion, wxSizerFlags().Align( wxALIGN_CENTRE_HORIZONTAL ) );
    if( VersionFromString( currentVersion_ ) < VersionFromString( newestVersion_ ) )
    {
        pNewVersion->SetForegroundColour( *wxBLUE );
        wxStaticText* pOldVersion = new wxStaticText( pPanel, wxID_ANY, wxString::Format( wxT( "(current version is %s)" ), currentVersion_.c_str() ) );
        pTopDownSizer->Add( pOldVersion, wxSizerFlags().Align( wxALIGN_CENTRE_HORIZONTAL ) );
    }

    if( olderDriverVersions.empty() )
    {
        pTopDownSizer->AddSpacer( 25 );
        pTopDownSizer->Add( pDateText, wxSizerFlags().Align( wxALIGN_CENTRE_HORIZONTAL ) );
        pTopDownSizer->Add( pDate, wxSizerFlags().Align( wxALIGN_CENTRE_HORIZONTAL ) );
        pTopDownSizer->AddSpacer( 25 );
    }
    else
    {
        wxStaticText* pDriversNotUpToDateText = new wxStaticText( pPanel, wxID_ANY, wxString::Format( wxT( "%s, following drivers are not up to date:" ), VersionFromString( currentVersion_ ) < VersionFromString( newestVersion_ ) ? "Furthermore" : "However" ) );
        wxStaticText* pDriversUpdateRecommendationText = new wxStaticText( pPanel, wxID_ANY, wxT( "It is recommended that all installed drivers are of the same version!" ) );
        pDriversNotUpToDateText->SetFont( tmpFont );
        pDriversUpdateRecommendationText->SetFont( tmpFont );

        pTopDownSizer->AddSpacer( 10 );
        pTopDownSizer->Add( pDateText, wxSizerFlags().Align( wxALIGN_CENTRE_HORIZONTAL ) );
        pTopDownSizer->Add( pDate, wxSizerFlags().Align( wxALIGN_CENTRE_HORIZONTAL ) );
        pTopDownSizer->AddSpacer( 25 );
        pTopDownSizer->Add( pDriversNotUpToDateText, wxSizerFlags().Align( wxALIGN_CENTRE_HORIZONTAL ) );

        StringToStringMap::const_iterator it = olderDriverVersions_.begin();
        StringToStringMap::const_iterator itEND = olderDriverVersions_.end();
        for( ; it != itEND; it++ )
        {
            wxStaticText* pOldDriverText = new wxStaticText( pPanel, wxID_ANY, wxString::Format( wxT( "%s(Version %s)" ), ( it->first ).mb_str(), ( it->second ).mb_str() ) );
            pOldDriverText->SetForegroundColour( *wxRED );
            pTopDownSizer->Add( pOldDriverText, wxSizerFlags().Align( wxALIGN_CENTRE_HORIZONTAL ) );
        }
        pTopDownSizer->AddSpacer( 10 );
        pTopDownSizer->Add( pDriversUpdateRecommendationText, wxSizerFlags().Align( wxALIGN_CENTRE_HORIZONTAL ) );
        pTopDownSizer->AddSpacer( 25 );
    }

    pTopDownSizer->Add( pWhatsNewText, wxSizerFlags().Align( wxALIGN_CENTRE_HORIZONTAL ) );
    pTopDownSizer->AddSpacer( 10 );
    pTopDownSizer->Add( pWhatsNew, wxSizerFlags().Expand() );
    pTopDownSizer->AddSpacer( 10 );

    pDateText->SetFont( tmpFont );
    pWhatsNewText->SetFont( tmpFont );
    tmpFont.SetWeight( wxFONTWEIGHT_BOLD );
    pDate->SetFont( tmpFont );
    tmpFont.SetPointSize( tmpFont.GetPointSize() + 1 );
    pNewVersion->SetFont( tmpFont );

    // lower line of buttons
    wxBoxSizer* pButtonSizer = new wxBoxSizer( wxHORIZONTAL );
    pButtonSizer->AddStretchSpacer( 100 );
    if( ( !olderDriverVersions_.empty() ) ||
        ( VersionFromString( currentVersion_ ) < VersionFromString( newestVersion_ ) ) )
    {
        pBtnOk_ = new wxButton( pPanel, widBtnOk, wxT( "&Go To Download Page" ) );
        pButtonSizer->Add( pBtnOk_, wxSizerFlags().Border( wxALL, 7 ) );
    }
    pBtnCancel_ = new wxButton( pPanel, widBtnCancel, wxT( "&Ok" ) );
    pButtonSizer->Add( pBtnCancel_, wxSizerFlags().Border( wxALL, 7 ) );
    pTopDownSizer->Add( pButtonSizer, wxSizerFlags().Expand() );

    FinalizeDlgCreation( pPanel, pTopDownSizer );
    SetSize( 640, -1 );
    Center();
}

//=============================================================================
//============== Implementation FindFeatureDlg ================================
//=============================================================================
BEGIN_EVENT_TABLE( FindFeatureDlg, OkAndCancelDlg )
    EVT_CHECKBOX( widCBMatchCase, FindFeatureDlg::OnMatchCaseChanged )
    EVT_LISTBOX( widLBFeatureList, FindFeatureDlg::OnFeatureListSelect )
    EVT_LISTBOX_DCLICK( widLBFeatureList, FindFeatureDlg::OnFeatureListDblClick )
    EVT_TEXT( widTCFeatureName, FindFeatureDlg::OnFeatureNameTextChanged )
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
FindFeatureDlg::FindFeatureDlg( PropGridFrameBase* pParent, const NameToFeatureMap& nameToFeatureMap, const bool boMatchCaseActive ) :
    OkAndCancelDlg( pParent, wxID_ANY, wxT( "Find Feature" ), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX ),
    pParent_( pParent ), nameToFeatureMap_( nameToFeatureMap )
//-----------------------------------------------------------------------------
{
    /*
        |-------------------------------------|
        | pTopDownSizer                       |
        |                spacer               |
        |                message              |
        |                spacer               |
        | |---------------------------------| |
        | | pLBFeatureList_                 | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pButtonSizer                    | |
        | |---------------------------------| |
        |-------------------------------------|
    */

    wxPanel* pPanel = new wxPanel( this );

    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    pTopDownSizer->AddSpacer( 10 );
    pTopDownSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( " Type the name of feature you are looking for." ) ) );
    pTopDownSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( " " ) ) );
    pTopDownSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( " A Double-click on a list item will automatically select the feature and close this dialog. " ) ) );
    pTopDownSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( " " ) ) );
    pTopDownSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( " Use '|' to combine multiple search strings(whitespaces will be interpreted as part of the " ) ) );
    pTopDownSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( " tokens to search for). " ) ) );
    pTopDownSizer->AddSpacer( 10 );
    pTCFeatureName_ = new wxTextCtrl( pPanel, widTCFeatureName );
    pTopDownSizer->Add( pTCFeatureName_, wxSizerFlags().Expand() );
    pTopDownSizer->AddSpacer( 10 );
    pCBMatchCase_ = new wxCheckBox( pPanel, widCBMatchCase, wxT( "Match &case" ) );
    pCBMatchCase_->SetValue( boMatchCaseActive );
    pTopDownSizer->Add( pCBMatchCase_ );
    pTopDownSizer->AddSpacer( 10 );
    wxArrayString features;
    BuildFeatureList( features );
    pLBFeatureList_ = new wxListBox( pPanel, widLBFeatureList, wxDefaultPosition, wxDefaultSize, features );
    pTopDownSizer->Add( pLBFeatureList_, wxSizerFlags( 6 ).Expand() );
    AddButtons( pPanel, pTopDownSizer );
    FinalizeDlgCreation( pPanel, pTopDownSizer );
    SetSize( 200, 250 );
}

//-----------------------------------------------------------------------------
void FindFeatureDlg::BuildFeatureList( wxArrayString& features, const bool boMatchCase /* = false */, const wxString& pattern /* = wxEmptyString */ ) const
//-----------------------------------------------------------------------------
{
    NameToFeatureMap::const_iterator it = nameToFeatureMap_.begin();
    const NameToFeatureMap::const_iterator itEND = nameToFeatureMap_.end();
    wxArrayString searchTokens = wxSplit( pattern, wxT( '|' ) );
    wxArrayString::size_type searchTokenCount = searchTokens.size();
    while( it != itEND )
    {
        bool boAddToList = pattern.IsEmpty();
        if( !boAddToList )
        {
            const wxString candidate( boMatchCase ? it->first : it->first.Lower() );
            for( wxArrayString::size_type i = 0; i < searchTokenCount; i++ )
            {
                if( candidate.Find( searchTokens[i].c_str() ) != wxNOT_FOUND )
                {
                    boAddToList = true;
                    break;
                }
            }
        }

        if( boAddToList )
        {
            Component c( it->second->GetComponent() );
            if( c.visibility() != cvInvisible )
            {
                const size_t index = features.Add( it->first );
                if( GlobalDataStorage::Instance()->GetComponentVisibility() < c.visibility() )
                {
                    features[index].Append( wxString::Format( wxT( " (%s level)" ), ConvertedString( c.visibilityAsString() ).c_str() ) );
                }
                if( !c.isVisible() )
                {
                    features[index].Append( wxT( " (currently invisible)" ) );
                }
            }
        }
        ++it;
    }
}

//-----------------------------------------------------------------------------
void FindFeatureDlg::OnFeatureListDblClick( wxCommandEvent& e )
//-----------------------------------------------------------------------------
{
    SelectFeatureInPropertyGrid( e.GetSelection() );
    Close();
}

//-----------------------------------------------------------------------------
void FindFeatureDlg::SelectFeatureInPropertyGrid( const wxString& selection )
//-----------------------------------------------------------------------------
{
    NameToFeatureMap::const_iterator it = nameToFeatureMap_.find( selection );
    if( it != nameToFeatureMap_.end() )
    {
        pParent_->SelectPropertyInPropertyGrid( it->second );
    }
}

//-----------------------------------------------------------------------------
void FindFeatureDlg::SelectFeatureInPropertyGrid( const int selection )
//-----------------------------------------------------------------------------
{
    SelectFeatureInPropertyGrid( pLBFeatureList_->GetString( selection ) );
}

//-----------------------------------------------------------------------------
void FindFeatureDlg::UpdateFeatureList( void )
//-----------------------------------------------------------------------------
{
    wxString pattern( pCBMatchCase_->GetValue() ? pTCFeatureName_->GetValue() : pTCFeatureName_->GetValue().Lower() );
    while( !pattern.IsEmpty() && ( pattern.Last() == wxT( '|' ) ) )
    {
        pattern.RemoveLast();
    }
    pLBFeatureList_->Clear();
    wxArrayString features;
    BuildFeatureList( features, pCBMatchCase_->GetValue(), pattern );
    if( !features.empty() )
    {
        pLBFeatureList_->InsertItems( features, 0 );
    }
}

//=============================================================================
//============== Implementation DetailedFeatureInfoDlg ========================
//=============================================================================
//-----------------------------------------------------------------------------
DetailedFeatureInfoDlg::DetailedFeatureInfoDlg( wxWindow* pParent, Component comp ) :
    OkAndCancelDlg( pParent, wxID_ANY, wxT( "Detailed Feature Info" ), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX ),
    pLogWindow_( 0 )
//-----------------------------------------------------------------------------
{
    fixedPitchStyle_.SetFont( wxFont( 10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL ) );
    fixedPitchStyle_.SetTextColour( *wxBLUE );
    wxFont font( 10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD );
    //font.SetUnderlined( true );
    fixedPitchStyleBold_.SetFont( font );
    /*
        |-------------------------------------|
        | pFlexGridSizer                      |
        |                spacer               |
        | |---------------------------------| |
        | | pButtonSizer                    | |
        | |---------------------------------| |
        |-------------------------------------|
    */
    wxPanel* pPanel = new wxPanel( this );
    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    pLogWindow_ = new wxTextCtrl( pPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxBORDER_NONE | wxTE_RICH | wxTE_READONLY );
    AddFeatureInfo( wxT( "Component Name: " ), ConvertedString( comp.name() ) );
    AddFeatureInfo( wxT( "Component Display Name: " ), ConvertedString( comp.displayName() ) );
    AddFeatureInfo( wxT( "Component Description: " ), ConvertedString( comp.docString() ) );
    AddFeatureInfo( wxT( "Component Type: " ), ConvertedString( comp.typeAsString() ) );
    AddFeatureInfo( wxT( "Component Visibility: " ), ConvertedString( comp.visibilityAsString() ) );
    AddFeatureInfo( wxT( "Component Representation: " ), ConvertedString( comp.representationAsString() ) );
    AddFeatureInfo( wxT( "Component Flags: " ), ConvertedString( comp.flagsAsString() ) );
    AddFeatureInfo( wxT( "Component Default State: " ), comp.isDefault() ? wxT( "Yes" ) : wxT( "No" ) );
    {
        vector<mvIMPACT::acquire::Component> selectingFeatures;
        comp.selectingFeatures( selectingFeatures );
        ostringstream oss;
        if( selectingFeatures.empty() )
        {
            oss << "NO OTHER FEATURE";
        }
        else
        {
            PropData::AppendSelectorInfo( oss, selectingFeatures );
        }
        AddFeatureInfo( wxT( "Component Is Selected By: " ), ConvertedString( oss.str() ) );
    }
    {
        vector<mvIMPACT::acquire::Component> selectedFeatures;
        comp.selectedFeatures( selectedFeatures );
        ostringstream oss;
        if( selectedFeatures.empty() )
        {
            oss << "NO OTHER FEATURE";
        }
        else
        {
            PropData::AppendSelectorInfo( oss, selectedFeatures );
        }
        AddFeatureInfo( wxT( "Component Selects: " ), ConvertedString( oss.str() ) );
    }
    AddFeatureInfo( wxT( "Component Handle(HOBJ): " ), wxString::Format( wxT( "0x%08x" ), comp.hObj() ) );
    AddFeatureInfo( wxT( "Component Changed Counter: " ), wxString::Format( wxT( "%d" ), comp.changedCounter() ) );
    AddFeatureInfo( wxT( "Component Changed Counter(Attributes): " ), wxString::Format( wxT( "%d" ), comp.changedCounterAttr() ) );
    const TComponentType type( comp.type() );
    if( type == ctList )
    {
        ComponentList list( comp );
        AddFeatureInfo( wxT( "List Content Descriptor: " ), ConvertedString( list.contentDescriptor() ) );
        AddFeatureInfo( wxT( "List Size: " ), wxString::Format( wxT( "%d" ), list.size() ) );
    }
    else if( type == ctMeth )
    {
        Method meth( comp );
        AddFeatureInfo( wxT( "Method Parameter List: " ), ConvertedString( meth.paramList() ) );
        AddFeatureInfo( wxT( "Method Signature: " ), MethodObject::BuildFriendlyName( meth.hObj() ) );
    }
    else if( type & ctProp )
    {
        Property prop( comp );
        AddFeatureInfo( wxT( "Property String Format String: " ), ConvertedString( prop.stringFormatString() ) );
        const int valCnt = static_cast<int>( prop.valCount() );
        AddFeatureInfo( wxT( "Property Value Count: " ), wxString::Format( wxT( "%d" ), valCnt ) );
        AddFeatureInfo( wxT( "Property Value Count(max): " ), wxString::Format( wxT( "%u" ), prop.maxValCount() ) );
        if( ( prop.type() == ctPropString ) && ( prop.flags() & cfContainsBinaryData ) )
        {
            // The 'prop.type()' check is only needed because some drivers with versions < 1.12.33
            // did incorrectly specify the 'cfContainsBinaryData' flag even though the data type was not 'ctPropString'...
            for( int i = 0; i < valCnt; i++ )
            {
                PropertyS propS( comp );
                wxString binaryDataRAW( ConvertedString( BinaryDataToString( propS.readBinary( i ) ) ) );
                wxString binaryDataFormatted;
                BinaryDataDlg::FormatData( binaryDataRAW, binaryDataFormatted, 64, 8 );
                binaryDataFormatted.RemoveLast(); // Remove the last '\n' as this will be added by 'AddFeatureInfo' as well!
                AddFeatureInfo( wxString::Format( wxT( "Property Value[%d]: " ), i ), wxEmptyString );
                AddFeatureInfo( wxEmptyString, binaryDataFormatted );
                AddFeatureInfo( wxString::Format( wxT( "Property Value Binary Buffer Size[%d]: " ), i ), wxString::Format( wxT( "%u" ), propS.binaryDataBufferSize( i ) ) );
            }
        }
        else
        {
            for( int i = 0; i < valCnt; i++ )
            {
                AddFeatureInfo( wxString::Format( wxT( "Property Value[%d]: " ), i ), ConvertedString( prop.readS( i, string( ( prop.flags() & cfAllowValueCombinations ) ? "\"%s\" " : "" ) ) ) );
            }
        }
        AddFeatureInfo( wxT( "Property Value(min): " ), ConvertedString( prop.hasMinValue() ? prop.readS( plMinValue ) : string( "NOT DEFINED" ) ) );
        AddFeatureInfo( wxT( "Property Value(max): " ), ConvertedString( prop.hasMaxValue() ? prop.readS( plMaxValue ) : string( "NOT DEFINED" ) ) );
        AddFeatureInfo( wxT( "Property Value(inc): " ), ConvertedString( prop.hasStepWidth() ? prop.readS( plStepWidth ) : string( "NOT DEFINED" ) ) );
        if( ( type == ctPropString ) && ( prop.flags() & cfContainsBinaryData ) )
        {
            PropertyS propS( comp );
            AddFeatureInfo( wxT( "Property Value Binary Buffer Size(max): " ), wxString::Format( wxT( "%u" ), propS.binaryDataBufferMaxSize() ) );
        }

        if( prop.hasDict() )
        {
            const wxString formatString = ConvertedString( prop.stringFormatString() );
            if( type == ctPropInt )
            {
                vector<pair<string, int> > dict;
                PropertyI( prop ).getTranslationDict( dict );
                const vector<pair<string, int> >::size_type vSize = dict.size();
                for( vector<pair<string, int> >::size_type i = 0; i < vSize; i++ )
                {
                    const wxString value = wxString::Format( formatString.c_str(), dict[i].second );
                    AddFeatureInfo( wxString::Format( wxT( "Property Translation Dictionary Entry[%u]: " ), static_cast<unsigned int>( i ) ), wxString::Format( wxT( "%s(%s)" ), ConvertedString( dict[i].first ).c_str(), value.c_str() ) );
                }
            }
            else if( type == ctPropInt64 )
            {
                vector<pair<string, int64_type> > dict;
                PropertyI64( prop ).getTranslationDict( dict );
                const vector<pair<string, int64_type> >::size_type vSize = dict.size();
                for( vector<pair<string, int64_type> >::size_type i = 0; i < vSize; i++ )
                {
                    const wxString value = wxString::Format( formatString.c_str(), dict[i].second );
                    AddFeatureInfo( wxString::Format( wxT( "Property Translation Dictionary Entry[%u]: " ), static_cast<unsigned int>( i ) ), wxString::Format( wxT( "%s(%s)" ), ConvertedString( dict[i].first ).c_str(), value.c_str() ) );
                }
            }
            else if( type == ctPropFloat )
            {
                vector<pair<string, double> > dict;
                PropertyF( prop ).getTranslationDict( dict );
                const vector<pair<string, double> >::size_type vSize = dict.size();
                for( vector<pair<string, double> >::size_type i = 0; i < vSize; i++ )
                {
                    const wxString value = wxString::Format( formatString.c_str(), dict[i].second );
                    AddFeatureInfo( wxString::Format( wxT( "Property Translation Dictionary Entry[%u]: " ), static_cast<unsigned int>( i ) ), wxString::Format( wxT( "%s(%s)" ), ConvertedString( dict[i].first ).c_str(), value.c_str() ) );
                }
            }
        }
    }
    pTopDownSizer->Add( pLogWindow_, wxSizerFlags( 6 ).Expand() );
    pLogWindow_->ScrollLines( -( 256 * 256 ) ); // make sure the text control always shows the beginning of the help text
    AddButtons( pPanel, pTopDownSizer );
    FinalizeDlgCreation( pPanel, pTopDownSizer );
    SetSize( 700, 700 );
}

//-----------------------------------------------------------------------------
void DetailedFeatureInfoDlg::AddFeatureInfo( const wxString& infoName, const wxString& info )
//-----------------------------------------------------------------------------
{
    WriteToTextCtrl( pLogWindow_, infoName, fixedPitchStyleBold_ );
    if( !infoName.IsEmpty() && ( infoName.Last() != wxChar( wxT( ' ' ) ) ) )
    {
        WriteToTextCtrl( pLogWindow_, wxT( " " ), fixedPitchStyle_ );
    }
    WriteToTextCtrl( pLogWindow_, info, fixedPitchStyle_ );
    WriteToTextCtrl( pLogWindow_, wxT( "\n" ) );
}

//=============================================================================
//============== Implementation BinaryDataDlg =================================
//=============================================================================
BEGIN_EVENT_TABLE( BinaryDataDlg, OkAndCancelDlg )
    EVT_TEXT( widTCBinaryData, BinaryDataDlg::OnBinaryDataTextChanged )
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
BinaryDataDlg::BinaryDataDlg( wxWindow* pParent, const wxString& featureName, const wxString& value ) :
    OkAndCancelDlg( pParent, wxID_ANY, wxString::Format( wxT( "Binary Data Editor(%s)" ), featureName.c_str() ), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX ),
    pTCBinaryData_( 0 ), pTCAsciiData_( 0 )
//-----------------------------------------------------------------------------
{
    /*
        |-------------------------------------|
        | pTopDownSizer                       |
        |                spacer               |
        | |---------------------------------| |
        | | pLeftRightSizer                 | |
        | |---------------------------------| |
        | |---------------------------------| |
        | | pButtonSizer                    | |
        | |---------------------------------| |
        |-------------------------------------|
    */

    wxPanel* pPanel = new wxPanel( this );

    wxFont fixedPitchFont( 10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
    fixedPitchStyle_.SetFont( fixedPitchFont );

    pTCBinaryData_ = new wxTextCtrl( pPanel, widTCBinaryData, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxBORDER_NONE | wxTE_RICH2, HEXStringValidator_ );
    pTCBinaryData_->SetDefaultStyle( fixedPitchStyle_ );
    pTCAsciiData_ = new wxTextCtrl( pPanel, widTCAsciiData, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxBORDER_NONE | wxTE_RICH2 | wxTE_READONLY );
    pTCAsciiData_->SetDefaultStyle( fixedPitchStyle_ );
    WriteToTextCtrl( pTCBinaryData_, value, fixedPitchStyle_ );
    ReformatBinaryData();

    wxBoxSizer* pLeftRightSizer = new wxBoxSizer( wxHORIZONTAL );
    pLeftRightSizer->Add( pTCBinaryData_, wxSizerFlags( 2 ).Expand() );
    pLeftRightSizer->Add( pTCAsciiData_, wxSizerFlags( 1 ).Expand() );

    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    pTopDownSizer->AddSpacer( 10 );
    pTopDownSizer->Add( pLeftRightSizer, wxSizerFlags( 6 ).Expand() );
    AddButtons( pPanel, pTopDownSizer );
    FinalizeDlgCreation( pPanel, pTopDownSizer );
    SetSize( 560, 200 );
}

//-----------------------------------------------------------------------------
size_t BinaryDataDlg::FormatData( const wxString& data, wxString& formattedData, const int lineLength, const int fieldLength )
//-----------------------------------------------------------------------------
{
    const size_t len = data.Length();
    for( size_t i = 0; i < len; i++ )
    {
        formattedData.Append( data[i] );
        if( i > 0 )
        {
            if( ( ( i + 1 ) % lineLength ) == 0 )
            {
                formattedData.Append( wxT( "\n" ) );
            }
            else if( ( ( i + 1 ) % fieldLength ) == 0 )
            {
                formattedData.Append( wxT( " " ) );
            }
        }
    }
    return formattedData.Length();
}

//-----------------------------------------------------------------------------
wxString BinaryDataDlg::GetBinaryData( void ) const
//-----------------------------------------------------------------------------
{
    wxString data( pTCBinaryData_->GetValue() );
    RemoveSeparatorChars( data );
    return data;
}

//-----------------------------------------------------------------------------
void BinaryDataDlg::OnBinaryDataTextChanged( wxCommandEvent& )
//-----------------------------------------------------------------------------
{
    ReformatBinaryData();
    UpdateAsciiData();
}

//-----------------------------------------------------------------------------
void BinaryDataDlg::ReformatBinaryData( void )
//-----------------------------------------------------------------------------
{
    wxString data( pTCBinaryData_->GetValue() );
    long x = 0;
    long y = 0;
    pTCBinaryData_->PositionToXY( pTCBinaryData_->GetInsertionPoint(), &x, &y );

    const size_t pos = static_cast<size_t>( ( y * 36 ) + x );
    if( ( ( x == 9 ) || ( x == 18 ) || ( x == 27 ) ) &&
        ( ( data.Length() > pos ) && ( data[pos] == wxT( ' ' ) ) ) )
    {
        ++x;
    }
    RemoveSeparatorChars( data );
    wxString formattedData;
    FormatData( data, formattedData, 32, 8 );
#if wxCHECK_VERSION(2, 7, 1)
    pTCBinaryData_->ChangeValue( formattedData ); // this function will NOT generate a wxEVT_COMMAND_TEXT_UPDATED event.
#else
    pTCBinaryData_->SetValue( formattedData ); // this function will generate a wxEVT_COMMAND_TEXT_UPDATED event and has been declared deprecated.
#endif // #if wxCHECK_VERSION(2, 7, 1)
    pTCBinaryData_->SetStyle( 0, pTCBinaryData_->GetLastPosition(), fixedPitchStyle_ );
    if( x == 36 )
    {
        if( lastDataLength_ > formattedData.Length() )
        {

        }
        else if( lastDataLength_ < formattedData.Length() )
        {
            x = 1;
            ++y;
        }
    }
    lastDataLength_ = formattedData.Length();
    pTCBinaryData_->SetInsertionPoint( pTCBinaryData_->XYToPosition( x, y ) );
}

//-----------------------------------------------------------------------------
void BinaryDataDlg::RemoveSeparatorChars( wxString& data )
//-----------------------------------------------------------------------------
{
    data.Replace( wxT( " " ), wxT( "" ) );
    data.Replace( wxT( "\n" ), wxT( "" ) );
    data.Replace( wxT( "\r" ), wxT( "" ) );
}

//-----------------------------------------------------------------------------
void BinaryDataDlg::UpdateAsciiData( void )
//-----------------------------------------------------------------------------
{
    string binaryDataRawANSI( GetBinaryData().mb_str() );
    string binaryData( BinaryDataFromString( binaryDataRawANSI ) );
    const string::size_type len = binaryData.size();
    for( string::size_type i = 0; i < len; i++ )
    {
        if( static_cast<unsigned char>( binaryData[i] ) < 32 ) // 32 is the whitespace
        {
            binaryData[i] = '.';
        }
    }
    wxString formattedData;
    FormatData( ConvertedString( binaryData ), formattedData, 16, 4 );
#if wxCHECK_VERSION(2, 7, 1)
    pTCAsciiData_->ChangeValue( formattedData ); // this function will NOT generate a wxEVT_COMMAND_TEXT_UPDATED event.
#else
    pTCAsciiData_->SetValue( formattedData ); // this function will generate a wxEVT_COMMAND_TEXT_UPDATED event and has been declared deprecated.
#endif // #if wxCHECK_VERSION(2, 7, 1)
    pTCAsciiData_->SetStyle( 0, pTCAsciiData_->GetLastPosition(), fixedPitchStyle_ );
}

//=============================================================================
//============== Implementation AssignSettingsToDisplaysDlg ===================
//=============================================================================
//-----------------------------------------------------------------------------
AssignSettingsToDisplaysDlg::AssignSettingsToDisplaysDlg( wxWindow* pParent, const wxString& title,
        const vector<pair<string, int> >& settings, const SettingToDisplayDict& settingToDisplayDict, size_t displayCount )
    : OkAndCancelDlg( pParent, wxID_ANY, title )
//-----------------------------------------------------------------------------
{
    /*
        |-------------------------------------|
        | pTopDownSizer                       |
        |                spacer               |
        |                message              |
        |                spacer               |
        | |---------------------------------| |
        | | pGridSizer                      | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pButtonSizer                    | |
        | |---------------------------------| |
        |-------------------------------------|
    */

    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    pTopDownSizer->AddSpacer( 10 );
    wxPanel* pPanel = new wxPanel( this, wxID_ANY, wxPoint( 5, 5 ) );
    pTopDownSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( " Whenever a request is returned by\n the driver it will be drawn onto the\n display the used capture setting\n has been assigned to." ) ) );
    pTopDownSizer->AddSpacer( 10 );
    const int colCount = static_cast<int>( settings.size() );
    ctrls_.resize( colCount );
    wxFlexGridSizer* pGridSizer = new wxFlexGridSizer( 2 );
    wxArrayString choices;
    for( size_t i = 0; i < displayCount; i++ )
    {
        choices.Add( wxString::Format( wxT( "Display %u" ), static_cast<unsigned int>( i ) ) );
    }
    for( int i = 0; i < colCount; i++ )
    {
        pGridSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxString( wxT( " " ) ) + ConvertedString( settings[i].first ) ) );
        SettingToDisplayDict::const_iterator it = settingToDisplayDict.find( settings[i].second );
        wxArrayString::size_type selection = 0;
        if( settingToDisplayDict.empty() )
        {
            selection = i % displayCount;
        }
        else
        {
            selection = ( it != settingToDisplayDict.end() ) ? static_cast<wxArrayString::size_type>( it->second ) : 0;
        }
        ctrls_[i] = new wxComboBox( pPanel, widFirst + i, choices[selection], wxDefaultPosition, wxDefaultSize, choices, wxCB_DROPDOWN | wxCB_READONLY );
        pGridSizer->Add( ctrls_[i], wxSizerFlags().Expand() );
    }
    pTopDownSizer->Add( pGridSizer );
    AddButtons( pPanel, pTopDownSizer );
    FinalizeDlgCreation( pPanel, pTopDownSizer );
}

//=============================================================================
//============== Implementation RawImageImportDlg =============================
//=============================================================================
//-----------------------------------------------------------------------------
RawImageImportDlg::RawImageImportDlg( PropGridFrameBase* pParent, const wxString& title, const wxFileName& fileName )
    : OkAndCancelDlg( pParent, wxID_ANY, title ), pParent_( pParent )
//-----------------------------------------------------------------------------
{
    /*
        |-------------------------------------|
        | pTopDownSizer                       |
        |                spacer               |
        | |---------------------------------| |
        | | pGridSizer                      | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pButtonSizer                    | |
        | |---------------------------------| |
        |-------------------------------------|
    */

    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    pTopDownSizer->AddSpacer( 10 );

    wxPanel* pPanel = new wxPanel( this );
    wxFlexGridSizer* pGridSizer = new wxFlexGridSizer( 2 );
    wxArrayString pixelFormatChoices;
    pixelFormatChoices.Add( wxT( "Mono8" ) );
    pixelFormatChoices.Add( wxT( "Mono10" ) );
    pixelFormatChoices.Add( wxT( "Mono12" ) );
    pixelFormatChoices.Add( wxT( "Mono12Packed_V1" ) );
    pixelFormatChoices.Add( wxT( "Mono12Packed_V2" ) );
    pixelFormatChoices.Add( wxT( "Mono14" ) );
    pixelFormatChoices.Add( wxT( "Mono16" ) );
    pixelFormatChoices.Add( wxT( "BGR888Packed" ) );
    pixelFormatChoices.Add( wxT( "BGR101010Packed_V2" ) );
    pixelFormatChoices.Add( wxT( "RGB888Packed" ) );
    pixelFormatChoices.Add( wxT( "RGB101010Packed" ) );
    pixelFormatChoices.Add( wxT( "RGB121212Packed" ) );
    pixelFormatChoices.Add( wxT( "RGB141414Packed" ) );
    pixelFormatChoices.Add( wxT( "RGB161616Packed" ) );
    pixelFormatChoices.Add( wxT( "RGBx888Packed" ) );
    pixelFormatChoices.Add( wxT( "RGB888Planar" ) );
    pixelFormatChoices.Add( wxT( "RGBx888Planar" ) );
    pixelFormatChoices.Add( wxT( "YUV411_UYYVYY_Packed" ) );
    pixelFormatChoices.Add( wxT( "YUV422Packed" ) );
    pixelFormatChoices.Add( wxT( "YUV422_UYVYPacked" ) );
    pixelFormatChoices.Add( wxT( "YUV422_10Packed" ) );
    pixelFormatChoices.Add( wxT( "YUV422_UYVY_10Packed" ) );
    pixelFormatChoices.Add( wxT( "YUV444_UYVPacked" ) );
    pixelFormatChoices.Add( wxT( "YUV444_UYV_10Packed" ) );
    pixelFormatChoices.Add( wxT( "YUV444Packed" ) );
    pixelFormatChoices.Add( wxT( "YUV444_10Packed" ) );
    pixelFormatChoices.Add( wxT( "YUV422Planar" ) );

    wxArrayString bayerParityChoices;
    bayerParityChoices.Add( wxT( "Undefined" ) );
    bayerParityChoices.Add( wxT( "Red-green" ) );
    bayerParityChoices.Add( wxT( "Green-red" ) );
    bayerParityChoices.Add( wxT( "Blue-green" ) );
    bayerParityChoices.Add( wxT( "Green-blue" ) );

    // The file name is expected to look like this:
    // <name(don't care).<width>x<height>.<pixel format>(BayerPattern=<pattern>).raw
    wxString nakedName = fileName.GetName().BeforeFirst( wxT( '.' ) );
    wxString resolution = fileName.GetName().AfterFirst( wxT( '.' ) ).BeforeFirst( wxT( '.' ) );
    wxString width( resolution.BeforeFirst( wxT( 'x' ) ) );
    wxString height( resolution.AfterFirst( wxT( 'x' ) ) );
    wxString format = fileName.GetName().AfterLast( wxT( '.' ) );
    wxString formatNaked = format.BeforeFirst( wxT( '(' ) );
    wxString bayerPattern = format.AfterFirst( wxT( '=' ) ).BeforeFirst( wxT( ')' ) );

    if( nakedName.IsEmpty() || resolution.IsEmpty() || format.IsEmpty() )
    {
        pParent_->WriteLogMessage( wxString::Format( wxT( "Failed to extract file format from %s(%s %sx%s %s).\n" ), fileName.GetFullPath().c_str(), nakedName.c_str(), width.c_str(), height.c_str(), format.c_str() ) );
    }

    const size_t pixelFormatCount = pixelFormatChoices.Count();
    wxString initialPixelFormat = pixelFormatChoices[0];
    for( size_t i = 1; i < pixelFormatCount; i++ )
    {
        if( formatNaked == pixelFormatChoices[i] )
        {
            initialPixelFormat = pixelFormatChoices[i];
            break;
        }
    }

    const size_t bayerParityCount = bayerParityChoices.Count();
    wxString initialbayerParity = bayerParityChoices[0];
    for( size_t i = 1; i < bayerParityCount; i++ )
    {
        if( bayerPattern == bayerParityChoices[i] )
        {
            initialbayerParity = bayerParityChoices[i];
            break;
        }
    }

    pGridSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( " Pixel Format:" ) ) );
    pCBPixelFormat_ = new wxComboBox( pPanel, wxID_ANY, initialPixelFormat, wxDefaultPosition, wxDefaultSize, pixelFormatChoices, wxCB_DROPDOWN | wxCB_READONLY );
    pGridSizer->Add( pCBPixelFormat_, wxSizerFlags().Expand() );

    pGridSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( " Bayer Parity:" ) ) );
    pCBBayerParity_ = new wxComboBox( pPanel, wxID_ANY, initialbayerParity, wxDefaultPosition, wxDefaultSize, bayerParityChoices, wxCB_DROPDOWN | wxCB_READONLY );
    pGridSizer->Add( pCBBayerParity_, wxSizerFlags().Expand() );

    long w;
    width.ToLong( &w );
    width = wxString::Format( wxT( "%d" ), w ); // get rid of incorrect additional characters (in case someone did name the file 'bla.1024sdfgsdx1024@blub.Mono8.raw' this removes the 'sdfgsd' sequence
    pGridSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( " Width:" ) ) );
    pSCWidth_ = new wxSpinCtrl( pPanel, wxID_ANY, width, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1024 * 256, w );
    pGridSizer->Add( pSCWidth_, wxSizerFlags().Expand() );

    long h;
    height.ToLong( &h );
    height = wxString::Format( wxT( "%d" ), h ); // get rid of incorrect additional characters (in case someone did name the file 'bla.1024sdfgsdx1024@blub.Mono8.raw' this removes the '@blub' sequence
    pGridSizer->Add( new wxStaticText( pPanel, wxID_ANY, wxT( " Height:" ) ) );
    pSCHeight_ = new wxSpinCtrl( pPanel, wxID_ANY, height, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1024 * 256, h );
    pGridSizer->Add( pSCHeight_, wxSizerFlags().Expand() );

    pTopDownSizer->Add( pGridSizer );
    AddButtons( pPanel, pTopDownSizer );
    FinalizeDlgCreation( pPanel, pTopDownSizer );
}

//-----------------------------------------------------------------------------
wxString RawImageImportDlg::GetFormat( void ) const
//-----------------------------------------------------------------------------
{
    wxString format( pCBPixelFormat_->GetValue() );
    if( pCBBayerParity_->GetValue() != wxT( "Undefined" ) )
    {
        format.Append( wxString::Format( wxT( "(BayerParity: %s)" ), pCBBayerParity_->GetValue().c_str() ) );
    }
    return format;
}

//-----------------------------------------------------------------------------
long RawImageImportDlg::GetWidth( void ) const
//-----------------------------------------------------------------------------
{
    return pSCWidth_->GetValue();
}

//-----------------------------------------------------------------------------
long RawImageImportDlg::GetHeight( void ) const
//-----------------------------------------------------------------------------
{
    return pSCHeight_->GetValue();
}

//=============================================================================
//============== Implementation OptionsDlg ====================================
//=============================================================================
//-----------------------------------------------------------------------------
OptionsDlg::OptionsDlg( PropGridFrameBase* pParent, const map<TWarnings, bool>& initialWarningConfiguration,
                        const map<TAppearance, bool>& initialAppearanceConfiguration,
                        const map<TPropertyGrid, bool>& initialPropertyGridConfiguration,
                        const map<TMiscellaneous, bool>& initialMiscellaneousConfiguration )
    : OkAndCancelDlg( pParent, wxID_ANY, wxT( "Options" ) ), pParent_( pParent ), pWarningConfiguration_( 0 ),
      pAppearanceConfiguration_( 0 ), pPropertyGridConfiguration_( 0 ), pMiscellaneousConfiguration_( 0 ),
      initialWarningConfiguration_(), initialAppearanceConfiguration_(), initialPropertyGridConfiguration_(),
      initialMiscellaneousConfiguration_(), boShowQuickSetupOnDeviceOpen_( false )
//-----------------------------------------------------------------------------
{
    /*
        |-------------------------------------|
        | pTopDownSizer                       |
        |                spacer               |
        | |---------------------------------| |
        | | pAppearanceConfiguration        | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pWarningConfiguration           | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pPropertyGridConfiguration      | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pMiscellaneousConfiguration     | |
        | |---------------------------------| |
        |                spacer               |
        | |---------------------------------| |
        | | pButtonSizer                    | |
        | |---------------------------------| |
        |-------------------------------------|
    */

    wxBoxSizer* pTopDownSizer = new wxBoxSizer( wxVERTICAL );
    pTopDownSizer->AddSpacer( 10 );

    wxPanel* pPanel = new wxPanel( this );

    wxArrayString choices;
    choices.resize( aMAX );
    choices[aShowLeftToolBar] = wxT( "Show Left Tool Bar" );
    choices[aShowUpperToolBar] = wxT( "Show Upper Tool Bar" );
    choices[aShowStatusBar] =  wxT( "Show Status Bar" );
    CreateCheckListBox( pPanel, pTopDownSizer, &pAppearanceConfiguration_, choices, wxT( "Appearance: " ), initialAppearanceConfiguration );

    choices.resize( wMAX );
    choices[wWarnOnOutdatedFirmware] = wxT( "Warn On Outdated Firmware" );
    choices[wWarnOnReducedDriverPerformance] = wxT( "Warn On Reduced Driver Performance" );
#if defined(linux) || defined(__linux) || defined(__linux__)
    choices[wWarnOnPotentialNetworkUSBBufferIssues] = wxT( "Warn On Potential Network/USB Buffer Issues" );
#endif // #if defined(linux) || defined(__linux) || defined(__linux__)
    choices[wWarnOnPotentialFirewallIssues] = wxT( "Warn On Potential Firewall Issues" );
    CreateCheckListBox( pPanel, pTopDownSizer, &pWarningConfiguration_, choices, wxT( "Warnings: " ), initialWarningConfiguration );

    choices.resize( pgMAX );
    choices[pgDisplayToolTips] = wxT( "Display Tool Tips" );
    choices[pgUseSelectorGrouping] = wxT( "Use Selector Grouping" );
    choices[pgPreferDisplayNames] = wxT( "Prefer Display Names" );
    choices[pgCreateEditorsWithSlider] = wxT( "Create Editors With Slider" );
    choices[pgDisplayPropertyIndicesAsHex] = wxT( "Display Property Indices As Hex" );
    CreateCheckListBox( pPanel, pTopDownSizer, &pPropertyGridConfiguration_, choices, wxT( "Property Grid: " ), initialPropertyGridConfiguration );

    choices.resize( mMAX );
    choices[mAllowFastSingleFrameAcquisition] = wxT( "Allow Fast Single Frame Acquisition" );
    choices[mDisplayDetailedInformationOnCallbacks] = wxT( "Display Detailed Information On Callbacks" );
    choices[mDisplayMethodExecutionErrors] = wxT( "Display Method Execution Errors" );
    choices[mDisplayFeatureChangeTimeConsumption] = wxT( "Display Feature Change Time Consumption" );
    wxStaticBoxSizer* pMiscellaneousConfigurationSizer = CreateCheckListBox( pPanel, pTopDownSizer, &pMiscellaneousConfiguration_, choices, wxT( "Miscellaneous: " ), initialMiscellaneousConfiguration );

    pCBShowQuickSetupOnDeviceOpen_ = new wxCheckBox( pPanel, wxID_ANY, wxT( "Show Quick Setup On Device Open" ) );
    pMiscellaneousConfigurationSizer->AddSpacer( 5 );
    pMiscellaneousConfigurationSizer->Add( pCBShowQuickSetupOnDeviceOpen_ );

    AddButtons( pPanel, pTopDownSizer );
    FinalizeDlgCreation( pPanel, pTopDownSizer );
}

//-----------------------------------------------------------------------------
void OptionsDlg::BackupCurrentState( void )
//-----------------------------------------------------------------------------
{
    StoreCheckListBoxStateToMap( pWarningConfiguration_, initialWarningConfiguration_ );
    StoreCheckListBoxStateToMap( pAppearanceConfiguration_, initialAppearanceConfiguration_ );
    StoreCheckListBoxStateToMap( pPropertyGridConfiguration_, initialPropertyGridConfiguration_ );
    StoreCheckListBoxStateToMap( pMiscellaneousConfiguration_, initialMiscellaneousConfiguration_ );
    boShowQuickSetupOnDeviceOpen_ = pCBShowQuickSetupOnDeviceOpen_->IsChecked();
}

//-----------------------------------------------------------------------------
template<typename _Ty>
wxStaticBoxSizer* OptionsDlg::CreateCheckListBox( wxPanel* pPanel, wxBoxSizer* pTopDownSizer, wxCheckListBox** ppCheckListBox, const wxArrayString& choices, const wxString& title, const std::map<_Ty, bool>& initialConfiguration )
//-----------------------------------------------------------------------------
{
    *ppCheckListBox = new wxCheckListBox( pPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices );
    RestoreCheckListBoxStateFromMap( *ppCheckListBox, initialConfiguration );
    wxStaticBoxSizer* pConfigurationSizer = new wxStaticBoxSizer( wxVERTICAL, pPanel, title );
    pConfigurationSizer->Add( *ppCheckListBox, wxSizerFlags().Expand() );
    pTopDownSizer->Add( pConfigurationSizer, wxSizerFlags().Expand() );
    pTopDownSizer->AddSpacer( 10 );
    return pConfigurationSizer;
}

//-----------------------------------------------------------------------------
template<typename _Ty>
void OptionsDlg::RestoreCheckListBoxStateFromMap( wxCheckListBox* pCheckListBox, const std::map<_Ty, bool>& stateMap )
//-----------------------------------------------------------------------------
{
    typename std::map<_Ty, bool>::const_iterator it = stateMap.begin();
    const typename std::map<_Ty, bool>::const_iterator itEND = stateMap.end();
    while( it != itEND )
    {
        if( it->first < static_cast<_Ty>( pCheckListBox->GetCount() ) )
        {
            pCheckListBox->Check( it->first, it->second );
        }
        ++it;
    }
}

//-----------------------------------------------------------------------------
void OptionsDlg::RestorePreviousState( void )
//-----------------------------------------------------------------------------
{
    RestoreCheckListBoxStateFromMap( pWarningConfiguration_, initialWarningConfiguration_ );
    RestoreCheckListBoxStateFromMap( pAppearanceConfiguration_, initialAppearanceConfiguration_ );
    RestoreCheckListBoxStateFromMap( pPropertyGridConfiguration_, initialPropertyGridConfiguration_ );
    RestoreCheckListBoxStateFromMap( pMiscellaneousConfiguration_, initialMiscellaneousConfiguration_ );
    pCBShowQuickSetupOnDeviceOpen_->SetValue( boShowQuickSetupOnDeviceOpen_ );
}

//-----------------------------------------------------------------------------
template<typename _Ty>
void OptionsDlg::StoreCheckListBoxStateToMap( wxCheckListBox* pCheckListBox, std::map<_Ty, bool>& stateMap )
//-----------------------------------------------------------------------------
{
    stateMap.clear();
    const unsigned int count = pCheckListBox->GetCount();
    for( unsigned int i = 0; i < count; i++ )
    {
        stateMap.insert( make_pair( static_cast<_Ty>( i ), pCheckListBox->IsChecked( i ) ) );
    }
}
