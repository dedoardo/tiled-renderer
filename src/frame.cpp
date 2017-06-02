/* frame.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/frame.hpp>

// camy
#include <camy/buckets.hpp>

namespace camy
{
    Frame::Frame() {}

    Frame::~Frame() {}

    void Frame::add_bucket(IBucket& bucket, rsize pass)
    {
        CAMY_ASSERT(pass < API::MAX_PASSES);
        m_buckets[pass].append(&bucket);
    }

    void Frame::remove_bucket(IBucket& bucket)
    {
        for (rsize i = 0; i < API::MAX_PASSES; ++i)
        {
            for (rsize j = 0; j < m_buckets[i].count(); ++j)
            {
                if (m_buckets[i][j] == &bucket)
                {
                    m_buckets[i].remove(j);
                    return;
                }
            }
        }
    }

    void Frame::render()
    {
        rsize current_pass = 0;
        while (current_pass < API::MAX_PASSES)
        {
            rsize flushed = 0;
            rsize to_flush = m_buckets[current_pass].count();
            while (flushed < to_flush)
            {
                for (rsize i = 0; i < m_buckets[current_pass].count(); ++i)
                {
                    IBucket* bucket = m_buckets[current_pass][i];
                    if (bucket->is_ready())
                    {
                        API::rc().flush(bucket->to_command_list());
                        ++flushed;
                    }
                }
            }
            ++current_pass;
        }
    }
}