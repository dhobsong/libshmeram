LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
#copy meram.conf to /system/etc/meram.conf
files := meram.conf
dest_dir := $(TARGET_OUT)/etc
copy_to := $(addprefix $(dest_dir)/,$(files))

$(copy_to) : $(dest_dir)/% : $(LOCAL_PATH)/% | $(ACP)
        $(transform-prebuilt-to-target)

ALL_PREBUILT += $(copy_to)
LOCAL_MODULE := meram.conf
LOCAL_SRC_FILES := meram.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
