// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/notification/download_item_notification.h"

#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/download/download_crx_util.h"
#include "chrome/browser/download/download_item_model.h"
#include "chrome/browser/download/notification/download_notification_manager.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "chrome/browser/notifications/profile_notification.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/mime_util/mime_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_interrupt_reasons.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"
#include "grit/theme_resources.h"
#include "net/base/mime_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/time_format.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/image/image.h"
#include "ui/message_center/message_center.h"

using base::UserMetricsAction;

namespace {

const char kDownloadNotificationNotifierId[] =
    "chrome://downloads/notification/id-notifier";

// Maximum size of preview image. If the image exceeds this size, don't show the
// preview image.
const int64 kMaxImagePreviewSize = 10 * 1024 * 1024;  // 10 MB

std::string ReadNotificationImage(const base::FilePath& file_path) {
  DCHECK(content::BrowserThread::GetBlockingPool()->RunsTasksOnCurrentThread());

  std::string data;
  bool ret = base::ReadFileToString(file_path, &data);
  if (!ret)
    return std::string();

  DCHECK_LE(data.size(), static_cast<size_t>(kMaxImagePreviewSize));

  return data;
}

void RecordButtonClickAction(DownloadCommands::Command command) {
  switch (command) {
    case DownloadCommands::SHOW_IN_FOLDER:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Button_ShowInFolder"));
      break;
    case DownloadCommands::OPEN_WHEN_COMPLETE:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Button_OpenWhenComplete"));
      break;
    case DownloadCommands::ALWAYS_OPEN_TYPE:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Button_AlwaysOpenType"));
      break;
    case DownloadCommands::PLATFORM_OPEN:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Button_PlatformOpen"));
      break;
    case DownloadCommands::CANCEL:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Button_Cancel"));
      break;
    case DownloadCommands::DISCARD:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Button_Discard"));
      break;
    case DownloadCommands::KEEP:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Button_Keep"));
      break;
    case DownloadCommands::LEARN_MORE_SCANNING:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Button_LearnScanning"));
      break;
    case DownloadCommands::LEARN_MORE_INTERRUPTED:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Button_LearnInterrupted"));
      break;
    case DownloadCommands::PAUSE:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Button_Pause"));
      break;
    case DownloadCommands::RESUME:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Button_Resume"));
      break;
  }
}

}  // anonymous namespace

DownloadItemNotification::DownloadItemNotification(
    content::DownloadItem* item,
    DownloadNotificationManagerForProfile* manager)
    : item_(item),
      weak_factory_(this) {
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();

  message_center::RichNotificationData data;
  // Creates the notification instance. |title| and |body| will be overridden
  // by UpdateNotificationData() below.
  notification_.reset(new Notification(
      message_center::NOTIFICATION_TYPE_PROGRESS,
      GURL(kDownloadNotificationOrigin),  // origin_url
      base::string16(),                   // title
      base::string16(),                   // body
      bundle.GetImageNamed(IDR_DOWNLOAD_NOTIFICATION_DOWNLOADING),
      message_center::NotifierId(message_center::NotifierId::SYSTEM_COMPONENT,
                                 kDownloadNotificationNotifierId),
      base::string16(),                    // display_source
      base::UintToString(item_->GetId()),  // tag
      data, watcher()));

  notification_->set_progress(0);
  notification_->set_never_timeout(false);
}

DownloadItemNotification::~DownloadItemNotification() {
  if (image_decode_status_ == IN_PROGRESS)
    ImageDecoder::Cancel(this);
}

void DownloadItemNotification::OnNotificationClose() {
  visible_ = false;

  if (image_decode_status_ == IN_PROGRESS) {
    image_decode_status_ = NOT_STARTED;
    ImageDecoder::Cancel(this);
  }
}

