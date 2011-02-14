LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
	external/libmeram/include \
	external/libuiomux/include \

LOCAL_CFLAGS := -DCONFIG_FILE=\"/system/etc/meram.conf\"

LOCAL_SRC_FILES := \
	meram.c \
	ipmmui.c

LOCAL_SHARED_LIBRARIES := libcutils libuiomux

LOCAL_MODULE := libmeram
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
