# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from integration_tests import network_metrics
from telemetry.page import page_test
from telemetry.value import scalar


class ChromeProxyMetricException(page_test.MeasurementFailure):
  pass


CHROME_PROXY_VIA_HEADER = 'Chrome-Compression-Proxy'
CHROME_PROXY_VIA_HEADER_DEPRECATED = '1.1 Chrome Compression Proxy'


class ChromeProxyResponse(network_metrics.HTTPResponse):
  """ Represents an HTTP response from a timeleine event."""
  def __init__(self, event):
    super(ChromeProxyResponse, self).__init__(event)

  def ShouldHaveChromeProxyViaHeader(self):
    resp = self.response
    # Ignore https and data url
    if resp.url.startswith('https') or resp.url.startswith('data:'):
      return False
    # Ignore 304 Not Modified and cache hit.
    if resp.status == 304 or resp.served_from_cache:
      return False
    # Ignore invalid responses that don't have any header. Log a warning.
    if not resp.headers:
      logging.warning('response for %s does not any have header '
                      '(refer=%s, status=%s)',
                      resp.url, resp.GetHeader('Referer'), resp.status)
      return False
    return True

  def HasChromeProxyViaHeader(self):
    via_header = self.response.GetHeader('Via')
    if not via_header:
      return False
    vias = [v.strip(' ') for v in via_header.split(',')]
    # The Via header is valid if it is the old format or the new format
    # with 4-character version prefix, for example,
    # "1.1 Chrome-Compression-Proxy".
    return (CHROME_PROXY_VIA_HEADER_DEPRECATED in vias or
            any(v[4:] == CHROME_PROXY_VIA_HEADER for v in vias))

  def IsValidByViaHeader(self):
    return (not self.ShouldHaveChromeProxyViaHeader() or
            self.HasChromeProxyViaHeader())

  def GetChromeProxyClientType(self):
    """Get the client type directive from the Chrome-Proxy request header.

    Returns:
        The client type directive from the Chrome-Proxy request header for the
        request that lead to this response. For example, if the request header
        "Chrome-Proxy: c=android" is present, then this method would return
        "android". Returns None if no client type directive is present.
    """
    if 'Chrome-Proxy' not in self.response.request_headers:
      return None

    chrome_proxy_request_header = self.response.request_headers['Chrome-Proxy']
    values = [v.strip() for v in chrome_proxy_request_header.split(',')]
    for value in values:
      kvp = value.split('=', 1)
      if len(kvp) == 2 and kvp[0].strip() == 'c':
        return kvp[1].strip()
    return None


