//-----------------------------------------------------------------------------
#include "DataConversion.h"

using namespace std;

//-----------------------------------------------------------------------------
string charToFormat( int c )
//-----------------------------------------------------------------------------
{
    switch( c )
    {
    case 'i':
        return string( "%d" );
    case 'I':
        return string( MY_FMT_I64 );
    case 'f':
        return string( "%f" );
    case 's':
        return string( "%s" );
    case 'p':
        return string( "%p" );
    }
    return string( "" );
}

//-----------------------------------------------------------------------------
string charToType( int c )
//-----------------------------------------------------------------------------
{
    switch( c )
    {
    case 'v':
        return string( "void" );
    case 'p':
        return string( "void*" );
    case 'i':
        return string( "int" );
    case 'I':
        return string( "int64" );
    case 'f':
        return string( "float" );
    case 's':
        return string( "char*" );
    }
    return string( "" );
}
