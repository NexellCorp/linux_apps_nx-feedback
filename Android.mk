LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES:= external/tinyalsa/include
LOCAL_SRC_FILES:= test-nx-feedback.c
LOCAL_MODULE := test-nx-feedback
LOCAL_SHARED_LIBRARIES:= libcutils libutils libtinyalsa
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Werror

include $(BUILD_EXECUTABLE)
