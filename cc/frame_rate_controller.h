// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CCFrameRateController_h
#define CCFrameRateController_h

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/time.h"
#include "CCTimer.h"

namespace cc {

class CCThread;
class CCTimeSource;

class CCFrameRateControllerClient {
public:
    // Throttled is true when we have a maximum number of frames pending.
    virtual void vsyncTick(bool throttled) = 0;

protected:
    virtual ~CCFrameRateControllerClient() {}
};

class CCFrameRateControllerTimeSourceAdapter;

class CCFrameRateController : public CCTimerClient {
public:
    explicit CCFrameRateController(scoped_refptr<CCTimeSource>);
    // Alternate form of CCFrameRateController with unthrottled frame-rate.
    explicit CCFrameRateController(CCThread*);
    virtual ~CCFrameRateController();

    void setClient(CCFrameRateControllerClient* client) { m_client = client; }

    void setActive(bool);

    // Use the following methods to adjust target frame rate.
    //
    // Multiple frames can be in-progress, but for every didBeginFrame, a
    // didFinishFrame should be posted.
    //
    // If the rendering pipeline crashes, call didAbortAllPendingFrames.
    void didBeginFrame();
    void didFinishFrame();
    void didAbortAllPendingFrames();
    void setMaxFramesPending(int); // 0 for unlimited.

    // This returns null for unthrottled frame-rate.
    base::TimeTicks nextTickTime();

    void setTimebaseAndInterval(base::TimeTicks timebase, base::TimeDelta interval);
    void setSwapBuffersCompleteSupported(bool);

protected:
    friend class CCFrameRateControllerTimeSourceAdapter;
    void onTimerTick();

    void postManualTick();

    // CCTimerClient implementation (used for unthrottled frame-rate).
    virtual void onTimerFired() OVERRIDE;

    CCFrameRateControllerClient* m_client;
    int m_numFramesPending;
    int m_maxFramesPending;
    scoped_refptr<CCTimeSource> m_timeSource;
    scoped_ptr<CCFrameRateControllerTimeSourceAdapter> m_timeSourceClientAdapter;
    bool m_active;
    bool m_swapBuffersCompleteSupported;

    // Members for unthrottled frame-rate.
    bool m_isTimeSourceThrottling;
    scoped_ptr<CCTimer> m_manualTicker;
};

}  // namespace cc

#endif // CCFrameRateController_h