void DownloadItemNotification::OnNotificationClick() {
  if (item_->IsDangerous()) {
    content::RecordAction(
        UserMetricsAction("DownloadNotification.Click_Dangerous"));
    // Do nothing.
    return;
  }

  switch (item_->GetState()) {
    case content::DownloadItem::IN_PROGRESS:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Click_InProgress"));
      item_->SetOpenWhenComplete(!item_->GetOpenWhenComplete());  // Toggle
      break;
    case content::DownloadItem::CANCELLED:
    case content::DownloadItem::INTERRUPTED:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Click_Stopped"));
      GetBrowser()->OpenURL(content::OpenURLParams(
          GURL(chrome::kChromeUIDownloadsURL), content::Referrer(),
          NEW_FOREGROUND_TAB, ui::PAGE_TRANSITION_LINK,
          false /* is_renderer_initiated */));
      CloseNotificationByUser();
      break;
    case content::DownloadItem::COMPLETE:
      content::RecordAction(
          UserMetricsAction("DownloadNotification.Click_Completed"));
      item_->OpenDownload();
      CloseNotificationByUser();
      break;
    case content::DownloadItem::MAX_DOWNLOAD_STATE:
      NOTREACHED();
  }
}

void DownloadItemNotification::OnNotificationButtonClick(int button_index) {
  if (button_index < 0 ||
      static_cast<size_t>(button_index) >= button_actions_->size()) {
    // Out of boundary.
    NOTREACHED();
    return;
  }

  DownloadCommands::Command command = button_actions_->at(button_index);
  RecordButtonClickAction(command);

  if (command != DownloadCommands::PAUSE &&
      command != DownloadCommands::RESUME) {
    CloseNotificationByUser();
  }

  DownloadCommands(item_).ExecuteCommand(command);

  // Shows the notification again after clicking "Keep" on dangerous download.
  if (command == DownloadCommands::KEEP) {
    show_next_ = true;
    Update();
  }
}

// DownloadItem::Observer methods
void DownloadItemNotification::OnDownloadUpdated(content::DownloadItem* item) {
  DCHECK_EQ(item, item_);

  Update();
}

std::string DownloadItemNotification::GetNotificationId() const {
  return base::UintToString(item_->GetId());
}

void DownloadItemNotification::CloseNotificationByNonUser() {
  const std::string& notification_id = watcher()->id();
  const ProfileID profile_id = NotificationUIManager::GetProfileID(profile());

  g_browser_process->notification_ui_manager()->
      CancelById(notification_id, profile_id);
}

void DownloadItemNotification::CloseNotificationByUser() {
  const std::string& notification_id = watcher()->id();
  const ProfileID profile_id = NotificationUIManager::GetProfileID(profile());
  const std::string notification_id_in_message_center =
      ProfileNotification::GetProfileNotificationId(notification_id,
                                                    profile_id);

  g_browser_process->notification_ui_manager()->
      CancelById(notification_id, profile_id);

  // When the message center is visible, |NotificationUIManager::CancelByID()|
  // delays the close hence the notification is not closed at this time. But
  // from the viewpoint of UX of MessageCenter, we should close it immediately
  // because it's by user action. So, we request closing of it directlly to
  // MessageCenter instance.
  // Note that: this calling has no side-effect even when the message center
  // is not opened.
  g_browser_process->message_center()->RemoveNotification(
      notification_id_in_message_center, true /* by_user */);
}

void DownloadItemNotification::Update() {
  auto download_state = item_->GetState();

  // When the download is just completed, interrupted or transitions to
  // dangerous, close the notification once and re-show it immediately so
  // it'll pop up.
  bool popup =
      ((item_->IsDangerous() && !previous_dangerous_state_) ||
       (download_state == content::DownloadItem::COMPLETE &&
        previous_download_state_ != content::DownloadItem::COMPLETE) ||
       (download_state == content::DownloadItem::INTERRUPTED &&
        previous_download_state_ != content::DownloadItem::INTERRUPTED));

  if (visible_) {
    UpdateNotificationData(popup ? UPDATE_AND_POPUP : UPDATE);
  } else {
    if (show_next_ || popup) {
      UpdateNotificationData(ADD);
      visible_ = true;
    }
  }

  show_next_ = false;
  previous_download_state_ = item_->GetState();
  previous_dangerous_state_ = item_->IsDangerous();
}

