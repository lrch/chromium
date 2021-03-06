// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/native/aw_message_port_service_impl.h"

#include "android_webview/browser/aw_browser_context.h"
#include "android_webview/browser/aw_message_port_message_filter.h"
#include "android_webview/native/aw_contents.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/message_port_provider.h"
#include "jni/AwMessagePortService_jni.h"

namespace android_webview {

using base::android::AttachCurrentThread;
using base::android::ConvertUTF16ToJavaString;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaIntArray;
using content::BrowserThread;
using content::MessagePortProvider;

//static
AwMessagePortServiceImpl* AwMessagePortServiceImpl::GetInstance() {
  return static_cast<AwMessagePortServiceImpl*>(
      AwBrowserContext::GetDefault()->GetMessagePortService());
}

AwMessagePortServiceImpl::AwMessagePortServiceImpl() {
}

AwMessagePortServiceImpl::~AwMessagePortServiceImpl() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_AwMessagePortService_unregisterNativeAwMessagePortService(env,
                                                                 obj.obj());
}

void AwMessagePortServiceImpl::Init(JNIEnv* env, jobject obj) {
  java_ref_ = JavaObjectWeakGlobalRef(env, obj);
}

void AwMessagePortServiceImpl::CreateMessageChannel(
    JNIEnv* env,
    jobject callback,
    scoped_refptr<AwMessagePortMessageFilter> filter) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  ScopedJavaGlobalRef<jobject>* j_callback = new ScopedJavaGlobalRef<jobject>();
  j_callback->Reset(env, callback);

  int* portId1 = new int;
  int* portId2 = new int;
  BrowserThread::PostTaskAndReply(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&AwMessagePortServiceImpl::CreateMessageChannelOnIOThread,
                 base::Unretained(this),
                 filter,
                 portId1,
                 portId2),
      base::Bind(&AwMessagePortServiceImpl::OnMessageChannelCreated,
                 base::Unretained(this),
                 base::Owned(j_callback),
                 base::Owned(portId1),
                 base::Owned(portId2)));
}

void AwMessagePortServiceImpl::OnConvertedMessage(
    int message_port_id,
    const base::ListValue& message,
    const std::vector<int>& sent_message_port_ids) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> jobj = java_ref_.get(env);
  if (jobj.is_null())
    return;

  if (message.GetSize() != 1) {
    NOTREACHED();
    return;
  }

  base::string16 value;
  if (!message.GetString(0, &value)) {
    LOG(WARNING) << "Converting post message to a string failed for port "
        << message_port_id;
    return;
  }
  ScopedJavaLocalRef<jstring> jmsg = ConvertUTF16ToJavaString(env, value);
  ScopedJavaLocalRef<jintArray> jports =
      ToJavaIntArray(env, sent_message_port_ids);
  Java_AwMessagePortService_onPostMessage(env,
                                          jobj.obj(),
                                          message_port_id,
                                          jmsg.obj(),
                                          jports.obj());
}

void AwMessagePortServiceImpl::OnMessagePortMessageFilterClosing(
    AwMessagePortMessageFilter* filter) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  for (MessagePorts::iterator iter = ports_.begin();
       iter != ports_.end(); iter++) {
    if (iter->second == filter) {
      ports_.erase(iter);
    }
  }
}

void AwMessagePortServiceImpl::CreateMessageChannelOnIOThread(
    scoped_refptr<AwMessagePortMessageFilter> filter,
    int* portId1,
    int* portId2) {
  content::MessagePortProvider::CreateMessageChannel(filter.get(), portId1,
      portId2);
  AddPort(*portId1, filter.get());
  AddPort(*portId2, filter.get());
}

void AwMessagePortServiceImpl::OnMessageChannelCreated(
    ScopedJavaGlobalRef<jobject>* callback,
    int* port1,
    int* port2) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_AwMessagePortService_onMessageChannelCreated(env, obj.obj(), *port1,
      *port2, callback->obj());
}

void AwMessagePortServiceImpl::AddPort(int message_port_id,
                                       AwMessagePortMessageFilter* filter) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (ports_.count(message_port_id)) {
    NOTREACHED();
    return;
  }
  ports_[message_port_id] = filter;
}

bool RegisterAwMessagePortService(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

// static
jlong InitAwMessagePortService(JNIEnv* env, jobject obj) {
  AwMessagePortServiceImpl* service = AwMessagePortServiceImpl::GetInstance();
  service->Init(env, obj);
  return reinterpret_cast<intptr_t>(service);
}

}  // namespace android_webview
