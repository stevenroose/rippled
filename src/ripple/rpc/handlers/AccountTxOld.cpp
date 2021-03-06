//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2014 Ripple Labs Inc.

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


namespace ripple {

// {
//   account: account,
//   ledger_index_min: ledger_index,
//   ledger_index_max: ledger_index,
//   binary: boolean,              // optional, defaults to false
//   count: boolean,               // optional, defaults to false
//   descending: boolean,          // optional, defaults to false
//   offset: integer,              // optional, defaults to 0
//   limit: integer                // optional
// }
Json::Value doAccountTxOld (RPC::Context& context)
{
    RippleAddress   raAccount;
    std::uint32_t offset
            = context.params_.isMember ("offset")
            ? context.params_["offset"].asUInt () : 0;
    int limit = context.params_.isMember ("limit")
            ? context.params_["limit"].asUInt () : -1;
    bool bBinary = context.params_.isMember ("binary")
            && context.params_["binary"].asBool ();
    bool bDescending = context.params_.isMember ("descending")
            && context.params_["descending"].asBool ();
    bool bCount = context.params_.isMember ("count")
            && context.params_["count"].asBool ();
    std::uint32_t   uLedgerMin;
    std::uint32_t   uLedgerMax;
    std::uint32_t   uValidatedMin;
    std::uint32_t   uValidatedMax;
    bool bValidated  = context.netOps_.getValidatedRange (
        uValidatedMin, uValidatedMax);

    if (!context.params_.isMember ("account"))
        return rpcError (rpcINVALID_PARAMS);

    if (!raAccount.setAccountID (context.params_["account"].asString ()))
        return rpcError (rpcACT_MALFORMED);

    if (offset > 3000)
        return rpcError (rpcATX_DEPRECATED);

    context.loadType_ = Resource::feeHighBurdenRPC;

    // DEPRECATED
    if (context.params_.isMember ("ledger_min"))
    {
        context.params_["ledger_index_min"]   = context.params_["ledger_min"];
        bDescending = true;
    }

    // DEPRECATED
    if (context.params_.isMember ("ledger_max"))
    {
        context.params_["ledger_index_max"]   = context.params_["ledger_max"];
        bDescending = true;
    }

    if (context.params_.isMember ("ledger_index_min")
        || context.params_.isMember ("ledger_index_max"))
    {
        std::int64_t iLedgerMin  = context.params_.isMember ("ledger_index_min")
                ? context.params_["ledger_index_min"].asInt () : -1;
        std::int64_t iLedgerMax  = context.params_.isMember ("ledger_index_max")
                ? context.params_["ledger_index_max"].asInt () : -1;

        if (!bValidated && (iLedgerMin == -1 || iLedgerMax == -1))
        {
            // Don't have a validated ledger range.
            return rpcError (rpcLGR_IDXS_INVALID);
        }

        uLedgerMin  = iLedgerMin == -1 ? uValidatedMin : iLedgerMin;
        uLedgerMax  = iLedgerMax == -1 ? uValidatedMax : iLedgerMax;

        if (uLedgerMax < uLedgerMin)
        {
            return rpcError (rpcLGR_IDXS_INVALID);
        }
    }
    else
    {
        Ledger::pointer l;
        Json::Value ret = RPC::lookupLedger (context.params_, l, context.netOps_);

        if (!l)
            return ret;

        uLedgerMin = uLedgerMax = l->getLedgerSeq ();
    }

    int count = 0;

#ifndef BEAST_DEBUG

    try
    {
#endif

        Json::Value ret (Json::objectValue);

        ret["account"] = raAccount.humanAccountID ();
        Json::Value& jvTxns = (ret["transactions"] = Json::arrayValue);

        if (bBinary)
        {
            auto txns = context.netOps_.getAccountTxsB (
                raAccount, uLedgerMin, uLedgerMax, bDescending, offset, limit,
                context.role_ == Config::ADMIN);

            for (auto it = txns.begin (), end = txns.end (); it != end; ++it)
            {
                ++count;
                Json::Value& jvObj = jvTxns.append (Json::objectValue);

                std::uint32_t  uLedgerIndex = std::get<2> (*it);
                jvObj["tx_blob"]            = std::get<0> (*it);
                jvObj["meta"]               = std::get<1> (*it);
                jvObj["ledger_index"]       = uLedgerIndex;
                jvObj["validated"]
                        = bValidated
                        && uValidatedMin <= uLedgerIndex
                        && uValidatedMax >= uLedgerIndex;

            }
        }
        else
        {
            auto txns = context.netOps_.getAccountTxs (
                raAccount, uLedgerMin, uLedgerMax, bDescending, offset, limit,
                context.role_ == Config::ADMIN);

            for (auto it = txns.begin (), end = txns.end (); it != end; ++it)
            {
                ++count;
                Json::Value&    jvObj = jvTxns.append (Json::objectValue);

                if (it->first)
                    jvObj["tx"]             = it->first->getJson (1);

                if (it->second)
                {
                    std::uint32_t uLedgerIndex = it->second->getLgrSeq ();

                    jvObj["meta"]           = it->second->getJson (0);
                    jvObj["validated"]
                            = bValidated
                            && uValidatedMin <= uLedgerIndex
                            && uValidatedMax >= uLedgerIndex;
                }

            }
        }

        //Add information about the original query
        ret["ledger_index_min"] = uLedgerMin;
        ret["ledger_index_max"] = uLedgerMax;
        ret["validated"]
                = bValidated
                && uValidatedMin <= uLedgerMin
                && uValidatedMax >= uLedgerMax;
        ret["offset"]           = offset;

        // We no longer return the full count but only the count of returned
        // transactions. Computing this count was two expensive and this API is
        // deprecated anyway.
        if (bCount)
            ret["count"]        = count;

        if (context.params_.isMember ("limit"))
            ret["limit"]        = limit;


        return ret;
#ifndef BEAST_DEBUG
    }
    catch (...)
    {
        return rpcError (rpcINTERNAL);
    }

#endif
}

} // ripple
