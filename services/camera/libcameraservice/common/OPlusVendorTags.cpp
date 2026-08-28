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

#define LOG_TAG "OPlusVendorTags"

#include "OPlusVendorTags.h"

#include <camera/VendorTagDescriptor.h>
#include <system/camera_metadata.h>
#include <system/camera_vendor_tags.h>
#include <utils/Log.h>

#include <cstring>
#include <vector>

namespace android {

namespace {

struct OPlusTagEntry {
    const char* name;
    int type;
};

// Vendor tags consumed by the OPlus camera app / APS engine (com.oplus section)
// that the camx vendor-tag registry on this port does not provide. Extracted
// from the failing VendorTagDescriptor lookups in the OPlus camera app logs.
static const OPlusTagEntry kOPlusTags[] = {
    {"aicolor.rear.enable", TYPE_BYTE},
    {"algo.visualization.enable", TYPE_BYTE},
    {"aps.sat.preview.master.pipeline", TYPE_BYTE},
    {"aps.turbo.raw.scene", TYPE_BYTE},
    {"asd.hdr.scope", TYPE_INT32},
    {"burst.capture.single", TYPE_BYTE},
    {"burst.capture.type", TYPE_INT32},
    {"caller.package.name", TYPE_BYTE},
    {"camera.3d.api.state", TYPE_BYTE},
    {"camera.configure.thermal.level", TYPE_INT32},
    {"camera.is.turn.on", TYPE_BYTE},
    {"camera.pi.enable", TYPE_BYTE},
    {"camera.pi.enable_list", TYPE_BYTE},
    {"camera.pip.preview.sensor", TYPE_INT32},
    {"capture.flash.need", TYPE_BYTE},
    {"capture.hdr.support", TYPE_BYTE},
    {"capture.job.type", TYPE_INT32},
    {"capture.request.need.preview.stream", TYPE_BYTE},
    {"capture.request.picture.size.scale", TYPE_FLOAT},
    {"config.aeExposureCompensation", TYPE_INT32},
    {"control.face.dr", TYPE_BYTE},
    {"defer.force.start", TYPE_BYTE},
    {"draw.frame.delay", TYPE_INT32},
    {"externalFlashStatus", TYPE_INT32},
    {"externalFlashTime", TYPE_INT32},
    {"facebeauty.custom", TYPE_BYTE},
    {"fallback.stable", TYPE_BYTE},
    {"filter.mode", TYPE_INT32},
    {"flash.IntensityControl", TYPE_INT32},
    {"flash.snapshot.trigger.list", TYPE_BYTE},
    {"flash.status", TYPE_INT32},
    {"hal.fluency", TYPE_INT32},
    {"ipe.sequence", TYPE_INT32},
    {"iris.aperture.switching", TYPE_BYTE},
    {"is.capture.thumbnail", TYPE_BYTE},
    {"is.sdk.camera.package", TYPE_BYTE},
    {"lsd.enable", TYPE_BYTE},
    {"ManualExposure.snapshot.camIndex", TYPE_INT32},
    {"mipiraw.online.bpc", TYPE_BYTE},
    {"multiobj.info.visualization", TYPE_BYTE},
    {"naturetone.state", TYPE_INT32},
    {"night.se.enable", TYPE_BYTE},
    {"only.zoom.change", TYPE_BYTE},
    {"outflash.flashtype", TYPE_INT32},
    {"picture.offset.time", TYPE_INT32},
    {"preview.hdr.support", TYPE_BYTE},
    {"preview.outflash.connected", TYPE_BYTE},
    {"process.pid", TYPE_INT32},
    {"rear.remosaic.enable", TYPE_BYTE},
    {"rtb.skip.slave.frame", TYPE_BYTE},
    {"sod.enable", TYPE_BYTE},
    {"TR.processing.state", TYPE_INT32},
    {"videoFrameJitter", TYPE_INT32},
    {"wireless.charging.result", TYPE_BYTE},
    {"zoom.frame.info", TYPE_BYTE},
};

constexpr size_t kOPlusTagCount = sizeof(kOPlusTags) / sizeof(kOPlusTags[0]);
constexpr const char* kOPlusSectionName = "com.oplus";

struct MergedOps {
    vendor_tag_ops_t ops;
    const hardware::camera2::params::VendorTagDescriptor* base;
    uint32_t baseTagCount;
    uint32_t extraTagIds[kOPlusTagCount];
};

int mergedGetTagCount(const vendor_tag_ops_t* v) {
    const MergedOps* m = reinterpret_cast<const MergedOps*>(v);
    return static_cast<int>(m->baseTagCount + kOPlusTagCount);
}

void mergedGetAllTags(const vendor_tag_ops_t* v, uint32_t* tagArray) {
    const MergedOps* m = reinterpret_cast<const MergedOps*>(v);
    m->base->getTagArray(tagArray);
    std::memcpy(tagArray + m->baseTagCount, m->extraTagIds,
                sizeof(m->extraTagIds));
}

const char* mergedGetSectionName(const vendor_tag_ops_t* v, uint32_t tag) {
    const MergedOps* m = reinterpret_cast<const MergedOps*>(v);
    if (m->base->getSectionName(tag) != nullptr) {
        return m->base->getSectionName(tag);
    }
    for (size_t i = 0; i < kOPlusTagCount; ++i) {
        if (m->extraTagIds[i] == tag) {
            return kOPlusSectionName;
        }
    }
    return nullptr;
}

const char* mergedGetTagName(const vendor_tag_ops_t* v, uint32_t tag) {
    const MergedOps* m = reinterpret_cast<const MergedOps*>(v);
    if (m->base->getSectionName(tag) != nullptr) {
        return m->base->getTagName(tag);
    }
    for (size_t i = 0; i < kOPlusTagCount; ++i) {
        if (m->extraTagIds[i] == tag) {
            return kOPlusTags[i].name;
        }
    }
    return nullptr;
}

int mergedGetTagType(const vendor_tag_ops_t* v, uint32_t tag) {
    const MergedOps* m = reinterpret_cast<const MergedOps*>(v);
    if (m->base->getSectionName(tag) != nullptr) {
        return m->base->getTagType(tag);
    }
    for (size_t i = 0; i < kOPlusTagCount; ++i) {
        if (m->extraTagIds[i] == tag) {
            return kOPlusTags[i].type;
        }
    }
    return -1;
}

}  // namespace

status_t mergeOPlusVendorTags(const sp<VendorTagDescriptor>& src,
                              sp<VendorTagDescriptor>& out) {
    if (src == nullptr) {
        return OK;
    }

    const int baseCount = src->getTagCount();
    if (baseCount < 0) {
        return BAD_VALUE;
    }

    std::vector<uint32_t> baseTags(static_cast<size_t>(baseCount));
    src->getTagArray(baseTags.data());

    // Allocate IDs for the OPlus tags just above the highest ID the HAL
    // already uses, so we never collide with existing vendor tag IDs.
    uint32_t nextId = CAMERA_METADATA_VENDOR_TAG_BOUNDARY;
    for (const uint32_t tag : baseTags) {
        if (tag >= nextId) {
            nextId = tag + 1;
        }
    }
    if (nextId == 0xffffffffu) {
        ALOGE("%s: no vendor tag ID space left (%#x)", __FUNCTION__, nextId);
        return NO_MEMORY;
    }

    MergedOps m;
    std::memset(&m, 0, sizeof(m));
    m.base = src.get();
    m.baseTagCount = static_cast<uint32_t>(baseCount);
    for (size_t i = 0; i < kOPlusTagCount; ++i) {
        m.extraTagIds[i] = nextId++;
    }

    m.ops.get_tag_count = mergedGetTagCount;
    m.ops.get_all_tags = mergedGetAllTags;
    m.ops.get_section_name = mergedGetSectionName;
    m.ops.get_tag_name = mergedGetTagName;
    m.ops.get_tag_type = mergedGetTagType;

    return VendorTagDescriptor::createDescriptorFromOps(&m.ops, out);
}

}  // namespace android
