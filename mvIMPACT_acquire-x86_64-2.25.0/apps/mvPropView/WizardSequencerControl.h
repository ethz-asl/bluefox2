//-----------------------------------------------------------------------------
#ifndef WizardSequencerControlH
#define WizardSequencerControlH WizardSequencerControlH
//-----------------------------------------------------------------------------
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#include <mvIMPACT_CPP/mvIMPACT_acquire_GenICam.h>
#include "PropViewFrame.h"
#include "ValuesFromUserDlg.h"

//-----------------------------------------------------------------------------
class WizardSequencerControl : public OkAndCancelDlg
//-----------------------------------------------------------------------------
{
public:
    explicit WizardSequencerControl( PropViewFrame* pParent, const wxString& title, mvIMPACT::acquire::Device* pDev, size_t displayCount, SequencerSetToDisplayMap& setToDisplayTable );
    virtual ~WizardSequencerControl();

    const SequencerSetToDisplayMap& GetSetToDisplayTable( void );

private:
    //-----------------------------------------------------------------------------
    /// \brief GUI elements for a single sequencer set
    struct SequencerSetControls
    //-----------------------------------------------------------------------------
    {
        wxPanel* pSetPanel;
        wxBoxSizer* pSetVerticalSizer;
        wxBoxSizer* pSetHorizontalSizer;
        wxBoxSizer* pOptionsSizer;
        wxPropertyGrid* pSetPropertyGrid;
        wxSpinCtrl* pSequencerSetNextSC;
        wxComboBox* pDisplayToUseCB;
    };
    typedef std::map<int64_type, SequencerSetControls*> SetNrToControlsMap;
    typedef std::vector<std::pair<HOBJ, wxCheckBox*> > SequencerFeatureContainer;

    //-----------------------------------------------------------------------------
    enum TWidgetIDs_SequencerControl
    //-----------------------------------------------------------------------------
    {
        widMainFrame = wxID_HIGHEST,
        widDisplayComboBox,
        widNextSetSpinControl,
        widSequencerSetsNotebook,
        widSettingsCheckBox,
        widAddAnotherSetButton,
        widRemoveSelectedSetButton,
        widSetSelectedSetAsStartSetButton,
        widAutoAssignSetsToDisplays,
        widStaticBitmapWarning
    };

    //-----------------------------------------------------------------------------
    enum TSequencerWizardError
    //-----------------------------------------------------------------------------
    {
        sweReferenceToInactiveSet,
        sweSetsNotBeingReferenced,
        sweStartingSetNotActive
    };

    //-----------------------------------------------------------------------------
    /// \brief Necessary Information for Sequencer Set Management Errors
    struct SequencerErrorInfo
    //-----------------------------------------------------------------------------
    {
        int64_type setNumber;
        TSequencerWizardError errorType;
        int64_type nextSet;
        explicit SequencerErrorInfo( int64_type nr, TSequencerWizardError error, int64_type nextNr ) : setNumber( nr ), errorType( error ), nextSet( nextNr ) {}
        bool operator==( const SequencerErrorInfo& rhs )
        {
            return ( setNumber == rhs.setNumber ) &&
                   ( errorType == rhs.errorType ) &&
                   ( nextSet == rhs.nextSet );
        }
    };

    PropViewFrame*                      pParent_;
    Device*                             pDev_;
    GenICam::SequencerControl           sc_;
    mutable SequencerSetToDisplayMap    setToDisplayTable_;
    std::vector<SequencerErrorInfo>     currentErrors_;
    SequencerFeatureContainer           sequencerFeatures_;
    std::map<int64_type, int64_type>    sequencerNextSetMap_;
    SetNrToControlsMap                  sequencerSetControlsMap_;
    int64_type                          startingSequencerSet_;
    int64_type                          maxSequencerSetNumber_;
    size_t                              displayCount_;
    bool                                boCounterDurationCapability_;

    wxNotebook*                         pSetSettingsNotebook_;
    wxStaticBitmap*                     pStaticBitmapWarning_;
    wxStaticText*                       pInfoText_;
    wxStaticText*                       pStartingSetText_;

    static const wxString               s_displayStringPrefix_;
    static const wxString               s_sequencerSetStringPrefix_;

    void                                AnalyzeSequencerSetFeatures( wxPanel* pMainPanel );
    void                                AddError( const SequencerErrorInfo& errorInfo );
    void                                CheckIfActiveSetsAreReferencedAtLeastOnce( void );
    void                                CheckIfReferencedSetsAreActive( void );
    void                                CheckIfStartingSetIsAnActiveSequencerSet( int64_type setNumber );
    void                                ComposeSetPanel( size_t displayCount, int64_type setCount, int64_type setNumber, bool boInsertOnFirstFreePosition );
    SetNrToControlsMap::const_iterator  GetSelectedSequencerSetControls( void ) const;
    int64_type                          GetSelectedSequencerSetNr( void ) const;
    SetNrToControlsMap::const_iterator  GetSequencerSetControlsFromPageIndex( size_t pageNr ) const;
    int64_type                          GetSequencerSetNrFromPageIndex( size_t pageNr ) const;
    void                                HandleAutomaticNextSetSettingsOnInsert( unsigned int tabPageNumber, unsigned int setNumber );
    void                                HandleAutomaticNextSetSettingsOnRemove( unsigned int tabPageNumber, unsigned int setNumber );
    void                                InquireCurrentlyActiveSequencerSets( void );
    void                                LoadSequencerSet( int64_type setNumber );
    void                                LoadSequencerSets( void );
    void                                RemoveError( const SequencerErrorInfo& errorInfo );
    void                                RemoveSetPanel( void );
    void                                ReadPropertiesOfSequencerSet( SetNrToControlsMap::iterator& it );
    void                                RefreshCurrentPropertyGrid( void );
    bool                                SaveSequencerSets( void );
    void                                SaveSequencerSetsAndEnd( int returnCode );
    void                                SelectSequencerSetAndCallMethod( int64_type setNr, Method& meth );
    void                                UpdateError( const wxString& msg, const wxBitmap& icon ) const;
    void                                UpdateErrors( void ) const;
    void                                UpdatePropertyGrid( SequencerSetControls* pSelectedSequencerSet );

    //----------------------------------GUI Functions----------------------------------
    void                                OnBtnApply( wxCommandEvent& )
    {
        SaveSequencerSetsAndEnd( wxID_APPLY );
    }
    void                                OnBtnCancel( wxCommandEvent& )
    {
        EndModal( wxID_CANCEL );
    }
    void                                OnBtnOk( wxCommandEvent& )
    {
        SaveSequencerSetsAndEnd( wxID_OK );
    }
    void                                OnBtnAddAnotherSet( wxCommandEvent& e );
    void                                OnBtnRemoveSelectedSet( wxCommandEvent& e );
    void                                OnBtnSetSelectedSetAsStartSet( wxCommandEvent& e );
    void                                OnBtnAutoAssignSetsToDisplays( wxCommandEvent& e );
    void                                OnNBSequencerSetsPageChanged( wxNotebookEvent& e );
    void                                OnNextSetSpinControlChanged( wxSpinEvent& e );
    void                                OnPropertyChanged( wxPropertyGridEvent& e );
    void                                OnSettingsCheckBoxChecked( wxCommandEvent& e );

    DECLARE_EVENT_TABLE()
};

#endif // WizardSequencerControlH
