// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_TEST_WEB_THREAD_H_
#define IOS_WEB_PUBLIC_TEST_TEST_WEB_THREAD_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "ios/web/public/web_thread.h"

namespace base {
class MessageLoop;
}

namespace web {

class TestWebThreadImpl;

// TODO(ios): once WebThread is no longer implemented in term of BrowserThread,
// introduces TestWebThreadBundle and deprecate TestWebThread
// http://crbug.com/272091
//
// A WebThread for unit tests; this lets unit tests in ios/chrome create
// WebThread instances.
class TestWebThread {
 public:
  explicit TestWebThread(WebThread::ID identifier);
  TestWebThread(WebThread::ID identifier, base::MessageLoop* message_loop);

  ~TestWebThread();

  // Provides a subset of the capabilities of the Thread interface to enable
  // certain unit tests. To avoid a stronger dependency of the internals of
  // WebThread, do no provide the full Thread interface.

  // Starts the thread with a generci message loop.
  bool Start();

  // Starts the thread with an IOThread message loop.
  bool StartIOThread();

  // Stops the thread.
  void Stop();

  // Returns true if the thread is running.
  bool IsRunning();

 private:
  scoped_ptr<TestWebThreadImpl> impl_;

  DISALLOW_COPY_AND_ASSIGN(TestWebThread);
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_TEST_WEB_THREAD_H_
