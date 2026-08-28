/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <utils/Errors.h>
#include <utils/StrongPointer.h>

namespace android {

class VendorTagDescriptor;

/**
 * Merges the com.oplus.* vendor tags that the OPlus camera stack (APS engine,
 * unit.sdk) requires but which the camx vendor-tag registry on this device does
 * not enumerate into a copy of |src|.
 *
 * The returned descriptor preserves all tags present in |src| and appends the
 * OPlus tags to the (already existing) "com.oplus" section. This lets the
 * OPlus camera app resolve tags such as aps.sat.preview.master.pipeline,
 * TR.processing.state, caller.package.name etc. at initialization, which on a
 * stock OOS build are provided by the ODM algo libraries.
 *
 * Returns OK on success (including when there is nothing to merge), or a
 * negative error code. On failure |out| is left untouched.
 */
status_t mergeOPlusVendorTags(const sp<VendorTagDescriptor>& src,
                              sp<VendorTagDescriptor>& out);

}  // namespace android