void DownloadItemNotification::UpdateNotificationData(
    NotificationUpdateType type) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DownloadItemModel model(item_);
  DownloadCommands command(item_);

  if (type == UPDATE_AND_POPUP && IsNotificationVisible()) {
    // If the notification is already visible as popup or in the notification
    // center, doesn't pop it up.
    type = UPDATE;
  }

  notification_->set_title(GetTitle());
  notification_->set_message(GetStatusString());

  if (item_->IsDangerous()) {
    notification_->set_type(message_center::NOTIFICATION_TYPE_BASE_FORMAT);

    // Show icon.
    if (model.MightBeMalicious()) {
      SetNotificationIcon(IDR_DOWNLOAD_NOTIFICATION_WARNING_BAD);
      notification_->set_priority(message_center::DEFAULT_PRIORITY);
    } else {
      SetNotificationIcon(IDR_DOWNLOAD_NOTIFICATION_WARNING_UNWANTED);
      notification_->set_priority(message_center::HIGH_PRIORITY);
    }
  } else {
    notification_->set_priority(message_center::DEFAULT_PRIORITY);

    bool is_off_the_record = item_->GetBrowserContext() &&
                             item_->GetBrowserContext()->IsOffTheRecord();

    switch (item_->GetState()) {
      case content::DownloadItem::IN_PROGRESS: {
        int percent_complete = item_->PercentComplete();
        if (percent_complete >= 0) {
          notification_->set_type(message_center::NOTIFICATION_TYPE_PROGRESS);
          notification_->set_progress(percent_complete);
        } else {
          notification_->set_type(
              message_center::NOTIFICATION_TYPE_BASE_FORMAT);
          notification_->set_progress(0);
        }
        if (is_off_the_record) {
          SetNotificationIcon(IDR_DOWNLOAD_NOTIFICATION_INCOGNITO);
        } else {
          SetNotificationIcon(IDR_DOWNLOAD_NOTIFICATION_DOWNLOADING);
        }
        break;
      }
      case content::DownloadItem::COMPLETE:
        DCHECK(item_->IsDone());

        // Shows a notifiation as progress type once so the visible content will
        // be updated.
        // Note: only progress-type notification's content will be updated
        // immediately when the message center is visible.
        if (type == UPDATE_AND_POPUP) {
          notification_->set_type(message_center::NOTIFICATION_TYPE_PROGRESS);
        } else {
          notification_->set_type(
              message_center::NOTIFICATION_TYPE_BASE_FORMAT);
        }

        notification_->set_progress(100);

        if (is_off_the_record) {
          SetNotificationIcon(IDR_DOWNLOAD_NOTIFICATION_INCOGNITO);
        } else {
          SetNotificationIcon(IDR_DOWNLOAD_NOTIFICATION_DOWNLOADING);
        }
        break;
      case content::DownloadItem::CANCELLED:
        // Confgirms that a download is cancelled by user action.
        DCHECK(item_->GetLastReason() ==
                   content::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED ||
               item_->GetLastReason() ==
                   content::DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN);

        CloseNotificationByUser();
        return;  // Skips the remaining since the notification has closed.
      case content::DownloadItem::INTERRUPTED:
        // Shows a notifiation as progress type once so the visible content will
        // be updated. (same as the case of type = COMPLETE)
        if (type == UPDATE_AND_POPUP) {
          notification_->set_type(message_center::NOTIFICATION_TYPE_PROGRESS);
        } else {
          notification_->set_type(
              message_center::NOTIFICATION_TYPE_BASE_FORMAT);
        }

        notification_->set_progress(0);
        SetNotificationIcon(IDR_DOWNLOAD_NOTIFICATION_ERROR);
        break;
      case content::DownloadItem::MAX_DOWNLOAD_STATE:  // sentinel
        NOTREACHED();
    }
  }

  std::vector<message_center::ButtonInfo> notification_actions;
  scoped_ptr<std::vector<DownloadCommands::Command>> actions(
      GetExtraActions().Pass());

  button_actions_.reset(new std::vector<DownloadCommands::Command>);
  for (auto it = actions->begin(); it != actions->end(); it++) {
    button_actions_->push_back(*it);
    message_center::ButtonInfo button_info =
        message_center::ButtonInfo(GetCommandLabel(*it));
    button_info.icon = command.GetCommandIcon(*it);
    notification_actions.push_back(button_info);
  }
  notification_->set_buttons(notification_actions);

  if (type == ADD) {
    g_browser_process->notification_ui_manager()->
        Add(*notification_, profile());
  } else if (type == UPDATE || type == UPDATE_AND_POPUP) {
    g_browser_process->notification_ui_manager()->
        Update(*notification_, profile());

    if (type == UPDATE_AND_POPUP) {
      CloseNotificationByNonUser();
      // Changes the type from PROGRESS to BASE_FORMAT.
      notification_->set_type(message_center::NOTIFICATION_TYPE_BASE_FORMAT);
      g_browser_process->notification_ui_manager()->
          Add(*notification_, profile());
    }
  } else {
    NOTREACHED();
  }

  if (item_->IsDone() && image_decode_status_ == NOT_STARTED) {
    // TODO(yoshiki): Add an UMA to collect statistics of image file sizes.

    if (item_->GetReceivedBytes() > kMaxImagePreviewSize)
      return;

    DCHECK(notification_->image().IsEmpty());

    image_decode_status_ = IN_PROGRESS;

    bool maybe_image = false;
    if (mime_util::IsSupportedImageMimeType(item_->GetMimeType())) {
      maybe_image = true;
    } else {
      std::string mime;
      base::FilePath::StringType extension_with_dot =
          item_->GetTargetFilePath().FinalExtension();
      if (!extension_with_dot.empty() &&
          net::GetWellKnownMimeTypeFromExtension(extension_with_dot.substr(1),
                                                 &mime) &&
          mime_util::IsSupportedImageMimeType(mime)) {
        maybe_image = true;
      }
    }

    if (maybe_image) {
      base::FilePath file_path = item_->GetFullPath();
      base::PostTaskAndReplyWithResult(
          content::BrowserThread::GetBlockingPool(), FROM_HERE,
          base::Bind(&ReadNotificationImage, file_path),
          base::Bind(&DownloadItemNotification::OnImageLoaded,
                     weak_factory_.GetWeakPtr()));
    }
  }
}

