// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/omnibox/omnibox_controller.h"

#include "base/metrics/histogram.h"
#include "chrome/browser/ui/omnibox/omnibox_client.h"
#include "chrome/browser/ui/omnibox/omnibox_edit_controller.h"
#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"
#include "chrome/browser/ui/omnibox/omnibox_popup_model.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/omnibox_popup_view.h"
#include "components/search/search.h"
#include "ui/gfx/geometry/rect.h"

OmniboxController::OmniboxController(OmniboxEditModel* omnibox_edit_model,
                                     OmniboxClient* client)
    : omnibox_edit_model_(omnibox_edit_model),
      client_(client),
      popup_(NULL),
      autocomplete_controller_(new AutocompleteController(
          client_->CreateAutocompleteProviderClient(),
          this,
          AutocompleteClassifier::kDefaultOmniboxProviders)),
      weak_ptr_factory_(this) {}

OmniboxController::~OmniboxController() {
}

void OmniboxController::StartAutocomplete(
    const AutocompleteInput& input) const {
  ClearPopupKeywordMode();
  popup_->SetHoveredLine(OmniboxPopupModel::kNoMatch);

  // We don't explicitly clear OmniboxPopupModel::manually_selected_match, as
  // Start ends up invoking OmniboxPopupModel::OnResultChanged which clears it.
  autocomplete_controller_->Start(input);
}

void OmniboxController::OnResultChanged(bool default_match_changed) {
  const bool was_open = popup_->IsOpen();
  if (default_match_changed) {
    // The default match has changed, we need to let the OmniboxEditModel know
    // about new inline autocomplete text (blue highlight).
    const AutocompleteResult::const_iterator match(result().default_match());
    if (match != result().end()) {
      current_match_ = *match;
      omnibox_edit_model_->OnCurrentMatchChanged();
    } else {
      InvalidateCurrentMatch();
      popup_->OnResultChanged();
      omnibox_edit_model_->OnPopupDataChanged(base::string16(), NULL,
                                              base::string16(), false);
    }
  } else {
    popup_->OnResultChanged();
  }

  if (!popup_->IsOpen() && was_open) {
    // Accept the temporary text as the user text, because it makes little sense
    // to have temporary text when the popup is closed.
    omnibox_edit_model_->AcceptTemporaryTextAsUserText();
  }

  // Note: The client outlives |this|, so bind a weak pointer to the callback
  // passed in to eliminate the potential for crashes on shutdown.
  client_->OnResultChanged(result(), default_match_changed,
                           base::Bind(&OmniboxController::SetAnswerBitmap,
                                      weak_ptr_factory_.GetWeakPtr()));
}

void OmniboxController::InvalidateCurrentMatch() {
  current_match_ = AutocompleteMatch();
}

void OmniboxController::ClearPopupKeywordMode() const {
  if (popup_->IsOpen() &&
      popup_->selected_line_state() == OmniboxPopupModel::KEYWORD)
    popup_->SetSelectedLineState(OmniboxPopupModel::NORMAL);
}

void OmniboxController::SetAnswerBitmap(const SkBitmap& bitmap) {
  popup_->SetAnswerBitmap(bitmap);
}
