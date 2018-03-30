//-----------------------------------------------------------------------------
#ifndef DrawingCanvasH
#define DrawingCanvasH DrawingCanvasH
//-----------------------------------------------------------------------------
#include <wx/pen.h>
#include <wx/scrolwin.h>

//-----------------------------------------------------------------------------
template<typename _Ty>
inline const _Ty& saveAssign( const _Ty& val, const _Ty& min, const _Ty& max )
//-----------------------------------------------------------------------------
{
    return ( ( val > max ) ? max : ( val < min ) ? min : val );
}

//-----------------------------------------------------------------------------
class DrawingCanvas : public wxScrolledWindow
//-----------------------------------------------------------------------------
{
    DECLARE_EVENT_TABLE()
protected:
    bool                        m_boActive;
    mutable wxCriticalSection   m_critSect;
    virtual void    HandleSizeEvent( wxSizeEvent& e );
public:
    explicit        DrawingCanvas() : wxScrolledWindow() {}
    explicit        DrawingCanvas( wxWindow* parent, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition,
                                   const wxSize& size = wxDefaultSize, long style = wxBORDER_NONE,
                                   const wxString& name = wxT( "DrawingCanvas" ), bool boActive = false );
    bool            IsActive( void ) const
    {
        return m_boActive;
    }
    void            OnSize( wxSizeEvent& );
    virtual void    SetActive( bool boActive );
    //-----------------------------------------------------------------------------
    static unsigned short GetMono12Packed_V1Pixel( const unsigned char* const pBuffer, int pixel )
    //-----------------------------------------------------------------------------
    {
        const int offset = pixel + pixel / 2;
        if( pixel % 2 )
        {
            return static_cast<unsigned short>( pBuffer[offset] >> 4 ) | static_cast<unsigned short>( pBuffer[offset + 1] << 4 );
        }
        return static_cast<unsigned short>( pBuffer[offset] ) | static_cast<unsigned short>( ( pBuffer[offset + 1] & 0xF ) << 8 );
    }
    //-----------------------------------------------------------------------------
    static unsigned short GetMonoPacked_V2Pixel( const unsigned char* const pBuffer, int pixel, int shift )
    //-----------------------------------------------------------------------------
    {
        const int offset = ( 3 * pixel ) / 2;
        if( pixel % 2 )
        {
            return saveAssign( static_cast<unsigned short>( static_cast<unsigned short>( pBuffer[offset + 1] << shift ) | static_cast<unsigned short>( pBuffer[offset] >> 4 ) ), static_cast<unsigned short>( 0 ), static_cast<unsigned short>( ( 1 << ( 8 + shift ) ) - 1 ) );
        }
        return saveAssign( static_cast<unsigned short>( static_cast<unsigned short>( pBuffer[offset] << shift ) | static_cast<unsigned short>( pBuffer[offset + 1] & 0xF ) ), static_cast<unsigned short>( 0 ), static_cast<unsigned short>( ( 1 << ( 8 + shift ) ) - 1 ) );
    }
    //-----------------------------------------------------------------------------
    static unsigned short GetMono12Packed_V2Pixel( const unsigned char* const pBuffer, int pixel )
    //-----------------------------------------------------------------------------
    {
        return GetMonoPacked_V2Pixel( pBuffer, pixel, 4 );
    }
    //-----------------------------------------------------------------------------
    static void GetBGR101010Packed_V2Pixel( unsigned int pixel, unsigned short& red, unsigned short& green, unsigned short& blue )
    //-----------------------------------------------------------------------------
    {
        red   = static_cast<unsigned short>(  pixel          & 0x3FF );
        green = static_cast<unsigned short>( ( pixel >> 10 ) & 0x3FF );
        blue  = static_cast<unsigned short>( ( pixel >> 20 ) & 0x3FF );
    }
};

//-----------------------------------------------------------------------------
struct AOI
//-----------------------------------------------------------------------------
{
    int         m_incX;
    int         m_incY;
    int         m_incW;
    int         m_incH;
    int         m_minW;
    int         m_minH;
    wxRect      m_rect;
    wxPen       m_pen;
    bool        m_boDrawOnTop;
    bool        m_boCanDrag;
    wxString    m_description;
    explicit AOI() : m_incX( 1 ), m_incY( 1 ), m_incW( 1 ), m_incH( 1 ), m_minW( 1 ), m_minH( 1 ),
        m_rect( 0, 0, 0, 0 ), m_pen( *wxBLACK_PEN ), m_boDrawOnTop( false ), m_boCanDrag( true ), m_description( wxEmptyString ) {}
    explicit AOI( const wxRect& r, const wxPen& p, const wxString& desc ) : m_incX( 1 ), m_incY( 1 ), m_incW( 1 ), m_incH( 1 ), m_minW( 1 ), m_minH( 1 ),
        m_rect( r ), m_pen( p ), m_boDrawOnTop( false ), m_boCanDrag( true ), m_description( desc ) {}
    explicit AOI( const wxRect& r, const wxPen& p, bool boDrawOnTop, bool boCanDrag, const wxString& desc,
                  int incX = 1, int incY = 1, int incW = 1, int incH = 1, int minW = 1, int minH = 1 ) : m_incX( incX ), m_incY( incY ), m_incW( incW ), m_incH( incH ),
        m_minW( minW ), m_minH( minH ), m_rect( r ), m_pen( p ), m_boDrawOnTop( boDrawOnTop ), m_boCanDrag( boCanDrag ), m_description( desc ) {}
};

#endif // DrawingCanvasH
