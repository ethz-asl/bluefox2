#ifdef _MSC_VER         // is Microsoft compiler?
#   if _MSC_VER < 1300  // is 'old' VC 6 compiler?
#       pragma warning( disable : 4786 ) // 'identifier was truncated to '255' characters in the debug information'
#   endif // #if _MSC_VER < 1300
#endif // #ifdef _MSC_VER
#include <algorithm>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <apps/Common/exampleHelper.h>
#include <mvIMPACT_CPP/mvIMPACT_acquire_GenICam.h>
#if defined(linux) || defined(__linux) || defined(__linux__)
#   include <stdio.h>
#   include <unistd.h>
#else
#   include <windows.h>
#   include <process.h>
#   include <mvDisplay/Include/mvIMPACT_acquire_display.h>
using namespace mvIMPACT::acquire::display;
#endif // #if defined(linux) || defined(__linux) || defined(__linux__)

using namespace std;
using namespace mvIMPACT::acquire;

static bool s_boTerminated = false;

//=============================================================================
//================= Data type definitions =====================================
//=============================================================================
//-----------------------------------------------------------------------------
struct ThreadParameter
//-----------------------------------------------------------------------------
{
    Device* pDev;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    ImageDisplayWindow displayWindow;
#endif // #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    explicit ThreadParameter( Device* p ) : pDev( p )
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        // initialise display window
        // IMPORTANT: It's NOT save to create multiple display windows in multiple threads!!!
        , displayWindow( "mvIMPACT_acquire sample, Device " + pDev->serial.read() )
#endif // #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    {}
};

//=============================================================================
//================= function declarations =====================================
//=============================================================================
static void                     configureDevice( Device* pDev );
static void                     configureBlueCOUGAR_X( Device* pDev );
static void                     configureVirtualDevice( Device* pDev );
static bool                     isDeviceSupportedBySample( const Device* const pDev );
static unsigned int DMR_CALL    liveThread( void* pData );
static void                     reportProblemAndExit( Device* pDev, const string& prologue, const string& epilogue = "" );

//=============================================================================
//================= implementation ============================================
//=============================================================================
//-----------------------------------------------------------------------------
inline bool isBlueCOUGAR_X( const Device* const pDev )
//-----------------------------------------------------------------------------
{
    return ( pDev->product.read().find( "mvBlueCOUGAR-X" ) == 0 ) ||
           ( pDev->product.read().find( "mvBlueCOUGAR-2X" ) == 0 ) ||
           ( pDev->product.read().find( "mvBlueCOUGAR-3X" ) == 0 );
}

//-----------------------------------------------------------------------------
// Check if all features needed by this application are actually available
// and set up these features in a way that the multi-part streaming is active.
// If a crucial feature is missing/not available this function will terminate
// with an error message.
void configureDevice( Device* pDev )
//-----------------------------------------------------------------------------
{
    if( pDev->product.read() == "VirtualDevice" )
    {
        configureVirtualDevice( pDev );
    }
    else if( isBlueCOUGAR_X( pDev ) )
    {
        configureBlueCOUGAR_X( pDev );
    }
    else
    {
        reportProblemAndExit( pDev, "The selected device is not supported by this application!" );
    }
}

//-----------------------------------------------------------------------------
void configureVirtualDevice( Device* pDev )
//-----------------------------------------------------------------------------
{
    CameraSettingsVirtualDevice cs( pDev );
    if( !cs.bufferPartCount.isValid() )
    {
        reportProblemAndExit( pDev, "This version of the mvVirtualDevice driver does not support the multi-part format. An update will fix this problem!" );
    }

    cs.bufferPartCount.write( cs.bufferPartCount.getMaxValue() );

    // because every jpg image is written to disk we limit the frames per sec to 5 fps
    cs.frameDelay_us.write( 200000 );
}

