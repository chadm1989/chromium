// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/web_app_win.h"

#include <shlobj.h>

#include "base/command_line.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/md5.h"
#include "base/memory/scoped_ptr.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/shortcut.h"
#include "base/win/windows_version.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/web_applications/update_shortcut_worker_win.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "chrome/installer/util/util_constants.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/extension.h"
#include "net/base/mime_util.h"
#include "ui/base/win/shell.h"
#include "ui/gfx/icon_util.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_family.h"

namespace {

const base::FilePath::CharType kIconChecksumFileExt[] =
    FILE_PATH_LITERAL(".ico.md5");

const base::FilePath::CharType kAppShimExe[] =
    FILE_PATH_LITERAL("app_shim.exe");

// Calculates checksum of an icon family using MD5.
// The checksum is derived from all of the icons in the family.
void GetImageCheckSum(const gfx::ImageFamily& image, base::MD5Digest* digest) {
  DCHECK(digest);
  base::MD5Context md5_context;
  base::MD5Init(&md5_context);

  for (gfx::ImageFamily::const_iterator it = image.begin(); it != image.end();
       ++it) {
    SkBitmap bitmap = it->AsBitmap();

    SkAutoLockPixels image_lock(bitmap);
    base::StringPiece image_data(
        reinterpret_cast<const char*>(bitmap.getPixels()), bitmap.getSize());
    base::MD5Update(&md5_context, image_data);
  }

  base::MD5Final(digest, &md5_context);
}

// Saves |image| as an |icon_file| with the checksum.
bool SaveIconWithCheckSum(const base::FilePath& icon_file,
                          const gfx::ImageFamily& image) {
  if (!IconUtil::CreateIconFileFromImageFamily(image, icon_file))
    return false;

  base::MD5Digest digest;
  GetImageCheckSum(image, &digest);

  base::FilePath cheksum_file(icon_file.ReplaceExtension(kIconChecksumFileExt));
  return base::WriteFile(cheksum_file,
                         reinterpret_cast<const char*>(&digest),
                         sizeof(digest)) == sizeof(digest);
}

// Returns true if |icon_file| is missing or different from |image|.
bool ShouldUpdateIcon(const base::FilePath& icon_file,
                      const gfx::ImageFamily& image) {
  base::FilePath checksum_file(
      icon_file.ReplaceExtension(kIconChecksumFileExt));

  // Returns true if icon_file or checksum file is missing.
  if (!base::PathExists(icon_file) ||
      !base::PathExists(checksum_file))
    return true;

  base::MD5Digest persisted_image_checksum;
  if (sizeof(persisted_image_checksum) != base::ReadFile(checksum_file,
                      reinterpret_cast<char*>(&persisted_image_checksum),
                      sizeof(persisted_image_checksum)))
    return true;

  base::MD5Digest downloaded_image_checksum;
  GetImageCheckSum(image, &downloaded_image_checksum);

  // Update icon if checksums are not equal.
  return memcmp(&persisted_image_checksum, &downloaded_image_checksum,
                sizeof(base::MD5Digest)) != 0;
}

// Returns true if |shortcut_file_name| matches profile |profile_path|, and has
// an --app-id flag.
bool IsAppShortcutForProfile(const base::FilePath& shortcut_file_name,
                             const base::FilePath& profile_path) {
  base::string16 cmd_line_string;
  if (base::win::ResolveShortcut(shortcut_file_name, NULL, &cmd_line_string)) {
    cmd_line_string = L"program " + cmd_line_string;
    base::CommandLine shortcut_cmd_line =
        base::CommandLine::FromString(cmd_line_string);
    return shortcut_cmd_line.HasSwitch(switches::kProfileDirectory) &&
           shortcut_cmd_line.GetSwitchValuePath(switches::kProfileDirectory) ==
               profile_path.BaseName() &&
           shortcut_cmd_line.HasSwitch(switches::kAppId);
  }

  return false;
}

// Finds shortcuts in |shortcut_path| that match profile for |profile_path| and
// extension with title |shortcut_name|.
// If |shortcut_name| is empty, finds all shortcuts matching |profile_path|.
std::vector<base::FilePath> FindAppShortcutsByProfileAndTitle(
    const base::FilePath& shortcut_path,
    const base::FilePath& profile_path,
    const base::string16& shortcut_name) {
  std::vector<base::FilePath> shortcut_paths;

  if (shortcut_name.empty()) {
    // Find all shortcuts for this profile.
    base::FileEnumerator files(shortcut_path, false,
                               base::FileEnumerator::FILES,
                               FILE_PATH_LITERAL("*.lnk"));
    base::FilePath shortcut_file = files.Next();
    while (!shortcut_file.empty()) {
      if (IsAppShortcutForProfile(shortcut_file, profile_path))
        shortcut_paths.push_back(shortcut_file);
      shortcut_file = files.Next();
    }
  } else {
    // Find all shortcuts matching |shortcut_name|.
    base::FilePath base_path = shortcut_path.
        Append(web_app::internals::GetSanitizedFileName(shortcut_name)).
        AddExtension(FILE_PATH_LITERAL(".lnk"));

    const int fileNamesToCheck = 10;
    for (int i = 0; i < fileNamesToCheck; ++i) {
      base::FilePath shortcut_file = base_path;
      if (i > 0) {
        shortcut_file = shortcut_file.InsertBeforeExtensionASCII(
            base::StringPrintf(" (%d)", i));
      }
      if (base::PathExists(shortcut_file) &&
          IsAppShortcutForProfile(shortcut_file, profile_path)) {
        shortcut_paths.push_back(shortcut_file);
      }
    }
  }

  return shortcut_paths;
}

// Creates application shortcuts in a given set of paths.
// |shortcut_paths| is a list of directories in which shortcuts should be
// created. If |creation_reason| is SHORTCUT_CREATION_AUTOMATED and there is an
// existing shortcut to this app for this profile, does nothing (succeeding).
// Returns true on success, false on failure.
// Must be called on the FILE thread.
bool CreateShortcutsInPaths(
    const base::FilePath& web_app_path,
    const web_app::ShortcutInfo& shortcut_info,
    const std::vector<base::FilePath>& shortcut_paths,
    web_app::ShortcutCreationReason creation_reason,
    std::vector<base::FilePath>* out_filenames) {
  // Ensure web_app_path exists.
  if (!base::PathExists(web_app_path) &&
      !base::CreateDirectory(web_app_path)) {
    return false;
  }

  // Generates file name to use with persisted ico and shortcut file.
  base::FilePath icon_file =
      web_app::internals::GetIconFilePath(web_app_path, shortcut_info.title);
  if (!web_app::internals::CheckAndSaveIcon(icon_file, shortcut_info.favicon,
                                            false)) {
    return false;
  }

  base::FilePath chrome_exe;
  if (!PathService::Get(base::FILE_EXE, &chrome_exe)) {
    NOTREACHED();
    return false;
  }

  // Working directory.
  base::FilePath working_dir(chrome_exe.DirName());

  base::CommandLine cmd_line(base::CommandLine::NO_PROGRAM);
  cmd_line = ShellIntegration::CommandLineArgsForLauncher(shortcut_info.url,
      shortcut_info.extension_id, shortcut_info.profile_path);

  // TODO(evan): we rely on the fact that command_line_string() is
  // properly quoted for a Windows command line.  The method on
  // base::CommandLine should probably be renamed to better reflect that
  // fact.
  base::string16 wide_switches(cmd_line.GetCommandLineString());

  // Sanitize description
  base::string16 description = shortcut_info.description;
  if (description.length() >= MAX_PATH)
    description.resize(MAX_PATH - 1);

  // Generates app id from web app url and profile path.
  std::string app_name(web_app::GenerateApplicationNameFromInfo(shortcut_info));
  base::string16 app_id(ShellIntegration::GetAppModelIdForProfile(
      base::UTF8ToUTF16(app_name), shortcut_info.profile_path));

  bool success = true;
  for (size_t i = 0; i < shortcut_paths.size(); ++i) {
    base::FilePath shortcut_file =
        shortcut_paths[i]
            .Append(
                 web_app::internals::GetSanitizedFileName(shortcut_info.title))
            .AddExtension(installer::kLnkExt);
    if (creation_reason == web_app::SHORTCUT_CREATION_AUTOMATED) {
      // Check whether there is an existing shortcut to this app.
      std::vector<base::FilePath> shortcut_files =
          FindAppShortcutsByProfileAndTitle(shortcut_paths[i],
                                            shortcut_info.profile_path,
                                            shortcut_info.title);
      if (!shortcut_files.empty())
        continue;
    }
    if (shortcut_paths[i] != web_app_path) {
      int unique_number =
          base::GetUniquePathNumber(shortcut_file,
                                    base::FilePath::StringType());
      if (unique_number == -1) {
        success = false;
        continue;
      } else if (unique_number > 0) {
        shortcut_file = shortcut_file.InsertBeforeExtensionASCII(
            base::StringPrintf(" (%d)", unique_number));
      }
    }
    base::win::ShortcutProperties shortcut_properties;
    shortcut_properties.set_target(chrome_exe);
    shortcut_properties.set_working_dir(working_dir);
    shortcut_properties.set_arguments(wide_switches);
    shortcut_properties.set_description(description);
    shortcut_properties.set_icon(icon_file, 0);
    shortcut_properties.set_app_id(app_id);
    shortcut_properties.set_dual_mode(false);
    if (!base::PathExists(shortcut_file.DirName()) &&
        !base::CreateDirectory(shortcut_file.DirName())) {
      NOTREACHED();
      return false;
    }
    success = base::win::CreateOrUpdateShortcutLink(
        shortcut_file, shortcut_properties,
        base::win::SHORTCUT_CREATE_ALWAYS) && success;
    if (out_filenames)
      out_filenames->push_back(shortcut_file);
  }

  return success;
}

// Gets the directories with shortcuts for an app, and deletes the shortcuts.
// This will search the standard locations for shortcuts named |title| that open
// in the profile with |profile_path|.
// |was_pinned_to_taskbar| will be set to true if there was previously a
// shortcut pinned to the taskbar for this app; false otherwise.
// If |web_app_path| is empty, this will not delete shortcuts from the web app
// directory. If |title| is empty, all shortcuts for this profile will be
// deleted.
// |shortcut_paths| will be populated with a list of directories where shortcuts
// for this app were found (and deleted). This will delete duplicate shortcuts,
// but only return each path once, even if it contained multiple deleted
// shortcuts. Both of these may be NULL.
void GetShortcutLocationsAndDeleteShortcuts(
    const base::FilePath& web_app_path,
    const base::FilePath& profile_path,
    const base::string16& title,
    bool* was_pinned_to_taskbar,
    std::vector<base::FilePath>* shortcut_paths) {
  DCHECK(content::BrowserThread::FILE);

  // Get all possible locations for shortcuts.
  web_app::ShortcutLocations all_shortcut_locations;
  all_shortcut_locations.in_quick_launch_bar = true;
  all_shortcut_locations.on_desktop = true;
  // Delete shortcuts from the Chrome Apps subdirectory.
  // This matches the subdir name set by CreateApplicationShortcutView::Accept
  // for Chrome apps (not URL apps, but this function does not apply for them).
  all_shortcut_locations.applications_menu_location =
      web_app::APP_MENU_LOCATION_SUBDIR_CHROMEAPPS;
  std::vector<base::FilePath> all_paths = web_app::internals::GetShortcutPaths(
      all_shortcut_locations);
  if (base::win::GetVersion() >= base::win::VERSION_WIN7 &&
      !web_app_path.empty()) {
    all_paths.push_back(web_app_path);
  }

  if (was_pinned_to_taskbar) {
    // Determine if there is a link to this app in the TaskBar pin directory.
    base::FilePath taskbar_pin_path;
    if (PathService::Get(base::DIR_TASKBAR_PINS, &taskbar_pin_path)) {
      std::vector<base::FilePath> taskbar_pin_files =
          FindAppShortcutsByProfileAndTitle(taskbar_pin_path, profile_path,
                                            title);
      *was_pinned_to_taskbar = !taskbar_pin_files.empty();
    } else {
      *was_pinned_to_taskbar = false;
    }
  }

  for (std::vector<base::FilePath>::const_iterator i = all_paths.begin();
       i != all_paths.end(); ++i) {
    std::vector<base::FilePath> shortcut_files =
        FindAppShortcutsByProfileAndTitle(*i, profile_path, title);
    if (shortcut_paths && !shortcut_files.empty()) {
      shortcut_paths->push_back(*i);
    }
    for (std::vector<base::FilePath>::const_iterator j = shortcut_files.begin();
         j != shortcut_files.end(); ++j) {
      // Any shortcut could have been pinned, either by chrome or the user, so
      // they are all unpinned.
      base::win::UnpinShortcutFromTaskbar(*j);
      base::DeleteFile(*j, false);
    }
  }
}

void CreateIconAndSetRelaunchDetails(
    const base::FilePath& web_app_path,
    const base::FilePath& icon_file,
    scoped_ptr<web_app::ShortcutInfo> shortcut_info,
    HWND hwnd) {
  DCHECK(content::BrowserThread::GetBlockingPool()->RunsTasksOnCurrentThread());

  base::CommandLine command_line = ShellIntegration::CommandLineArgsForLauncher(
      shortcut_info->url, shortcut_info->extension_id,
      shortcut_info->profile_path);

  base::FilePath chrome_exe;
  if (!PathService::Get(base::FILE_EXE, &chrome_exe)) {
    NOTREACHED();
    return;
  }
  command_line.SetProgram(chrome_exe);
  ui::win::SetRelaunchDetailsForWindow(command_line.GetCommandLineString(),
                                       shortcut_info->title, hwnd);

  if (!base::PathExists(web_app_path) && !base::CreateDirectory(web_app_path))
    return;

  ui::win::SetAppIconForWindow(icon_file.value(), hwnd);
  web_app::internals::CheckAndSaveIcon(icon_file, shortcut_info->favicon, true);
}

void OnShortcutInfoLoadedForSetRelaunchDetails(
    HWND hwnd,
    scoped_ptr<web_app::ShortcutInfo> shortcut_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Set window's icon to the one we're about to create/update in the web app
  // path. The icon cache will refresh on icon creation.
  base::FilePath web_app_path = web_app::GetWebAppDataDirectory(
      shortcut_info->profile_path, shortcut_info->extension_id,
      shortcut_info->url);
  base::FilePath icon_file =
      web_app::internals::GetIconFilePath(web_app_path, shortcut_info->title);
  content::BrowserThread::PostBlockingPoolTask(
      FROM_HERE, base::Bind(&CreateIconAndSetRelaunchDetails, web_app_path,
                            icon_file, base::Passed(&shortcut_info), hwnd));
}

// Creates an "app shim exe" by linking or copying the generic app shim exe.
// This is the binary that will be run when the user opens a file with this
// application. The name and icon of the binary will be used on the Open With
// menu. For this reason, we cannot simply launch chrome.exe. We give the app
// shim exe the same name as the application (with no ".exe" extension), so that
// the correct title will appear on the Open With menu. (Note: we also need a
// separate binary per app because Windows only allows a single association with
// each executable.)
// |path| is the full path of the shim binary to be created.
bool CreateAppShimBinary(const base::FilePath& path) {
  // TODO(mgiuca): Hard-link instead of copying, if on the same file system.
  // Get the Chrome version directory (the directory containing the chrome.dll
  // module). This is the directory where app_shim.exe is located.
  base::FilePath chrome_version_directory;
  if (!PathService::Get(base::DIR_MODULE, &chrome_version_directory)) {
    NOTREACHED();
    return false;
  }

  base::FilePath generic_shim_path =
      chrome_version_directory.Append(kAppShimExe);
  if (!base::CopyFile(generic_shim_path, path)) {
    if (!base::PathExists(generic_shim_path)) {
      LOG(ERROR) << "Could not find app shim exe at "
                 << generic_shim_path.value();
    } else {
      LOG(ERROR) << "Could not copy app shim exe to " << path.value();
    }
    return false;
  }

  return true;
}

// Gets the full command line for calling the shim binary. This will include a
// placeholder "%1" argument, which Windows will substitute with the filename
// chosen by the user.
base::CommandLine GetAppShimCommandLine(const base::FilePath& app_shim_path,
                                        const std::string& extension_id,
                                        const base::FilePath& profile_path) {
  // Get the command-line to pass to the shim (e.g., "chrome.exe --app-id=...").
  base::CommandLine chrome_cmd_line =
      ShellIntegration::CommandLineArgsForLauncher(GURL(), extension_id,
                                                   profile_path);
  chrome_cmd_line.AppendArg("%1");

  // Get the command-line for calling the shim (e.g.,
  // "app_shim [--chrome-sxs] -- --app-id=...").
  base::CommandLine shim_cmd_line(app_shim_path);
  // If this is a canary build, launch the shim in canary mode.
  if (InstallUtil::IsChromeSxSProcess())
    shim_cmd_line.AppendSwitch(installer::switches::kChromeSxS);
  // Ensure all subsequent switches are treated as args to the shim.
  shim_cmd_line.AppendArg("--");
  for (size_t i = 1; i < chrome_cmd_line.argv().size(); ++i)
    shim_cmd_line.AppendArgNative(chrome_cmd_line.argv()[i]);

  return shim_cmd_line;
}

// Gets the set of file extensions associated with a particular file handler.
// Uses both the MIME types and extensions.
void GetHandlerFileExtensions(const extensions::FileHandlerInfo& handler,
                              std::set<base::string16>* exts) {
  for (const auto& mime : handler.types) {
    std::vector<base::string16> mime_type_extensions;
    net::GetExtensionsForMimeType(mime, &mime_type_extensions);
    exts->insert(mime_type_extensions.begin(), mime_type_extensions.end());
  }
  for (const auto& ext : handler.extensions)
    exts->insert(base::UTF8ToUTF16(ext));
}

// Creates operating system file type associations for a given app.
// This is the platform specific implementation of the CreateFileAssociations
// function, and is executed on the FILE thread.
// Returns true on success, false on failure.
bool CreateFileAssociationsForApp(
    const std::string& extension_id,
    const base::string16& title,
    const base::FilePath& profile_path,
    const extensions::FileHandlersInfo& file_handlers_info) {
  base::FilePath web_app_path =
      web_app::GetWebAppDataDirectory(profile_path, extension_id, GURL());
  base::FilePath file_name = web_app::internals::GetSanitizedFileName(title);

  // The progid is "chrome-APPID-HANDLERID". This is the internal name Windows
  // will use for file associations with this application.
  base::string16 progid_base = L"chrome-";
  progid_base += base::UTF8ToUTF16(extension_id);

  // Create the app shim binary (see CreateAppShimBinary for rationale). Get the
  // command line for the shim.
  base::FilePath app_shim_path = web_app_path.Append(file_name);
  if (!CreateAppShimBinary(app_shim_path))
    return false;

  base::CommandLine shim_cmd_line(
      GetAppShimCommandLine(app_shim_path, extension_id, profile_path));

  // TODO(mgiuca): Get the file type name from the manifest, or generate a
  // default one. (If this is blank, Windows will generate one of the form
  // '<EXT> file'.)
  base::string16 file_type_name = L"";

  // TODO(mgiuca): Generate a new icon for this application's file associations
  // that looks like a page with the application icon inside.
  base::FilePath icon_file =
      web_app::internals::GetIconFilePath(web_app_path, title);

  // Create a separate file association (ProgId) for each handler. This allows
  // each handler to have its own filetype name and icon, and also a different
  // command line (so the app can see which handler was invoked).
  size_t num_successes = 0;
  for (const auto& handler : file_handlers_info) {
    base::string16 progid = progid_base + L"-" + base::UTF8ToUTF16(handler.id);

    std::set<base::string16> exts;
    GetHandlerFileExtensions(handler, &exts);

    if (ShellUtil::AddFileAssociations(progid, shim_cmd_line, file_type_name,
                                       icon_file, exts)) {
      ++num_successes;
    }
  }

  if (num_successes == 0) {
    // There were no successes; delete the shim.
    base::DeleteFile(app_shim_path, false);
  } else {
    // There were some successes; tell Windows Explorer to update its cache.
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT,
                   nullptr, nullptr);
  }