void DownloadItemNotification::OnDownloadRemoved(content::DownloadItem* item) {
  // The given |item| may be already free'd.
  DCHECK_EQ(item, item_);

  // Removing the notification causes calling |NotificationDelegate::Close()|.
  if (g_browser_process->notification_ui_manager()) {
    g_browser_process->notification_ui_manager()->CancelById(
        watcher()->id(), NotificationUIManager::GetProfileID(profile()));
  }

  item_ = nullptr;
}

void DownloadItemNotification::SetNotificationIcon(int resource_id) {
  if (image_resource_id_ == resource_id)
    return;
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
  image_resource_id_ = resource_id;
  notification_->set_icon(bundle.GetImageNamed(image_resource_id_));
}

void DownloadItemNotification::OnImageLoaded(const std::string& image_data) {
  if (image_data.empty())
    return;

  // TODO(yoshiki): Set option to reduce the image size to supress memory usage.
  ImageDecoder::Start(this, image_data);
}

void DownloadItemNotification::OnImageDecoded(const SkBitmap& decoded_image) {
  gfx::Image image = gfx::Image::CreateFrom1xBitmap(decoded_image);
  notification_->set_image(image);
  image_decode_status_ = DONE;
  UpdateNotificationData(UPDATE);
}

void DownloadItemNotification::OnDecodeImageFailed() {
  DCHECK(notification_->image().IsEmpty());

  image_decode_status_ = FAILED;
  UpdateNotificationData(UPDATE);
}

