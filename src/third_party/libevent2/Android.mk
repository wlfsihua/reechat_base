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

LOCAL_MODULE := event2

SOURCE_PATH := $(LOCAL_PATH)

MY_FILES_SUFFIX := %.cpp %.c %.cc

rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

SRC_FILES := $(call rwildcard, $(SOURCE_PATH)/,$(MY_FILES_SUFFIX))

not-containing = $(foreach v,$2,$(if $(findstring $1,$v),,$v))
MY_SRC_FILES := $(call not-containing,sample,$(SRC_FILES))
MY_SRC_FILES := $(call not-containing,test,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,arc4random.c,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,buffer_iocp.c,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,bufferevent_async.c,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,bufferevent_openssl.c,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,event_iocp.c,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,win32,$(MY_SRC_FILES))
MY_SRC_FILES := $(call not-containing,kqueue.c,$(MY_SRC_FILES))


LOCAL_SRC_FILES :=  $(MY_SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES := $(SOURCE_PATH)/compat \
					$(SOURCE_PATH)/include \
					$(SOURCE_PATH)/override/android

include $(BUILD_STATIC_LIBRARY)