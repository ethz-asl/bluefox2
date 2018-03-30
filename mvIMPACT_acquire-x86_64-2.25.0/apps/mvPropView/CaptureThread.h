//-----------------------------------------------------------------------------
#ifndef CaptureThreadH
#define CaptureThreadH CaptureThreadH
//-----------------------------------------------------------------------------
#include "DevData.h"
#include <deque>
#include <list>
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#include <wx/thread.h>
#include <wx/string.h>

class wxEvtHandler;
class wxWindow;
struct RequestInfoData;

typedef std::pair<int, RequestData*> RequestDataEntry;
typedef std::deque<RequestDataEntry> RequestContainer;

#define MULTI_SETTING_HACK 1

void QueueEvent( wxEvtHandler* pEventHandler, wxEvent* pEvent );

//-----------------------------------------------------------------------------
enum TEndOfSequenceReason
//-----------------------------------------------------------------------------
{
    eosrRecordingDone,
    eosrRecordingDone_OutOfRAM,
    eosrMultiFrameDone
};

//-----------------------------------------------------------------------------
class CaptureThread : public wxThread
//-----------------------------------------------------------------------------
{
    static unsigned int                         m_activeInstance;
    volatile bool                               m_boLive;
    volatile bool                               m_boRecord;
    volatile bool                               m_boContinuousRecording;
    volatile bool                               m_boForwardIncompleteFrames;
    mutable wxCriticalSection                   m_critSect;
    mutable wxCriticalSection                   m_critSectConfig;
    mutable int                                 m_currentRequest;
    static unsigned int                         m_instanceCnt;
    unsigned int                                m_instanceNr;
    wxString                                    m_lastErrorMessage;
    std::string                                 m_currentPlotInfoPath;
    mutable int                                 m_nextPendingRequest;           // for legacy mode used for a single display. DO NOT REMOVE UNTIL YOU ARE ABSOLUTELY SURE THAT THIS DOESN'T BREAK ANYTHING
    std::list<int>::size_type                   m_pendingRequestsForDisplayMax; // for new mode used with 2 or more displays
    mutable std::list<int>                      m_pendingRequestsForDisplay;    // for new mode used with 2 or more displays
    int                                         m_parentWindowID;
    mvIMPACT::acquire::Device*                  m_pDev;
    mvIMPACT::acquire::FunctionInterface*       m_pFuncInterface;
    mvIMPACT::acquire::ImageRequestControl      m_imageRequestControl;
    mvIMPACT::acquire::SystemSettings           m_systemSettings;
    mvIMPACT::acquire::TImageProcessingMode     m_imageProcessingMode;
#if MULTI_SETTING_HACK
    int                                         m_settingIndex;
    std::vector<std::pair<std::string, int> >   m_settings;
    std::vector<unsigned int>                   m_pendingRequests;
    std::vector<int64_type>                     m_lastPendingRequestFrameNr;
    std::vector<unsigned int>                   m_requestFailed;
    std::vector<unsigned int>::size_type        m_settingCount;
    unsigned int                                m_currentSetting;
    TCaptureSettingUsageMode                    m_captureSettingUsageMode;
#endif // #if MULTI_SETTING_HACK
    wxEvtHandler*                               m_pParentWindowEventHandler;
    RequestContainer                            m_recordedSequence;
    size_t                                      m_recordedSequenceMemoryConsumption;
    size_t                                      m_currentSequenceIndexToDisplay;
    size_t                                      m_currentCountToRecord;
    size_t                                      m_totalCountToRecord;
    size_t                                      m_maxNumberToRequest;
    size_t                                      m_numberRequested;
    size_t                                      m_numberReturned;
    size_t                                      m_skippedImageCount;
    mutable size_t                              m_capturedImagesCount;
    mutable size_t                              m_capturedImagesSentToDisplayCount;
    mutable double                              m_percentageOfImagesSentToDisplay;
    int                                         m_captureQueueDepth;
    int                                         m_currentlyActiveRequests;
    /// The desired part index to return for multi-part buffers
    unsigned int                                m_bufferPartIndex;
    /// A large internal image buffer that can store multiple images that shall be displayed as a single one.
    mutable mvIMPACT::acquire::ImageBufferDesc  m_multipleImageBuffer;
    /// The number of images that fit into \c m_multipleImageBuffer assuming the size does not vary on a frame to frame basis.
    int                                         m_multipleImageBufferImageCount;
    /// The current line offset in \c m_multipleImageBuffer. Here the next images captured will be copied into the large buffer.
    mutable int                                 m_multipleImageBufferLineOffset;

    void                                        CheckedImageRequestUnlock( const int requestNr ) const;
    bool                                        CheckSequenceRestart( bool boInformUserOnRecordStart ) const;
    void                                        CollectBufferInfo( ComponentIterator it, ComponentIterator itImageProcessingResults, std::vector<wxString>& infoStrings, const wxString& path = wxT( "" ) ) const;
    void                                        CollectBufferInfo_ImageProcessingResults( ComponentIterator it, std::vector<wxString>& infoStrings ) const;
    void                                        DecreaseRecordedSequenceMemoryConsumption( const size_t size )
    {
        m_recordedSequenceMemoryConsumption = ( m_recordedSequenceMemoryConsumption >= size ) ? m_recordedSequenceMemoryConsumption - size : 0;
    }
    int                                         FreeAndUnlockLastRequestFromSequence( int requestNotToUnlock );
    void                                        FreeLastRequestFromSequence( void );
    void                                        FreeLastRequestFromSequence( RequestDataEntry& entry );
    void                                        GetRequestData( RequestData& requestData, const int requestNr, const bool boDeepCopy ) const;
    template<typename _Ty, typename _Tx>
    void                                        GetRequestData_BayerInfo( RequestData& requestData, _Ty& imageBayerMosaicParity, _Tx& imagePixelFormat ) const;
    void                                        GetRequestInfoData( RequestInfoData* pReqInfo, const Request* pReq, HOBJ settingUsed ) const;
    int                                         GetSettingUsedForRequest( const Request* pRequest ) const
    {
        return ( pRequest->infoSettingUsed.isValid() ) ? pRequest->infoSettingUsed.read() : INVALID_ID;
    }

    void                                        InternalGetImageInfo( std::vector<wxString>& infoStrings, const Request* pRequest ) const;
    bool                                        InternalSetLiveMode( bool boOn, bool boInformUserOnRecordStart = true );
    bool                                        InternalSetRecordMode( bool boOn, bool boUnlockCurrentRequest );
    bool                                        MustFreeMemory( void ) const;
    void                                        QueueEvent( wxEvent* pEvent ) const
    {
        ::QueueEvent( m_pParentWindowEventHandler, pEvent );
    }
    int                                         RequestImages( int* pFramesRequested = 0 );
    void                                        SendImageSkippedEvent( const int settingUsed );
    void                                        SendImageReadyEvent( void );
    void                                        UnlockPendingRequests( void );
    void                                        UpdateMultiImageBuffer( const ImageBufferDesc& desc ) const;
protected:
    void*                                       Entry( void );
public:
    explicit                                    CaptureThread( mvIMPACT::acquire::FunctionInterface* pFuncInterface, mvIMPACT::acquire::Device* pDev, wxWindow* pParentWindow, unsigned int pendingImageQueueDepth );
    ~CaptureThread();
    size_t                                      DisplaySequenceRequest( size_t nr = 0 );
    size_t                                      DisplaySequenceRequestNext( void )
    {
        return DisplaySequenceRequest( m_currentSequenceIndexToDisplay + 1 );
    }
    size_t                                      DisplaySequenceRequestPrev( void )
    {
        return DisplaySequenceRequest( ( m_currentSequenceIndexToDisplay != 0 ) ? ( m_currentSequenceIndexToDisplay - 1 ) : 0 );
    }
    void                                        FreeSequence( int requestNotToUnlock = INVALID_ID );
    int                                         GetCaptureQueueDepth( void ) const
    {
        return m_captureQueueDepth;
    }
    bool                                        GetContinuousRecording( void ) const
    {
        return m_boContinuousRecording;
    }
    size_t                                      GetCurrentSequenceIndex( void ) const
    {
        return m_currentSequenceIndexToDisplay;
    }
    unsigned int                                GetBufferPartIndex( void ) const
    {
        return m_bufferPartIndex;
    }
    void                                        GetImageData( RequestData& requestData, const TImageDataRetrievalMode mode ) const;
    void                                        GetImageInfo( std::vector<wxString>& infoStrings ) const;
    long                                        GetImagesPerDisplayCount( void ) const
    {
        return m_multipleImageBufferImageCount;
    }
    bool                                        GetLiveMode( void ) const
    {
        return m_boLive;
    }
    size_t                                      GetMultiFrameSequenceSize( void ) const
    {
        return m_maxNumberToRequest;
    }
    bool                                        GetRecordMode( void ) const
    {
        return m_boRecord;
    }
    size_t                                      GetRecordSequenceSize( void ) const
    {
        return m_totalCountToRecord;
    }
    RequestContainer::size_type                 GetRecordedSequenceSize( void ) const
    {
        return m_recordedSequence.size();
    }
    size_t                                      GetSkippedImageCount( void );
    double                                      GetPercentageOfImagesSentToDisplay( void ) const;
    int                                         RecordSequence( void );
    int                                         RequestSingle( bool boInformUserOnRecordStart );
    void                                        SetActive( void );
    void                                        SetBufferPartIndex( unsigned int index )
    {
        m_bufferPartIndex = index;
    }
    void                                        SetCaptureMode( bool boForwardIncompleteFrames );
    void                                        SetCaptureQueueDepth( int queueDepth );
#if MULTI_SETTING_HACK
    void                                        SetCaptureSettingUsageMode( TCaptureSettingUsageMode mode );
#endif // #if MULTI_SETTING_HACK
    void                                        SetContinuousRecording( bool boContinuous );
    void                                        SetCurrentPlotInfoPath( const std::string& currentPlotInfoPath );
    void                                        SetImageProcessingMode( TImageProcessingMode mode );
    void                                        SetImagesPerDisplayCount( long imagesPerDisplayCount );
    bool                                        SetLiveMode( bool boOn, bool boInformUserOnRecordStart = true );
    void                                        SetMultiFrameSequenceSize( size_t sequenceSize );
    void                                        SetPendingImageQueueDepth( unsigned int value );
    bool                                        SetRecordMode( bool boOn );
    void                                        SetRecordSequenceSize( size_t sequenceSize );
    void                                        UnlockRequest( int nr, bool boForceUnlock = false );
#if MULTI_SETTING_HACK
    void                                        UpdateSettingTable( void );
    int                                         RequestReset( void );
    enum
    {
        MB = 1024 * 1024,
        MAX_MEMORY_FOR_RECORDING_ON_32_BIT = 768 * MB
    };
#endif // #if MULTI_SETTING_HACK
};

#endif // CaptureThreadH
