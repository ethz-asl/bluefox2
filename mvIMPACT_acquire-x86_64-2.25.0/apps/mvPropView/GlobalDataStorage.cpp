//-----------------------------------------------------------------------------
#include <algorithm>
#include <apps/Common/wxAbstraction.h>
#include <common/STLHelper.h>
#include "GlobalDataStorage.h"
#include "WizardIcons.h"

using namespace std;

GlobalDataStorage* GlobalDataStorage::pInstance_ = 0;

//-----------------------------------------------------------------------------
GlobalDataStorage::GlobalDataStorage() : componentVisibility_( cvBeginner ), boComponentVisibilitySupported_( false ),
    LIST_BACKGROUND_COLOUR_( 230, 230, 230 ), PROPERTY_TEXT_COLOUR_( 5, 165, 5 ), INVISIBLE_EXPERT_FEATURE_COLOUR_( acGreenPastel ),
    INVISIBLE_GURU_FEATURE_COLOUR_( acYellowPastel ), INVISIBLE_FEATURE_COLOUR_( acRedPastel )
//-----------------------------------------------------------------------------
{
    bitmapHashTable_.insert( make_pair( bIcon_ColorPreset, new wxBitmap( wizard_color_preset_xpm ) ) );
    bitmapHashTable_.insert( make_pair( bIcon_ColorPreset_Disabled, new wxBitmap( wizard_color_preset_disabled_xpm ) ) );
    bitmapHashTable_.insert( make_pair( bIcon_GreyPreset, new wxBitmap( wizard_gray_preset_xpm ) ) );
    bitmapHashTable_.insert( make_pair( bIcon_FactoryPreset, new wxBitmap( wizard_factory_preset_xpm ) ) );
    bitmapHashTable_.insert( make_pair( bIcon_Tacho, new wxBitmap( wizard_tacho_xpm ) ) );
    bitmapHashTable_.insert( make_pair( bIcon_Diamond, new wxBitmap( wizard_diamond_xpm ) ) );
    bitmapHashTable_.insert( make_pair( bIcon_Warning, new wxBitmap( wizard_warning_xpm ) ) );
    bitmapHashTable_.insert( make_pair( bIcon_Empty, new wxBitmap( wizard_empty_xpm ) ) );
}

//-----------------------------------------------------------------------------
GlobalDataStorage::~GlobalDataStorage()
//-----------------------------------------------------------------------------
{
    for_each( bitmapHashTable_.begin(), bitmapHashTable_.end(), ptr_fun( DeleteSecond<const TBitmap, wxBitmap*> ) );
    pInstance_ = 0;
}

//-----------------------------------------------------------------------------
const wxBitmap* GlobalDataStorage::GetBitmap( TBitmap bitmapType ) const
//-----------------------------------------------------------------------------
{
    const std::map<TBitmap, wxBitmap*>::const_iterator it = bitmapHashTable_.find( bitmapType );
    return ( it == bitmapHashTable_.end() ) ? 0 : it->second;
}

//-----------------------------------------------------------------------------
const wxColour& GlobalDataStorage::GetPropGridColour( TPropGridColour colour ) const
//-----------------------------------------------------------------------------
{
    switch( colour )
    {
    case pgcInvisibleExpertFeature:
        return INVISIBLE_EXPERT_FEATURE_COLOUR_;
    case pgcInvisibleGuruFeature:
        return INVISIBLE_GURU_FEATURE_COLOUR_;
    case pgcInvisibleFeature:
        return INVISIBLE_FEATURE_COLOUR_;
    case pgcListBackground:
        return LIST_BACKGROUND_COLOUR_;
    case pgcContentDescriptorText:
        return *wxBLUE;
    case pgcPropertyText:
        break;
    }
    return PROPERTY_TEXT_COLOUR_;
}

//-----------------------------------------------------------------------------
GlobalDataStorage* GlobalDataStorage::Instance( void )
//-----------------------------------------------------------------------------
{
    if( !pInstance_ )
    {
        pInstance_ = new GlobalDataStorage();
    }
    return pInstance_;
}
