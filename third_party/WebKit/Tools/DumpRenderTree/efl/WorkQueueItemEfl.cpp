/*
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WorkQueueItemEfl.h"

#include "DumpRenderTree.h"

#include <EWebKit.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/wtf/text/CString.h>

bool LoadItem::invoke() const
{
    Evas_Object* targetFrame;

    if (m_target.isEmpty())
        targetFrame = mainFrame;
    else
        targetFrame = ewk_frame_child_find(mainFrame, m_target.utf8().data());

    ewk_frame_uri_set(targetFrame, m_url.utf8().data());

    return true;
}

bool LoadHTMLStringItem::invoke() const
{
    if (m_unreachableURL.isEmpty())
        ewk_frame_contents_set(mainFrame, m_content.utf8().data(), 0, 0, 0, m_baseURL.utf8().data());
    else
        ewk_frame_contents_alternate_set(mainFrame, m_content.utf8().data(), 0, 0, 0, m_baseURL.utf8().data(), m_unreachableURL.utf8().data());

    return true;
}

bool ReloadItem::invoke() const
{
    ewk_view_reload(browser);
    return true;
}

bool ScriptItem::invoke() const
{
    return ewk_frame_script_execute(mainFrame, m_script.utf8().data());
}

bool BackForwardItem::invoke() const
{
    Ewk_History* history = ewk_view_history_get(browser);

    if (m_howFar == 1)
        ewk_history_forward(history);
    else if (m_howFar == -1)
        ewk_history_back(history);
    else {
        const Ewk_History_Item* item = ewk_history_history_item_nth_get(history, m_howFar);
        ewk_history_history_item_set(history, item);
    }

    return true;
}
