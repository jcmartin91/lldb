//===-- SBCommunication.cpp -------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/API/SBCommunication.h"
#include "lldb/API/SBBroadcaster.h"
#include "lldb/Core/Communication.h"
#include "lldb/Core/ConnectionFileDescriptor.h"
#include "lldb/Core/Log.h"

using namespace lldb;
using namespace lldb_private;



SBCommunication::SBCommunication() :
    m_opaque (NULL),
    m_opaque_owned (false)
{
}

SBCommunication::SBCommunication(const char * broadcaster_name) :
    m_opaque (new Communication (broadcaster_name)),
    m_opaque_owned (true)
{
    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));

    if (log)
        log->Printf ("SBCommunication::SBCommunication (broadcaster_name=\"%s\") => "
                     "SBCommunication(%p)", broadcaster_name, m_opaque);
}

SBCommunication::~SBCommunication()
{
    if (m_opaque && m_opaque_owned)
        delete m_opaque;
    m_opaque = NULL;
    m_opaque_owned = false;
}

bool
SBCommunication::IsValid () const
{
    return m_opaque != NULL;
}

bool
SBCommunication::GetCloseOnEOF ()
{
    if (m_opaque)
        return m_opaque->GetCloseOnEOF ();
    return false;
}

void
SBCommunication::SetCloseOnEOF (bool b)
{
    if (m_opaque)
        m_opaque->SetCloseOnEOF (b);
}

ConnectionStatus
SBCommunication::Connect (const char *url)
{
    if (m_opaque)
    {
        if (!m_opaque->HasConnection ())
            m_opaque->SetConnection (new ConnectionFileDescriptor());
        return m_opaque->Connect (url, NULL);
    }
    return eConnectionStatusNoConnection;
}

ConnectionStatus
SBCommunication::AdoptFileDesriptor (int fd, bool owns_fd)
{
    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));

    ConnectionStatus status = eConnectionStatusNoConnection;
    if (m_opaque)
    {
        if (m_opaque->HasConnection ())
        {
            if (m_opaque->IsConnected())
                m_opaque->Disconnect();
        }
        m_opaque->SetConnection (new ConnectionFileDescriptor (fd, owns_fd));
        if (m_opaque->IsConnected())
            status = eConnectionStatusSuccess;
        else
            status = eConnectionStatusLostConnection;
    }

    if (log)
        log->Printf ("SBCommunication(%p)::AdoptFileDescriptor (fd=%d, ownd_fd=%i) => %s", 
                     m_opaque, fd, owns_fd, Communication::ConnectionStatusAsCString (status));

    return status;
}


ConnectionStatus
SBCommunication::Disconnect ()
{
    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));

    ConnectionStatus status= eConnectionStatusNoConnection;
    if (m_opaque)
        status = m_opaque->Disconnect ();

    if (log)
        log->Printf ("SBCommunication(%p)::Disconnect () => %s", m_opaque,
                     Communication::ConnectionStatusAsCString (status));

    return status;
}

bool
SBCommunication::IsConnected () const
{
    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    bool result = false;
    if (m_opaque)
        result = m_opaque->IsConnected ();

    if (log)
        log->Printf ("SBCommunication(%p)::IsConnected () => %i", m_opaque, result);

    return false;
}

size_t
SBCommunication::Read (void *dst, size_t dst_len, uint32_t timeout_usec, ConnectionStatus &status)
{
    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    if (log)
        log->Printf ("SBCommunication(%p)::Read (dst=%p, dst_len=%zu, timeout_usec=%u, &status)...", 
                     m_opaque, dst, dst_len, timeout_usec);
    size_t bytes_read = 0;
    if (m_opaque)
        bytes_read = m_opaque->Read (dst, dst_len, timeout_usec, status, NULL);
    else
        status = eConnectionStatusNoConnection;

    if (log)
        log->Printf ("SBCommunication(%p)::Read (dst=%p, dst_len=%zu, timeout_usec=%u, &status=%s) => %zu", 
                     m_opaque, dst, dst_len, timeout_usec, Communication::ConnectionStatusAsCString (status),
                     bytes_read);
    return bytes_read;
}


size_t
SBCommunication::Write (const void *src, size_t src_len, ConnectionStatus &status)
{
    size_t bytes_written = 0;
    if (m_opaque)
        bytes_written = m_opaque->Write (src, src_len, status, NULL);
    else
        status = eConnectionStatusNoConnection;

    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    if (log)
        log->Printf ("SBCommunication(%p)::Write (src=%p, src_len=%zu, &status=%s) => %zu", 
                     m_opaque, src, src_len, Communication::ConnectionStatusAsCString (status), bytes_written);

    return 0;
}

bool
SBCommunication::ReadThreadStart ()
{
    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));

    bool success = false;
    if (m_opaque)
        success = m_opaque->StartReadThread ();

    log = lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API);
    if (log)
        log->Printf ("SBCommunication(%p)::ReadThreadStart () => %i", m_opaque, success);

    return success;
}


bool
SBCommunication::ReadThreadStop ()
{
    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    if (log)
        log->Printf ("SBCommunication(%p)::ReadThreadStop ()...", m_opaque);

    bool success = false;
    if (m_opaque)
        success = m_opaque->StopReadThread ();

    if (log)
        log->Printf ("SBCommunication(%p)::ReadThreadStop () => %i", m_opaque, success);

    return success;
}

bool
SBCommunication::ReadThreadIsRunning ()
{
    bool result = false;
    if (m_opaque)
        result = m_opaque->ReadThreadIsRunning ();
    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    if (log)
        log->Printf ("SBCommunication(%p)::ReadThreadIsRunning () => %i", m_opaque, result);
    return result;
}

bool
SBCommunication::SetReadThreadBytesReceivedCallback
(
    ReadThreadBytesReceived callback,
    void *callback_baton
)
{
    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));

    bool result = false;
    if (m_opaque)
    {
        m_opaque->SetReadThreadBytesReceivedCallback (callback, callback_baton);
        result = true;
    }

    if (log)
        log->Printf ("SBCommunication(%p)::SetReadThreadBytesReceivedCallback (callback=%p, baton=%p) => %i",
                     m_opaque, callback, callback_baton, result);

    return result;
}

SBBroadcaster
SBCommunication::GetBroadcaster ()
{
    SBBroadcaster broadcaster (m_opaque, false);

    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));

    if (log)
        log->Printf ("SBCommunication(%p)::GetBroadcaster () => SBBroadcaster (%p)",
                     m_opaque, broadcaster.get());

    return broadcaster;
}

const char *
SBCommunication::GetBroadcasterClass ()
{
    return Communication::GetStaticBroadcasterClass().AsCString();
}

//
//void
//SBCommunication::CreateIfNeeded ()
//{
//    if (m_opaque == NULL)
//    {
//        static uint32_t g_broadcaster_num;
//        char broadcaster_name[256];
//        ::snprintf (name, broadcaster_name, "%p SBCommunication", this);
//        m_opaque = new Communication (broadcaster_name);
//        m_opaque_owned = true;
//    }
//    assert (m_opaque);
//}
//
//