//-----------------------------------------------------------------------------
void configureBlueCOUGAR_X( Device* pDev )
//-----------------------------------------------------------------------------
{
    // make sure the device is running with default parameters
    GenICam::UserSetControl usc( pDev );
    usc.userSetSelector.writeS( "Default" );
    usc.userSetLoad.call();

    // now set up the device
    GenICam::TransportLayerControl tlc( pDev );
    if( !tlc.gevGVSPExtendedIDMode.isValid() )
    {
        reportProblemAndExit( pDev, "This device does not support the multi-part format. A firmware update will fix this problem!" );
    }

    //enable extended ID mode - this is mandatory in order to stream JPEG and/or multi part data
    tlc.gevGVSPExtendedIDMode.writeS( "On" );

    //enable JPEGWithRaw mode
    GenICam::AcquisitionControl ac( pDev );
    ac.mvFeatureMode.writeS( "mvJPEGWithRaw" );
}

//-----------------------------------------------------------------------------
// This function will allow to select devices that support the GenICam interface
// layout(these are devices, that claim to be compliant with the GenICam standard)
// and that are bound to drivers that support the user controlled start and stop
// of the internal acquisition engine. Other devices will not be listed for
// selection as the code of the example relies on these features in the code.
bool isDeviceSupportedBySample( const Device* const pDev )
//-----------------------------------------------------------------------------
{
    if( ( pDev->product.read() == "VirtualDevice" ) ||
        isBlueCOUGAR_X( pDev ) )
    {
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
unsigned int DMR_CALL liveThread( void* pData )
//-----------------------------------------------------------------------------
{
    ThreadParameter* pThreadParameter = reinterpret_cast<ThreadParameter*>( pData );
    unsigned int cnt = 0;

    // establish access to the statistic properties
    Statistics statistics( pThreadParameter->pDev );
    // create an interface to the device found
    mvIMPACT::acquire::FunctionInterface fi( pThreadParameter->pDev );

    // Send all requests to the capture queue. There can be more than 1 queue for some devices, but for this sample
    // we will work with the default capture queue. If a device supports more than one capture or result
    // queue, this will be stated in the manual. If nothing is mentioned about it, the device supports one
    // queue only. This loop will send all requests currently available to the driver. To modify the number of requests
    // use the property mvIMPACT::acquire::SystemSettings::requestCount at runtime or the property
    // mvIMPACT::acquire::Device::defaultRequestCount BEFORE opening the device.
    TDMR_ERROR result = DMR_NO_ERROR;
    while( ( result = static_cast<TDMR_ERROR>( fi.imageRequestSingle() ) ) == DMR_NO_ERROR ) {};
    if( result != DEV_NO_FREE_REQUEST_AVAILABLE )
    {
        cout << "'FunctionInterface.imageRequestSingle' returned with an unexpected result: " << result
             << "(" << mvIMPACT::acquire::ImpactAcquireException::getErrorCodeAsString( result ) << ")" << endl;
    }

    manuallyStartAcquisitionIfNeeded( pThreadParameter->pDev, fi );
    // run thread loop
    mvIMPACT::acquire::Request* pRequest = 0;
    // we always have to keep at least 2 images as the displayWindow module might want to repaint the image, thus we
    // can free it unless we have a assigned the displayWindow to a new buffer.
    mvIMPACT::acquire::Request* pPreviousRequest = 0;
    const unsigned int timeout_ms = 500;
    while( !s_boTerminated )
    {
        // wait for results from the default capture queue
        int requestNr = fi.imageRequestWaitFor( timeout_ms );
        pRequest = fi.isRequestNrValid( requestNr ) ? fi.getRequest( requestNr ) : 0;
        if( pRequest )
        {
            if( pRequest->isOK() )
            {
                const unsigned int bufferPartCount = pRequest->getBufferPartCount();
                ++cnt;
                // here we can display some statistical information every 100th image
                if( cnt % 100 == 0 )
                {
                    cout << "Info from " << pThreadParameter->pDev->serial.read();

                    if( bufferPartCount == 0 )
                    {
                        cout << " NOT running in multi-part mode";
                    }
                    else
                    {
                        cout << " running in multi-part mode, delivering " << bufferPartCount << " parts";
                    }
                    cout << ": " << statistics.framesPerSecond.name() << ": " << statistics.framesPerSecond.readS()
                         << ", " << statistics.errorCount.name() << ": " << statistics.errorCount.readS()
                         << ", " << statistics.captureTime_s.name() << ": " << statistics.captureTime_s.readS() << endl;
                }
                if( bufferPartCount == 0 )
                {
                    // the device is not running in multi-part mode
#if defined(linux) || defined(__linux) || defined(__linux__)
                    cout << "Image captured: " << pRequest->imageOffsetX.read() << "x" << pRequest->imageOffsetY.read() << "@" << pRequest->imageWidth.read() << "x" << pRequest->imageHeight.read() << endl;
#else
                    ImageDisplay& display = pThreadParameter->displayWindow.GetImageDisplay();
                    display.SetImage( pRequest );
                    display.Update();
#endif  // #if defined(linux) || defined(__linux) || defined(__linux__)
                }
                else
                {
#if !defined(linux) && !defined(__linux) && !defined(__linux__)
                    // we have just one display so only show the first buffer part that can be displayed there, output text for all others
                    bool boImageDisplayed = false;
                    ImageDisplay& display = pThreadParameter->displayWindow.GetImageDisplay();
#endif // #if !defined(linux) && !defined(__linux) && !defined(__linux__)
                    for( unsigned int i = 0; i < bufferPartCount; i++ )
                    {
                        const BufferPart& bufferPart = pRequest->getBufferPart( i );
                        switch( bufferPart.dataType.read() )
                        {
                        case bpdt2DImage:
                        case bpdt2DPlaneBiplanar:
                        case bpdt2DPlaneTriplanar:
                        case bpdt2DPlaneQuadplanar:
                        case bpdt3DImage:
                        case bpdt3DPlaneBiplanar:
                        case bpdt3DPlaneTriplanar:
                        case bpdt3DPlaneQuadplanar:
                        case bpdtConfidenceMap:
                            {
#if !defined(linux) && !defined(__linux) && !defined(__linux__)
                                if( !boImageDisplayed )
                                {
                                    display.SetImage( bufferPart.getImageBufferDesc().getBuffer() );
                                    display.Update();
                                    boImageDisplayed = true;
                                }
                                else
#endif // #if !defined(linux) && !defined(__linux) && !defined(__linux__)
                                {
                                    cout << "Buffer Part handled: " << bufferPart.dataType.readS() << "(index " << i << "), dimensions: " << bufferPart.offsetX.read() << "x" << bufferPart.offsetY.read() << "@" << bufferPart.width.read() << "x" << bufferPart.height.read() << endl;
                                }
                            }
                            break;
                        case bpdtJPEG:
                        case bpdtJPEG2000:
                            {
                                // store JPEG data into a file. Please note that at higher frame/data rates this might result in lost images during the capture
                                // process as storing the images to disc might be very slow. Real world applications should therefore move these operations
                                // into a thread. Probably it makes sense to copy the JPEG image into another RAM location as well in order to free this capture
                                // buffer as fast as possible.
                                ostringstream oss;
                                oss << "Buffer.part" << i
                                    << "id" << std::setfill( '0' ) << std::setw( 16 ) << pRequest->infoFrameID.read() << "."
                                    << "ts" << std::setfill( '0' ) << std::setw( 16 ) << pRequest->infoTimeStamp_us.read() << "."
                                    << "jpeg";
                                FILE* pFile = fopen( oss.str().c_str(), "wb" );
                                if( pFile )
                                {
                                    if( fwrite( bufferPart.address.read(), static_cast<size_t>( bufferPart.dataSize.read() ), 1, pFile ) != 1 )
                                    {
                                        reportProblemAndExit( pThreadParameter->pDev, "Could not write file '" +  oss.str() + "'" );
                                    }
                                    fclose( pFile );
                                    cout << "Buffer Part handled: " << bufferPart.dataType.readS() << "(index " << i << "): Stored to disc as '" << oss.str() << "'." << endl;
                                }
                            }
                            break;
                        case bpdtUnknown:
                            cout << "Buffer Part" << "(index " << i << ") has an unknown data type. Skipped!" << endl;
                            break;
                        }
                    }
                }
            }
            else
            {
                cout << "Error: " << pRequest->requestResult.readS() << endl;
            }
            if( pPreviousRequest )
            {
                // this image has been displayed thus the buffer is no longer needed...
                pPreviousRequest->unlock();
            }
            pPreviousRequest = pRequest;
            // send a new image request into the capture queue
            fi.imageRequestSingle();
        }
        else
        {
            // If the error code is -2119(DEV_WAIT_FOR_REQUEST_FAILED), the documentation will provide
            // additional information under TDMR_ERROR in the interface reference
            cout << "imageRequestWaitFor failed (" << requestNr << ", " << ImpactAcquireException::getErrorCodeAsString( requestNr ) << ")"
                 << ", timeout value too small?" << endl;
        }
#if defined(linux) || defined(__linux) || defined(__linux__)
        s_boTerminated = waitForInput( 0, STDOUT_FILENO ) == 0 ? false : true; // break by STDIN
#endif // #if defined(linux) || defined(__linux) || defined(__linux__)
    }
    manuallyStopAcquisitionIfNeeded( pThreadParameter->pDev, fi );

#if !defined(linux) && !defined(__linux) && !defined(__linux__)
    // stop the displayWindow from showing freed memory
    pThreadParameter->displayWindow.GetImageDisplay().SetImage( reinterpret_cast<Request*>( 0 ) );
#endif // #if !defined(linux) && !defined(__linux) && !defined(__linux__)

    // In this sample all the next lines are redundant as the device driver will be
    // closed now, but in a real world application a thread like this might be started
    // several times an then it becomes crucial to clean up correctly.

    // free the last potentially locked request
    if( pRequest )
    {
        pRequest->unlock();
    }
    // clear all queues
    fi.imageRequestReset( 0, 0 );
    return 0;
}

//-----------------------------------------------------------------------------
// When a crucial feature needed for this example application is not available
// this function will get called. It reports an error and then terminates the
// application.
void reportProblemAndExit( Device* pDev, const string& prologue, const string& epilogue /* = "" */ )
//-----------------------------------------------------------------------------
{
    cout << prologue << " by device " << pDev->serial.read() << "(" << pDev->product.read() << ", Firmware version: " << pDev->firmwareVersion.readS() << ")." << epilogue << endl
         << "Press [ENTER] to end the application..." << endl;
    cin.get();
    exit( 42 );
}

//-----------------------------------------------------------------------------
int main( int /*argc*/, char* /*argv*/[] )
//-----------------------------------------------------------------------------
{
    DeviceManager devMgr;
    Device* pDev = getDeviceFromUserInput( devMgr, isDeviceSupportedBySample );
    if( !pDev )
    {
        cout << "Unable to continue!";
        cout << "Press [ENTER] to end the application" << endl;
        cin.get();
        return 1;
    }

    try
    {
        cout << "Initialising the device. This might take some time..." << endl << endl;
        pDev->open();
    }
    catch( const ImpactAcquireException& e )
    {
        // this e.g. might happen if the same device is already opened in another process...
        cout << "An error occurred while opening device " << pDev->serial.read()
             << "(error code: " << e.getErrorCodeAsString() << ")." << endl
             << "Press [ENTER] to end the application..." << endl;
        cin.get();
        return 1;
    }

    try
    {
        configureDevice( pDev );
        // start the execution of the 'live' thread.
        cout << "Press [ENTER] to end the application" << endl;

        ThreadParameter threadParam( pDev );
#if defined(linux) || defined(__linux) || defined(__linux__)
        liveThread( &threadParam );
#else
        unsigned int dwThreadID;
        HANDLE hThread = ( HANDLE )_beginthreadex( 0, 0, liveThread, ( LPVOID )( &threadParam ), 0, &dwThreadID );
        cin.get();
        s_boTerminated = true;
        WaitForSingleObject( hThread, INFINITE );
        CloseHandle( hThread );
#endif  // #if defined(linux) || defined(__linux) || defined(__linux__)
    }
    catch( const ImpactAcquireException& e )
    {
        // this e.g. might happen if the same device is already opened in another process...
        cout << "An error occurred while setting up device " << pDev->serial.read()
             << "(error code: " << e.getErrorCodeAsString() << ")." << endl
             << "Press [ENTER] to end the application..." << endl;
        cin.get();
        return 1;
    }
    return 0;
}
