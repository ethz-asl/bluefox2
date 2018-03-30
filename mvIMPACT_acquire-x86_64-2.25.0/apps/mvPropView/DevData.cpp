#include "DevData.h"

//=============================================================================
//================= Implementation RequestData ================================
//=============================================================================

const wxString RequestData::UNKNOWN_PIXEL_FORMAT_STRING_( wxT( "unknown" ) );

//-----------------------------------------------------------------------------
RequestData::RequestData() : image_( 1 ), pixelFormat_( UNKNOWN_PIXEL_FORMAT_STRING_ ),
    requestInfo_(), requestInfoStrings_(), requestNr_( INVALID_ID ), bufferPartIndex_( 0 )
//-----------------------------------------------------------------------------
{
    image_.getBuffer()->pixelFormat = ibpfMono8;
}

//=============================================================================
//================= Implementation helper functions ===========================
//=============================================================================
//-----------------------------------------------------------------------------
void SetRequestDataFromWxImage( RequestData& requestData, const wxImage& image )
//-----------------------------------------------------------------------------
{
    requestData.image_ = mvIMPACT::acquire::ImageBufferDesc( ibpfRGB888Packed, image.GetWidth(), image.GetHeight() );
    requestData.pixelFormat_ = wxString( wxT( "RGB888Packed" ) );
    requestData.bayerParity_ = bmpUndefined;
    unsigned char* pDst = static_cast<unsigned char*>( requestData.image_.getBuffer()->vpData );
    const int W = image.GetWidth();
    const int H = image.GetHeight();
    for( int y = 0; y < H; y++ )
    {
        for( int x = 0; x < W; x++ )
        {
            *pDst++ = image.GetBlue( x, y );
            *pDst++ = image.GetGreen( x, y );
            *pDst++ = image.GetRed( x, y );
        }
    }
}