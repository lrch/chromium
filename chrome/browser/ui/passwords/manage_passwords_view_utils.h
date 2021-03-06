// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PASSWORDS_MANAGE_PASSWORDS_VIEW_UTILS_H_
#define CHROME_BROWSER_UI_PASSWORDS_MANAGE_PASSWORDS_VIEW_UTILS_H_

#include <string>

namespace autofill {
struct PasswordForm;
}

// Returns the origin URI in a format which can be presented to a user based of
// |password_from| field values. For web URIs |languages| is using in order to
// determine whether a URI is 'comprehensible' to a user who understands
// languages listed.
std::string GetHumanReadableOrigin(const autofill::PasswordForm& password_form,
                                   const std::string& languages);

#endif  // CHROME_BROWSER_UI_PASSWORDS_MANAGE_PASSWORDS_VIEW_UTILS_H_