  return num_successes == file_handlers_info.size();
}

}  // namespace

namespace web_app {

base::FilePath CreateShortcutInWebAppDir(
    const base::FilePath& web_app_dir,
    scoped_ptr<ShortcutInfo> shortcut_info) {
  std::vector<base::FilePath> paths;
  paths.push_back(web_app_dir);
  std::vector<base::FilePath> out_filenames;
  base::FilePath web_app_dir_shortcut =
      web_app_dir.Append(internals::GetSanitizedFileName(shortcut_info->title))
          .AddExtension(installer::kLnkExt);
  if (!PathExists(web_app_dir_shortcut)) {
    CreateShortcutsInPaths(web_app_dir, *shortcut_info, paths,
                           SHORTCUT_CREATION_BY_USER, &out_filenames);
    DCHECK_EQ(out_filenames.size(), 1u);
    DCHECK_EQ(out_filenames[0].value(), web_app_dir_shortcut.value());
  } else {
    internals::CheckAndSaveIcon(
        internals::GetIconFilePath(web_app_dir, shortcut_info->title),
        shortcut_info->favicon, true);
  }
  return web_app_dir_shortcut;
}

void UpdateRelaunchDetailsForApp(Profile* profile,
                                 const extensions::Extension* extension,
                                 HWND hwnd) {
  web_app::GetShortcutInfoForApp(
      extension,
      profile,
      base::Bind(&OnShortcutInfoLoadedForSetRelaunchDetails, hwnd));
}

void UpdateShortcutsForAllApps(Profile* profile,
                               const base::Closure& callback) {
  callback.Run();
}

namespace internals {

bool CheckAndSaveIcon(const base::FilePath& icon_file,
                      const gfx::ImageFamily& image,
                      bool refresh_shell_icon_cache) {
  if (!ShouldUpdateIcon(icon_file, image))
    return true;

  if (!SaveIconWithCheckSum(icon_file, image))
    return false;

  if (refresh_shell_icon_cache) {
    // Refresh shell's icon cache. This call is quite disruptive as user would
    // see explorer rebuilding the icon cache. It would be great that we find
    // a better way to achieve this.
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, NULL,
                   NULL);
  }
  return true;
}

bool CreatePlatformShortcuts(
    const base::FilePath& web_app_path,
    scoped_ptr<ShortcutInfo> shortcut_info,
    const extensions::FileHandlersInfo& file_handlers_info,
    const ShortcutLocations& creation_locations,
    ShortcutCreationReason creation_reason) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::FILE);

  // Nothing to do on Windows for hidden apps.
  if (creation_locations.applications_menu_location == APP_MENU_LOCATION_HIDDEN)
    return true;

  // Shortcut paths under which to create shortcuts.
  std::vector<base::FilePath> shortcut_paths =
      GetShortcutPaths(creation_locations);

  bool pin_to_taskbar = creation_locations.in_quick_launch_bar &&
                        (base::win::GetVersion() >= base::win::VERSION_WIN7);

  // Create/update the shortcut in the web app path for the "Pin To Taskbar"
  // option in Win7. We use the web app path shortcut because we will overwrite
  // it rather than appending unique numbers if the shortcut already exists.
  // This prevents pinned apps from having unique numbers in their names.
  if (pin_to_taskbar)
    shortcut_paths.push_back(web_app_path);

  if (shortcut_paths.empty())
    return false;

  if (!CreateShortcutsInPaths(web_app_path, *shortcut_info, shortcut_paths,
                              creation_reason, NULL))
    return false;

  if (pin_to_taskbar) {
    base::FilePath file_name = GetSanitizedFileName(shortcut_info->title);
    // Use the web app path shortcut for pinning to avoid having unique numbers
    // in the application name.
    base::FilePath shortcut_to_pin = web_app_path.Append(file_name).
        AddExtension(installer::kLnkExt);
    if (!base::win::PinShortcutToTaskbar(shortcut_to_pin))
      return false;
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableAppsFileAssociations)) {
    CreateFileAssociationsForApp(
        shortcut_info->extension_id, shortcut_info->title,
        shortcut_info->profile_path, file_handlers_info);
  }

  return true;
}