class ChromeProxyMetric(network_metrics.NetworkMetric):
  """A Chrome proxy timeline metric."""

  def __init__(self):
    super(ChromeProxyMetric, self).__init__()
    self.compute_data_saving = True

  def SetEvents(self, events):
    """Used for unittest."""
    self._events = events

  def ResponseFromEvent(self, event):
    return ChromeProxyResponse(event)

  def AddResults(self, tab, results):
    raise NotImplementedError

  def AddResultsForDataSaving(self, tab, results):
    resources_via_proxy = 0
    resources_from_cache = 0
    resources_direct = 0

    super(ChromeProxyMetric, self).AddResults(tab, results)
    for resp in self.IterResponses(tab):
      if resp.response.served_from_cache:
        resources_from_cache += 1
      if resp.HasChromeProxyViaHeader():
        resources_via_proxy += 1
      else:
        resources_direct += 1

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'resources_via_proxy', 'count',
        resources_via_proxy))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'resources_from_cache', 'count',
        resources_from_cache))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'resources_direct', 'count', resources_direct))

  def AddResultsForHeaderValidation(self, tab, results):
    via_count = 0

    for resp in self.IterResponses(tab):
      if resp.IsValidByViaHeader():
        via_count += 1
      else:
        r = resp.response
        raise ChromeProxyMetricException, (
            '%s: Via header (%s) is not valid (refer=%s, status=%d)' % (
                r.url, r.GetHeader('Via'), r.GetHeader('Referer'), r.status))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'checked_via_header', 'count', via_count))

  def AddResultsForClientVersion(self, tab, results):
    for resp in self.IterResponses(tab):
      r = resp.response
      if resp.response.status != 200:
        raise ChromeProxyMetricException, ('%s: Response is not 200: %d' %
                                           (r.url, r.status))
      if not resp.IsValidByViaHeader():
        raise ChromeProxyMetricException, ('%s: Response missing via header' %
                                           (r.url))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'version_test', 'count', 1))

  def GetClientTypeFromRequests(self, tab):
    """Get the Chrome-Proxy client type value from requests made in this tab.

    Returns:
        The client type value from the first request made in this tab that
        specifies a client type in the Chrome-Proxy request header. See
        ChromeProxyResponse.GetChromeProxyClientType for more details about the
        Chrome-Proxy client type. Returns None if none of the requests made in
        this tab specify a client type.
    """
    for resp in self.IterResponses(tab):
      client_type = resp.GetChromeProxyClientType()
      if client_type:
        return client_type
    return None

  def AddResultsForClientType(self, tab, results, client_type,
                              bypass_for_client_type):
    via_count = 0
    bypass_count = 0

    for resp in self.IterResponses(tab):
      if resp.HasChromeProxyViaHeader():
        via_count += 1
        if client_type.lower() == bypass_for_client_type.lower():
          raise ChromeProxyMetricException, (
              '%s: Response for client of type "%s" has via header, but should '
              'be bypassed.' % (
                  resp.response.url, bypass_for_client_type, client_type))
      elif resp.ShouldHaveChromeProxyViaHeader():
        bypass_count += 1
        if client_type.lower() != bypass_for_client_type.lower():
          raise ChromeProxyMetricException, (
              '%s: Response missing via header. Only "%s" clients should '
              'bypass for this page, but this client is "%s".' % (
                  resp.response.url, bypass_for_client_type, client_type))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'via', 'count', via_count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'bypass', 'count', bypass_count))

  def AddResultsForBypass(self, tab, results):
    bypass_count = 0

    for resp in self.IterResponses(tab):
      if resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            '%s: Should not have Via header (%s) (refer=%s, status=%d)' % (
                r.url, r.GetHeader('Via'), r.GetHeader('Referer'), r.status))
      bypass_count += 1

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'bypass', 'count', bypass_count))

  def AddResultsForCorsBypass(self, tab, results):
    eligible_response_count = 0
    bypass_count = 0
    bypasses = {}
    for resp in self.IterResponses(tab):
      logging.warn('got a resource %s' % (resp.response.url))

    for resp in self.IterResponses(tab):
      if resp.ShouldHaveChromeProxyViaHeader():
        eligible_response_count += 1
        if not resp.HasChromeProxyViaHeader():
          bypass_count += 1
        elif resp.response.status == 502:
          bypasses[resp.response.url] = 0

    for resp in self.IterResponses(tab):
      if resp.ShouldHaveChromeProxyViaHeader():
        if not resp.HasChromeProxyViaHeader():
          if resp.response.status == 200:
            if (bypasses.has_key(resp.response.url)):
              bypasses[resp.response.url] = bypasses[resp.response.url] + 1

    for url in bypasses:
      if bypasses[url] == 0:
        raise ChromeProxyMetricException, (
              '%s: Got a 502 without a subsequent 200' % (url))
      elif bypasses[url] > 1:
        raise ChromeProxyMetricException, (
              '%s: Got a 502 and multiple 200s: %d' % (url, bypasses[url]))
    if bypass_count == 0:
      raise ChromeProxyMetricException, (
          'At least one response should be bypassed. '
          '(eligible_response_count=%d, bypass_count=%d)\n' % (
              eligible_response_count, bypass_count))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'cors_bypass', 'count', bypass_count))

  def AddResultsForBlockOnce(self, tab, results):
    eligible_response_count = 0
    bypass_count = 0

    for resp in self.IterResponses(tab):
      if resp.ShouldHaveChromeProxyViaHeader():
        eligible_response_count += 1
        if not resp.HasChromeProxyViaHeader():
          bypass_count += 1

    if eligible_response_count <= 1:
      raise ChromeProxyMetricException, (
          'There should be more than one DRP eligible response '
          '(eligible_response_count=%d, bypass_count=%d)\n' % (
              eligible_response_count, bypass_count))
    elif bypass_count != 1:
      raise ChromeProxyMetricException, (
          'Exactly one response should be bypassed. '
          '(eligible_response_count=%d, bypass_count=%d)\n' % (
              eligible_response_count, bypass_count))
    else:
      results.AddValue(scalar.ScalarValue(
          results.current_page, 'eligible_responses', 'count',
          eligible_response_count))
      results.AddValue(scalar.ScalarValue(
          results.current_page, 'bypass', 'count', bypass_count))

  def AddResultsForSafebrowsingOn(self, tab, results):
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'safebrowsing', 'timeout responses', 1))

  def AddResultsForSafebrowsingOff(self, tab, results):
    response_count = 0
    for resp in self.IterResponses(tab):
      # Data reduction proxy should return the real response for sites with
      # malware.
      response_count += 1
      if not resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            '%s: Safebrowsing feature should be off for desktop and webview.\n'
            'Reponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))

    if response_count == 0:
      raise ChromeProxyMetricException, (
          'Safebrowsing test failed: No valid responses received')

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'safebrowsing', 'responses', response_count))

  def AddResultsForHTTPFallback(self, tab, results):
    via_fallback_count = 0

    for resp in self.IterResponses(tab):
      if resp.ShouldHaveChromeProxyViaHeader():
        # All responses should have come through the HTTP fallback proxy, which
        # means that they should have the via header, and if a remote port is
        # defined, it should be port 80.
        if (not resp.HasChromeProxyViaHeader() or
            (resp.remote_port and resp.remote_port != 80)):
          r = resp.response
          raise ChromeProxyMetricException, (
              '%s: Should have come through the fallback proxy.\n'
              'Reponse: remote_port=%s status=(%d, %s)\nHeaders:\n %s' % (
                  r.url, str(resp.remote_port), r.status, r.status_text,
                  r.headers))
        via_fallback_count += 1

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'via_fallback', 'count', via_fallback_count))

  def AddResultsForHTTPToDirectFallback(self, tab, results):
    via_fallback_count = 0
    bypass_count = 0
    responses = self.IterResponses(tab)

    # The very first response should be through the HTTP fallback proxy.
    fallback_resp = next(responses, None)
    if not fallback_resp:
      raise ChromeProxyMetricException, 'There should be at least one response.'
    elif (not fallback_resp.HasChromeProxyViaHeader() or
          fallback_resp.remote_port != 80):
      r = fallback_resp.response
      raise ChromeProxyMetricException, (
          'Response for %s should have come through the fallback proxy.\n'
          'Reponse: remote_port=%s status=(%d, %s)\nHeaders:\n %s' % (
              r.url, str(fallback_resp.remote_port), r.status, r.status_text,
              r.headers))
    else:
      via_fallback_count += 1

    # All other responses should have been bypassed.
    for resp in responses:
      if resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should not have via header.\n'
            'Reponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))
      else:
        bypass_count += 1

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'via_fallback', 'count', via_fallback_count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'bypass', 'count', bypass_count))

  def AddResultsForReenableAfterBypass(
      self, tab, results, bypass_seconds_min, bypass_seconds_max):
    """Verify results for a re-enable after bypass test.

    Args:
        tab: the tab for the test.
        results: the results object to add the results values to.
        bypass_seconds_min: the minimum duration of the bypass.
        bypass_seconds_max: the maximum duration of the bypass.
    """
    bypass_count = 0
    via_count = 0

    for resp in self.IterResponses(tab):
      if resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should not have via header.\n'
            'Reponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))
      else:
        bypass_count += 1

    # Wait until 30 seconds before the bypass should expire, and fetch a page.
    # It should not have the via header because the proxy should still be
    # bypassed.
    time.sleep(bypass_seconds_min - 30)

    tab.ClearCache(force=True)
    before_metrics = ChromeProxyMetric()
    before_metrics.Start(results.current_page, tab)
    tab.Navigate('http://chromeproxy-test.appspot.com/default')
    tab.WaitForJavaScriptExpression('performance.timing.loadEventStart', 10)
    before_metrics.Stop(results.current_page, tab)

    for resp in before_metrics.IterResponses(tab):
      if resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should not have via header; proxy should still '
            'be bypassed.\nReponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))
      else:
        bypass_count += 1

    # Wait until 30 seconds after the bypass should expire, and fetch a page. It
    # should have the via header since the proxy should no longer be bypassed.
    time.sleep((bypass_seconds_max + 30) - (bypass_seconds_min - 30))

    tab.ClearCache(force=True)
    after_metrics = ChromeProxyMetric()
    after_metrics.Start(results.current_page, tab)
    tab.Navigate('http://chromeproxy-test.appspot.com/default')
    tab.WaitForJavaScriptExpression('performance.timing.loadEventStart', 10)
    after_metrics.Stop(results.current_page, tab)

    for resp in after_metrics.IterResponses(tab):
      if not resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should have via header; proxy should no longer '
            'be bypassed.\nReponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))
      else:
        via_count += 1

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'bypass', 'count', bypass_count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'via', 'count', via_count))
