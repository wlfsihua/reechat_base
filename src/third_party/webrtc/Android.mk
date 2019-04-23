# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := rtc_base

SOURCE_PATH := $(LOCAL_PATH)/rtc_base

MY_FILES_SUFFIX := %.cpp %.c %.cc

rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

SRC_FILES := $(call rwildcard, $(SOURCE_PATH)/,$(MY_FILES_SUFFIX))

not-containing = $(foreach v,$2,$(if $(findstring $1,$v),,$v))
MY_SRC_FILES := $(call not-containing,unittest,$(SRC_FILES))
MY_SRC_FILES := $(call not-containing,win,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,helpers.cc,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,mac,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,messagedigest.cc,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,ssl,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,rate_limiter.cc,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,gcd,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,test,$(MY_SRC_FILES))

LOCAL_SRC_FILES :=  $(MY_SRC_FILES:$(LOCAL_PATH)/%=%)
LOCAL_SRC_FILES +=  $(SOURCE_PATH)/nethelpers.cc

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../ \
					$(LOCAL_PATH)/../jsoncpp/source/include

LOCAL_CFLAGS += -DANDROID -DWEBRTC_POSIX -DWEBRTC_ANDROID -DWEBRTC_LINUX -DWEBRTC_BUILD_LIBEVENT

LOCAL_CFLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -Wno-narrowing

include $(BUILD_STATIC_LIBRARY)