void UpdatePlatformShortcuts(
    const base::FilePath& web_app_path,
    const base::string16& old_app_title,
    scoped_ptr<ShortcutInfo> shortcut_info,
    const extensions::FileHandlersInfo& file_handlers_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::FILE);

  // Generates file name to use with persisted ico and shortcut file.
  base::FilePath file_name =
      web_app::internals::GetSanitizedFileName(shortcut_info->title);

  if (old_app_title != shortcut_info->title) {
    // The app's title has changed. Delete all existing app shortcuts and
    // recreate them in any locations they already existed (but do not add them
    // to locations where they do not currently exist).
    bool was_pinned_to_taskbar;
    std::vector<base::FilePath> shortcut_paths;
    GetShortcutLocationsAndDeleteShortcuts(
        web_app_path, shortcut_info->profile_path, old_app_title,
        &was_pinned_to_taskbar, &shortcut_paths);
    CreateShortcutsInPaths(web_app_path, *shortcut_info, shortcut_paths,
                           SHORTCUT_CREATION_BY_USER, NULL);
    // If the shortcut was pinned to the taskbar,
    // GetShortcutLocationsAndDeleteShortcuts will have deleted it. In that
    // case, re-pin it.
    if (was_pinned_to_taskbar) {
      base::FilePath file_name = GetSanitizedFileName(shortcut_info->title);
      // Use the web app path shortcut for pinning to avoid having unique
      // numbers in the application name.
      base::FilePath shortcut_to_pin = web_app_path.Append(file_name).
          AddExtension(installer::kLnkExt);
      base::win::PinShortcutToTaskbar(shortcut_to_pin);
    }
  }

  // Update the icon if necessary.
  base::FilePath icon_file =
      GetIconFilePath(web_app_path, shortcut_info->title);
  CheckAndSaveIcon(icon_file, shortcut_info->favicon, true);
}

