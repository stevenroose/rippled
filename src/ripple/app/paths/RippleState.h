//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLE_RIPPLESTATE_H
#define RIPPLE_RIPPLESTATE_H

namespace ripple {

//
// A ripple line's state.
// - Isolate ledger entry format.
//

class RippleState
{
public:
    typedef std::shared_ptr <RippleState> pointer;

public:
    RippleState () = delete;

    virtual ~RippleState () { }

    static RippleState::pointer makeItem (
        Account const& accountID, SerializedLedgerEntry::ref ledgerEntry);

    LedgerEntryType getType ()
    {
        return ltRIPPLE_STATE;
    }

    Account const& getAccountID () const
    {
        return  mViewLowest ? mLowID : mHighID;
    }

    Account const& getAccountIDPeer () const
    {
        return !mViewLowest ? mLowID : mHighID;
    }

    // True, Provided auth to peer.
    bool getAuth () const
    {
        return mFlags & (mViewLowest ? lsfLowAuth : lsfHighAuth);
    }

    bool getAuthPeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowAuth : lsfHighAuth);
    }

    bool getNoRipple () const
    {
        return mFlags & (mViewLowest ? lsfLowNoRipple : lsfHighNoRipple);
    }

    bool getNoRipplePeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowNoRipple : lsfHighNoRipple);
    }

    /** Have we set the freeze flag on our peer */
    bool getFreeze () const
    {
        return mFlags & (mViewLowest ? lsfLowFreeze : lsfHighFreeze);
    }

    /** Has the peer set the freeze flag on us */
    bool getFreezePeer () const
    {
        return mFlags & (!mViewLowest ? lsfLowFreeze : lsfHighFreeze);
    }

    STAmount const& getBalance () const
    {
        return mBalance;
    }

    STAmount const& getLimit () const
    {
        return  mViewLowest ? mLowLimit : mHighLimit;
    }

    STAmount const& getLimitPeer () const
    {
        return !mViewLowest ? mLowLimit : mHighLimit;
    }

    std::uint32_t getQualityIn () const
    {
        return ((std::uint32_t) (mViewLowest ? mLowQualityIn : mHighQualityIn));
    }

    std::uint32_t getQualityOut () const
    {
        return ((std::uint32_t) (mViewLowest ? mLowQualityOut : mHighQualityOut));
    }

    SerializedLedgerEntry::pointer getSLE ()
    {
        return mLedgerEntry;
    }

    const SerializedLedgerEntry& peekSLE () const
    {
        return *mLedgerEntry;
    }

    SerializedLedgerEntry& peekSLE ()
    {
        return *mLedgerEntry;
    }

    Json::Value getJson (int);

    Blob getRaw () const;

private:
    RippleState (
        SerializedLedgerEntry::ref ledgerEntry,
        Account const& viewAccount);

private:
    SerializedLedgerEntry::pointer  mLedgerEntry;

    bool                            mViewLowest;

    std::uint32_t                   mFlags;

    STAmount const&                 mLowLimit;
    STAmount const&                 mHighLimit;

    Account const&                  mLowID;
    Account const&                  mHighID;

    std::uint64_t                   mLowQualityIn;
    std::uint64_t                   mLowQualityOut;
    std::uint64_t                   mHighQualityIn;
    std::uint64_t                   mHighQualityOut;

    STAmount                        mBalance;
};

std::vector <RippleState::pointer>
getRippleStateItems (
    Account const& accountID,
    Ledger::ref ledger);

} // ripple

#endif
