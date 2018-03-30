//-----------------------------------------------------------------------------
#include <algorithm>
#include <apps/Common/wxAbstraction.h>
#include "PackageDescriptionParser.h"
#include <map>

typedef std::map<std::string, std::string> StringMap;

using namespace std;

//-----------------------------------------------------------------------------
bool ExtractStringAttribute( const StringMap& attrMap, const std::string& attrName, wxString& destination )
//-----------------------------------------------------------------------------
{
    StringMap::const_iterator it = attrMap.find( attrName );
    if( it != attrMap.end() )
    {
        destination = ConvertedString( it->second );
    }
    return ( ( it == attrMap.end() ) ? false : true );
}

//-----------------------------------------------------------------------------
bool ExtractVersionInformation( const StringMap& attrMap, const std::string& tagName, Version& versionEntry )
//-----------------------------------------------------------------------------
{
    StringMap::const_iterator it = attrMap.find( tagName );
    if( it != attrMap.end() )
    {
        versionEntry = VersionFromString( ConvertedString( it->second ) );
    }
    return ( ( it == attrMap.end() ) ? false : true );
}

const string PackageDescriptionFileParser::m_ATTR_NAME( "Name" );
const string PackageDescriptionFileParser::m_ATTR_TYPE( "Type" );
const string PackageDescriptionFileParser::m_ATTR_DESCRIPTION( "Description" );
const string PackageDescriptionFileParser::m_ATTR_VERSION( "Version" );
const string PackageDescriptionFileParser::m_ATTR_REVISION_MIN( "RevisionMin" );
const string PackageDescriptionFileParser::m_ATTR_REVISION_MAX( "RevisionMax" );

//-----------------------------------------------------------------------------
PackageDescriptionFileParser::TTagType PackageDescriptionFileParser::GetTagType( const char* tagName ) const
//-----------------------------------------------------------------------------
{
    if( strcmp( tagName, "PackageDescription" ) == 0 )
    {
        return ttPackageDescription;
    }
    else if( strcmp( tagName, "File" ) == 0 )
    {
        return ttFile;
    }
    else if( strcmp( tagName, "SuitableProductKey" ) == 0 )
    {
        return ttSuitableProductKey;
    }
    return ttInvalid;
}

//-----------------------------------------------------------------------------
void PackageDescriptionFileParser::OnPostCreate( void )
//-----------------------------------------------------------------------------
{
    m_packageDescriptionVersion.major_ = 1;
    m_packageDescriptionVersion.minor_ = 0;
    EnableStartElementHandler();
}

//-----------------------------------------------------------------------------
void PackageDescriptionFileParser::OnStartElement( const XML_Char* pszName, const XML_Char** papszAttrs )
//-----------------------------------------------------------------------------
{
    if( ( m_packageDescriptionVersion.major_ != 1 ) ||
        ( m_packageDescriptionVersion.minor_ != 0 ) )
    {
        m_lastError = wxString::Format( wxT( "Unsupported package description version: %ld.%ld.\n" ), m_packageDescriptionVersion.major_, m_packageDescriptionVersion.minor_ );
        return;
    }

    // build attribute map
    StringMap attrMap;
    int i = 0;
    while( papszAttrs[i] )
    {
        attrMap.insert( pair<string, string>( papszAttrs[i], papszAttrs[i + 1] ) );
        i += 2;
    }

    switch( GetTagType( pszName ) )
    {
    case ttPackageDescription:
        {
            if( !ExtractVersionInformation( attrMap, m_ATTR_VERSION, m_packageDescriptionVersion ) )
            {
                m_lastError = wxString( wxT( "Version information tag is missing in package format description" ) );
                m_packageDescriptionVersion = Version();
                break;
            }
        }
        break;
    case ttFile:
        {
            FileEntry entry;
            if( !ExtractStringAttribute( attrMap, m_ATTR_DESCRIPTION, entry.description_ ) ||
                !ExtractStringAttribute( attrMap, m_ATTR_NAME, entry.name_ ) ||
                !ExtractStringAttribute( attrMap, m_ATTR_TYPE, entry.type_ ) ||
                !ExtractVersionInformation( attrMap, m_ATTR_VERSION, entry.version_ ) )
            {
                m_lastError = wxString( wxT( "At least one required attribute for the file tag is missing" ) );
                break;
            }
            m_results.push_back( entry );
        }
        break;
    case ttSuitableProductKey:
        {
            if( m_results.empty() )
            {
                m_lastError = wxString( wxT( "Orphaned 'SuitableProductKey' tag detected" ) );
                break;
            }

            SuitableProductKey data;
            if( !ExtractStringAttribute( attrMap, m_ATTR_NAME, data.name_ ) ||
                !ExtractVersionInformation( attrMap, m_ATTR_REVISION_MAX, data.revisionMax_ ) ||
                !ExtractVersionInformation( attrMap, m_ATTR_REVISION_MIN, data.revisionMin_ ) )
            {
                // at least one required attribute is missing
                break;
            }

            m_results[m_results.size() - 1].suitableProductKeys_.insert( make_pair( data.name_, data ) );
        }
        break;
    default:
        // invalid tag...
        break;
    }
}