void DeletePlatformShortcuts(const base::FilePath& web_app_path,
                             scoped_ptr<ShortcutInfo> shortcut_info) {
  GetShortcutLocationsAndDeleteShortcuts(web_app_path,
                                         shortcut_info->profile_path,
                                         shortcut_info->title, NULL, NULL);

  // If there are no more shortcuts in the Chrome Apps subdirectory, remove it.
  base::FilePath chrome_apps_dir;
  if (ShellUtil::GetShortcutPath(
          ShellUtil::SHORTCUT_LOCATION_START_MENU_CHROME_APPS_DIR,
          BrowserDistribution::GetDistribution(),
          ShellUtil::CURRENT_USER,
          &chrome_apps_dir)) {
    if (base::IsDirectoryEmpty(chrome_apps_dir))
      base::DeleteFile(chrome_apps_dir, false);
  }
}

void DeleteAllShortcutsForProfile(const base::FilePath& profile_path) {
  GetShortcutLocationsAndDeleteShortcuts(base::FilePath(), profile_path, L"",
                                         NULL, NULL);

  // If there are no more shortcuts in the Chrome Apps subdirectory, remove it.
  base::FilePath chrome_apps_dir;
  if (ShellUtil::GetShortcutPath(
          ShellUtil::SHORTCUT_LOCATION_START_MENU_CHROME_APPS_DIR,
          BrowserDistribution::GetDistribution(),
          ShellUtil::CURRENT_USER,
          &chrome_apps_dir)) {
    if (base::IsDirectoryEmpty(chrome_apps_dir))
      base::DeleteFile(chrome_apps_dir, false);
  }
}

