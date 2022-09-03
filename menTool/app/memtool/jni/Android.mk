LOCAL_PATH := $(call my-dir)
 
include $(CLEAR_VARS)
 
LOCAL_MODULE    := memtool
LOCAL_SRC_FILES := memtool.cpp
# LOCAL_C_INCLUDES += ../jni/
 
# include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)