scoped_ptr<std::vector<DownloadCommands::Command>>
DownloadItemNotification::GetExtraActions() const {
  scoped_ptr<std::vector<DownloadCommands::Command>> actions(
      new std::vector<DownloadCommands::Command>());

  if (item_->IsDangerous()) {
    DownloadItemModel model(item_);
    if (model.MightBeMalicious()) {
      actions->push_back(DownloadCommands::LEARN_MORE_SCANNING);
    } else {
      actions->push_back(DownloadCommands::DISCARD);
      actions->push_back(DownloadCommands::KEEP);
    }
    return actions.Pass();
  }

  switch (item_->GetState()) {
    case content::DownloadItem::IN_PROGRESS:
      if (!item_->IsPaused())
        actions->push_back(DownloadCommands::PAUSE);
      else
        actions->push_back(DownloadCommands::RESUME);
      actions->push_back(DownloadCommands::CANCEL);
      break;
    case content::DownloadItem::CANCELLED:
    case content::DownloadItem::INTERRUPTED:
      if (item_->CanResume())
        actions->push_back(DownloadCommands::RESUME);
      break;
    case content::DownloadItem::COMPLETE:
      actions->push_back(DownloadCommands::SHOW_IN_FOLDER);
      break;
    case content::DownloadItem::MAX_DOWNLOAD_STATE:
      NOTREACHED();
  }
  return actions.Pass();
}

base::string16 DownloadItemNotification::GetTitle() const {
  base::string16 title_text;
  DownloadItemModel model(item_);

  if (item_->IsDangerous()) {
    if (model.MightBeMalicious()) {
      return l10n_util::GetStringUTF16(
          IDS_PROMPT_BLOCKED_MALICIOUS_DOWNLOAD_TITLE);
    } else {
      return l10n_util::GetStringUTF16(
          IDS_CONFIRM_KEEP_DANGEROUS_DOWNLOAD_TITLE);
    }
  }

  base::string16 file_name =
      item_->GetFileNameToReportUser().LossyDisplayName();
  switch (item_->GetState()) {
    case content::DownloadItem::IN_PROGRESS:
      title_text = l10n_util::GetStringFUTF16(
          IDS_DOWNLOAD_STATUS_IN_PROGRESS_TITLE, file_name);
      break;
    case content::DownloadItem::COMPLETE:
      title_text = l10n_util::GetStringFUTF16(
          IDS_DOWNLOAD_STATUS_DOWNLOADED_TITLE, file_name);
      break;
    case content::DownloadItem::INTERRUPTED:
      title_text = l10n_util::GetStringFUTF16(
          IDS_DOWNLOAD_STATUS_DOWNLOAD_FAILED_TITLE, file_name);
      break;
    case content::DownloadItem::CANCELLED:
      title_text = l10n_util::GetStringFUTF16(
          IDS_DOWNLOAD_STATUS_DOWNLOAD_FAILED_TITLE, file_name);
      break;
    case content::DownloadItem::MAX_DOWNLOAD_STATE:
      NOTREACHED();
  }
  return title_text;
}

base::string16 DownloadItemNotification::GetCommandLabel(
    DownloadCommands::Command command) const {
  int id = -1;
  switch (command) {
    case DownloadCommands::OPEN_WHEN_COMPLETE:
      if (item_ && !item_->IsDone())
        id = IDS_DOWNLOAD_STATUS_OPEN_WHEN_COMPLETE;
      else
        id = IDS_DOWNLOAD_STATUS_OPEN_WHEN_COMPLETE;
      break;
    case DownloadCommands::PAUSE:
      // Only for non menu.
      id = IDS_DOWNLOAD_LINK_PAUSE;
      break;
    case DownloadCommands::RESUME:
      // Only for non menu.
      id = IDS_DOWNLOAD_LINK_RESUME;
      break;
    case DownloadCommands::SHOW_IN_FOLDER:
      id = IDS_DOWNLOAD_LINK_SHOW;
      break;
    case DownloadCommands::DISCARD:
      id = IDS_DISCARD_DOWNLOAD;
      break;
    case DownloadCommands::KEEP:
      id = IDS_CONFIRM_DOWNLOAD;
      break;
    case DownloadCommands::CANCEL:
      id = IDS_DOWNLOAD_LINK_CANCEL;
      break;
    case DownloadCommands::LEARN_MORE_SCANNING:
      id = IDS_DOWNLOAD_LINK_LEARN_MORE_SCANNING;
      break;
    case DownloadCommands::ALWAYS_OPEN_TYPE:
    case DownloadCommands::PLATFORM_OPEN:
    case DownloadCommands::LEARN_MORE_INTERRUPTED:
      // Only for menu.
      NOTREACHED();
      return base::string16();
  }
  CHECK(id != -1);
  return l10n_util::GetStringUTF16(id);
}

