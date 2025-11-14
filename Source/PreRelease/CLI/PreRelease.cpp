/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

#include <print>
#include "PreRelease/Enums.h"
#include "PreRelease/Resources.h"

auto main() -> int {
    std::println("Resources Create: {}", Resources_Create().To_UTF8().c_str());
    std::println("Enums Create: {}", Enums_Create().To_UTF8().c_str());
}