std::vector<base::FilePath> GetShortcutPaths(
    const ShortcutLocations& creation_locations) {
  // Shortcut paths under which to create shortcuts.
  std::vector<base::FilePath> shortcut_paths;
  // Locations to add to shortcut_paths.
  struct {
    bool use_this_location;
    ShellUtil::ShortcutLocation location_id;
  } locations[] = {
    {
      creation_locations.on_desktop,
      ShellUtil::SHORTCUT_LOCATION_DESKTOP
    }, {
      creation_locations.applications_menu_location ==
          APP_MENU_LOCATION_ROOT,
      ShellUtil::SHORTCUT_LOCATION_START_MENU_ROOT
    }, {
      creation_locations.applications_menu_location ==
          APP_MENU_LOCATION_SUBDIR_CHROME,
      ShellUtil::SHORTCUT_LOCATION_START_MENU_CHROME_DIR
    }, {
      creation_locations.applications_menu_location ==
          APP_MENU_LOCATION_SUBDIR_CHROMEAPPS,
      ShellUtil::SHORTCUT_LOCATION_START_MENU_CHROME_APPS_DIR
    }, {
      // For Win7+, |in_quick_launch_bar| indicates that we are pinning to
      // taskbar. This needs to be handled by callers.
      creation_locations.in_quick_launch_bar &&
          base::win::GetVersion() < base::win::VERSION_WIN7,
      ShellUtil::SHORTCUT_LOCATION_QUICK_LAUNCH
    }
  };

  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  // Populate shortcut_paths.
  for (int i = 0; i < arraysize(locations); ++i) {
    if (locations[i].use_this_location) {
      base::FilePath path;
      if (!ShellUtil::GetShortcutPath(locations[i].location_id,
                                      dist,
                                      ShellUtil::CURRENT_USER,
                                      &path)) {
        NOTREACHED();
        continue;
      }
      shortcut_paths.push_back(path);
    }
  }
  return shortcut_paths;
}

base::FilePath GetIconFilePath(const base::FilePath& web_app_path,
                               const base::string16& title) {
  return web_app_path.Append(GetSanitizedFileName(title))
      .AddExtension(FILE_PATH_LITERAL(".ico"));
}

}  // namespace internals

void UpdateShortcutForTabContents(content::WebContents* web_contents) {
  // UpdateShortcutWorker will delete itself when it's done.
  UpdateShortcutWorker* worker = new UpdateShortcutWorker(web_contents);
  worker->Run();
}

}  // namespace web_app