base::string16 DownloadItemNotification::GetWarningStatusString() const {
  // Should only be called if IsDangerous().
  DCHECK(item_->IsDangerous());
  base::string16 elided_filename =
      item_->GetFileNameToReportUser().LossyDisplayName();
  switch (item_->GetDangerType()) {
    case content::DOWNLOAD_DANGER_TYPE_DANGEROUS_URL: {
      return l10n_util::GetStringUTF16(IDS_PROMPT_MALICIOUS_DOWNLOAD_URL);
    }
    case content::DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE: {
      if (download_crx_util::IsExtensionDownload(*item_)) {
        return l10n_util::GetStringUTF16(
            IDS_PROMPT_DANGEROUS_DOWNLOAD_EXTENSION);
      } else {
        return l10n_util::GetStringFUTF16(IDS_PROMPT_DANGEROUS_DOWNLOAD,
                                          elided_filename);
      }
    }
    case content::DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT:
    case content::DOWNLOAD_DANGER_TYPE_DANGEROUS_HOST: {
      return l10n_util::GetStringFUTF16(IDS_PROMPT_MALICIOUS_DOWNLOAD_CONTENT,
                                        elided_filename);
    }
    case content::DOWNLOAD_DANGER_TYPE_UNCOMMON_CONTENT: {
      return l10n_util::GetStringFUTF16(IDS_PROMPT_UNCOMMON_DOWNLOAD_CONTENT,
                                        elided_filename);
    }
    case content::DOWNLOAD_DANGER_TYPE_POTENTIALLY_UNWANTED: {
      return l10n_util::GetStringFUTF16(IDS_PROMPT_DOWNLOAD_CHANGES_SETTINGS,
                                        elided_filename);
    }
    case content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS:
    case content::DOWNLOAD_DANGER_TYPE_MAYBE_DANGEROUS_CONTENT:
    case content::DOWNLOAD_DANGER_TYPE_USER_VALIDATED:
    case content::DOWNLOAD_DANGER_TYPE_MAX: {
      break;
    }
  }
  NOTREACHED();
  return base::string16();
}

base::string16 DownloadItemNotification::GetInProgressSubStatusString() const {
  // The download is a CRX (app, extension, theme, ...) and it is being
  // unpacked and validated.
  if (item_->AllDataSaved() &&
      download_crx_util::IsExtensionDownload(*item_)) {
    return l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_CRX_INSTALL_RUNNING);
  }

  // "Paused"
  if (item_->IsPaused())
    return l10n_util::GetStringUTF16(IDS_DOWNLOAD_PROGRESS_PAUSED);

  base::TimeDelta time_remaining;
  // time_remaining is only known if the download isn't paused.
  bool time_remaining_known = (!item_->IsPaused() &&
                               item_->TimeRemaining(&time_remaining));


  // A download scheduled to be opened when complete.
  if (item_->GetOpenWhenComplete()) {
    // "Opening when complete"
    if (!time_remaining_known)
      return l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_OPEN_WHEN_COMPLETE);

    // "Opening in 10 secs"
    return l10n_util::GetStringFUTF16(
        IDS_DOWNLOAD_STATUS_OPEN_IN,
        ui::TimeFormat::Simple(ui::TimeFormat::FORMAT_DURATION,
                               ui::TimeFormat::LENGTH_SHORT, time_remaining));
  }

  // In progress download with known time left: "10 secs left"
  if (time_remaining_known) {
    return ui::TimeFormat::Simple(ui::TimeFormat::FORMAT_REMAINING,
                               ui::TimeFormat::LENGTH_SHORT, time_remaining);
  }

  // "In progress"
  if (item_->GetReceivedBytes() > 0)
    return l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_IN_PROGRESS_SHORT);

  // "Starting..."
  return l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_STARTING);
}

