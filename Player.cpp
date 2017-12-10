// Player.cpp
//
// implementation of the Player

#include "Player.h"
#include "TileSet.h"
#include "Game.h"
#include "Dungeon.h"
#include "DisplayText.h"

extern CGame *g_pGame;

void CPlayer::Init()
{
	// Initialize all the player stuff, baby.
	m_TileSet = new CTileset("Resources/Courier.png", 32, 32 );

	// This will likely get moved somewhere else. --Jimbo
	SpawnPlayer();
}

bool CPlayer::Update(float fCurTime)
{
    DisplayStats();
    DisplayInventory();
    DisplayEquipment();
	return true;
}

void CPlayer::Draw()
{
	Uint8 player_tile = '@' - ' ' - 1;//TileIDs[TILE_IDX_PLAYER] - ' ' - 1;
	JVector vSize(1,1);
    JColor player_color(255,255,255,255);

	PreDraw();
    m_TileSet->SetTileColor(player_color);
	m_TileSet->DrawTile( player_tile, m_vPos, vSize, true );
	PostDraw();
}

void CPlayer::PreDraw()
{
	g_pGame->GetDungeon()->PreDraw();
}

void CPlayer::PostDraw()
{
	g_pGame->GetDungeon()->PostDraw();
}

JResult CPlayer::SpawnPlayer()
{
	printf("Trying to spawn player...");
	JVector vTryPos;
	while( !m_bHasSpawned )
	{
		vTryPos.Init( (float)Util::GetRandom(0, DUNG_WIDTH-1), (float)Util::GetRandom(0, DUNG_HEIGHT-1) );

		//printf("Trying to spawn player at <%.2f %.2f>...\n", vTryPos.x, vTryPos.y);
		//g_pGame->GetMsgs()->Printf( "Trying to spawn player at <%.2f %.2f>...\n", vTryPos.x, vTryPos.y );

		// try changing this to "iswalkable" -- might need to move that to dungeon. --Jimbo
		if( g_pGame->GetDungeon()->IsWalkableFor(vTryPos, true) == DUNG_COLL_NO_COLLISION )
		{
			m_vPos = vTryPos;
			m_bHasSpawned = true;
			printf("Success!\n");
			//g_pGame->GetMsgs()->Printf( "Success!\n" );
		}
	}

	return JSUCCESS;
}

void CPlayer::DisplayStats()
{
    g_pGame->GetStats()->Clear();
    g_pGame->GetStats()->Printf("AC:%d\n", (int)m_fArmorClass);
    g_pGame->GetStats()->Printf("HP:%d\n", (int)m_fHitPoints);
    g_pGame->GetStats()->Printf("\n");
    g_pGame->GetStats()->Printf("Damage:%s\n", m_szDamage);
    g_pGame->GetStats()->Printf("+to Hit:%d\n", (int)m_fToHitModifier);
    g_pGame->GetStats()->Printf("+to Dam:%d\n", (int)m_fDamageModifier);
    g_pGame->GetStats()->Printf("\n");
    g_pGame->GetStats()->Printf("\n");
    g_pGame->GetStats()->Printf("Level:%d\n", (int)m_fLevel);
    g_pGame->GetStats()->Printf("Exp:%d\n", (int)m_fExperience);
}

void CPlayer::DisplayInventory()
{
    CLink<CItem> *pLink = m_llInventory->GetHead();
    CItem *pItem;
    g_pGame->GetInv()->Clear();
    g_pGame->GetInv()->Printf("You are carrying:\n");
    char cInventoryId = 'a';
    while(pLink != NULL)
    {
        pItem = pLink->m_lpData;
        g_pGame->GetInv()->Printf("%c - %s\n", cInventoryId, pItem->GetName());
        
        if( cInventoryId < 'z' )
        {
            cInventoryId++;
        }
        else
        {
            printf("Inventory past first page not shown.\n");
            break;
        }
        pLink = m_llInventory->GetNext(pLink);
    }
    
}

void CPlayer::DisplayEquipment()
{
    CLink<CItem> *pLink = m_llEquipment->GetHead();
    CItem *pItem;
    g_pGame->GetEquip()->Clear();
    g_pGame->GetEquip()->Printf("You are wearing:\n");
    char cEquipmentId = 'a';
    while(pLink != NULL)
    {
        pItem = pLink->m_lpData;
        g_pGame->GetEquip()->Printf("%c - %s\n", cEquipmentId, pItem->GetName());
        
        if( cEquipmentId < 'z' )
        {
            cEquipmentId++;
        }
        else
        {
            printf("Equipment is limited to N items, one each for specific body parts.\n");
            break;
        }
        pLink = m_llEquipment->GetNext(pLink);
    }
    
}

