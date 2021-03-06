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

#ifndef RIPPLE_HTTP_PLAINPEER_H_INCLUDED
#define RIPPLE_HTTP_PLAINPEER_H_INCLUDED

#include <ripple/http/impl/Peer.h>

namespace ripple {
namespace HTTP {

class PlainPeer
    : public Peer <PlainPeer>
    , public std::enable_shared_from_this <PlainPeer>
{
private:
    friend class Peer <PlainPeer>;
    using socket_type = boost::asio::ip::tcp::socket;
    socket_type socket_;

public:
    template <class ConstBufferSequence>
    PlainPeer (Door& door, beast::Journal journal, endpoint_type endpoint,
        ConstBufferSequence const& buffers, socket_type&& socket);

    void
    accept();

private:
    void
    do_request();

    void
    do_close();
};

//------------------------------------------------------------------------------

template <class ConstBufferSequence>
PlainPeer::PlainPeer (Door& door, beast::Journal journal,
    endpoint_type endpoint, ConstBufferSequence const& buffers,
        boost::asio::ip::tcp::socket&& socket)
    : Peer (door, socket.get_io_service(), journal, endpoint, buffers)
    , socket_(std::move(socket))
{
}

void
PlainPeer::accept ()
{
    door_.server().handler().onAccept (session());
    if (! socket_.is_open())
        return;

    boost::asio::spawn (strand_, std::bind (&PlainPeer::do_read,
        shared_from_this(), std::placeholders::_1));
}

void
PlainPeer::do_request()
{
    // Perform half-close when Connection: close and not SSL
    error_code ec;
    if (! message_.keep_alive())
        socket_.shutdown (socket_type::shutdown_receive, ec);

    if (! ec)
    {
        ++request_count_;
        door_.server().handler().onRequest (session());
        return;
    }

    if (ec)
        fail (ec, "request");
}

void
PlainPeer::do_close()
{
    error_code ec;
    socket_.shutdown (socket_type::shutdown_send, ec);
}

}
}

#endif