base::string16 DownloadItemNotification::GetStatusString() const {
  if (item_->IsDangerous())
    return GetWarningStatusString();

  DownloadItemModel model(item_);
  base::string16 sub_status_text;
  switch (item_->GetState()) {
    case content::DownloadItem::IN_PROGRESS:
      sub_status_text = GetInProgressSubStatusString();
      break;
    case content::DownloadItem::COMPLETE:
      // If the file has been removed: Removed
      if (item_->GetFileExternallyRemoved()) {
        sub_status_text =
            l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_REMOVED);
      }
      // Otherwise, no sub status text.
      break;
    case content::DownloadItem::CANCELLED:
      // "Cancelled"
      sub_status_text =
          l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_CANCELLED);
      break;
    case content::DownloadItem::INTERRUPTED: {
      content::DownloadInterruptReason reason = item_->GetLastReason();
      if (reason != content::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED) {
        // "Failed - <REASON>"
        base::string16 interrupt_reason = model.GetInterruptReasonText();
        DCHECK(!interrupt_reason.empty());
        sub_status_text = l10n_util::GetStringFUTF16(
            IDS_DOWNLOAD_STATUS_INTERRUPTED, interrupt_reason);
      } else {
        // Same as DownloadItem::CANCELLED.
        sub_status_text =
            l10n_util::GetStringUTF16(IDS_DOWNLOAD_STATUS_CANCELLED);
      }
      break;
    }
    default:
      NOTREACHED();
  }

  // The hostname. (E.g.:"example.com" or "127.0.0.1")
  base::string16 host_name = base::UTF8ToUTF16(item_->GetURL().host());

  // Indication of progress. (E.g.:"100/200 MB" or "100 MB")
  base::string16 size_ratio = model.GetProgressSizesString();

  if (sub_status_text.empty()) {
    // Download completed: "3.4/3.4 MB from example.com"
    DCHECK_EQ(item_->GetState(), content::DownloadItem::COMPLETE);
    return l10n_util::GetStringFUTF16(
        IDS_DOWNLOAD_NOTIFICATION_STATUS_COMPLETED,
        size_ratio, host_name);
  }

  // Download is not completed: "3.4/3.4 MB from example.com, <SUB STATUS>"
  return l10n_util::GetStringFUTF16(
      IDS_DOWNLOAD_NOTIFICATION_STATUS,
      size_ratio, host_name, sub_status_text);
}

Browser* DownloadItemNotification::GetBrowser() const {
  chrome::ScopedTabbedBrowserDisplayer browser_displayer(
      profile(), chrome::GetActiveDesktop());
  DCHECK(browser_displayer.browser());
  return browser_displayer.browser();
}

Profile* DownloadItemNotification::profile() const {
  return Profile::FromBrowserContext(item_->GetBrowserContext());
}

bool DownloadItemNotification::IsNotificationVisible() const {
  const std::string& notification_id = watcher()->id();
  const ProfileID profile_id = NotificationUIManager::GetProfileID(profile());
  const std::string notification_id_in_message_center =
      ProfileNotification::GetProfileNotificationId(notification_id,
                                                    profile_id);

  message_center::NotificationList::Notifications visible_notifications =
      g_browser_process->message_center()->GetVisibleNotifications();
  for (const auto& notification : visible_notifications) {
    if (notification->id() == notification_id_in_message_center)
      return true;
  }
  return false;
}