void CPlayer::PickUp( JVector &vPickupPos )
{
    CItem *pItem = g_pGame->GetDungeon()->PickUp(vPickupPos);
    pItem->m_pllLink = m_llInventory->Add(pItem, pItem->m_id->m_dwIndex);
    g_pGame->GetDungeon()->GetTile(vPickupPos)->m_pCurItem = NULL;
    g_pGame->GetMsgs()->Printf( "You have a %s.\n", pItem->GetName() );
}

bool CPlayer::IsWieldable(CLink<CItem> *pItem)
{
    bool retval = false;
    switch( pItem->m_lpData->m_id->m_dwIndex )
    {
        case ITEM_IDX_SWORD:
        case ITEM_IDX_SHIELD:
        case ITEM_IDX_ARMOR:
        case ITEM_IDX_BOW:
        case ITEM_IDX_XBOW:
        case ITEM_IDX_CLOAK:
        case ITEM_IDX_GLOVES:
        case ITEM_IDX_RING:
        case ITEM_IDX_BOOTS:
        case ITEM_IDX_TORCH:
        case ITEM_IDX_AMULET:
            retval = true;
            break;
        default:
            retval = false;
            break;
    }
    return retval;
}

bool CPlayer::Wield(CLink<CItem> *pLink)
{
    CItem *pItem = pLink->m_lpData;
    
    // You can only wield one thing of a given type at a time
    CLink<CItem> *pCurrEquip = m_llEquipment->GetLink(pItem->m_id->m_dwIndex, true);
    if( pCurrEquip != NULL && pCurrEquip->m_dwIndex == pLink->m_dwIndex )
    {
        // So if you're already wearing something of this type, remove it and put it back in inventory
        if( Remove(pCurrEquip) )
        {
            g_pGame->GetMsgs()->Printf("You were wielding the %s...", pCurrEquip->m_lpData->GetName());
        }
        else
        {
            return false;
        }
    }
    // Now put on the new item.
    m_llInventory->Remove(pLink, false);
    pItem->m_pllLink = m_llEquipment->Add(pItem, pItem->m_id->m_dwIndex);
    m_fArmorClass += pItem->m_id->m_fBaseAC + pItem->m_id->m_fACBonus;
    if( pItem->m_id->m_szBaseDamage != NULL ) strcpy(m_szDamage, pItem->m_id->m_szBaseDamage);
    m_fDamageModifier += pItem->m_id->m_fBonusToDamage;
    m_fToHitModifier += pItem->m_id->m_fBonusToHit;
    
    return true;
}

bool CPlayer::IsRemovable(CLink<CItem> *pLink)
{
    CItem *pItem = pLink->m_lpData;
    if( pItem->m_dwFlags & ITEM_FLAG_CURSED )
    {
        return false;
    }
    return true;
}

bool CPlayer::Remove(CLink<CItem> *pLink)
{
    CItem *pItem = pLink->m_lpData;
    if(pItem->m_dwFlags & ITEM_FLAG_CURSED)
    {
        g_pGame->GetMsgs()->Printf("You can't remove the %s... it seems to be cursed.\n", pItem->GetName());
        return false;
    }
    m_llEquipment->Remove(pLink, false);
    pItem->m_pllLink = m_llInventory->Add(pItem, pItem->m_id->m_dwIndex);
    m_fArmorClass -= pItem->m_id->m_fBaseAC + pItem->m_id->m_fACBonus;
    if( pItem->m_id->m_szBaseDamage != NULL ) strcpy(m_szDamage, PLAYER_BASE_DAMAGE);
    m_fDamageModifier -= pItem->m_id->m_fBonusToDamage;
    m_fToHitModifier -= pItem->m_id->m_fBonusToHit;

    
    return true;
}

float CPlayer::Attack()
{
    float fRoll = Util::Roll( "1d100" );
    float fHitMod = g_pGame->GetPlayer()->m_fToHitModifier;
    
    printf( "You rolled: %.2f, +to-hit Bonus: %.2f = Total: %.2f\n", fRoll, fHitMod, fRoll + fHitMod );
    
    fRoll += fHitMod;
    
    return fRoll;
}

float CPlayer::Damage( float fDamageMult )
{
    float fDamage = ( Util::Roll( m_szDamage ) + m_fDamageModifier ) * fDamageMult;
    printf( "You did %.2f damage (damagemult: %.2f). ", fDamage, fDamageMult );

    return fDamage;
}

void CPlayer::OnKillMonster( CMonster *pMon )
{
    m_fExperience += pMon->m_md->m_fExpValue / m_fLevel;
    GainLevel();
}

void CPlayer::GainLevel()
{
    while( (int) m_fExperience > (int) m_pClass->m_fExpNeeded[(int)m_fLevel-1] )
    {
        m_fLevel++;
        float fAddedHP = Util::Roll(m_pClass->m_szHD);
        m_fHitPoints += fAddedHP;
        m_fCurHitPoints += fAddedHP;
        g_pGame->GetMsgs()->Printf("Welcome to level %d.\n", (int)m_fLevel);
        // TODO: Gain Spells or &c here.
    }
    
}
